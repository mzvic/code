#include <grpcpp/grpcpp.h>
#include <sys/mman.h>
#include <sys/wait.h>

#include "core.grpc.pb.h"
#include "log.h"
#include "reactor/server_reactor.h"

#define SERVER_ADDRESS "0.0.0.0:50051"

#define STACK_SIZE (1024 * 1024)

using namespace core;
using namespace google::protobuf;

bool exit_flag = false;
int pid = 0;

list<ServerUpstreamReactor<Bundle, Empty> *> publisher_reactors;
list<ServerDownstreamReactor<Bundle, Interests> *> subscriber_reactors;

mutex publisher_mutex;
mutex subscriber_mutex;
mutex inbound_mutex;

void OnServerUpstreamReactorDone(void *publisher_reactor) {
  unique_lock<mutex> plck(publisher_mutex);

//  cout << "Removing Publisher Reactor" << endl;
//  LOG("Removing Publisher ");

  publisher_reactors.remove((ServerUpstreamReactor<Bundle, Empty> *) publisher_reactor);

//  cout << "Publishers count: " << publisher_reactors.size() << endl;
  LOG("Removing publisher reactor. Count: " << publisher_reactors.size());
}

//void OnServerUpstreamReactorReady(void *publisher_reactor) {
//  unique_lock<mutex> plck(publisher_mutex);
//
//  cout << "No more data from client, Publisher Reactor is ready to send data back" << endl;
//}

void OnServerDownstreamReactorDone(void *subscriber_reactor) {
  unique_lock<mutex> slck(subscriber_mutex);

//  cout << "Removing Subscriber Reactor" << endl;
//  LOG("Removing Subscriber Reactor");

  subscriber_reactors.remove((ServerDownstreamReactor<Bundle, Interests> *) subscriber_reactor);

//  cout << "Subscribers count: " << subscriber_reactors.size() << endl;
  LOG("Removing subscriber reactor. Count: " << subscriber_reactors.size());
}

void ProcessInboundMessage(const Bundle &bundle) {
  unique_lock<mutex> slck(subscriber_mutex);
  unique_lock<mutex> ilck(inbound_mutex);

// There is no need to implement a local inbound queue, processing here is minimal since they are enqueued at each reactor
  for (auto const &kSubscriberReactor : subscriber_reactors) {
// If there is no particular interest, or the explicitly defined ones, enqueue this message
	if (kSubscriberReactor->GetRequest()->types().empty() || (find(kSubscriberReactor->GetRequest()->types().begin(), kSubscriberReactor->GetRequest()->types().end(), bundle.type()) != kSubscriberReactor->GetRequest()->types().end()))
	  kSubscriberReactor->EnqueueOutboundMessage(bundle);
  }
}

class BrokerServiceImpl final : public Broker::CallbackService {
 public:
  __attribute__((unused)) ServerReadReactor<Bundle> *Publish(CallbackServerContext *context, Empty *response) override {
	unique_lock<mutex> plck(publisher_mutex);

//	cout << "Creating new Publisher Reactor" << endl;
//	LOG("Creating new publisher");

	publisher_reactors.push_back(new ServerUpstreamReactor<Bundle, Empty>(response));

	publisher_reactors.back()->SetInboundCallback(&ProcessInboundMessage);
//	publisher_reactors.back()->SetReadyCallback(&OnServerUpstreamReactorReady);
	publisher_reactors.back()->SetDoneCallback(&OnServerUpstreamReactorDone);

//	cout << "Publishers count: " << publisher_reactors.size() << endl;
	LOG("Creating new publisher reactor. Count: " << publisher_reactors.size());

	return publisher_reactors.back();
  }

  __attribute__((unused)) ServerWriteReactor<Bundle> *Subscribe(CallbackServerContext *context, const Interests *request) override {
	unique_lock<mutex> slck(subscriber_mutex);

//	cout << "Creating new Subscriber Reactor ";
//	LOG("Creating new Subscriber Reactor");

//	if (request->types().empty()) {
//	  cout << "with no interests. Sending all messages" << endl;
//	} else {
//	  cout << "with interests:";
//	  for (const auto &kElem : request->types())
//		cout << " " << kElem;
//	  cout << endl;
//	}

	subscriber_reactors.push_back(new ServerDownstreamReactor<Bundle, Interests>(request));

	subscriber_reactors.back()->SetDoneCallback(&OnServerDownstreamReactorDone);

//	cout << "Subscribers count: " << subscriber_reactors.size() << endl;
	LOG("Creating new subscriber reactor. Count: " << subscriber_reactors.size());

	return subscriber_reactors.back();
  }
};

int RunServer(void *) {
  BrokerServiceImpl service;
  ServerBuilder builder;

  // grpc::EnableDefaultHealthCheckService(true);
  // grpc::reflection::InitProtoReflectionServerBuilderPlugin();

  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(SERVER_ADDRESS, grpc::InsecureServerCredentials());
  builder.SetMaxReceiveMessageSize(-1);

  // Configure channel
  //	builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 300);
  //	builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 300);
  //	builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);

  builder.RegisterService(&service);

  unique_ptr<Server> server(builder.BuildAndStart());

//  cout << "Server listening on " << SERVER_ADDRESS << endl;
  LOG("Listening on " << SERVER_ADDRESS);

  server->Wait();

  return 0;
}

void HandleSignal(int) {
  if (pid == 0)        // Children ignore signals
	return;

//  cout << "Exiting" << endl;
  LOG("Exiting");

  exit_flag = true;

  kill(pid, SIGTERM);
}
int main() {
  char *stack;

  signal(SIGINT, HandleSignal);

  while (!exit_flag) {
//	cout << "Starting new child process" << endl;
	LOG("Starting new child process");

	stack = static_cast<char *>(mmap(nullptr, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0));

	pid = clone(RunServer, stack + STACK_SIZE, SIGCHLD, nullptr);

	waitpid(pid, nullptr, 0);

	munmap(stack, STACK_SIZE);
  }

  return 0;
}
