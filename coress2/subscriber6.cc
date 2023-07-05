#include <csignal>
#include "core.grpc.pb.h"
#include "client.h"

using namespace core;

bool exit_flag = false;
mutex signal_mutex;
condition_variable signal_cv;

void ProcessBundle(const Bundle &bundle) {
  cout << "New bundle received: Type: " << bundle.type() << endl;
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

