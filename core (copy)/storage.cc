#include <condition_variable>
#include <thread>
#include <grpcpp/grpcpp.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <algorithm>
#include <array>
#include <sys/stat.h>  // mkdir
#include <libgen.h>    // dirname
#include <unistd.h>    // access
#include <string>
#include <sstream>
#include "core.grpc.pb.h"
#include "hdf5_hl.h"
#include "log.h"
#include "reactor/server_reactor.h"

#define SERVER_ADDRESS "0.0.0.0:50052"
#define SERVER_SHUTDOWN_TIMEOUT 1000

#define STACK_SIZE (1024 * 1024)
#define COMPRESSION_LEVEL 7   // A number between 0 (none, fastest) to 9 (best, slowest)
#define CHUNK_SIZE 5    // Number of rows per chunk. It progressively impacts the compression time
#define DATASET_NAME_COUNTS "Counts"
#define DATASET_NAME_FFT "FFT"
#define DATASET_NAME_TWISTORR_MONITOR "TwisTorr"
#define DATASET_NAME_RIGOL_MONITOR "Rigol"
#define DATASET_NAME_LASER_MONITOR "Laser"
#define DATASET_NAME_EG_MONITOR "ElectronGun"

int FFT_FULL_SIZE;
#define COUNTS_SIZE 1000

using namespace core;
using namespace google::protobuf;
using namespace std::chrono;

std::string Filename_from_GUI = "storage.h5";
std::vector<std::string> data_ids;

typedef struct {
  double timestamp_;
  uint8_t counts[COUNTS_SIZE];
} CountsEntry;

typedef struct {
  double timestamp_;
  double* fft_full_;
} FFTEntry;

typedef struct {
  double timestamp_;
  uint32_t pump1_current_mon;
  uint32_t pump1_voltage_mon;
  uint32_t pump1_power_mon;
  uint32_t pump1_frequency_mon;
  uint32_t pump1_temperature_mon;
  uint32_t pump2_current_mon;
  uint32_t pump2_voltage_mon;
  uint32_t pump2_power_mon;
  uint32_t pump2_frequency_mon;
  uint32_t pump2_temperature_mon;
  double pressure_1;
  double pressure_2;
  uint8_t ups_status;
} TwisTorrMonitorEntry;

typedef struct {
  double timestamp_;
  double trap_voltage_mon;
  double trap_voltage_offset_mon;
  double trap_frequency_mon;
  std::string trap_function_mon;
  uint8_t trap_function_generator_status;
} RigolMonitorEntry;

typedef struct {
  double timestamp_;
  double laser_voltage_mon;
  uint8_t laser_status_mon;
} LaserMonitorEntry;

typedef struct {
  double timestamp_;
  uint8_t status;
  std::string status_flags;
  float energy_voltage;
  float focus_voltage;
  float wehnelt_voltage;
  float emission_current;
  uint16_t time_per_dot;
  float pos_x;
  float pos_y;
  float area_x;
  float area_y;
  float grid_x;
  float grid_y;    
  
} ElectronGunMonitorEntry;

std::array<std::string, 1> counts_param = {"apd_counts_full"};
std::array<std::string, 1> fft_param = {"apd_fft_full"};
std::array<std::string, 13> twistorr_param = {"pump1_current", "pump1_voltage", "pump1_power", "pump1_frequency", "pump1_temperature","pump2_current", "pump2_voltage", "pump2_power", "pump2_frequency", "pump2_temperature", "pressure_1", "pressure_2", "ups_status"};
std::array<std::string, 5> rigol_param = {"rigol_voltage", "rigol_voltage_offset", "rigol_frequency", "rigol_function", "rigol_status"};
std::array<std::string, 2> laser_param = {"laser_voltage", "laser_state"};
std::array<std::string, 13> electron_gun_param = {"energy_voltage", "focus_voltage", "wehnelt_voltage", "emission_current", "time_per_dot", "pos_x", "pos_y", "area_x", "area_y", "grid_x", "grid_y", "status", "status_flags"};


bool exit_flag = false;
int pid = 0;


int seconds_per_file = 1215752191;

time_t new_file_time;

unique_ptr<CountsEntry> counts_entry;
unique_ptr<FFTEntry> fft_entry;
unique_ptr<TwisTorrMonitorEntry> twistorr_monitor_entry;
unique_ptr<RigolMonitorEntry> rigol_monitor_entry;
unique_ptr<LaserMonitorEntry> laser_monitor_entry;
unique_ptr<ElectronGunMonitorEntry> electron_gun_monitor_entry;

list<ServerUpstreamReactor<Bundle, Empty> *> pusher_reactors;
list<ServerDownstreamReactor<Bundle, Query> *> puller_reactors;

queue<Bundle> inbound_queue;

mutex pusher_mutex;
mutex puller_mutex;
mutex inbound_queue_mutex;
mutex data_storage_mutex;
condition_variable inbound_queue_cv;
condition_variable data_storage_cv;

hid_t fid, counts_ptable, fft_ptable, twistorr_monitor_ptable, rigol_monitor_ptable, laser_monitor_ptable, electron_gun_monitor_ptable;

void OnServerUpstreamReactorDone(void *pusher_reactor) {
  unique_lock<mutex> plck(pusher_mutex);
  pusher_reactors.remove((ServerUpstreamReactor<Bundle, Empty> *) pusher_reactor);
  LOG("Removing pusher reactor. Count: " << pusher_reactors.size());
}

void OnServerDownstreamReactorDone(void *puller_reactor) {
  unique_lock<mutex> plck(puller_mutex);
  puller_reactors.remove((ServerDownstreamReactor<Bundle, Query> *) puller_reactor);
  LOG("Removing puller reactor. Count: " << puller_reactors.size());
}

void ProcessInboundBundle(const Bundle &bundle) {
  unique_lock<mutex> iqlck(inbound_queue_mutex);
  inbound_queue.push(bundle);
  LOG("Processing inbound bundle of type: " << bundle.type());
  // Notify reading thread of this new element
  if (inbound_queue.size() == 1)
	inbound_queue_cv.notify_one();
}

class StorageServiceImpl final : public Storage::CallbackService {
 public:
  __attribute__((unused)) ServerReadReactor<Bundle> *Push(CallbackServerContext *context, Empty *empty) override {
	unique_lock<mutex> plck(pusher_mutex);
	pusher_reactors.push_back(new ServerUpstreamReactor<Bundle, Empty>(empty));
	pusher_reactors.back()->SetInboundCallback(&ProcessInboundBundle);
	pusher_reactors.back()->SetDoneCallback(&OnServerUpstreamReactorDone);
	LOG("Creating new pusher reactor. Count: " << pusher_reactors.size());
	return pusher_reactors.back();
  }

  __attribute__((unused)) ServerWriteReactor<Bundle> *Pull(CallbackServerContext *context, const Query *query) override {
	unique_lock<mutex> pck(puller_mutex);
	puller_reactors.push_back(new ServerDownstreamReactor<Bundle, Query>(query));
	puller_reactors.back()->SetDoneCallback(&OnServerDownstreamReactorDone);
	LOG("Creating new puller reactor. Count: " << puller_reactors.size());
	return puller_reactors.back();
  }
};


std::string intToBinaryString(int n) {
    std::string binaryString;
    for (int i = sizeof(n) * 8 - 1; i >= 0; --i) {
        binaryString += ((n >> i) & 1) ? '1' : '0';
    }
    return binaryString;
}

void WriteData(const Bundle &bundle) {
  auto start = high_resolution_clock::now();
  const auto &kValue = bundle.value();
  switch (bundle.type()) {
	case STORAGE_APD_FULL:
	  if (std::any_of(counts_param.begin(), counts_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {
		  // Here we construct the storage entry
		  LOG("Writing counts record");
		  // Create entry
		  counts_entry->timestamp_ = (double) bundle.timestamp().seconds() + (double) bundle.timestamp().nanos() / 1000000000L;
		  for (int i = 0; i < COUNTS_SIZE; i++)
			counts_entry->counts[i] = kValue.Get(i);
		  LOG("Parsing done");
		  if (H5PTappend(counts_ptable, 1, counts_entry.get()) < 0)
			LOG("Error appending entry");
		  if (H5Fflush(fid, H5F_SCOPE_GLOBAL) < 0)
			LOG("Error flushing data");
		  LOG("A counts entry has been written");
	  }	
	break;	

    case STORAGE_FFT_FULL:
      if (std::any_of(fft_param.begin(), fft_param.end(), [&](const auto& param) { 
          return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); 
      })) {
        LOG("Writing FFT record");
        fft_entry->timestamp_ = (double) bundle.timestamp().seconds() + (double) bundle.timestamp().nanos() / 1000000000L;
        
        // ASIGNAR MEMORIA SI NO EXISTE
        if (!fft_entry->fft_full_) {
          fft_entry->fft_full_ = new double[FFT_FULL_SIZE];
          LOG("FFT memory allocated for: " << FFT_FULL_SIZE << " elements");
        }
        
        int data_points = std::min(static_cast<int>(kValue.size()), FFT_FULL_SIZE);
        
        for (int i = 0; i < data_points; i++)
          fft_entry->fft_full_[i] = kValue.Get(i);
        for (int i = data_points; i < FFT_FULL_SIZE; i++)
          fft_entry->fft_full_[i] = 0.0;
        
        LOG("Parsing done");
        if (H5PTappend(fft_ptable, 1, fft_entry.get()) < 0)
          LOG("Error appending entry");
        if (H5Fflush(fid, H5F_SCOPE_GLOBAL) < 0)
          LOG("Error flushing data");
        LOG("An FFT entry has been written");
      }
    break;

	case STORAGE_TT_MON:
	  if (std::any_of(twistorr_param.begin(), twistorr_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {
		  // Here we construct the storage entry
		  LOG("Writing twistorr monitor record");
		  // Create entry
		  twistorr_monitor_entry->timestamp_ = (double) bundle.timestamp().seconds() + (double) bundle.timestamp().nanos() / 1000000000L;
		  twistorr_monitor_entry->pump1_current_mon = kValue.Get(1);
		  twistorr_monitor_entry->pump1_voltage_mon = kValue.Get(2);
		  twistorr_monitor_entry->pump1_power_mon = kValue.Get(3);
		  twistorr_monitor_entry->pump1_frequency_mon = kValue.Get(4);
		  twistorr_monitor_entry->pump1_temperature_mon = kValue.Get(5);	
		  twistorr_monitor_entry->pump2_current_mon = kValue.Get(7);
		  twistorr_monitor_entry->pump2_voltage_mon = kValue.Get(8);
		  twistorr_monitor_entry->pump2_power_mon = kValue.Get(9);
		  twistorr_monitor_entry->pump2_frequency_mon = kValue.Get(10);
		  twistorr_monitor_entry->pump2_temperature_mon = kValue.Get(11);
		  twistorr_monitor_entry->pressure_1 = kValue.Get(12);
		  twistorr_monitor_entry->pressure_2 = kValue.Get(13);	
		  twistorr_monitor_entry->ups_status = kValue.Get(14);			    	  
		  LOG("Parsing done");
		  if (H5PTappend(twistorr_monitor_ptable, 1, twistorr_monitor_entry.get()) < 0)
			LOG("Error appending entry");
		  if (H5Fflush(fid, H5F_SCOPE_GLOBAL) < 0)
			LOG("Error flushing data");
		  LOG("A twistorr monitor entry has been written");
	  }
	break;

	case STORAGE_RIGOL_MON:
	  if (std::any_of(rigol_param.begin(), rigol_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {
		  // Here we construct the storage entry
		  LOG("Writing rigol monitor record");
		  // Create entry
		  rigol_monitor_entry->timestamp_ = (double) bundle.timestamp().seconds() + (double) bundle.timestamp().nanos() / 1000000000L;
		  rigol_monitor_entry->trap_voltage_mon = kValue.Get(0);
		  rigol_monitor_entry->trap_voltage_offset_mon = kValue.Get(1);
		  rigol_monitor_entry->trap_frequency_mon = kValue.Get(2);
		  //rigol_monitor_entry->trap_function_mon = kValue.Get(3); 
		  int function_value = kValue.Get(3);
		  std::string trap_function;
		  switch (function_value){
            case 1:
              trap_function = "SIN";
              break;
            case 2:
              trap_function = "SQUARE";
              break;
            case 3:
              trap_function = "RAMP";
              break;
            case 4:
              trap_function = "PULSE";
              break;
            case 5:
              trap_function = "DC";
              break;
            case 0:
            default:
              trap_function = "UNKNOWN";
              break;
		  }
		  rigol_monitor_entry->trap_function_mon = trap_function;
		  rigol_monitor_entry->trap_function_generator_status = kValue.Get(4);
		  LOG("Parsing done");
		  if (H5PTappend(rigol_monitor_ptable, 1, rigol_monitor_entry.get()) < 0)
			LOG("Error appending entry");
		  if (H5Fflush(fid, H5F_SCOPE_GLOBAL) < 0)
			LOG("Error flushing data");
		  LOG("A rigol monitor entry has been written");
	  }
	break; 

	case STORAGE_LASER_MON:
	  if (std::any_of(laser_param.begin(), laser_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {
		  // Here we construct the storage entry
		  LOG("Writing laser monitor record");
		  // Create entry
		  laser_monitor_entry->timestamp_ = (double) bundle.timestamp().seconds() + (double) bundle.timestamp().nanos() / 1000000000L;
		  laser_monitor_entry->laser_voltage_mon = kValue.Get(0);
		  laser_monitor_entry->laser_status_mon = kValue.Get(1);
		  LOG("Parsing done");
		  if (H5PTappend(laser_monitor_ptable, 1, laser_monitor_entry.get()) < 0)
			LOG("Error appending entry");
		  if (H5Fflush(fid, H5F_SCOPE_GLOBAL) < 0)
			LOG("Error flushing data");
		  LOG("A laser monitor entry has been written");
    }
  break;

	case STORAGE_EG_MON:
	  if (std::any_of(electron_gun_param.begin(), electron_gun_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {
		  // Here we construct the storage entry
		  LOG("Writing electron gun monitor record");
		  // Create entry
		  electron_gun_monitor_entry->timestamp_ = (double) bundle.timestamp().seconds() + (double) bundle.timestamp().nanos() / 1000000000L;
		  electron_gun_monitor_entry->status = kValue.Get(0);
		  electron_gun_monitor_entry->status_flags = intToBinaryString(kValue.Get(1));
		  electron_gun_monitor_entry->energy_voltage = kValue.Get(2);
		  electron_gun_monitor_entry->focus_voltage = kValue.Get(3);
		  electron_gun_monitor_entry->wehnelt_voltage = kValue.Get(4);
		  electron_gun_monitor_entry->emission_current = kValue.Get(5);
    	  electron_gun_monitor_entry->time_per_dot = kValue.Get(12);
		  electron_gun_monitor_entry->pos_x = kValue.Get(6);
		  electron_gun_monitor_entry->pos_y = kValue.Get(7);
		  electron_gun_monitor_entry->area_x = kValue.Get(8);
		  electron_gun_monitor_entry->area_y = kValue.Get(9);
		  electron_gun_monitor_entry->grid_x = kValue.Get(10);
		  electron_gun_monitor_entry->grid_y = kValue.Get(11); 
		  LOG("Parsing done");
		  if (H5PTappend(electron_gun_monitor_ptable, 1, electron_gun_monitor_entry.get()) < 0)
			LOG("Error appending entry");
		  if (H5Fflush(fid, H5F_SCOPE_GLOBAL) < 0)
			LOG("Error flushing data");
		  LOG("A electron gun monitor entry has been written");
	  } 
	break;

	default:
	  break;
  }

  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<microseconds>(stop - start);
  LOG("Writing Time: " << duration.count() << " us");
}

void InboundQueueProcessing() {
  while (!exit_flag) {
	{
	  unique_lock<mutex> iqlck(inbound_queue_mutex);
	  inbound_queue_cv.wait(iqlck, [] {
		return !inbound_queue.empty() || exit_flag;
	  });
	}

	if (!exit_flag && !inbound_queue.empty())
	  WriteData(inbound_queue.front());

	if (!exit_flag && !inbound_queue.empty()) {
	  unique_lock<mutex> iqlck(inbound_queue_mutex);

	  inbound_queue.pop();
	}
  }
}

static hid_t MakeFFTEntryType() {
  hid_t type_id_fft = H5I_INVALID_HID;
  
  if (std::any_of(fft_param.begin(), fft_param.end(), [&](const auto& param) { 
      return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); 
  })) {
    size_t data_size = sizeof(double) + sizeof(double) * FFT_FULL_SIZE;
    
    LOG("Creating FFT compound type with size: " << data_size << " bytes");
    
    type_id_fft = H5Tcreate(H5T_COMPOUND, data_size);
    if (type_id_fft < 0) {
      LOG("Error creating FFT compound type");
      return H5I_INVALID_HID;
    }
    
    if (H5Tinsert(type_id_fft, "Timestamp", 0, H5T_NATIVE_DOUBLE) < 0) {
      H5Tclose(type_id_fft);
      LOG("Error inserting timestamp in FFT type");
      return H5I_INVALID_HID;
    }
    
    hsize_t dims[1] = {static_cast<hsize_t>(FFT_FULL_SIZE)};
    hid_t array_type = H5Tarray_create(H5T_NATIVE_DOUBLE, 1, dims);
    if (array_type < 0) {
      H5Tclose(type_id_fft);
      LOG("Error creating FFT array type");
      return H5I_INVALID_HID;
    }
    
    if (H5Tinsert(type_id_fft, "FFT Full", sizeof(double), array_type) < 0) {
      H5Tclose(array_type);
      H5Tclose(type_id_fft);
      LOG("Error inserting FFT data in type");
      return H5I_INVALID_HID;
    }
    
    H5Tclose(array_type);
    LOG("FFT compound type created successfully");
  } else {
    LOG("No FFT parameters specified, skipping FFT type creation");
  }

  return type_id_fft;
}

static hid_t MakeCountsEntryType() {
  hid_t type_id_counts, data_id_counts;

  if (std::any_of(counts_param.begin(), counts_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {
	  // Create the memory data type
	  type_id_counts = H5Tcreate(H5T_COMPOUND, sizeof(CountsEntry));
	  if (type_id_counts < 0)
		return H5I_INVALID_HID;

	  // Insert timestamp in data type
	  if (H5Tinsert(type_id_counts, "Timestamp", HOFFSET(CountsEntry, timestamp_), H5T_NATIVE_DOUBLE) < 0)
		return H5I_INVALID_HID;

	  // Create and insert data array in data type
	  hsize_t size = COUNTS_SIZE;
	  //data_id_counts = H5Tarray_create(H5T_NATIVE_DOUBLE, 1, &size);
	  data_id_counts = H5Tarray_create(H5T_NATIVE_INT8, 1, &size);
	  if (H5Tinsert(type_id_counts, "Counts", HOFFSET(CountsEntry, counts), data_id_counts) < 0){
	  	H5Tclose(data_id_counts);
	  	return H5I_INVALID_HID;
	  }
  }

  return type_id_counts;
}

static hid_t MakeTwisTorrMonitorEntryType() {
  hid_t type_id_tt;
  // Insert timestamp in data type
  if (std::any_of(twistorr_param.begin(), twistorr_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {
	  // Create the memory data type
	  type_id_tt = H5Tcreate(H5T_COMPOUND, sizeof(TwisTorrMonitorEntry));
	  if (type_id_tt < 0)
		return H5I_INVALID_HID;
	  if (H5Tinsert(type_id_tt, "Timestamp", HOFFSET(TwisTorrMonitorEntry, timestamp_), H5T_NATIVE_DOUBLE) < 0){
			H5Tclose(type_id_tt);
			return H5I_INVALID_HID;
	  }
  } 
  // Insert other parameters
  for (const auto &id : data_ids) {
    if (id == "pump1_current") {
      if (H5Tinsert(type_id_tt, "305FS_Current", HOFFSET(TwisTorrMonitorEntry, pump1_current_mon), H5T_NATIVE_INT32) < 0){
      	H5Tclose(type_id_tt);
        return H5I_INVALID_HID;
      }
    } else if (id == "pump1_voltage") {
      if (H5Tinsert(type_id_tt, "305FS_Voltage", HOFFSET(TwisTorrMonitorEntry, pump1_voltage_mon), H5T_NATIVE_INT32) < 0){
      	H5Tclose(type_id_tt);
        return H5I_INVALID_HID;
      }
    } else if (id == "pump1_power") {
      if (H5Tinsert(type_id_tt, "305FS_Power", HOFFSET(TwisTorrMonitorEntry, pump1_power_mon), H5T_NATIVE_INT32) < 0){
      	H5Tclose(type_id_tt);
        return H5I_INVALID_HID;
      }
    } else if (id == "pump1_frequency") {
      if (H5Tinsert(type_id_tt, "305FS_Frequency", HOFFSET(TwisTorrMonitorEntry, pump1_frequency_mon), H5T_NATIVE_INT32) < 0){
      	H5Tclose(type_id_tt);
        return H5I_INVALID_HID;
      }
    } else if (id == "pump1_temperature") {
      if (H5Tinsert(type_id_tt, "305FS_Temperature", HOFFSET(TwisTorrMonitorEntry, pump1_temperature_mon), H5T_NATIVE_INT32) < 0){
				H5Tclose(type_id_tt);
        return H5I_INVALID_HID;
      }
    } else if (id == "pump2_current") {
      if (H5Tinsert(type_id_tt, "74FS_Current", HOFFSET(TwisTorrMonitorEntry, pump2_current_mon), H5T_NATIVE_INT32) < 0){
				H5Tclose(type_id_tt);
        return H5I_INVALID_HID;
      }
    } else if (id == "pump2_voltage") {
      if (H5Tinsert(type_id_tt, "74FS_Voltage", HOFFSET(TwisTorrMonitorEntry, pump2_voltage_mon), H5T_NATIVE_INT32) < 0){
      	H5Tclose(type_id_tt);
        return H5I_INVALID_HID;
      }
    } else if (id == "pump2_power") {
      if (H5Tinsert(type_id_tt, "74FS_Power", HOFFSET(TwisTorrMonitorEntry, pump2_power_mon), H5T_NATIVE_INT32) < 0){
      	H5Tclose(type_id_tt);
        return H5I_INVALID_HID;
      }
    } else if (id == "pump2_frequency") {
      if (H5Tinsert(type_id_tt, "74FS_Frequency", HOFFSET(TwisTorrMonitorEntry, pump2_frequency_mon), H5T_NATIVE_INT32) < 0){
      	H5Tclose(type_id_tt);
        return H5I_INVALID_HID;
      }
    } else if (id == "pump2_temperature") {
      if (H5Tinsert(type_id_tt, "74FS_Temperature", HOFFSET(TwisTorrMonitorEntry, pump2_temperature_mon), H5T_NATIVE_INT32) < 0){
				H5Tclose(type_id_tt);
        return H5I_INVALID_HID;
      }
    } else if (id == "pressure_1") {
      if (H5Tinsert(type_id_tt, "FRG-702_Pressure", HOFFSET(TwisTorrMonitorEntry, pressure_1), H5T_NATIVE_DOUBLE) < 0){
      	H5Tclose(type_id_tt);
        return H5I_INVALID_HID;
      }
    } else if (id == "pressure_2") {
      if (H5Tinsert(type_id_tt, "CDG-500_Pressure", HOFFSET(TwisTorrMonitorEntry, pressure_2), H5T_NATIVE_DOUBLE) < 0){
      	H5Tclose(type_id_tt);
        return H5I_INVALID_HID;
      }
    } else if (id == "ups_status") {
      if (H5Tinsert(type_id_tt, "UPS batteries", HOFFSET(TwisTorrMonitorEntry, ups_status), H5T_NATIVE_INT8) < 0){
      	H5Tclose(type_id_tt);
        return H5I_INVALID_HID;
      }
    }
  }

  return type_id_tt;
}

static hid_t MakeRigolMonitorEntryType() {
  hid_t type_id_rgl;
  // Insert timestamp in data type
  if (std::any_of(rigol_param.begin(), rigol_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {
	  // Create the memory data type
	  type_id_rgl = H5Tcreate(H5T_COMPOUND, sizeof(RigolMonitorEntry));
	  if (type_id_rgl < 0)
		return H5I_INVALID_HID;

    if (H5Tinsert(type_id_rgl, "Timestamp", HOFFSET(RigolMonitorEntry, timestamp_), H5T_NATIVE_DOUBLE) < 0){
    	H5Tclose(type_id_rgl);
	    return H5I_INVALID_HID;    	
    }
  } 
  // Insert other parameters
  for (const auto &id : data_ids) {

    if (id == "rigol_voltage") {
	    if (H5Tinsert(type_id_rgl, "ParticleTrap_Voltage", HOFFSET(RigolMonitorEntry, trap_voltage_mon), H5T_NATIVE_DOUBLE) < 0){
	    	H5Tclose(type_id_rgl);
		    return H5I_INVALID_HID;
	    }
    } else if (id == "rigol_voltage_offset") {
	    if (H5Tinsert(type_id_rgl, "PatricleTrap_VoltOffset", HOFFSET(RigolMonitorEntry, trap_voltage_offset_mon), H5T_NATIVE_DOUBLE) < 0){
	    	H5Tclose(type_id_rgl);
		    return H5I_INVALID_HID;
	    }
    } else if (id == "rigol_frequency") {
	    if (H5Tinsert(type_id_rgl, "PatricleTrap_Frequency", HOFFSET(RigolMonitorEntry, trap_frequency_mon), H5T_NATIVE_DOUBLE) < 0){
				H5Tclose(type_id_rgl);
		    return H5I_INVALID_HID;
	    }
    } else if (id == "rigol_function") {
            hid_t string_type = H5Tcopy(H5T_C_S1);
            H5Tset_size(string_type, H5T_VARIABLE);
            if (H5Tinsert(type_id_rgl, "PatricleTrap_Function", HOFFSET(RigolMonitorEntry, trap_function_mon), string_type) < 0) {
                H5Tclose(type_id_rgl);
                H5Tclose(string_type);
                return H5I_INVALID_HID;
            }
            H5Tclose(string_type);
    } else if (id == "rigol_status") {
	    if (H5Tinsert(type_id_rgl, "PatricleTrap_Status", HOFFSET(RigolMonitorEntry, trap_function_generator_status), H5T_NATIVE_INT8) < 0){
				H5Tclose(type_id_rgl);
		    return H5I_INVALID_HID;
	    }
    }
  }

  return type_id_rgl;
}

static hid_t MakeLaserMonitorEntryType() {
  hid_t type_id_lsr;
  // Insert timestamp in data type
  if (std::any_of(laser_param.begin(), laser_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {
	  // Create the memory data type
	  type_id_lsr = H5Tcreate(H5T_COMPOUND, sizeof(LaserMonitorEntry));
	  if (type_id_lsr < 0)
		return H5I_INVALID_HID;

  	if (H5Tinsert(type_id_lsr, "Timestamp", HOFFSET(LaserMonitorEntry, timestamp_), H5T_NATIVE_DOUBLE) < 0){
  		H5Tclose(type_id_lsr);
			return H5I_INVALID_HID; 	
  	}
  } 
  // Insert other parameters
  for (const auto &id : data_ids) {
    if (id == "laser_voltage") {
		  if (H5Tinsert(type_id_lsr, "Laser_Voltage", HOFFSET(LaserMonitorEntry, laser_voltage_mon), H5T_NATIVE_DOUBLE) < 0){
		  	H5Tclose(type_id_lsr);
				return H5I_INVALID_HID;
		  }

	    } else if (id == "laser_state") {
		  if (H5Tinsert(type_id_lsr, "Laser_state", HOFFSET(LaserMonitorEntry, laser_status_mon), H5T_NATIVE_INT8) < 0){
		  	H5Tclose(type_id_lsr);
				return H5I_INVALID_HID;
		  }
    }
  }

  return type_id_lsr;
}

static hid_t MakeElectronGunMonitorEntryType() {
  hid_t type_id_eg;
  // Insert timestamp in data type
  if (std::any_of(electron_gun_param.begin(), electron_gun_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {
	  // Create the memory data type
	  type_id_eg = H5Tcreate(H5T_COMPOUND, sizeof(ElectronGunMonitorEntry));
	  if (type_id_eg < 0)
		return H5I_INVALID_HID;

		if (H5Tinsert(type_id_eg, "Timestamp", HOFFSET(ElectronGunMonitorEntry, timestamp_), H5T_NATIVE_DOUBLE) < 0){
			H5Tclose(type_id_eg);
			return H5I_INVALID_HID; 	
		}
	}
  // Insert other parameters
  for (const auto &id : data_ids) {
      
    if (id == "status") {
		  if (H5Tinsert(type_id_eg, "Operate status", HOFFSET(ElectronGunMonitorEntry, status), H5T_NATIVE_INT8) < 0){
		  	H5Tclose(type_id_eg);
				return H5I_INVALID_HID;
		  }
    } else if (id == "status_flags") {
            hid_t string_type = H5Tcopy(H5T_C_S1);
            H5Tset_size(string_type, H5T_VARIABLE);
            if (H5Tinsert(type_id_eg, "Status_flags", HOFFSET(ElectronGunMonitorEntry, status_flags), string_type) < 0) {
                H5Tclose(type_id_eg);
                H5Tclose(string_type);
                return H5I_INVALID_HID;
            }
            H5Tclose(string_type);
    } else if (id == "energy_voltage") {
		  if (H5Tinsert(type_id_eg, "Energy_voltage", HOFFSET(ElectronGunMonitorEntry, energy_voltage), H5T_NATIVE_FLOAT) < 0){
		  	H5Tclose(type_id_eg);
				return H5I_INVALID_HID;
		  }
    } else if (id == "focus_voltage") {
	  	if (H5Tinsert(type_id_eg, "Focus_voltage", HOFFSET(ElectronGunMonitorEntry, focus_voltage), H5T_NATIVE_FLOAT) < 0){
	  		H5Tclose(type_id_eg);
				return H5I_INVALID_HID;
	  	}
    } else if (id == "wehnelt_voltage") {
		  if (H5Tinsert(type_id_eg, "Wehnelt_voltage", HOFFSET(ElectronGunMonitorEntry, wehnelt_voltage), H5T_NATIVE_FLOAT) < 0){
		  	H5Tclose(type_id_eg);
				return H5I_INVALID_HID;
		  }
    } else if (id == "emission_current") {
		  if (H5Tinsert(type_id_eg, "Emission_current", HOFFSET(ElectronGunMonitorEntry, emission_current), H5T_NATIVE_FLOAT) < 0){
		  	H5Tclose(type_id_eg);
				return H5I_INVALID_HID;
		  }
    } else if (id == "time_per_dot") {
		  if (H5Tinsert(type_id_eg, "Time_per_dot", HOFFSET(ElectronGunMonitorEntry, time_per_dot), H5T_NATIVE_INT16) < 0){
		  	H5Tclose(type_id_eg);
				return H5I_INVALID_HID;
		  }
    } else if (id == "pos_x") {
		  if (H5Tinsert(type_id_eg, "Scan_position_X", HOFFSET(ElectronGunMonitorEntry, pos_x), H5T_NATIVE_FLOAT) < 0){
		  	H5Tclose(type_id_eg);
				return H5I_INVALID_HID;
		  }
    } else if (id == "pos_y") {
		  if (H5Tinsert(type_id_eg, "Scan_position_Y", HOFFSET(ElectronGunMonitorEntry, pos_y), H5T_NATIVE_FLOAT) < 0){
		  	H5Tclose(type_id_eg);
				return H5I_INVALID_HID;
		  }
    } else if (id == "area_x") {
		  if (H5Tinsert(type_id_eg, "Scan_area_X", HOFFSET(ElectronGunMonitorEntry, area_x), H5T_NATIVE_FLOAT) < 0){
		  	H5Tclose(type_id_eg);
				return H5I_INVALID_HID;
		  }
    } else if (id == "area_y") {
		  if (H5Tinsert(type_id_eg, "Scan_area_Y", HOFFSET(ElectronGunMonitorEntry, area_y), H5T_NATIVE_FLOAT) < 0){
		  	H5Tclose(type_id_eg);
				return H5I_INVALID_HID;
		  }
    } else if (id == "grid_x") {
		  if (H5Tinsert(type_id_eg, "Scan_grid_X", HOFFSET(ElectronGunMonitorEntry, grid_x), H5T_NATIVE_FLOAT) < 0){
		  	H5Tclose(type_id_eg);
				return H5I_INVALID_HID;
		  }
    } else if (id == "grid_y") {
		  if (H5Tinsert(type_id_eg, "Scan_grid_Y", HOFFSET(ElectronGunMonitorEntry, grid_y), H5T_NATIVE_FLOAT) < 0){
		  	H5Tclose(type_id_eg);
				return H5I_INVALID_HID;
		  }
    }
  }

  return type_id_eg;
}


int OpenDataStorage() {
  hid_t plist_id;
  time_t raw_time;
  tm *timeinfo;
  char FILENAME[300];
  // Create filename
  raw_time = new_file_time - seconds_per_file;
  timeinfo = localtime(&raw_time);
  snprintf(FILENAME, sizeof(FILENAME), "%s_-_%d-%.2d-%.2d_%.2d:%.2d:%.2d.h5",
           Filename_from_GUI.c_str(),
           timeinfo->tm_year + 1900,
           timeinfo->tm_mon + 1,
           timeinfo->tm_mday,
           timeinfo->tm_hour,
           timeinfo->tm_min,
           timeinfo->tm_sec);

  // Crear directorio si no existe (de forma recursiva)
  std::string filename_str = FILENAME;
  size_t last_slash = filename_str.find_last_of('/');
  if (last_slash != std::string::npos) {
    std::string dir_path = filename_str.substr(0, last_slash);
    std::stringstream path_builder;
    std::string partial;

    for (size_t i = 0; i < dir_path.size(); ++i) {
      char c = dir_path[i];
      partial += c;

      if (c == '/' || i == dir_path.size() - 1) {
        if (!partial.empty() && access(partial.c_str(), F_OK) != 0) {
          if (mkdir(partial.c_str(), 0755) != 0 && errno != EEXIST) {
            LOG(std::string("Error creating directory: ") + partial + " (" + strerror(errno) + ")");
            return -1;
          }
        }
      }
    }
  }

  // Test file access
  if (access(FILENAME, W_OK) == 0) {
	// File exists and is writable, so open it
	fid = H5Fopen(FILENAME, H5F_ACC_RDWR, H5P_DEFAULT);
	if (fid < 0) {
	  LOG("File exists, but it could not be opened");

	  return -1;
	} else {
	  LOG("File successfully opened");

	  if (std::any_of(counts_param.begin(), counts_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {
		  counts_ptable = H5PTopen(fid, DATASET_NAME_COUNTS);
		  if (counts_ptable == H5I_BADID) {
			LOG("Error opening counts dataset");
			return -1;
		  }
		}

		if (std::any_of(fft_param.begin(), fft_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {
		  fft_ptable = H5PTopen(fid, DATASET_NAME_FFT);
		  if (fft_ptable == H5I_BADID) {
			LOG("Error opening fft dataset");
			return -1;
		  }
		}

		if (std::any_of(twistorr_param.begin(), twistorr_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {		
		  twistorr_monitor_ptable = H5PTopen(fid, DATASET_NAME_TWISTORR_MONITOR);
		  if (twistorr_monitor_ptable == H5I_BADID) {
			LOG("Error opening twistorr monitor dataset");
			return -1;
		  }
		}

		if (std::any_of(rigol_param.begin(), rigol_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {
		  rigol_monitor_ptable = H5PTopen(fid, DATASET_NAME_RIGOL_MONITOR);
		  if (rigol_monitor_ptable == H5I_BADID) {
			LOG("Error opening rigol monitor dataset");
			return -1;
		  }
		}

		if (std::any_of(laser_param.begin(), laser_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {	
		  laser_monitor_ptable = H5PTopen(fid, DATASET_NAME_LASER_MONITOR);
		  if (laser_monitor_ptable == H5I_BADID) {
			LOG("Error opening monitor dataset");
			return -1;
		  }
		}

		if (std::any_of(electron_gun_param.begin(), electron_gun_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {
		  electron_gun_monitor_ptable = H5PTopen(fid, DATASET_NAME_EG_MONITOR);
		  if (electron_gun_monitor_ptable == H5I_BADID) {
			LOG("Error opening electron gun monitor dataset");
			return -1;
		  }
		}

	}

  } else {
	if (errno == ENOENT) {
	  // File doesn't exist, so create a new one
	  LOG("File not found, creating a new one");

	  fid = H5Fcreate(FILENAME, H5F_ACC_EXCL, H5P_DEFAULT, H5P_DEFAULT);
	  if (fid < 0) {
		LOG("Error creating file");
		return -1;
	  }

	  // Create property list
	  plist_id = H5Pcreate(H5P_DATASET_CREATE);
	  H5Pset_deflate(plist_id, COMPRESSION_LEVEL); // Compression level
	  if (plist_id < 0) {
		LOG("Error setting compression algorithm");
		return -1;
	  }

	  // Create counts table
	  if (std::any_of(counts_param.begin(), counts_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {
		  counts_ptable = H5PTcreate(fid, DATASET_NAME_COUNTS, MakeCountsEntryType(), (hsize_t) CHUNK_SIZE, plist_id);
		  if (counts_ptable == H5I_BADID) {
			LOG("Error creating counts dataset inside file");
			return -1;
		  }
		}

	  // Create FFT table
	  if (std::any_of(fft_param.begin(), fft_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {
		  fft_ptable = H5PTcreate(fid, DATASET_NAME_FFT, MakeFFTEntryType(), (hsize_t) CHUNK_SIZE, plist_id);
		  if (fft_ptable == H5I_BADID) {
			LOG("Error creating FFT dataset inside file");
			return -1;
		  }
		}

	  // Create twistorr table
		if (std::any_of(twistorr_param.begin(), twistorr_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {	  
		  twistorr_monitor_ptable = H5PTcreate(fid, DATASET_NAME_TWISTORR_MONITOR, MakeTwisTorrMonitorEntryType(), (hsize_t) CHUNK_SIZE, plist_id);
		  if (twistorr_monitor_ptable == H5I_BADID) {
			LOG("Error creating twistorr monitor dataset inside file");
			return -1;
		  }
		}

	  // Create rigol table
		if (std::any_of(rigol_param.begin(), rigol_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {  
		  rigol_monitor_ptable = H5PTcreate(fid, DATASET_NAME_RIGOL_MONITOR, MakeRigolMonitorEntryType(), (hsize_t) CHUNK_SIZE, plist_id);
		  if (rigol_monitor_ptable == H5I_BADID) {
			LOG("Error creating rigol monitor dataset inside file");
			return -1;
		  }
		}  

	  // Create laser table
		if (std::any_of(laser_param.begin(), laser_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {	  
		  laser_monitor_ptable = H5PTcreate(fid, DATASET_NAME_LASER_MONITOR, MakeLaserMonitorEntryType(), (hsize_t) CHUNK_SIZE, plist_id);
		  if (laser_monitor_ptable == H5I_BADID) {
			LOG("Error creating laser monitor dataset inside file");
			return -1;
		  }
		}  

	  // Create electron gun table
		if (std::any_of(electron_gun_param.begin(), electron_gun_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {	  
		  electron_gun_monitor_ptable = H5PTcreate(fid, DATASET_NAME_EG_MONITOR, MakeElectronGunMonitorEntryType(), (hsize_t) CHUNK_SIZE, plist_id);
		  if (electron_gun_monitor_ptable == H5I_BADID) {
			LOG("Error creating electron gun monitor dataset inside file");
			return -1;
		  }	  	  	  
		}

	} else {
	  // File exists but is not writable
	  LOG("File access denied");
	  return -1;
	}
  }

  return 0;
}

int CloseDataStorage() {
  
  // Close the counts packet table
  if (std::any_of(laser_param.begin(), laser_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {
	  if (H5PTclose(counts_ptable) < 0) {
		LOG("Error closing counts dataset");
		return -1;
	  }
	}

  // Close the FFT packet table
  if (std::any_of(fft_param.begin(), fft_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {
	  if (H5PTclose(fft_ptable) < 0) {
		LOG("Error closing FFT dataset");
		return -1;
	  }
	}

  // Close the twistorr monitor packet table
	if (std::any_of(twistorr_param.begin(), twistorr_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {	  
	  if (H5PTclose(twistorr_monitor_ptable) < 0) {
		LOG("Error closing twistorr monitor dataset");
		return -1;
	  }
	}

  // Close the rigol monitor packet table
	if (std::any_of(rigol_param.begin(), rigol_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {
	  if (H5PTclose(rigol_monitor_ptable) < 0) {
		LOG("Error closing rigol monitor dataset");
		return -1;
	  }
	}

  // Close the laser monitor packet table
	if (std::any_of(laser_param.begin(), laser_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {	  
		if (H5PTclose(laser_monitor_ptable) < 0) {
		LOG("Error closing laser monitor dataset");
		return -1;
		}
	}

  // Close the electron gun monitor packet table
	if (std::any_of(electron_gun_param.begin(), electron_gun_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {	  
	  if (H5PTclose(electron_gun_monitor_ptable) < 0) {
		LOG("Error closing electron gun monitor dataset");
		return -1;
	  }
	}

  // Close the file
  if (H5Fclose(fid) < 0) {
	LOG("Error closing file");
	return -1;
  }

  return 0;
}

void ManageDataStorage() {
  while (!exit_flag) {
	{
	  unique_lock<mutex> dslck(data_storage_mutex);

	  auto next_file_time = system_clock::from_time_t(new_file_time);

	  data_storage_cv.wait_until(dslck, next_file_time, [] {
		return exit_flag;
	  });
      new_file_time += seconds_per_file;  
	  if (!exit_flag) {
		CloseDataStorage();
		OpenDataStorage();
	  }
	}
  }
}

int RunServer(void *) {
  StorageServiceImpl service;
  ServerBuilder builder;
  sigset_t set;
  int s;
  thread inbound_queue_thread, manage_data_storage_thread;

  // Log de diagnóstico
  LOG("=== Starting Storage Server ===");
  LOG("FFT_FULL_SIZE: " << FFT_FULL_SIZE);
  LOG("FFT entry estimated size: " << (sizeof(FFTEntry) + FFT_FULL_SIZE * sizeof(double)) / (1024*1024) << " MB");
  
  // Crear entradas
  if (std::any_of(counts_param.begin(), counts_param.end(), [&](const auto& param) { 
      return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); 
  })) {
    counts_entry = make_unique<CountsEntry>();
    LOG("Counts entry created");
  }
  
    if (std::any_of(fft_param.begin(), fft_param.end(), [&](const auto& param) { 
        return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); 
    })) {
      fft_entry = make_unique<FFTEntry>();
      fft_entry->fft_full_ = nullptr; 
    }
  
	if (std::any_of(twistorr_param.begin(), twistorr_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {
	  twistorr_monitor_entry = make_unique<TwisTorrMonitorEntry>();
	}
	if (std::any_of(rigol_param.begin(), rigol_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {
	  rigol_monitor_entry = make_unique<RigolMonitorEntry>();
	}
	if (std::any_of(laser_param.begin(), laser_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {
	  laser_monitor_entry = make_unique<LaserMonitorEntry>();
	}
  if (std::any_of(electron_gun_param.begin(), electron_gun_param.end(), [&](const auto& param) { return std::find(data_ids.begin(), data_ids.end(), param) != data_ids.end(); })) {
	  electron_gun_monitor_entry = make_unique<ElectronGunMonitorEntry>();
	}				
  
  // Open data storage
  if (OpenDataStorage() < 0) {
	LOG("Error opening data storage");

	return 1;
  } else {
	LOG("Data storage successfully opened");
  }

  // Create and start InboundQueueProcessing thread
  inbound_queue_thread = thread(&InboundQueueProcessing);

  // Create and start ManageDataStorage thread
  manage_data_storage_thread = thread(&ManageDataStorage);
  
  // Initialize GRPC Server
  // grpc::EnableDefaultHealthCheckService(true);
  // grpc::reflection::InitProtoReflectionServerBuilderPlugin();

  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(SERVER_ADDRESS, InsecureServerCredentials());
  builder.SetMaxReceiveMessageSize(-1);

//  // Configure channel
//  builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 300);
//  builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 300);
//  builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
//  builder.AddChannelArgument(GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA, 0);
//  builder.AddChannelArgument(GRPC_ARG_MAX_CONNECTION_IDLE_MS, 100);

  builder.RegisterService(&service);

  unique_ptr<Server> server(builder.BuildAndStart());

  LOG("Listening on " << SERVER_ADDRESS);

  // Register signal processing and wait
  // Children ignore SIGINT (Ctrl-C) signal. They exit by SIGTERM sent by the parent
  signal(SIGINT, SIG_IGN);

  // Block SIGTERM signal
  sigemptyset(&set);
  sigaddset(&set, SIGTERM);
  sigprocmask(SIG_BLOCK, &set, nullptr);

  LOG("Server up and running");

  // Wait for SIGTERM signal from parent
  sigwait(&set, &s);

  // Start shutdown procedure
  LOG("Shutting down server");

  // Stop InboundQueueProcessing thread. No more writings after this
  exit_flag = true;
  inbound_queue_cv.notify_one();
  data_storage_cv.notify_one();
  inbound_queue_thread.join();
  manage_data_storage_thread.join();
  
  // Close data storage
  if (CloseDataStorage() < 0)
	LOG("Error closing data storage");
  else
	LOG("Data storage successfully closed");

  // Terminate all reactors
  {
	unique_lock<mutex> pushlck(pusher_mutex);
	unique_lock<mutex> pulllck(puller_mutex);

	for (auto const &kPusherReactor : pusher_reactors)
	  kPusherReactor->Terminate();

	for (auto const &kPullerReactor : puller_reactors)
	  kPullerReactor->Terminate(false);
  }

  // GRPC server shutdown must be done on a separate thread to avoid hung ups (it does it sometimes anyway so let's add a timeout)
  thread shutdown_thread([&server] { server->Shutdown(chrono::system_clock::now() + chrono::milliseconds(SERVER_SHUTDOWN_TIMEOUT)); });
  shutdown_thread.join();
  server->Wait();

  return 0;
}

void HandleSignal(int) {
  LOG("Exiting");

  exit_flag = true;

  kill(pid, SIGTERM);
}

int main(int argc, char *argv[]) {
  if (argc < 4) {  // Cambiado de 3 a 4
    std::cerr << "Usage: " << argv[0] << " <Filename_from_GUI> <Seconds per file> <FFT_FULL_SIZE> <Variable_1> <Variable_2> ..." << std::endl;
    std::cerr << "Example: " << argv[0] << " experiment1 3600 5000001 apd_fft_full" << std::endl;
    return 1;
  }  

  Filename_from_GUI = argv[1];
  
  // Leer seconds_per_file
  try {
    seconds_per_file = std::stoi(argv[2]);
    std::cout << "Seconds per file: " << seconds_per_file << std::endl;
  } catch (const std::invalid_argument &e) {
    std::cerr << "Error: argv[2] no es un número válido." << std::endl;
    return 1;
  } catch (const std::out_of_range &e) {
    std::cerr << "Error: argv[2] está fuera del rango de un int." << std::endl;
    return 1;
  }

  // Leer FFT_FULL_SIZE - AÑADIR DEBUG
  std::cout << "Valor de argv[3]: '" << (argv[3] ? argv[3] : "NULL") << "'" << std::endl;
  
  try {
    FFT_FULL_SIZE = std::stoi(argv[3]); 
    std::cout << "FFT_FULL_SIZE: " << FFT_FULL_SIZE << std::endl;
  } catch (const std::invalid_argument &e) {
    std::cerr << "Error: argv[3] (FFT_FULL_SIZE) no es un número válido." << std::endl;
    return 1;
  } catch (const std::out_of_range &e) {
    std::cerr << "Error: argv[3] (FFT_FULL_SIZE) está fuera del rango." << std::endl;
    return 1;
  }

  time_t current_time = time(nullptr);
  new_file_time = current_time + seconds_per_file;  

  std::cout << "Nombre de archivo: " << Filename_from_GUI.c_str() << std::endl;
  
  // Los datos empiezan desde el argumento 4 en adelante
  data_ids.assign(argv + 4, argv + argc);  

  std::cout << "Tipos de datos especificados:" << std::endl;
  for (const auto &data_id : data_ids) {
    std::cout << "- " << data_id << std::endl;
  }
  
  // AÑADIR VERIFICACIÓN FINAL
  std::cout << "=== VERIFICACIÓN FINAL ===" << std::endl;
  std::cout << "FFT_FULL_SIZE: " << FFT_FULL_SIZE << std::endl;
  std::cout << "Número de data_ids: " << data_ids.size() << std::endl;
  
  char *stack;
  bool first_run = true;

  while (!exit_flag) {
	LOG("Starting new child process");

	stack = static_cast<char *>(mmap(nullptr, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0));

	pid = clone(RunServer, stack + STACK_SIZE, SIGCHLD, nullptr);

	// Register signal handler on first iteration
	if (first_run) {
	  signal(SIGINT, &HandleSignal);

	  first_run = false;
	}

	// Wait for clone
	waitpid(pid, nullptr, 0);

	munmap(stack, STACK_SIZE);
  }

  return 0;
}
