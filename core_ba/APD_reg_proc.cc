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

int64_t sec_0 = 9999999999;
int32_t nsec_0 = 999999999;            

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
    	strftime(buf, sizeof(buf), "APD_logs_%d-%m-%Y_%H:%M:%S", &t);
        printf("Creating APD log file...");  
		char filename[40];
		snprintf(filename, sizeof(filename), "%s.txt", buf);
		FILE *file = fopen(filename, "w");
		if (file == NULL) {
		    printf("Error creating APD log file...\n");
		}
		printf("Created file '%s'...\n", filename);

        while (reader->Read(&bundle)) {
            int64_t sec = bundle.timestamp().seconds();
            int32_t nsec = bundle.timestamp().nanos();
            int64_t sec_i = 0;
            int32_t nsec_i = 0;
            int accum = 0;
            if (sec < sec_0){
                sec_0 = sec;
                nsec_0 = nsec;
            }
            const google::protobuf::RepeatedField<int32_t>& apd_list = bundle.apd();
            sec_i = sec;    
            nsec_i = nsec;

            for (int i = 0; i < apd_list.size(); ++i) {
           		accum = accum + bundle.apd().Get(i);
				if (i == (apd_list.size()/10) - 1 ||
				    i == (apd_list.size()/5) - 1 ||
				    i == (apd_list.size()*3/10) - 1 ||
				    i == (apd_list.size()*2/5) - 1 ||
				    i == (apd_list.size()/2) - 1 ||
				    i == (apd_list.size()*3/5) - 1 ||
				    i == (apd_list.size()*7/10) - 1 ||
				    i == (apd_list.size()*4/5) - 1 ||
				    i == (apd_list.size()*9/10) - 1 ||
				    i == apd_list.size() - 1) {
					nsec_i = nsec + i * 9700;
					if (nsec_i > 999999999){
						sec_i = sec + 1;
						nsec_i = nsec_i%1000000000;
					}
                    int64_t sec_diff = sec_i - sec_0;
                    int32_t nsec_diff = nsec_i - nsec_0;
                    if (nsec_diff < 0) {
                        sec_diff--;
                        nsec_diff += 1000000000;
                    }                
                    fprintf(file, "%ld.%09d;%d\n", sec_diff, nsec_diff % 1000000000, accum);
                    accum = 0;  
				}           		
            }
  
        }       
        const Status status = reader->Finish();
        fclose(file);
        if (status.ok()) {
            std::cout << "Writing successfully finished" << std::endl;
        } else {
            std::cout << "Writing finished with error" << std::endl;
        }
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

