#include <condition_variable>
#include <thread>
#include <grpcpp/grpcpp.h>
#include <sys/mman.h>
#include <sys/wait.h>

#include "core.grpc.pb.h"
#include "hdf5_hl.h"
#include "log.h"
#include "reactor/server_reactor.h"

#define SERVER_ADDRESS "0.0.0.0:50052"
#define SERVER_SHUTDOWN_TIMEOUT 1000

#define STACK_SIZE (1024 * 1024)
#define FILENAME "storage.h5"
#define COMPRESSION_LEVEL 7   // A number between 0 (none, fastest) to 9 (best, slowest)
#define CHUNK_SIZE 5    // Number of rows per chunk. It progressively impacts the compression time
#define DATASET_NAME_FFT "FFT"
#define DATASET_NAME_MONITOR "Monitor"

#define FFT_FULL_SIZE 5000001

using namespace core;
using namespace google::protobuf;
using namespace std::chrono;

typedef struct {
  double timestamp_;
  double fft_full_[FFT_FULL_SIZE];
} FFTEntry;

typedef struct {
  double timestamp_;
  double var1_;
  int var2_;
} MonitorEntry;

bool exit_flag = false;
int pid = 0;

unique_ptr<FFTEntry> fft_entry;
unique_ptr<MonitorEntry> monitor_entry;

list<ServerUpstreamReactor<Bundle, Empty> *> pusher_reactors;
list<ServerDownstreamReactor<Bundle, Query> *> puller_reactors;

queue<Bundle> inbound_queue;

mutex pusher_mutex;
mutex puller_mutex;
mutex inbound_queue_mutex;
condition_variable inbound_queue_cv;

hid_t fid, fft_ptable, monitor_ptable;

void OnServerUpstreamReactorDone(void *pusher_reactor) {
  unique_lock<mutex> plck(pusher_mutex);

  pusher_reactors.remove((ServerUpstreamReactor<Bundle, Empty> *) pusher_reactor);

  LOG("Removing pusher reactor. Count: " << pusher_reactors.size());
}

//void OnServerUpstreamReactorReady(void *publisher_reactor) {
//  unique_lock<mutex> plck(publisher_mutex);
//
//  cout << "No more data from client, Publisher Reactor is ready to send data back" << endl;
//}

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

void WriteData(const Bundle &bundle) {
  auto start = high_resolution_clock::now();

  const auto &kValue = bundle.value();

  switch (bundle.type()) {
	case STORAGE_RECORD_FFT:
	  // Here we construct the storage entry
	  LOG("Writing FFT record");

	  // Create entry
	  fft_entry->timestamp_ = (double) bundle.timestamp().seconds() + (double) bundle.timestamp().nanos() / 1000000000L;

	  for (int i = 0; i < FFT_FULL_SIZE; i++)
		fft_entry->fft_full_[i] = kValue.Get(i);

	  LOG("Parsing done");

	  if (H5PTappend(fft_ptable, 1, fft_entry.get()) < 0)
		LOG("Error appending entry");

	  if (H5Fflush(fid, H5F_SCOPE_GLOBAL) < 0)
		LOG("Error flushing data");

	  LOG("An FFT entry has been written");

	  break;

	case STORAGE_RECORD_MONITOR:
	  // Here we construct the storage entry
	  LOG("Writing Monitor record");

	  // Create entry
	  monitor_entry->timestamp_ = (double) bundle.timestamp().seconds() + (double) bundle.timestamp().nanos() / 1000000000L;

	  monitor_entry->var1_ = kValue.Get(0);

	  monitor_entry->var2_ = (int) kValue.Get(1);

	  LOG("Parsing done");

	  if (H5PTappend(monitor_ptable, 1, monitor_entry.get()) < 0)
		LOG("Error appending entry");

	  if (H5Fflush(fid, H5F_SCOPE_GLOBAL) < 0)
		LOG("Error flushing data");

	  LOG("A Monitor entry has been written");

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
  hid_t type_id, data_id;

  // Create the memory data type
  type_id = H5Tcreate(H5T_COMPOUND, sizeof(FFTEntry));
  if (type_id < 0)
	return H5I_INVALID_HID;

  // Insert timestamp in data type
  if (H5Tinsert(type_id, "Timestamp", HOFFSET(FFTEntry, timestamp_), H5T_NATIVE_DOUBLE) < 0)
	return H5I_INVALID_HID;

  // Create and insert data array in data type
  hsize_t size = FFT_FULL_SIZE;
  data_id = H5Tarray_create(H5T_NATIVE_DOUBLE, 1, &size);
  if (H5Tinsert(type_id, "FFT Full", HOFFSET(FFTEntry, fft_full_), data_id) < 0)
	return H5I_INVALID_HID;

  return type_id;
}

static hid_t MakeMonitorEntryType() {
  hid_t type_id;

  // Create the memory data type
  type_id = H5Tcreate(H5T_COMPOUND, sizeof(MonitorEntry));
  if (type_id < 0)
	return H5I_INVALID_HID;

  // Insert timestamp in data type
  if (H5Tinsert(type_id, "Timestamp", HOFFSET(MonitorEntry, timestamp_), H5T_NATIVE_DOUBLE) < 0)
	return H5I_INVALID_HID;

  // Insert VAR1
  if (H5Tinsert(type_id, "Var1", HOFFSET(MonitorEntry, var1_), H5T_NATIVE_DOUBLE) < 0)
	return H5I_INVALID_HID;

  // Insert VAR2
  if (H5Tinsert(type_id, "Var2", HOFFSET(MonitorEntry, var2_), H5T_NATIVE_INT) < 0)
	return H5I_INVALID_HID;

  return type_id;
}

int OpenDataStorage() {
  hid_t plist_id;

  // Test file access
  if (access(FILENAME, W_OK) == 0) {
	// File exists and is writable, so open it
	fid = H5Fopen(FILENAME, H5F_ACC_RDWR, H5P_DEFAULT);
	if (fid < 0) {
	  LOG("File exists, but it could not be opened");

	  return -1;
	} else {
	  LOG("File successfully opened");

	  fft_ptable = H5PTopen(fid, DATASET_NAME_FFT);
	  if (fft_ptable == H5I_BADID) {
		LOG("Error opening fft dataset");

		return -1;
	  }

	  monitor_ptable = H5PTopen(fid, DATASET_NAME_MONITOR);
	  if (monitor_ptable == H5I_BADID) {
		LOG("Error opening monitor dataset");

		return -1;
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

	  // Create FFT table
	  fft_ptable = H5PTcreate(fid, DATASET_NAME_FFT, MakeFFTEntryType(), (hsize_t) CHUNK_SIZE, plist_id);
	  if (fft_ptable == H5I_BADID) {
		LOG("Error creating FFT dataset inside file");

		return -1;
	  }

	  monitor_ptable = H5PTcreate(fid, DATASET_NAME_MONITOR, MakeMonitorEntryType(), (hsize_t) CHUNK_SIZE, plist_id);
	  if (fft_ptable == H5I_BADID) {
		LOG("Error creating Monitor dataset inside file");

		return -1;
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
  // Close the FFT packet table
  if (H5PTclose(fft_ptable) < 0) {
	LOG("Error closing FFT dataset");

	return -1;
  }

  // Close the Monitor packet table
  if (H5PTclose(monitor_ptable) < 0) {
	LOG("Error closing Monitor dataset");

	return -1;
  }

  // Close the file
  if (H5Fclose(fid) < 0) {
	LOG("Error closing file");

	return -1;
  }

  return 0;
}

int RunServer(void *) {
  StorageServiceImpl service;
  ServerBuilder builder;
  sigset_t set;
  int s;
  thread inbound_queue_thread;

  // Create these big entries in child memory
  fft_entry = make_unique<FFTEntry>();
  monitor_entry = make_unique<MonitorEntry>();

  // Open data storage
  if (OpenDataStorage() < 0) {
	LOG("Error opening data storage");

	return 1;
  } else {
	LOG("Data storage successfully opened");
  }

  // Create and start InboundQueueProcessing thread
  inbound_queue_thread = thread(&InboundQueueProcessing);

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
  inbound_queue_thread.join();

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

int main() {
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
