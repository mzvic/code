#include <condition_variable>
#include <grpcpp/grpcpp.h>
#include <thread>

#include "core.grpc.pb.h"
#include "reactor/reactor.h"

using namespace reactor;
using namespace core;

using core::Broker;
using google::protobuf::Empty;
using google::protobuf::Timestamp;
using grpc::Channel;
using grpc::ClientContext;

PublisherClientReactor *publisherreactor;
std::mutex mu_;
std::condition_variable cv_;

class Publisher {
public:
	Publisher() {
		ClientContext context;
		Empty empty;

		//		stub_ = Broker::NewStub(channel);

		// Register reactor
		//		stub_->async()->Publish(&context, &empty, publisherreactor);
	}

	void publish() {
		Bundle bundle;
		Timestamp timestamp;

		struct timeval tv;
		gettimeofday(&tv, nullptr);

		timestamp.set_seconds(tv.tv_sec);
		timestamp.set_nanos(tv.tv_usec * 1000);

		bundle.set_allocated_timestamp(&timestamp);

		publisherreactor->Enqueue(bundle);
	}

private:
	std::unique_ptr<Broker::Stub> stub_;
};

int main(__attribute__((unused)) int argc, __attribute__((unused)) char **argv) {
	Publisher *mpublisher;
	std::string server_address("localhost:50051");

	std::unique_ptr<Broker::Stub> stub_;

	stub_ = Broker::NewStub(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));

	//	mpublisher = new Publisher(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));
	mpublisher = new Publisher();

	publisherreactor = new PublisherClientReactor(stub_.get());

	//	for (int i = 0; i < 10; i++) {
	//		mpublisher->publish();
	//
	//		std::cout << "Value " << i << " written" << std::endl;
	//
	//		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	//	}

	std::cout << "Here before" << std::endl;

	std::this_thread::sleep_for(std::chrono::milliseconds(10000));

	std::cout << "Here after" << std::endl;

	return 0;
}
