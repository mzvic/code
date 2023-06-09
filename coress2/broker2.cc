#include <grpcpp/grpcpp.h>

#include "core.grpc.pb.h"
#include "reactor/reactor.h"

using namespace reactor;
using namespace core;

using google::protobuf::Empty;
using grpc::CallbackServerContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerReadReactor;
using grpc::ServerWriteReactor;
using std::list;
using std::mutex;
using std::shared_ptr;

list<SubscriberServerReactor *> subscriber_reactors;
list<PublisherServerReactor *> publisher_reactors;

mutex publisher_mutex;
mutex subscriber_mutex;
mutex queue_mutex;

class BrokerServiceImpl final : public Broker::CallbackService, public ServerReactorInterface {
 public:
  ServerReadReactor<Bundle> *Publish(CallbackServerContext *context, Empty *reply) override {
	unique_lock<mutex> lck(publisher_mutex);

	cout << "Creating new Publisher Reactor" << endl;

	publisher_reactors.push_back(new PublisherServerReactor(this));

	cout << "Publishers count: " << publisher_reactors.size() << endl;

	return publisher_reactors.back();
  }

  ServerWriteReactor<Bundle> *Subscribe(CallbackServerContext *context, const Interests *request) override {
	unique_lock<mutex> lck(subscriber_mutex);

	cout << "Creating new Subscriber Reactor ";

	if (request->types().empty()) {
	  cout << "with no interests. Sending all messages" << endl;
	} else {
	  cout << "with interests:";
	  for (const auto &elem : request->types())
		cout << " " << elem;
	  cout << endl;
	}

	subscriber_reactors.push_back(new SubscriberServerReactor(this, request));

	cout << "Subscribers count: " << subscriber_reactors.size() << endl;

	return subscriber_reactors.back();
  }

  void ProcessMessage(Bundle &bundle) override {
	unique_lock<mutex> slck(subscriber_mutex);
	unique_lock<mutex> qlck(queue_mutex);

//	cout << "Processing a bundle" << endl;

	for (auto const &kSubscriberreactor : subscriber_reactors)
	  kSubscriberreactor->EnqueueMessage(bundle);
  }

  void OnPublisherServerReactorFinish(void *publisher_server_reactor) override {
	unique_lock<mutex> lck(publisher_mutex);

	cout << "Removing Publisher Reactor" << endl;

	publisher_reactors.remove((PublisherServerReactor *) publisher_server_reactor);

	cout << "Publishers count: " << publisher_reactors.size() << endl;
  }

  void OnSubscriberServerReactorFinish(void *subscriber_server_reactor) override {
	unique_lock<mutex> lck(subscriber_mutex);

	cout << "Removing Subscriber Reactor" << endl;

	subscriber_reactors.remove((SubscriberServerReactor *) subscriber_server_reactor);

	cout << "Subscribers count: " << subscriber_reactors.size() << endl;
  }
};

void RunServer() {
  BrokerServiceImpl service;

  string server_address("0.0.0.0:50051");

  // grpc::EnableDefaultHealthCheckService(true);
  // grpc::reflection::InitProtoReflectionServerBuilderPlugin();

  ServerBuilder builder;

  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

  // Configure channel
  //	builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 300);
  //	builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 300);
  //	builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);

  builder.RegisterService(&service);

  unique_ptr<Server> server(builder.BuildAndStart());

  cout << "Server listening on " << server_address << endl;

  server->Wait();
}

int main() {
  RunServer();

  return 0;
}
