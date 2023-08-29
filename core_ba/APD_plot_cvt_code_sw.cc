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
#include "client.h"

using namespace core;
using namespace std::chrono;
using namespace google::protobuf;

Bundle *publishing_bundle;
PublisherClient *publisher_client;

bool exit_flag = false;
mutex signal_mutex;
condition_variable signal_cv;

int subs_seconds = 0;
int subs_nanos = 0;
double subs_time = 0.0;
double time_i = 0.0;

const vector<int> interests = {1};
vector<float> subs_values(1000);
int accum = 0;

void HandleSignal(int) {
  unique_lock<mutex> slck(signal_mutex);
  cout << "Exiting..." << endl;
  exit_flag = true;
  signal_cv.notify_one();
}


void Send2Broker(const Timestamp &timestamp) {
    publishing_bundle->clear_value();
    int x = 0;
    for (int i = 0; i < subs_values.size(); ++i) {
        accum = accum + subs_values[i];	
        if (i == subs_values.size()/5 - 1 || i == subs_values.size()*2/5 - 1 || i == subs_values.size()*3/5 - 1 || i == subs_values.size()*4/5 - 1 || i == subs_values.size() - 1) {
            publishing_bundle->add_value(accum);
            time_i = subs_time + 0.002 * x;
            publishing_bundle->add_value(time_i);
            // cout << "Value: " << accum <<  endl;         
	        accum = 0;
	        x++;
        }
    }
  publisher_client->Publish(*publishing_bundle, timestamp);
}

void ProcessSub(const Bundle &bundle) {
  //cout << "New bundle received: Type: " << bundle.type();
  for (int i = 0; i < bundle.value().size(); i++) {
	subs_values[i] = bundle.value(i);
	//cout << "   " << bundle.value(i); 
  }  
  subs_seconds = bundle.timestamp().seconds();
  subs_nanos = bundle.timestamp().nanos();
  subs_time = subs_seconds + (double(subs_nanos)/1000000000);
  Send2Broker(bundle.timestamp());
  //cout << endl;
}

int main(__attribute__((unused)) int argc, __attribute__((unused)) char **argv) {
    unique_lock<mutex> slck(signal_mutex);
    SubscriberClient subscriber_client(&ProcessSub, vector<int>{DATA_APD_FULL});
    publisher_client = new PublisherClient();
    publishing_bundle = new Bundle();
    publishing_bundle->set_type(DATA_APD_CVT);
    std::signal(SIGINT, HandleSignal);  

    signal_cv.wait(slck, [] { return exit_flag; });

    free(publisher_client);
    free(publishing_bundle);
    return 0; 
}
