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

#define STACK_SIZE (1024 * 1024)
#define FILENAME "storage.h5"
#define COMPRESSION_LEVEL 9    // A number between 0 (none, fastest) to 9 (best, slowest)
#define DATASET_NAME "DATA"
#define FFT_FULL_SIZE 5000001
//#define FFT_FULL_SIZE 500001

using namespace core;
using namespace google::protobuf;
using namespace std::chrono;

typedef struct {
  double timestamp_;
  double fft_full_[FFT_FULL_SIZE];
//  char name[16];
//  int lati;
//  int longi;
//  float pressure;
//  double temperature;
} Entry;

//static Entry testEntries[8] = {{22, 3212, 1233, 2.5656, 5244.356546464},
//							   {34, 10, 10, 1.0F, 10.0},
//							   {324523, 20, 20, 2.0F, 20.0},
//							   {41212, 30, 30, 3.0F, 30.0},
//							   {21355, 40, 40, 4.0F, 40.0},
//							   {345345, 50, 50, 5.0F, 50.0},
//							   {78978, 60, 60, 6.0F, 60.0},
//							   {50345, 70, 70, 7.0F, 70.0}};

bool exit_flag = false;
int pid = 0;

unique_ptr<Entry> entry;

list<ServerUpstreamReactor<Bundle, Empty> *> pusher_reactors;
list<ServerDownstreamReactor<Bundle, Query> *> puller_reactors;

queue<Bundle> inbound_queue;

mutex pusher_mutex;
mutex puller_mutex;
mutex inbound_queue_mutex;
condition_variable inbound_queue_cv;
thread inbound_queue_thread;

hid_t fid, ptable;

void OnServerUpstreamReactorDone(void *pusher_reactor) {
  unique_lock<mutex> plck(pusher_mutex);

//  cout << "Removing Publisher Reactor" << endl;
//  LOG("Removing Publisher ");

  pusher_reactors.remove((ServerUpstreamReactor<Bundle, Empty> *) pusher_reactor);

//  cout << "Publishers count: " << publisher_reactors.size() << endl;
  LOG("Removing pusher reactor. Count: " << pusher_reactors.size());
}

//void OnServerUpstreamReactorReady(void *publisher_reactor) {
//  unique_lock<mutex> plck(publisher_mutex);
//
//  cout << "No more data from client, Publisher Reactor is ready to send data back" << endl;
//}

void OnServerDownstreamReactorDone(void *puller_reactor) {
  unique_lock<mutex> plck(puller_mutex);

//  cout << "Removing Subscriber Reactor" << endl;
//  LOG("Removing Subscriber Reactor");

  puller_reactors.remove((ServerDownstreamReactor<Bundle, Query> *) puller_reactor);

//  cout << "Subscribers count: " << subscriber_reactors.size() << endl;
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
//	ServerUpstreamReactor<Bundle, Empty> *server_upstream_reactor;

//	cout << "Creating new Pusher Reactor" << endl;

//	pusher_reactors.push_back(new PusherReactor(this));

//	cout << "Pushers count: " << pusher_reactors.size() << endl;

//	return pusher_reactors.back();


//	server_upstream_reactor = new ServerUpstreamReactor<Bundle, Empty>(empty);
	pusher_reactors.push_back(new ServerUpstreamReactor<Bundle, Empty>(empty));

//	server_upstream_reactor->SetInboundCallback(&ProcessInboundMessage);
	pusher_reactors.back()->SetInboundCallback(&ProcessInboundBundle);

	pusher_reactors.back()->SetDoneCallback(&OnServerUpstreamReactorDone);

	LOG("Creating new pusher reactor. Count: " << pusher_reactors.size());

//	return server_upstream_reactor;
	return pusher_reactors.back();
  }

  __attribute__((unused)) ServerWriteReactor<Bundle> *Pull(CallbackServerContext *context, const Query *query) override {
	unique_lock<mutex> pck(puller_mutex);
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
	puller_reactors.push_back(new ServerDownstreamReactor<Bundle, Query>(query));

	puller_reactors.back()->SetDoneCallback(&OnServerDownstreamReactorDone);
//
//	cout << "Pullers count: " << puller_reactors.size() << endl;
//
//	return puller_reactors.back();

	LOG("Creating new puller reactor. Count: " << puller_reactors.size());

//	return new ServerDownstreamReactor<Bundle, Query>(query);
	return puller_reactors.back();
  }
};

void WriteData(Bundle &bundle) {
  auto start = high_resolution_clock::now();

  const auto &kValue = bundle.value();

  switch (bundle.type()) {
	case STORAGE_RECORD:
	  // Here we construct the storage entry
	  LOG("Writing record");

	  // Create entry
	  entry->timestamp_ = (double) bundle.timestamp().seconds() + (double) bundle.timestamp().nanos() / 1000000000L;

	  for (int i = 0; i < FFT_FULL_SIZE; i++)
		entry->fft_full_[i] = kValue.Get(i);

	  LOG("Parsing done");

	  if (H5PTappend(ptable, 1, entry.get()) < 0) {
		LOG("Error appending entry");
	  }

	  LOG("An entry has been written");

	  break;

	default:
	  break;
  }

  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<microseconds>(stop - start);
  cout << "Writing Time: " << duration.count() << " us" << endl;
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

static hid_t MakeEntryType() {
  hid_t type_id, data_id;

  // Create the memory data type
  type_id = H5Tcreate(H5T_COMPOUND, sizeof(Entry));
  if (type_id < 0)
	return H5I_INVALID_HID;

  // Insert timestamp in data type
  if (H5Tinsert(type_id, "Timestamp", HOFFSET(Entry, timestamp_), H5T_NATIVE_DOUBLE) < 0)
	return H5I_INVALID_HID;

  // Create and insert data array in data type
  hsize_t size = FFT_FULL_SIZE;
  data_id = H5Tarray_create(H5T_NATIVE_DOUBLE, 1, &size);
  if (H5Tinsert(type_id, "FFTFull", HOFFSET(Entry, fft_full_), data_id) < 0)
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
	  part_t = MakeEntryType();
	  ptable = H5PTcreate(fid, DATASET_NAME, part_t, (hsize_t) 10, plist_id);
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

//  if (H5PTappend(ptable, 1, &(testEntries[0])) < 0) {
//	cout << "Error appending 1 entry" << endl;
//
//	return -1;
//  }
//
//  if (H5PTappend(ptable, 3, &(testEntries[1])) < 0) {
//	cout << "Error appending 3 entries" << endl;
//
//	return -1;
//  }

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
  sigset_t set;
  int s;

  // Create this big entry in child memory
  entry = make_unique<Entry>();

  // Open data storage
  if (OpenDataStorage() < 0) {
	LOG("Error opening data storage");

	return 1;
  } else {
	LOG("Data storage successfully opened");
  }

  // Create and start InboundQueueProcessing thread
  inbound_queue_thread = std::thread(&InboundQueueProcessing);

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

  exit_flag = true;

  // Terminate all reactors
  {
	unique_lock<mutex> pushlck(pusher_mutex);
	unique_lock<mutex> pulllck(puller_mutex);

	for (auto const &kPusherReactor : pusher_reactors)
	  kPusherReactor->Terminate();

	for (auto const &kPullerReactor : puller_reactors)
	  kPullerReactor->Terminate(false);
  }

  // GRPC server shutdown must be done on a separate thread to avoid hung ups
  thread shutdown_thread([&server] { server->Shutdown(); });
  shutdown_thread.join();
  server->Wait();

  // Stop InboundQueueProcessing thread. It will see the exit_flag
  inbound_queue_cv.notify_one();
  inbound_queue_thread.join();

  // Close data storage
  if (CloseDataStorage() < 0) {
	LOG("Error closing data storage");

	return 1;
  } else {
	LOG("Data storage successfully closed");

	return 0;
  }
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
