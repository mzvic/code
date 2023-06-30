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
using namespace std::chrono;
using namespace google::protobuf;
Bundle *publishing_bundle;
PublisherClient *publisher_client;


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
	publishing_bundle->clear_apdtime();
    publishing_bundle->clear_pa();
    if (apd_size == 1000){
	    for (int i = 0; i < bundle.apd().size(); ++i) {
		    accum = accum + bundle.apd().Get(i);	
		    if (i == apd_size/5 - 1 ||
		        i == apd_size*2/5 - 1 ||
		        i == apd_size*3/5 - 1 ||
		        i == apd_size*4/5 - 1 ||
		        i == apd_size - 1) {
			    nsec_i = nsec + i * 9700;
			    sec_i = sec;
			    if (nsec_i > 999999999){
				    sec_i = sec + 1;
				    nsec_i = nsec_i%1000000000;
			    }		    
                std::stringstream ss;
                ss << std::setw(9) << std::setfill('0') << nsec_i;
                std::string nsec_i_str = ss.str();
                double apdtime = std::stod(std::to_string(sec_i) + "." + nsec_i_str);
                publishing_bundle->add_apdtime(apdtime);
                publishing_bundle->add_pa(accum);
                //cout << "nsec_i: " << nsec_i <<  endl;
                
			    accum = 0;
		    }
	    }
	    publisher_client->Publish(*publishing_bundle);
	}    
}

int main(__attribute__((unused)) int argc, __attribute__((unused)) char **argv) {

    SubscriberClient subscriber_client(&Process);
    publisher_client = new PublisherClient();
    publishing_bundle = new Bundle();
    
    this_thread::sleep_for(std::chrono::milliseconds(99999999999999999));
    return 0;  
}
