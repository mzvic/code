#include <iostream>
#include <memory>
#include <string>
#include <google/protobuf/timestamp.pb.h>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <cstdio>
#include <vector>
#include "client.h"
#include <csignal>
#include "core.grpc.pb.h"
#include <thread>

using namespace core;
using namespace std::chrono;
using namespace google::protobuf;

std::string data_freq;
std::string data_fft;

int fft_frequencies = 0;
int c = 0;
long int t_0 = 0;
int T;

std::vector<float> data_freq_avg;
std::vector<float> data_fft_avg;
std::vector<int> delete_index;

vector<float> subs_values(fft_frequencies);

bool exit_flag = false;
std::mutex signal_mutex;
std::condition_variable signal_cv;

char filename[60];

const std::vector<int> interests = {4};

void HandleSignal(int) {
    std::unique_lock<std::mutex> slck(signal_mutex);
    std::cout << "Exiting..." << std::endl;
    exit_flag = true;
    signal_cv.notify_one();
}

void avg_fft(vector<float> subs_values, int fft_frequencies){
    FILE *file = fopen(filename, "a");
    data_freq_avg.resize(fft_frequencies);
    //data_freq_avg.assign(fft_frequencies, 0.0);
    data_fft_avg.resize(fft_frequencies);
    //data_fft_avg.assign(fft_frequencies, 0.0);
    int bundle_size = subs_values.size();
    //cout << "Bundle_size: "<< bundle_size << endl;
    if (bundle_size > 0) {
        delete_index.resize(1);
        delete_index.assign(1, 0);

        struct timeval tf;
        gettimeofday(&tf, nullptr);
        tf.tv_sec -= 14400;

        for (float i = 0; i < fft_frequencies; ++i) {
            data_freq_avg[i] = (i)/10;
        }
        for (float i = 0; i < (fft_frequencies); ++i) {// /10; ++i) {
            float freq_i = (i)/10;
            float fft_i = subs_values[i];
            data_fft_avg[i] = data_fft_avg[i] + fft_i;
        }
        //std::cout << data_fft_avg[500] << " ";
        //std::cout << data_freq_avg[500] << " ";
        c += 1;
        //std::cout << "C: " << c << std::endl;
        if ((tf.tv_sec >= t_0 + T) && (t_0 > 0)) {
            if (c > 0){
                for (int i = 0; i < data_fft_avg.size(); ++i) {
                    if (data_fft_avg[i] <= 0) {
                        delete_index.push_back(i);
                    } else {
                        data_fft_avg[i] = data_fft_avg[i] / c;
                    }
                }
            }
            for (int i = delete_index.size() - 1; i > 0; --i) {
                int index = delete_index[i];
                data_fft_avg.erase(data_fft_avg.begin() + index);
                data_freq_avg.erase(data_freq_avg.begin() + index);
            }

            t_0 = tf.tv_sec;
            fprintf(file, "%ld.%06ld\n",  tf.tv_sec, tf.tv_usec% 1000000);
            for (int i = 0; i < data_fft_avg.size(); ++i) {
                fprintf(file, "%f ", data_fft_avg[i]);
                //std::cout << data_fft_avg[i] << " ";    
            }                   
            //std::cout << std::endl;
            fprintf(file, "\n");
            for (int i = 0; i < data_freq_avg.size(); ++i) {
                fprintf(file, "%f ", data_freq_avg[i]);
                //std::cout << data_freq_avg[i] << " ";
            }
            //std::cout << "Amount of spectrum averaged: " << c << std::endl;
            fprintf(file, "\n\n");
            data_freq_avg.resize(fft_frequencies);
            data_freq_avg.assign(fft_frequencies, 0.0);
            data_fft_avg.resize(fft_frequencies);
            data_fft_avg.assign(fft_frequencies, 0.0);
            c = 0;
        } else if (t_0 == 0) {
            t_0 = tf.tv_sec;            
        }  
    }
    fclose(file);
}

void ProcessSub(const Bundle &bundle) {
  //cout << "Starting ProcessSub..." << endl;
  //fft_frequencies = bundle.value().size(); 
  while (fft_frequencies == 0){
    fft_frequencies = bundle.value().size();
  }
  
  //cout << fft_frequencies << endl; 
  subs_values.assign(fft_frequencies, 0.0);
  
  for (int i = 0; i < fft_frequencies; i++) {
	subs_values[i] = bundle.value(i);
	//cout << "   " << bundle.value(i); 
  }  
  avg_fft(subs_values, fft_frequencies); 
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " T" << std::endl;
        return 1;
    }

    T = std::stoi(argv[1]);
    std::unique_lock<std::mutex> slck(signal_mutex);

    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    ts.tv_sec -= 14400;
    struct tm t;
    gmtime_r(&ts.tv_sec, &t);
    char buf[50];
    strftime(buf, sizeof(buf), "[0.1Hz_res]FFT_avg_logs_%d-%m-%Y_%H:%M:%S", &t);
    printf("Creating [0.1Hz res] FFT log file...");
    
    snprintf(filename, sizeof(filename), "%s.txt", buf);
    
    printf("Created file '%s'...\n", filename);
   
    SubscriberClient subscriber_client(&ProcessSub, vector<int>{DATA_FFT_FULL});
     
    std::signal(SIGINT, HandleSignal);

    signal_cv.wait(slck, [] { return exit_flag; });

    return 0;
}

