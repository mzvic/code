#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>
#include <google/protobuf/timestamp.pb.h>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <cstdio>
#include <vector>

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

const int fft_frequencies = 50000;
int c = 0;
long int t_0 = 0;
int T;

std::vector<int> data_freq_avg;
std::vector<float> data_fft_avg;
std::vector<float> data_fft_i;
std::vector<int> delete_index;

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
    	char buf[40];
    	strftime(buf, sizeof(buf), "FFT_avg_logs_%d-%m-%Y_%H:%M:%S", &t);
        printf("Creating FFT avg. log file...");  
		char filename[50];
		snprintf(filename, sizeof(filename), "%s.txt", buf);
		FILE *file = fopen(filename, "w");
		if (file == NULL) {
		    printf("Error creating FFT avg. log file...\n");
		}
		printf("Created file '%s'...\n", filename);
        float freq_i, fft_i; 
        data_freq_avg.resize(fft_frequencies);
        data_freq_avg.assign(fft_frequencies, 0);
        data_fft_avg.resize(fft_frequencies);
        data_fft_avg.assign(fft_frequencies, 0.0);                 
        while (reader->Read(&bundle)) {
            data_fft_i.resize(fft_frequencies);
            data_fft_i.assign(fft_frequencies, 0.0);  
            delete_index.resize(1);
            delete_index.assign(1, 0);               
            struct timeval tf;
            gettimeofday(&tf, nullptr);
            tf.tv_sec -= 14400;
            int freq_size;
            freq_size = bundle.freq().size();
            if (freq_size > 0){
                for (int i = 0; i < fft_frequencies; ++i) {
                    data_freq_avg[i] = i + 1;
                }
	            float freq_i, fft_i;
	            for (int i = 0; i < bundle.freq().size(); ++i) {
		            freq_i = bundle.freq().Get(i);
		            fft_i = bundle.fft().Get(i);	
		            data_fft_i[freq_i-1] = fft_i;
	            }
                for (int i = 0; i < fft_frequencies; ++i) {
                    data_fft_avg[i] = data_fft_avg[i] + data_fft_i[i];
                }
                data_fft_i.assign(fft_frequencies, 0.0);   
                c += 1;    
	            if ((tf.tv_sec >= t_0 + T) and (t_0 > 0)){
                    for (int i = 0; i < data_fft_avg.size(); ++i) {
                        if (data_fft_avg[i] == 0) {
                            delete_index.push_back(i);
                        } else {
                            data_fft_avg[i] = data_fft_avg[i]/(c);
                        }
                    }
                    for (int i = delete_index.size() - 1; i > 0; --i) {
                        int index = delete_index[i];
                        data_fft_avg.erase(data_fft_avg.begin() + index);
                        data_freq_avg.erase(data_freq_avg.begin() + index);
                    }
	                t_0 = tf.tv_sec;
	                //std::cout << t_0 << std::endl; 
	                fprintf(file, "%ld.%06ld\n",  tf.tv_sec, tf.tv_usec% 1000000);
                    for (int i = 0; i < data_fft_avg.size(); ++i) {
                        fprintf(file, "%f ", data_fft_avg[i]);
                    //    std::cout << data_fft_avg[i] << " ";    
                    }                   
                    //std::cout << std::endl;
                    fprintf(file, "\n");
                    for (int i = 0; i < data_freq_avg.size(); ++i) {
                        fprintf(file, "%d ", data_freq_avg[i]);
                    //    std::cout << data_freq_avg[i] << " ";
                    }                   
                    //std::cout << std::endl;
                    //std::cout << c << " ";
                    //std::cout << std::endl;
                    //std::cout << std::endl;
                    fprintf(file, "\n\n"); 
                    data_freq_avg.resize(fft_frequencies);
                    data_freq_avg.assign(fft_frequencies, 0);
                    data_fft_avg.resize(fft_frequencies);
                    data_fft_avg.assign(fft_frequencies, 0.0);                      
                    c = 0;
	            } else if (t_0 == 0){
	                t_0 = tf.tv_sec;
	            }      
	        }        
        }       
        fclose(file);
    }

private:
    std::unique_ptr<Broker::Stub> stub_;
};

int main(int argc, char* argv[]) {
    T = std::stoi(argv[1]);
    const std::string server_address("localhost:50051");
    BrokerClient broker(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));
    broker.Subscribe();
    return 0;
}
