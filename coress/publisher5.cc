#include <thread>

#include "core.grpc.pb.h"
#include "client.h"

using namespace core;

int main(__attribute__((unused)) int argc, __attribute__((unused)) char **argv) {
  Bundle bundle;
  PublisherClient publisher_client;

  bundle.add_apd(0);

  for (int i = 0; i < 10; i++) {
	bundle.set_apd(0, i);

	publisher_client.Publish(bundle);

	std::cout << "Value " << i << " written" << std::endl;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(5000));

  for (int i = 0; i < 100; i++) {
	bundle.set_apd(0, i);

	publisher_client.Publish(bundle);

	std::cout << "Value " << i << " written" << std::endl;

	//		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  //	std::this_thread::sleep_for(std::chrono::milliseconds(20000));

  return 0;
}