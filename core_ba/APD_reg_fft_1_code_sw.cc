#include <iostream>
#include <memory>
#include <string>
#include <google/protobuf/timestamp.pb.h>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <cstdio>
#include <vector>
#include "broker_client.h"
#include <csignal>
#include "core.grpc.pb.h"
#include <thread>

using namespace core;
using namespace std::chrono;
using namespace google::protobuf;

// String variables to write averages to the file
std::string data_freq;
std::string data_fft;

// Window length in the FFT
int fft_frequencies = 0;

// Count for FFT averaging operation
int c = 0;

// Variable to see if the period in which the FFT average is obtained has passed
long int t_0 = 0;

// Period to average (in seconds)
int T;

// Vectors related to obtaining FFT averages and indexes to eliminate (zeros)
std::vector<float> data_freq_avg;
std::vector<float> data_fft_avg;
std::vector<int> delete_index;

// Vector containing the values of the bundle
vector<float> subs_values(fft_frequencies);

// Global variables
bool exit_flag = false;  // Used to signal the program to exit
std::mutex signal_mutex; // Mutex for synchronization
std::condition_variable signal_cv; // Condition variable for synchronization

// Variable for the name of the generated file
char filename[60];

// Function to handle the interrupt signal (SIGINT)
void HandleSignal(int) {
    std::unique_lock<std::mutex> slck(signal_mutex);
    std::cout << "Exiting..." << std::endl;
    exit_flag = true;
    signal_cv.notify_one();
}

// Function that gets the FFT averages and stores them in the file
void avg_fft(vector<float> subs_values, int fft_frequencies){
    // File opens
    FILE *file = fopen(filename, "a");
    
    // Length is assigned to the frequency and magnitude vectors
    data_freq_avg.resize(fft_frequencies/2);
    data_fft_avg.resize(fft_frequencies/2);
    
    // The length of the bundle is obtained
    int bundle_size = subs_values.size();
    // If a bundle arrives, proceed
    if (bundle_size > 0) {
        // The vector is restarted to obtain indices of zero values (values that are deleted)
        delete_index.resize(1);
        delete_index.assign(1, 0);
        
        // The timestamp of each average is obtained
        struct timeval tf;
        gettimeofday(&tf, nullptr);
        tf.tv_sec -= 14400;

        // Values are assigned to the frequency vector
        for (float i = 0; i < fft_frequencies/2; ++i) {
            data_freq_avg[i] = subs_values[i];
            
        }
        // A vector ('data_fft_avg') is generated with the accumulated magnitudes during the period indicated by the user
        for (float i = 0; i < (fft_frequencies/2); ++i) {// /10; ++i) {
            float fft_i = subs_values[i + fft_frequencies/2];          
            data_fft_avg[i] = data_fft_avg[i] + fft_i;
        }
        c += 1;
        
        // Check if the period entered by the user has passed
        if ((tf.tv_sec >= t_0 + T) && (t_0 > 0)) {
            // If the vector of accumulated values already has at least one value, proceed
            if (c > 0){
                // Here the vector containing the indices to be eliminated (zero values) is generated and the accumulated values are divided by 'c', in this way the average is obtained 
                for (int i = 0; i < data_fft_avg.size(); ++i) {
                    if (data_fft_avg[i] <= 0) {
                        delete_index.push_back(i);
                    } else {
                        data_fft_avg[i] = data_fft_avg[i] / c;
                    }
                }
            }
            // Zero values are deleted here (based on the 'delete_index' vector)
            for (int i = delete_index.size() - 1; i > 0; --i) {
                int index = delete_index[i];
                data_fft_avg.erase(data_fft_avg.begin() + index);
                data_freq_avg.erase(data_freq_avg.begin() + index);
            }
            
            // The timestamp is stored in the log file
            t_0 = tf.tv_sec;
            fprintf(file, "%ld.%06ld\n",  tf.tv_sec, tf.tv_usec% 1000000);
            
            // The magnitudes are stored in the log file
            for (int i = 0; i < data_fft_avg.size(); ++i) {
                fprintf(file, "%f ", data_fft_avg[i]);
            }                
            fprintf(file, "\n");
            
            // Frequencies are stored in the log file
            for (int i = 0; i < data_freq_avg.size(); ++i) {
                fprintf(file, "%f ", data_freq_avg[i]);
            }
            fprintf(file, "\n\n");
            
            // The number of spectra contained in the average is printed (commented out for now)
            //std::cout << "Amount of spectrum averaged: " << c << std::endl;

            // Vectors related to FFT averaging are reset
            data_freq_avg.resize(fft_frequencies/2);
            data_freq_avg.assign(fft_frequencies/2, 0.0);
            data_fft_avg.resize(fft_frequencies/2);
            data_fft_avg.assign(fft_frequencies/2, 0.0);
            
            // The count of averaged spectra is restarted
            c = 0;
        } else if (t_0 == 0) { // If the period entered by the user still does not pass, the time variable is updated (variable to check if it is possible to proceed to the previous if)
            t_0 = tf.tv_sec;          
        }  
    }
    // The file is closed
    fclose(file);
}

// Function in which the bundle is received from the broker and saved in a vector to later be processed in the previous function
void ProcessSub(const Bundle &bundle) {
  while (fft_frequencies == 0){
    fft_frequencies = bundle.value().size();
  }
  
  subs_values.assign(fft_frequencies, 0.0);
  
  for (int i = 0; i < fft_frequencies; i++) {
	subs_values[i] = bundle.value(i);
  }  
  
  avg_fft(subs_values, fft_frequencies); 
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " T" << std::endl;
        return 1;
    }
    
    // Period over which averages are obtained (entered by the user through the GUI)
    T = std::stoi(argv[1]);
    std::unique_lock<std::mutex> slck(signal_mutex);

    // Obtains the time at which data recording begins 
    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    ts.tv_sec -= 14400;
    struct tm t;
    gmtime_r(&ts.tv_sec, &t);
    
    // File name format
    char buf[50];
    strftime(buf, sizeof(buf), "[0.1Hz_res]FFT_avg_logs_%d-%m-%Y_%H:%M:%S", &t);
    printf("Creating [0.1Hz res] FFT log file...");
    snprintf(filename, sizeof(filename), "%s.txt", buf);
    
    // The file is opened to check that it was created correctly    
    FILE *file = fopen(filename, "w");
    if (file == NULL){
        printf("Error creating FFT log file...\n");
    }else{
        printf("Created file '%s'...\n", filename);}
    
    // The function that receives the bundle for processing is called   
    SubscriberClient subscriber_client(&ProcessSub, vector<int>{DATA_FFT_PARTIAL});
     
    std::signal(SIGINT, HandleSignal);

    signal_cv.wait(slck, [] { return exit_flag; });

    return 0;
}

