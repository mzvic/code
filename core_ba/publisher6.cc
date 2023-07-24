#include <thread>
#include <csignal>

#include "core.grpc.pb.h"
#include "client.h"

using namespace core;

bool exit_flag = false;
mutex signal_mutex;
condition_variable signal_cv;

//int frequency;
vector<double> frequencies;

void HandleSignal(int) {
  unique_lock<mutex> slck(signal_mutex);

  cout << "Exiting..." << endl;

  exit_flag = true;

  signal_cv.notify_one();
}

int main(int argc, char *argv[]) {
//  unique_lock<mutex> slck(signal_mutex);
  Bundle bundle;
  PublisherClient publisher_client;
  long sample;
  int signal;

  if (argc < 2) {
	cout << "Not enough arguments" << endl;

	return 1;
  }

  for (int i = 1; i < argc; i++)
	frequencies.push_back(stod(argv[i]));

//  frequency = stoi(argv[1]);

//  cout << "Sending: " << frequencies << " Hz test signals" << endl;

  std::signal(SIGINT, HandleSignal);

//  bundle.add_apd(0);

  // Just a testing signal
  bundle.set_type(DATA_APD_FULL);
  sample = 0;
  while (!exit_flag) {
	bundle.clear_value();

	for (int i = 0; i < 1000; i++) {
	  signal = 0;
	  for (const auto &kElem : frequencies)
		signal += (int) (100 * sin(2 * M_PI * (kElem * ((double) sample / 100000))));

	  bundle.add_value(signal);

	  sample++;
	}

	publisher_client.Publish(bundle);

	std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }


//  for (int i = 0; i < 100; i++) {
//	bundle.set_apd(0, i);
//
//	publisher_client.Publish(bundle);
//
//	std::cout << "Value " << i << " written" << std::endl;
//
//	//		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//  }

  //	std::this_thread::sleep_for(std::chrono::milliseconds(20000));

//  signal_cv.wait(slck, [] { return exit_flag; });

  return 0;
}
