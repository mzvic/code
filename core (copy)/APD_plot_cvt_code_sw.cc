#include <iostream>
#include <memory>
#include <string>
#include <google/protobuf/timestamp.pb.h>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <cstdio>
#include <boost/asio.hpp>
#include <iomanip>
#include <thread>
#include <csignal>
#include <vector>
#include "core.grpc.pb.h"
#include "broker_client.h"

using namespace core;
using namespace std::chrono;
using namespace google::protobuf;

Bundle *publishing_bundle;
PublisherClient *publisher_client;

// Global variables
bool exit_flag = false;  // Used to signal the program to exit
std::mutex signal_mutex; // Mutex for synchronization
std::condition_variable signal_cv; // Condition variable for synchronization

// Variables related to the timestamp of the bundle and the timestamp of each value to be plotted
int subs_seconds = 0;
int subs_nanos = 0;
double subs_time = 0.0;
double time_i = 0.0;

// Vector containing the values coming from the bundle
vector<float> subs_values(1000);

// Cumulative value of accounts
int accum = 0;

// Function to handle the interrupt signal (SIGINT)
void HandleSignal(int) {
  unique_lock<mutex> slck(signal_mutex);
  cout << "Exiting..." << endl;
  exit_flag = true;
  signal_cv.notify_one();
}

// Function that sends cumulative values to the broker
void Send2Broker(const Timestamp &timestamp) {
    // Clears the value of DATA_APD_CVT in the bundle
    publishing_bundle->clear_value();
    // Variable that keeps track to obtain the timestamp of each accumulated value
    int x = 0;
    // Operation to get 5 accumulated values per 1000, i.e. if the sample rate is 100kHz then this outputs data at 500Hz
    for (int i = 0; i < subs_values.size(); ++i) {
        accum = accum + subs_values[i];	
        if (i == subs_values.size()/5 - 1 || i == subs_values.size()*2/5 - 1 || i == subs_values.size()*3/5 - 1 || i == subs_values.size()*4/5 - 1 || i == subs_values.size() - 1) {
            // Add the cumulative value to the bundle
            publishing_bundle->add_value(accum);
            // Add the timestamp value to the bundle
            time_i = subs_time + 0.002 * x;
            publishing_bundle->add_value(time_i);
	        // Reset the cumulative value
	        accum = 0;
	        x++;
        }
    }
  // Send the data to the broker
  publisher_client->Publish(*publishing_bundle, timestamp);
}

// Function in which the bundle is received (data and timestamp) from the broker and saved in a vector to later be processed in the previous function
void ProcessSub(const Bundle &bundle) {
  for (int i = 0; i < bundle.value().size(); i++) {
	subs_values[i] = bundle.value(i);
  }  
  subs_seconds = bundle.timestamp().seconds();
  subs_nanos = bundle.timestamp().nanos();
  subs_time = subs_seconds + (double(subs_nanos)/1000000000);
  Send2Broker(bundle.timestamp());
}

int main(__attribute__((unused)) int argc, __attribute__((unused)) char **argv) {
    unique_lock<mutex> slck(signal_mutex);
    // The function that receives the bundle for processing is called    
    SubscriberClient subscriber_client(&ProcessSub, vector<int>{DATA_APD_FULL});
    // Publisher settings
    publisher_client = new PublisherClient();
    publishing_bundle = new Bundle();
    publishing_bundle->set_type(DATA_APD_CVT);
    std::signal(SIGINT, HandleSignal);  

    signal_cv.wait(slck, [] { return exit_flag; });

    free(publisher_client);
    free(publishing_bundle);
    return 0; 
}
