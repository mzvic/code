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
        FILE *pipe = popen("feedgnuplot --lines --ymin -500 --ymax 6000 --ylabel 'Counts' --domain --xlabel 'Time [MM:SS]' --timefmt '%s.%N' --stream '0.0054' --xlen 30 --set 'object rectangle from screen 0,0 to screen 1,1 fillcolor rgbcolor \"black\" behind' --set 'grid lc rgb \"green\"' --set 'border lc rgb \"green\"' --set 'key tc rgb \"white\"' --set 'x2label tc rgb \"white\"' --set 'y2label tc rgb \"white\"' --set 'xlabel tc rgb \"white\"' --set 'ylabel tc rgb \"white\"' --set 'xtics tc rgb \"white\"' --set 'ytics tc rgb \"white\"' --set 'term x11 title \"CoDE - Counts vs. Time\"' --unset 'key'", "w");       
        //--unset 'mouse'
        int accum = 0;
        while (reader->Read(&bundle)) {
            const google::protobuf::RepeatedField<int64_t>& sec_list = bundle.sec();
            const google::protobuf::RepeatedField<int32_t>& nan_list = bundle.nan();
            const google::protobuf::RepeatedField<int32_t>& apd_list = bundle.apd();
            for (int i = 0; i < apd_list.size(); ++i) {
		        accum = accum + apd_list.Get(i);
				if (i == ((apd_list.size()/4)-1 || (apd_list.size()/2)-1 || (apd_list.size()*3/4)-1 || apd_list.size()-1)){
					fprintf(pipe, "%ld.%09d %d\n", sec_list.Get(i), nan_list.Get(i)*1000, accum);
					fflush(pipe);
					accum = 0;
				}
            }
        }
        const Status status = reader->Finish();
        pclose(pipe);
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

