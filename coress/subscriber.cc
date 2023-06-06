#include <grpcpp/grpcpp.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <thread>

#include "core.grpc.pb.h"

using core::Broker;
using core::Bundle;
using google::protobuf::Empty;
using google::protobuf::Timestamp;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;

class BrokerClient {
public:
	explicit BrokerClient(const std::shared_ptr<Channel> &channel) {
		stub = Broker::NewStub(channel);
	}

	BrokerClient() = default;

	void Init() {
		//		stub = Broker::NewStub(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));
		Connect();
	}

	void Subscribe() {
		ClientContext context;
		Empty empty;
		Bundle bundle;
		Status status;

		std::unique_ptr<ClientReader<Bundle>> reader(stub->Subscribe(&context, empty));

		while (reader->Read(&bundle)) {
			std::cout << "A value has been read" << std::endl;
		}

		status = reader->Finish();

		if (status.ok())
			std::cout << "Reading successfully finished" << std::endl;
		else
			std::cout << "Reading finished with error" << std::endl;
	}

private:
	std::unique_ptr<Broker::Stub> stub;

	void Connect() {
		ClientContext context;
		Empty empty;

		for (int i = 0; i < 10; ++i) {
			std::shared_ptr<Channel> channel = grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials());
			stub = Broker::NewStub(channel);
			std::unique_ptr<ClientReader<Bundle>> reader(stub->Subscribe(&context, empty));
			std::cout << channel->GetState(true) << std::endl;
			//			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		}
	}
};

int main(int argc, char **argv) {
	std::string server_address("localhost:50051");

	//	grpc::ChannelArguments cArgs;
	//	cArgs.SetInt(GRPC_ARG_INITIAL_RECONNECT_BACKOFF_MS, 100);
	//	cArgs.SetInt(GRPC_ARG_MAX_RECONNECT_BACKOFF_MS, 1000);
	//	cArgs.SetInt(GRPC_ARG_MIN_RECONNECT_BACKOFF_MS, 100);
	//	// cArgs.SetInt(GRPC_ARG_MAX_RECEIVE_MESSAGE_LENGTH, 1000);
	//	// cArgs.SetInt(GRPC_ARG_MAX_SEND_MESSAGE_LENGTH, 1000);
	//	cArgs.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 3000);
	//	cArgs.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 3000);
	//	cArgs.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);

	// Instantiate the client. It requires a channel, out of which the actual RPCs are created
	//	BrokerClient broker(grpc::CreateCustomChannel(SERVER_ADDRESS, grpc::InsecureChannelCredentials(), cArgs));
	//	BrokerClient broker(grpc::CreateChannel(SERVER_ADDRESS, grpc::InsecureChannelCredentials()));
	BrokerClient broker{};
	broker.Init();

	//	broker.Subscribe();


	return 0;
}
