#include <iostream>
#include <memory>
#include <string>
#include <google/protobuf/timestamp.pb.h>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <cstdio>
#include <boost/asio.hpp>
#include <iomanip>
#include <thread>
#include "core.grpc.pb.h"
#include "client.h"

using namespace core;
boost::asio::io_service io_service;
std::shared_ptr<boost::asio::ip::tcp::socket> socket_;
std::string data2send;

void send2Socket(std::string data2send){
    boost::asio::write(*socket_, boost::asio::buffer(data2send));
    }

void Process(const Bundle &bundle) {
    Timestamp timestamp;
    int accum = 0;
    int apd_size;
    int64_t sec;
    int32_t nsec;
    sec = bundle.timestamp().seconds();
    nsec = bundle.timestamp().nanos();
    apd_size = bundle.apd().size();
    std::cout << "-----------> APD_size: " << apd_size << std::endl;
	for (int i = 0; i < bundle.apd().size(); ++i) {
		accum = accum + bundle.apd().Get(i);	
		//std::cout << bundle.apd().size() << std::endl;
		if (i == apd_size - 1) {
            std::stringstream ss;
            ss << std::setw(9) << std::setfill('0') << nsec;
            std::string nsec_i_str = ss.str();
            data2send = "";
            data2send = std::to_string(sec) + "." + nsec_i_str + " " + std::to_string(accum) + "\n";
            //std::cout << data2send << std::endl;
            send2Socket(data2send);
			accum = 0;
		}
	}
}

int main(__attribute__((unused)) int argc, __attribute__((unused)) char **argv) {
    socket_ = std::make_shared<boost::asio::ip::tcp::socket>(io_service);
    socket_->connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 12345));
    Bundle bundle;
    SubscriberClient subscriber_client(&Process);
    this_thread::sleep_for(std::chrono::milliseconds(99999999999999999));
    return 0;
}
