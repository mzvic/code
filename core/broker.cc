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
		std::unique_lock<std::mutex> lck(publisher_mutex);

		std::cout << "Creating new Publisher Reactor" << std::endl;

		publisher_reactors.push_back(new PublisherServerReactor(this));

		std::cout << "Publishers count: " << publisher_reactors.size() << std::endl;

		return publisher_reactors.back();
	}

	ServerWriteReactor<Bundle> *Subscribe(CallbackServerContext *context, const Empty *request) override {
		std::unique_lock<std::mutex> lck(subscriber_mutex);

		std::cout << "Creating new Subscriber Reactor" << std::endl;

		subscriber_reactors.push_back(new SubscriberServerReactor(this));

		std::cout << "Subscribers count: " << subscriber_reactors.size() << std::endl;

		return subscriber_reactors.back();
	}

	void ProcessMessage(Bundle &bundle) override {
		std::unique_lock<std::mutex> slck(subscriber_mutex);
		std::unique_lock<std::mutex> qlck(queue_mutex);

		std::cout << "Processing a bundle" << std::endl;

		for (auto const &subscriberreactor: subscriber_reactors)
			subscriberreactor->EnqueueMessage(bundle);
	}

	void OnPublisherServerReactorFinish(void *pPublisherServerReactor) override {
		std::unique_lock<std::mutex> lck(publisher_mutex);

		std::cout << "Removing Publisher Reactor" << std::endl;

		publisher_reactors.remove((PublisherServerReactor *) pPublisherServerReactor);

		std::cout << "Publishers count: " << publisher_reactors.size() << std::endl;
	}

	void OnSubscriberServerReactorFinish(void *pSubscriberServerReactor) override {
		std::unique_lock<std::mutex> lck(subscriber_mutex);

		std::cout << "Removing Subscriber Reactor" << std::endl;

		subscriber_reactors.remove((SubscriberServerReactor *) pSubscriberServerReactor);

		std::cout << "Subscribers count: " << subscriber_reactors.size() << std::endl;
	}
};

void RunServer() {
	BrokerServiceImpl service;

	std::string server_address("0.0.0.0:50051");

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

	std::unique_ptr<Server> server(builder.BuildAndStart());

	std::cout << "Server listening on " << server_address << std::endl;

	server->Wait();
}

int main(__attribute__((unused)) int argc, __attribute__((unused)) char **argv) {
	RunServer();

	return 0;
}
