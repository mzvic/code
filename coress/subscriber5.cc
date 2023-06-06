#include <thread>

#include "core.grpc.pb.h"
#include "client.h"

using namespace core;

void Process(const Bundle &bundle) {
  cout << "New bundle received: apd: " << bundle.apd(0) << endl;
}

int main(__attribute__((unused)) int argc, __attribute__((unused)) char **argv) {
  Bundle bundle;
  SubscriberClient subscriber_client(&Process);

  this_thread::sleep_for(std::chrono::milliseconds(20000));

  return 0;
}

