#include <csignal>

#include "core.grpc.pb.h"
#include "broker_client.h"
#include "storage_client.h"

using namespace core;
using namespace std::chrono;
using namespace google::protobuf;

unique_ptr<Bundle> publishing_bundle;
unique_ptr<PusherClient> pusher_client;

bool exit_flag = false;
mutex signal_mutex;
condition_variable signal_cv;

double last_monitor[2];

void ProcessBundle(const Bundle &bundle) {
  switch (bundle.type()) {
	case DATA_FFT_FULL:
	  publishing_bundle->set_type(STORAGE_RECORD_FFT);

	  publishing_bundle->clear_value();
	  publishing_bundle->mutable_value()->CopyFrom(bundle.value());

	  pusher_client->Push(*publishing_bundle, bundle.timestamp());

	  break;

	case DATA_APD_FULL:
	  last_monitor[0] = bundle.value().Get(0);

	  break;

	default:
	  break;
  }
}

void Monitor() {
  timespec ts{};

  while (!exit_flag) {
	// Get current time
	clock_gettime(CLOCK_REALTIME, &ts);

	// Set next event time
	ts.tv_sec++;
	ts.tv_nsec = 0;

	// Sleep until next event. Restart
	while (clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &ts, nullptr)) {}

	publishing_bundle->set_type(STORAGE_RECORD_MONITOR);

	publishing_bundle->clear_value();
	for (auto &elem : last_monitor)
	  publishing_bundle->add_value(elem);

	pusher_client->Push(*publishing_bundle);
  }
}

void HandleSignal(int) {
  unique_lock<mutex> slck(signal_mutex);

  cout << "Exiting..." << endl;

  exit_flag = true;

  signal_cv.notify_one();
}

int main() {
  unique_lock<mutex> slck(signal_mutex);
  thread monitor_thread;

  pusher_client = make_unique<PusherClient>();
  SubscriberClient subscriber_client(&ProcessBundle, vector<int>{DATA_APD_FULL, DATA_FFT_FULL});
  publishing_bundle = make_unique<Bundle>();

  monitor_thread = thread(&Monitor);

  // Register handler
  std::signal(SIGINT, HandleSignal);

  // Wait fot CTRL-C signal
  signal_cv.wait(slck, [] { return exit_flag; });

  monitor_thread.join();

  return 0;
}