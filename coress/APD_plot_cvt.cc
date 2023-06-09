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
    int64_t sec = 0;
    int32_t nsec = 0;   
    sec = bundle.timestamp().seconds();
    nsec = bundle.timestamp().nanos();
    int64_t sec_i = 0;
    int32_t nsec_i = 0;    
    apd_size = bundle.apd().size();
    data2send = "";
    //std::cout << "-----------> APD_size: " << apd_size << std::endl;
	for (int i = 0; i < bundle.apd().size(); ++i) {
		accum = accum + bundle.apd().Get(i);	
		//std::cout << bundle.apd().size() << std::endl;
		if (i == apd_size/5 - 1 ||
		    i == apd_size*2/5 - 1 ||
		    i == apd_size*3/5 - 1 ||
		    i == apd_size*4/5 - 1 ||
		    i == apd_size - 1) {
			nsec_i = nsec + i * 10000;
			sec_i = sec;
			if (nsec_i > 999999999){
				sec_i = sec + 1;
				nsec_i = nsec_i%1000000000;
			}		    
            std::stringstream ss;
            ss << std::setw(9) << std::setfill('0') << nsec_i;
            std::string nsec_i_str = ss.str();
            data2send = data2send + std::to_string(sec_i) + "." + nsec_i_str + " " + std::to_string(accum)+ " ";
            //std::cout << data2send << std::endl;
			accum = 0;
		}
	}
	data2send = data2send + "\n";
	send2Socket(data2send);
	//std::cout << data2send << apd_size << std::endl;
	data2send = "";
}

int main(__attribute__((unused)) int argc, __attribute__((unused)) char **argv) {
    socket_ = std::make_shared<boost::asio::ip::tcp::socket>(io_service);
    socket_->connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 12345));
    Bundle bundle;
    SubscriberClient subscriber_client(&Process);
    this_thread::sleep_for(std::chrono::milliseconds(99999999999999999));
    return 0;
}
