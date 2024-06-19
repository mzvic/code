
#include <csignal>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <iostream>

#include "core.grpc.pb.h"
#include "broker_client.h"
#include "storage_client.h"

using namespace core;
using namespace std::chrono;
using namespace google::protobuf;

std::unique_ptr<Bundle> publishing_bundle;
std::unique_ptr<PusherClient> pusher_client;
std::mutex publishing_bundle_mutex;

bool exit_flag = false;
std::mutex signal_mutex;
std::condition_variable signal_cv;

void ProcessBundle(const Bundle &bundle) {
    std::lock_guard<std::mutex> lock(publishing_bundle_mutex);

  switch (bundle.type()) {
	case DATA_APD_FULL:
	  publishing_bundle->set_type(STORAGE_APD_FULL);

	  publishing_bundle->clear_value();
	  publishing_bundle->mutable_value()->CopyFrom(bundle.value());

	  pusher_client->Push(*publishing_bundle, bundle.timestamp());

	  break;

	case DATA_FFT_FULL:
	  publishing_bundle->set_type(STORAGE_FFT_FULL);

	  publishing_bundle->clear_value();
	  publishing_bundle->mutable_value()->CopyFrom(bundle.value());

	  pusher_client->Push(*publishing_bundle, bundle.timestamp());

	  break;

	case DATA_TT_MON:
	  publishing_bundle->set_type(STORAGE_TT_MON);

	  publishing_bundle->clear_value();
	  publishing_bundle->mutable_value()->CopyFrom(bundle.value());

	  pusher_client->Push(*publishing_bundle, bundle.timestamp());

	  break;

	case DATA_RIGOL_MON:
	  publishing_bundle->set_type(STORAGE_RIGOL_MON);

	  publishing_bundle->clear_value();
	  publishing_bundle->mutable_value()->CopyFrom(bundle.value());

	  pusher_client->Push(*publishing_bundle, bundle.timestamp());

	  break;

	case DATA_LASER_MON:
	  publishing_bundle->set_type(STORAGE_LASER_MON);

	  publishing_bundle->clear_value();
	  publishing_bundle->mutable_value()->CopyFrom(bundle.value());

	  pusher_client->Push(*publishing_bundle, bundle.timestamp());

	  break;

	case DATA_EG_MON:
	  publishing_bundle->set_type(STORAGE_EG_MON);

	  publishing_bundle->clear_value();
	  publishing_bundle->mutable_value()->CopyFrom(bundle.value());

	  pusher_client->Push(*publishing_bundle, bundle.timestamp());

	  break;

	default:
	  break;
  }
}

void MonitorThread(std::function<void()> monitorFunction) {
    timespec ts{};

    while (!exit_flag) {
	    // Get current time
	    clock_gettime(CLOCK_REALTIME, &ts);

	    // Set next event time
	    ts.tv_sec++;
	    ts.tv_nsec = 0;

	    // Sleep until next event. Restart
        while (clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &ts, nullptr)) {}

        std::lock_guard<std::mutex> lock(publishing_bundle_mutex);
        monitorFunction();
    }
}

void HandleSignal(int) {
    std::unique_lock<std::mutex> slck(signal_mutex);
    std::cout << "Exiting..." << std::endl;
    exit_flag = true;
    signal_cv.notify_one();
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <DATA_TYPE_1> <DATA_TYPE_2> ..." << std::endl;
        return 1;
    }

    std::unique_lock<std::mutex> slck(signal_mutex);

	std::vector<std::string> data_types(argv + 1, argv + argc);

	std::cout << "Tipos de datos especificados:" << std::endl;
	for (const auto &data_type : data_types) {
	    std::cout << "- " << data_type << std::endl;
	}

	// mapping between data type strings and integer values
	std::unordered_map<std::string, int> dataTypeMapping = {
      {"DATA_APD_FULL", 1},
      {"DATA_FFT_FULL", 3},
      {"DATA_TT_MON", 8},
      {"DATA_RIGOL_MON", 10},
      {"DATA_LASER_MON", 12},
      {"DATA_EG_MON", 14}
	};

	std::vector<int> data_types_int;

	// string to int
	for (const auto &data_type : data_types) {
	    // check if data type exists in mapping
	    if (dataTypeMapping.find(data_type) != dataTypeMapping.end()) {
	        // add int to vector
	        data_types_int.push_back(dataTypeMapping[data_type]);
	    } else {
	        std::cerr << "Unknown data type: " << data_type << std::endl;
	    }
	}

    pusher_client = std::make_unique<PusherClient>();
    SubscriberClient subscriber_client(&ProcessBundle, {data_types_int});
    publishing_bundle = std::make_unique<Bundle>();

    std::signal(SIGINT, HandleSignal);
    signal_cv.wait(slck, [] { return exit_flag; });

    return 0;
}

