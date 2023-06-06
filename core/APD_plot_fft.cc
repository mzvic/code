#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>
#include <fftw3.h>
#include <gnuplot-iostream.h>
#include <chrono>
#include <thread>
#include <boost/asio.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>

#include "core.grpc.pb.h"

using core::Broker;
using core::Bundle;
using google::protobuf::Empty;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;

class BrokerClient {
public:
    BrokerClient(std::shared_ptr<Channel> channel) : stub_(Broker::NewStub(channel)) {
        boost::asio::io_service io_service;
        socket_ = std::make_shared<boost::asio::ip::tcp::socket>(io_service);
        socket_->connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 12352));
    }

    void Subscribe() {
        ClientContext context;
        Empty empty;
        Bundle bundle;
        std::unique_ptr<ClientReader<Bundle>> reader(stub_->Subscribe(&context, empty));
        auto update_interval = std::chrono::milliseconds(10); 

        while (reader->Read(&bundle)) {
            sendPlotData(bundle);
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

    void sendPlotData(const Bundle &bundle) {
        const google::protobuf::RepeatedField<double>& freq_list = bundle.freq();
        const google::protobuf::RepeatedField<double>& fft_list = bundle.fft();
        double freq_i, fft_i;

        if (fft_list.size() > 0) {
            double freq_max = bundle.freq(freq_list.size() - 1);
            std::string data;
            for (int i = 0; i < fft_list.size(); ++i) {
                if (i == 0){
                	freq_i = 0.001;
                } else {
                	freq_i = freq_list.Get(i);
                }
                fft_i = fft_list.Get(i);
                data = data + " " + std::to_string(freq_i) + " " + std::to_string(fft_i);
            }
            boost::asio::write(*socket_, boost::asio::buffer(data+"\n"));
            data ="";
        } else {
            std::cout << "."; // Si se recibe una lista en blanco
        }
    }
};

int main() {
    const std::string server_address("localhost:50051");
    BrokerClient broker(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));
    broker.Subscribe();
    return 0;
}    
