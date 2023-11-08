#include <csignal>
#include "core.grpc.pb.h"
#include "broker_client.h"

SubscriberClient *subscriber_client;

bool exit_flag = false;
mutex signal_mutex;
condition_variable signal_cv;

void ProcessBundle(const Bundle &bundle) {
  time_t timestamp_time;
  struct tm *timestamp_tm;
  char buffer_tm[64], buffer[92];

  timestamp_time = bundle.timestamp().seconds();
  timestamp_tm = localtime(&timestamp_time);
  strftime(buffer_tm, sizeof buffer_tm, "%Y-%m-%d %H:%M:%S", timestamp_tm);
  snprintf(buffer, sizeof buffer, "%s.%09d", buffer_tm, bundle.timestamp().nanos());

  cout << "Bundle received. Timestamp: " << buffer << " Type: " << bundle.type() << " Value size: " << bundle.value_size() << endl;
}

void HandleSignal(int) {
  unique_lock<mutex> slck(signal_mutex);

  LOG("Exiting");

  exit_flag = true;

  signal_cv.notify_one();
}

int main(int argc, char *argv[]) {
  unique_lock<mutex> slck(signal_mutex);
  vector<int> interests;

  for (int i = 1; i < argc; i++) {
	interests.push_back(stoi(argv[i]));
  }

  subscriber_client = new SubscriberClient(&ProcessBundle, interests);

  signal(SIGINT, HandleSignal);

  signal_cv.wait(slck, [] { return exit_flag; });

  delete subscriber_client;

  return 0;
}

