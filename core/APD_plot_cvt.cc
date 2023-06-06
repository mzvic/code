#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>
#include <google/protobuf/timestamp.pb.h>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <cstdio>
#include <boost/asio.hpp>
#include <iomanip>

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
    BrokerClient(std::shared_ptr<Channel> channel) : stub_(Broker::NewStub(channel)) {
        boost::asio::io_service io_service;
        socket_ = std::make_shared<boost::asio::ip::tcp::socket>(io_service);
        socket_->connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 12345));
    }

    void Subscribe() {
        ClientContext context;
        Empty empty;
        Bundle bundle;
        Timestamp timestamp;
        std::unique_ptr<ClientReader<Bundle>> reader(stub_->Subscribe(&context, empty));
        int accum, apd_size;
        int64_t sec;
        int32_t nsec;
        while (reader->Read(&bundle)) {
            sec = bundle.timestamp().seconds();
            nsec = bundle.timestamp().nanos();
            const google::protobuf::RepeatedField<int32_t>& apd_list = bundle.apd();
            apd_size = apd_list.size();
			for (int i = 0; i < apd_list.size(); ++i) {
				accum = accum + apd_list.Get(i);
				if (i == apd_size - 1) {
                    std::stringstream ss;
                    ss << std::setw(9) << std::setfill('0') << nsec;
                    std::string nsec_i_str = ss.str();
                    std::string data = std::to_string(sec) + "." + nsec_i_str + " " + std::to_string(accum) + "\n";
                    boost::asio::write(*socket_, boost::asio::buffer(data));
					accum = 0;
				}
			}
        }
        const Status status = reader->Finish();
        if (status.ok()) {
            std::cout << "Writing successfully finished" << std::endl;
        } else {
            std::cout << "Writing finished with error" << std::endl;
        }
    }

private:
    std::unique_ptr<Broker::Stub> stub_;
    std::shared_ptr<boost::asio::ip::tcp::socket> socket_;
};

int main() {
    const std::string server_address("localhost:50051");
    BrokerClient broker(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));
    broker.Subscribe();
    return 0;
}

