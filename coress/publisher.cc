#include <thread>

#include <grpcpp/grpcpp.h>

#include "core.grpc.pb.h"

using core::Broker;
using core::Bundle;
using google::protobuf::Empty;
using google::protobuf::Timestamp;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientWriter;
using grpc::Status;

class BrokerClient {
 public:
  BrokerClient(std::shared_ptr<Channel> channel) : stub(Broker::NewStub(channel)) {}

  void Publish() {
	ClientContext context;
	Empty empty;
	Bundle bundle;
	Timestamp timestamp;

	struct timeval tv;
	gettimeofday(&tv, nullptr);

	// timestamp.set_seconds(tv.tv_sec);
	// timestamp.set_nanos(tv.tv_usec * 1000);

	// Bundle.set_allocated_timestamp(&timestamp);

	// Bundle.set_test(10);

	std::unique_ptr<ClientWriter<Bundle>> writer(stub->Publish(&context, &empty));

	for (int i = 0; i < 10; i++) {
	  //			if (!writer->Write(bundle)) {
	  //				// Broken stream.
	  //				break;
	  //			}

	  std::cout << "Value " << i << " written" << std::endl;

	  std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	writer->WritesDone();

	std::cout << "Writing done" << std::endl;

	// std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	Status status = writer->Finish();

	if (status.ok())
	  std::cout << "Writing succesfully finished" << std::endl;
	else
	  std::cout << "Writing finished with error" << std::endl;

	// Bundle.release_timestamp();
  }

 private:
  std::unique_ptr<Broker::Stub> stub;
};

int main(int argc, char **argv) {
  std::string server_address("localhost:50051");

  //	grpc::ChannelArguments cArgs;
  //	cArgs.SetInt(StopNow();, 100);
  //	cArgs.SetInt(GRPC_ARG_MAX_RECONNECT_BACKOFF_MS, 1000);
  //	cArgs.SetInt(GRPC_ARG_MIN_RECONNECT_BACKOFF_MS, 100);
  // cArgs.SetInt(GRPC_ARG_MAX_RECEIVE_MESSAGE_LENGTH, 1000);
  // cArgs.SetInt(GRPC_ARG_MAX_SEND_MESSAGE_LENGTH, 1000);
  //	cArgs.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 300);
  //	cArgs.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 300);
  //	cArgs.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);

  // Instantiate the client. It requires a channel, out of which the actual RPCs are created
  //	BrokerClient broker(grpc::CreateCustomChannel(SERVER_ADDRESS, grpc::InsecureChannelCredentials(), cArgs));
  BrokerClient broker(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));

  broker.Publish();

  return 0;
}
