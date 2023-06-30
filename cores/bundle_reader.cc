#include <iostream>
#include <chrono>
#include <thread>
#include <grpcpp/grpcpp.h>
#include "core.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using core::Broker;
using core::Bundle;
using core::Empty;

Bundle process(const Bundle& bundle) {
    // std::cout << bundle.DebugString() << std::endl;
    return bundle;
}

class BrokerClient {
public:
    BrokerClient(std::shared_ptr<Channel> channel)
        : stub_(Broker::NewStub(channel)) {}

    void Subscribe() {
        Empty request;
        ClientContext context;
        std::unique_ptr<grpc::ClientReader<Bundle>> reader(
            stub_->Subscribe(&context, request));

        Bundle bundle;
        while (reader->Read(&bundle)) {
            process(bundle);
        }

        Status status = reader->Finish();
        if (!status.ok()) {
            std::cout << "Error: " << status.error_code() << ": "
                      << status.error_message() << std::endl;
        }
    }

private:
    std::unique_ptr<Broker::Stub> stub_;
};

int main() {
    grpc::ChannelArguments channel_args;
    channel_args.SetMaxReceiveMessageSize(-1);
    channel_args.SetMaxSendMessageSize(-1);
    auto channel = grpc::CreateCustomChannel("localhost:50051",
                                             grpc::InsecureChannelCredentials(),
                                             channel_args);

    BrokerClient client(channel);
    client.Subscribe();

    return 0;
}

