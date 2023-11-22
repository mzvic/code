#include <thread>
#include <csignal>

#include "core.grpc.pb.h"
#include "broker_client.h"

//unique_ptr<PublisherClient> publisher_client;

vector<double> frequencies;

bool exit_flag = false;
mutex signal_mutex;
condition_variable signal_cv;

void HandleSignal(int) {
  unique_lock<mutex> slck(signal_mutex);

  LOG("Exiting");

  exit_flag = true;
}

#pragma clang diagnostic ignored "-Wimplicit-const-int-float-conversion"
#pragma ide diagnostic ignored "cppcoreguidelines-narrowing-conversions"
#pragma ide diagnostic ignored "cert-msc50-cpp"
int main(int argc, char *argv[]) {
  Bundle bundle;
  long sample;
  int output;

  if (argc < 2) {
	cout << "Not enough arguments" << endl;

	return 1;
  }

//  publisher_client = new PublisherClient();
//  publisher_client = make_unique<PublisherClient>();
  PublisherClient publisher_client;

  for (int i = 1; i < argc; i++)
	frequencies.push_back(stod(argv[i]));

  signal(SIGINT, HandleSignal);

  // Just a testing output
  bundle.set_type(DATA_APD_FULL);
  sample = 0;
  while (!exit_flag) {
	bundle.clear_value();

	for (int i = 0; i < 1000; i++) {
	  output = 0;
	  for (const auto &kElem : frequencies)
		output += (int) (100 * sin(2 * M_PI * (kElem * ((double) sample / 100000))));

	  output += (((float) rand() / RAND_MAX) - 0.5f) * 10;    // Add some noise

	  bundle.add_value(output);

	  sample++;
	}

	publisher_client.Publish(bundle);

	std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

//  delete publisher_client;

  return 0;
}