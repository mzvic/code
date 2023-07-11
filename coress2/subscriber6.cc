#include <csignal>
#include "core.grpc.pb.h"
#include "client.h"

using namespace core;

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

  cout << "New bundle received. Timestamp: " << buffer << " Type: " << bundle.type() << " Value size: " << bundle.value_size() << endl;
}

void HandleSignal(int) {
  unique_lock<mutex> slck(signal_mutex);

  cout << "Exiting..." << endl;

  exit_flag = true;

  signal_cv.notify_one();
}

int main(int argc, char *argv[]) {
  unique_lock<mutex> slck(signal_mutex);
  vector<int> interests;

  for (int i = 1; i < argc; i++) {
	interests.push_back(stoi(argv[i]));
  }

  SubscriberClient subscriber_client(&ProcessBundle, interests);

  std::signal(SIGINT, HandleSignal);

//  this_thread::sleep_for(std::chrono::milliseconds(20000));
  signal_cv.wait(slck, [] { return exit_flag; });

  return 0;
}

