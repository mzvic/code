#include <csignal>

#include "core.grpc.pb.h"
#include "broker_client.h"
#include "storage_client.h"

using namespace core;
using namespace std::chrono;
using namespace google::protobuf;

Bundle *publishing_bundle;
PusherClient *pusher_client;
SubscriberClient *subscriber_client;

bool exit_flag = false;
mutex signal_mutex;
condition_variable signal_cv;

//void SendToStorage(const Timestamp &timestamp) {
//  cout << "Publishing data" << endl;
//
//  publishing_bundle->clear_value();
//
//  pusher_client->EnqueueOutboundMessage(*publishing_bundle, timestamp);
//}

void ProcessBundle(const Bundle &bundle) {
  publishing_bundle->mutable_timestamp()->CopyFrom(bundle.timestamp());

  publishing_bundle->clear_value();
  publishing_bundle->mutable_value()->CopyFrom(bundle.value());

  pusher_client->EnqueueOutboundMessage(*publishing_bundle);
}

void HandleSignal(int) {
  unique_lock<mutex> slck(signal_mutex);

  cout << "Exiting..." << endl;

  exit_flag = true;

  signal_cv.notify_one();
}

int main() {
  unique_lock<mutex> slck(signal_mutex);

  pusher_client = new PusherClient();
  subscriber_client = new SubscriberClient(&ProcessBundle, vector<int>{DATA_FFT_FULL});
  publishing_bundle = new Bundle();

  publishing_bundle->set_type(STORAGE_RECORD);

  // Register handler
  std::signal(SIGINT, HandleSignal);

  // Wait fot CTRL-C signal
  signal_cv.wait(slck, [] { return exit_flag; });

  delete subscriber_client;
  delete pusher_client;
  delete publishing_bundle;

  return 0;
}