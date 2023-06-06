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

class BrokerClient {
public:
    BrokerClient(std::shared_ptr<Channel> channel) : stub_(Broker::NewStub(channel)) {}

    void Subscribe() {
        ClientContext context;
        Empty empty;
        Bundle bundle;
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
            const google::protobuf::RepeatedField<int64_t>& sec_list = bundle.sec();
            const google::protobuf::RepeatedField<int32_t>& nan_list = bundle.nan();
            const google::protobuf::RepeatedField<int32_t>& apd_list = bundle.apd();
            for (int i = 0; i < apd_list.size(); ++i) {
		            //printf("%ld.%09d %d\n", sec_list.Get(i), nan_list.Get(i)*1000, apd_list.Get(i));
				fprintf(file, "%ld.%09d;%d\n", sec_list.Get(i), nan_list.Get(i)*1000, apd_list.Get(i));
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

