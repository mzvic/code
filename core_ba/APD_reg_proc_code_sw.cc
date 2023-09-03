#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>
#include <google/protobuf/timestamp.pb.h>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <cstdio>
#include "client.h"
#include "core.grpc.pb.h"
#include <csignal>
#include <thread>
#include <vector>

using namespace google::protobuf;
using namespace core;
using namespace std::chrono;

// Global variables
bool exit_flag = false;  // Used to signal the program to exit
std::mutex signal_mutex; // Mutex for synchronization
std::condition_variable signal_cv; // Condition variable for synchronization

// Time variables for logs
int64_t sec_0 = 9999999999;
int32_t nsec_0 = 999999999;
int64_t sec = 0;
int32_t nsec = 0;
int64_t sec_i = 0;
int32_t nsec_i = 0;

// Cumulative value variable
int accum = 0;

// Bundle size variables
int apd_size = 0;
vector<float> subs_values(apd_size);

// Variable for the name of the generated file
char filename[70];

int c = 0;
long int t_0 = 0;

// Function to handle the interrupt signal (SIGINT)
void HandleSignal(int){
    std::unique_lock<std::mutex> slck(signal_mutex);
    std::cout << "Exiting..." << std::endl;
    exit_flag = true;
    signal_cv.notify_one();
}

// Function to save logs
void save_reg(vector<float> subs_values){
    // File opens
    FILE *file = fopen(filename, "a"); 

    // The start time of registers is assigned (to subtract it from the instantaneous time and thus obtain registers starting from T = 0)
    if (sec < sec_0){ 
        sec_0 = sec;
        nsec_0 = nsec;
    }
    
    // The bundle time is assigned to the log time
    sec_i = sec;
    nsec_i = nsec;

    // Operation to obtain the counts cumulative values and send them to the broker
    for (int i = 0; i < subs_values.size(); ++i){
        accum = accum + subs_values[i];        
        // 'If' to obtain 10 accumulated counts for each vector of 1000 data, that is, for every 1000 data, 10 are obtained, therefore, the sampling frequency is divided into 100, and now it remains at 1kHz
        if (i == (subs_values.size() / 10) - 1 ||
            i == (subs_values.size() / 5) - 1 ||
            i == (subs_values.size() * 3 / 10) - 1 ||
            i == (subs_values.size() * 2 / 5) - 1 ||
            i == (subs_values.size() / 2) - 1 ||
            i == (subs_values.size() * 3 / 5) - 1 ||
            i == (subs_values.size() * 7 / 10) - 1 ||
            i == (subs_values.size() * 4 / 5) - 1 ||
            i == (subs_values.size() * 9 / 10) - 1 ||
            i == subs_values.size() - 1){
            
            // Operations to assign the appropriate timestamp to each accumulated value
            nsec_i = nsec + i * 9700;
            if (nsec_i > 999999999){
                sec_i = sec + 1;
                nsec_i = nsec_i % 1000000000;
            }
            int64_t sec_diff = sec_i - sec_0;
            int32_t nsec_diff = nsec_i - nsec_0;
            if (nsec_diff < 0){
                sec_diff--;
                nsec_diff += 1000000000;
            }
            // Data is written to the file
            fprintf(file, "%ld.%09d;%d\n", sec_diff, nsec_diff % 1000000000, accum);
            // The cumulative value is reset
            accum = 0;
        }
    }
    // File is closed
    fclose(file);
    
}

// Function in which the bundle is received from the broker (data and timestamp) and saved in a vector to later be saved in the previous function
void ProcessSub(const Bundle &bundle){
    apd_size = 0;
    while (apd_size == 0){
        apd_size = bundle.value().size();
        sec = bundle.timestamp().seconds();
        nsec = bundle.timestamp().nanos();
    }

    subs_values.assign(apd_size, 0.0);

    for (int i = 0; i < apd_size; i++){
        subs_values[i] = bundle.value(i);

    }
    save_reg(subs_values);
}


int main(){
    std::unique_lock<std::mutex> slck(signal_mutex);
    
    // Obtains the time at which data recording begins
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    ts.tv_sec -= 14400;
    struct tm t;
    gmtime_r(&ts.tv_sec, &t);
    char buf[40];
    
    // File name format
    strftime(buf, sizeof(buf), "[1kHz]APD_logs_%d-%m-%Y_%H:%M:%S", &t);
    printf("Creating APD log file...");
    snprintf(filename, sizeof(filename), "%s.txt", buf);
    
    // The file is opened to check that it was created correctly
    FILE *file = fopen(filename, "w");
    if (file == NULL){
        printf("Error creating APD log file...\n");
    }else{
        printf("Created file '%s'...\n", filename);
    }
    
    // The function that receives the bundle for processing is called
    SubscriberClient subscriber_client(&ProcessSub, vector<int>{DATA_APD_FULL});

    std::signal(SIGINT, HandleSignal);

    signal_cv.wait(slck, [] { return exit_flag; });
    return 0;
}
