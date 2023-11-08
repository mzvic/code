#include <grpcpp/grpcpp.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <condition_variable>
#include <thread>

#include "core.grpc.pb.h"
#include "hdf5_hl.h"
#include "log.h"
#include "reactor/server_reactor.h"

#define SERVER_ADDRESS "0.0.0.0:50052"

#define STACK_SIZE (1024 * 1024)
#define FILENAME "storage.h5"
#define COMPRESSION_LEVEL 9
#define DATASET_NAME "DATA"
#define DATA_SIZE 20

using namespace core;
using namespace google::protobuf;
//using namespace grpc;
//using namespace std;

typedef struct {
  long timestamp_;
  double data_[DATA_SIZE];
//  char name[16];
//  int lati;
//  int longi;
//  float pressure;
//  double temperature;
} Entry;

static Entry testEntries[8] = {{22, 3212, 1233, 2.5656, 5244.356546464},
							   {34, 10, 10, 1.0F, 10.0},
							   {324523, 20, 20, 2.0F, 20.0},
							   {41212, 30, 30, 3.0F, 30.0},
							   {21355, 40, 40, 4.0F, 40.0},
							   {345345, 50, 50, 5.0F, 50.0},
							   {78978, 60, 60, 6.0F, 60.0},
							   {50345, 70, 70, 7.0F, 70.0}};

bool exit_flag = false;
int pid = 0;

//list<PullerReactor *> puller_reactors;
//list<PusherReactor *> pusher_reactors;

queue<Bundle> inbound_queue;

//mutex pusher_mutex;
//mutex puller_mutex;
mutex inbound_queue_mutex;
condition_variable inbound_queue_cv;
thread inbound_queue_thread;

hid_t fid, ptable;

void ProcessInboundMessage(const Bundle &bundle) {
  unique_lock<mutex> iqlck(inbound_queue_mutex);

//	cout << "Processing a bundle" << endl;

  inbound_queue.push(bundle);

//  cout << "T:" << this_thread::get_id() << " Enqueueing. Queue size: " << queue_.size() << endl;

// Notify reading thread of this new element
  if (inbound_queue.size() == 1)
	inbound_queue_cv.notify_one();
}

//class StorageServiceImpl final : public Storage::CallbackService, public ServerUpstreamReactorInterface<Bundle>, public ServerDownstreamReactorInterface {
class StorageServiceImpl final : public Storage::CallbackService {
 public:
  __attribute__((unused)) ServerReadReactor<Bundle> *Push(CallbackServerContext *context, Empty *empty) override {
//	unique_lock<mutex> lck(pusher_mutex);
	ServerUpstreamReactor<Bundle, Empty> *server_upstream_reactor;

//	cout << "Creating new Pusher Reactor" << endl;

//	pusher_reactors.push_back(new PusherReactor(this));

//	cout << "Pushers count: " << pusher_reactors.size() << endl;

//	return pusher_reactors.back();

	server_upstream_reactor = new ServerUpstreamReactor<Bundle, Empty>(empty);

	server_upstream_reactor->SetInboundCallback(&ProcessInboundMessage);

	LOG("Creating new pusher reactor");

	return server_upstream_reactor;
  }

  __attribute__((unused)) ServerWriteReactor<Bundle> *Pull(CallbackServerContext *context, const Query *query) override {
//	unique_lock<mutex> lck(puller_mutex);


//	if (query->types().empty()) {
//	  cout << "with no interests. Sending all messages" << endl;
//	} else {
//	  cout << "with interests:";
//	  for (const auto &kElem : query->types())
//		cout << " " << kElem;
//	  cout << endl;
//	}

//	puller_reactors.push_back(new PullerReactor(this, query));
//
//	cout << "Pullers count: " << puller_reactors.size() << endl;
//
//	return puller_reactors.back();

	LOG("Creating new puller reactor");

	return new ServerDownstreamReactor<Bundle, Query>(query);
  }
};

void WriteData(Bundle &bundle) {
  switch (bundle.type()) {
	case STORAGE_FFT_FULL:

	  break;
  }
}

void InboundQueueProcessing() {
  while (!exit_flag) {
	{
	  unique_lock<mutex> iqlck(inbound_queue_mutex);

	  inbound_queue_cv.wait(iqlck, [] {
//		cout << "T:" << this_thread::get_id() << " Checking inbound_queue size: " << queue_.size() << endl;
//		return (!queue_.empty() || (state_ != RUNNING));
		return !inbound_queue.empty() || exit_flag;
	  });
	}

//	if ((state_ == RUNNING) && !queue_.empty())
	if (!exit_flag && !inbound_queue.empty()) {
//	  Process It!
	  WriteData(inbound_queue.front());
	}

//	if ((state_ == RUNNING) && !queue_.empty()) {
	if (!exit_flag && !inbound_queue.empty()) {
	  unique_lock<mutex> iqlck(inbound_queue_mutex);

	  inbound_queue.pop();
	}
  }
}

static hid_t MakeParticleType() {
  hid_t type_id, data_id;

  // Create the memory data type
  type_id = H5Tcreate(H5T_COMPOUND, sizeof(Entry));
  if (type_id < 0)
	return H5I_INVALID_HID;

  // Insert timestamp in data type
  if (H5Tinsert(type_id, "Timestamp", HOFFSET(Entry, timestamp_), H5T_NATIVE_LONG) < 0)
	return H5I_INVALID_HID;

  // Create and insert data array in data type
  hsize_t size = DATA_SIZE;
  data_id = H5Tarray_create(H5T_NATIVE_DOUBLE, 1, &size);
  if (H5Tinsert(type_id, "Data", HOFFSET(Entry, data_), data_id) < 0)
	return H5I_INVALID_HID;

  return type_id;
}

int OpenDataStorage() {
  hid_t plist_id, part_t;

  // Test file access
  if (access(FILENAME, W_OK) == 0) {
	// File exists and is writable, so open it
	fid = H5Fopen(FILENAME, H5F_ACC_RDWR, H5P_DEFAULT);
	if (fid < 0) {
	  LOG("File exists, but it could not be opened");

	  return -1;
	} else {
	  LOG("File successfully opened");

	  ptable = H5PTopen(fid, DATASET_NAME);
	  if (ptable == H5I_BADID) {
		LOG("Error opening dataset");

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

	  // Create property list of the table
	  plist_id = H5Pcreate(H5P_DATASET_CREATE);
	  H5Pset_deflate(plist_id, COMPRESSION_LEVEL); // Compression level
	  if (plist_id < 0) {
		LOG("Error setting compression algorithm");

		return -1;
	  }

	  // Create table
	  part_t = MakeParticleType();
	  ptable = H5PTcreate(fid, DATASET_NAME, part_t, (hsize_t) 100, plist_id);
	  if (ptable == H5I_BADID) {
		LOG("Error creating table inside file");

		return -1;
	  }
	} else {
	  // File exists but is not writable
	  LOG("File access denied");

	  return -1;
	}
  }

//  // Open HDF5 file
//  fid = H5Fopen(FILENAME, H5F_ACC_RDWR, H5P_DEFAULT);
//  if (fid < 0) {
//	cout << "File not found, creating a new one" << endl;
//
//	fid = H5Fcreate(FILENAME, H5F_ACC_EXCL, H5P_DEFAULT, H5P_DEFAULT);
//	if (fid < 0) {
//	  cout << "Error creating file" << endl;
//
//	  return 1;
//	}
//
//	// Create property list of the table
//	plist_id = H5Pcreate(H5P_DATASET_CREATE);
//	H5Pset_deflate(plist_id, COMPRESSION_LEVEL); // Compression level
//	if (plist_id < 0) {
//	  cout << "Error setting compression algorithm" << endl;
//
//	  return 1;
//	}
//
//	// Create table
//	part_t = MakeParticleType();
//	ptable = H5PTcreate(fid, DATASET_NAME, part_t, (hsize_t) 100, plist_id);
//	if (ptable == H5I_BADID) {
//	  cout << "Error creating table inside file" << endl;
//
//	  return 1;
//	}
//  } else {
//	cout << "File opened successfully" << endl;
//
//	ptable = H5PTopen(fid, DATASET_NAME);
//	if (ptable == H5I_BADID) {
//	  cout << "Error opening dataset" << endl;
//
//	  return 1;
//	}
//  }

  if (H5PTappend(ptable, 1, &(testEntries[0])) < 0) {
	cout << "Error appending 1 entry" << endl;

	return -1;
  }

  if (H5PTappend(ptable, 3, &(testEntries[1])) < 0) {
	cout << "Error appending 3 entries" << endl;

	return -1;
  }

//  /* Write one packet to the packet table */
//  err = H5PTappend(ptable, (hsize_t) 1, &(writeBuffer[0]));
//  if (err < 0)
//	goto out;
//
//  /* Write several packets to the packet table */
//  err = H5PTappend(ptable, (hsize_t) 4, &(writeBuffer[1]));
//  if (err < 0)
//	goto out;
//
  /* Get the number of packets in the packet table.  This should be five. */
//  hsize_t count;
//  H5PTget_num_packets(ptable, &count);
//
//  printf("Number of packets in packet table after five appends: %d\n", (int) count);

//
//  /* Initialize packet table's "current record" */
//  err = H5PTcreate_index(ptable);
//  if (err < 0)
//	goto out;
//
//  /* Iterate through packets, read each one back */
//  for (x = 0; x < 5; x++) {
//	err = H5PTget_next(ptable, (hsize_t) 1, &(readBuffer[x]));
//	if (err < 0)
//	  goto out;
//
//	printf("Packet %d's value is %d\n", x, readBuffer[x]);
//  }

  return 0;
}

int CloseDataStorage() {
  // Close the packet table
  if (H5PTclose(ptable) < 0) {
	LOG("Error closing table");

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

  if (OpenDataStorage() < 0) {
	LOG("Error opening data storage");

	return 1;
  }

//  string server_address(SERVER_ADDRESS);

  inbound_queue_thread = std::thread(&InboundQueueProcessing);

  // grpc::EnableDefaultHealthCheckService(true);
  // grpc::reflection::InitProtoReflectionServerBuilderPlugin();

  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(SERVER_ADDRESS, InsecureServerCredentials());
  builder.SetMaxReceiveMessageSize(-1);

  // Configure channel
  //	builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 300);
  //	builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 300);
  //	builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);

  builder.RegisterService(&service);

  unique_ptr<Server> server(builder.BuildAndStart());

  LOG("Listening on " << SERVER_ADDRESS);

  server->Wait();

  inbound_queue_thread.join();

  if (CloseDataStorage() < 0) {
	LOG("Error closing data storage");

	return 1;
  }

  return 0;
}

void HandleSignal(int) {
  if (pid == 0)        // Children ignore signals
	return;

  LOG("Exiting");

  exit_flag = true;

  inbound_queue_cv.notify_one();

  kill(pid, SIGTERM);
}
int main() {
  char *stack;

  signal(SIGINT, HandleSignal);

  while (!exit_flag) {
	LOG("Starting new child process");

	stack = static_cast<char *>(mmap(nullptr, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0));

	pid = clone(RunServer, stack + STACK_SIZE, SIGCHLD, nullptr);

	waitpid(pid, nullptr, 0);

	munmap(stack, STACK_SIZE);
  }

  return 0;
}
