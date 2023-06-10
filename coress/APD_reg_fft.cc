#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>
#include <google/protobuf/timestamp.pb.h>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <cstdio>

#include "core.grpc.pb.h"

using core::Broker;
using core::Bundle;
using google::protobuf::Empty;
using google::protobuf::Timestamp;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;

std::string data_freq;
std::string data_fft;

class BrokerClient {
public:
    BrokerClient(std::shared_ptr<Channel> channel) : stub_(Broker::NewStub(channel)) {}

    void Subscribe() {
        ClientContext context;
        Empty empty;
        Bundle bundle;
        Timestamp timestamp;
        std::unique_ptr<ClientReader<Bundle>> reader(stub_->Subscribe(&context, empty));

		struct timespec ts;
	    timespec_get(&ts, TIME_UTC);
	    ts.tv_sec -= 14400;
    	struct tm t;
    	gmtime_r(&ts.tv_sec, &t);
    	char buf[30];
    	strftime(buf, sizeof(buf), "FFT_logs_%d-%m-%Y_%H:%M:%S", &t);
        printf("Creating FFT log file...");  
		char filename[40];
		snprintf(filename, sizeof(filename), "%s.txt", buf);
		FILE *file = fopen(filename, "w");
		if (file == NULL) {
		    printf("Error creating APD log file...\n");
		}
		printf("Created file '%s'...\n", filename);
        float freq_i, fft_i;
        while (reader->Read(&bundle)) {
            struct timeval tf;
            gettimeofday(&tf, nullptr);
            tf.tv_sec -= 14400;
            int freq_size;
            freq_size = bundle.freq().size();
            if (freq_size > 0){
	            data_freq = "";
	            data_fft = "";
	            float freq_i, fft_i;
	            for (int i = 0; i < bundle.freq().size(); ++i) {
		            freq_i = bundle.freq().Get(i);
		            fft_i = bundle.fft().Get(i);	
		            data_freq = data_freq + std::to_string(freq_i) + " ";
		            data_fft = data_fft + std::to_string(fft_i) + " ";
	            }
	            data_freq = data_freq + "\n";
	            data_fft = data_fft + "\n\n";	
	            fprintf(file, "%ld.%06ld\n%s%s",  tf.tv_sec, tf.tv_usec% 1000000, data_freq.c_str(), data_fft.c_str());
	        }        
        }       
        //const Status status = reader->Finish();
        fclose(file);
    }

private:
    std::unique_ptr<Broker::Stub> stub_;
};

int main() {
    const std::string server_address("localhost:50051");
    BrokerClient broker(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));
    broker.Subscribe();
    return 0;
}
