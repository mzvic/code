#include <algorithm>
#include <csignal>
#include <fftw3.h>
//#include <boost/asio.hpp>
#include <boost/circular_buffer.hpp>
#include <cstdlib> 
#include <iostream>

#include "core.grpc.pb.h"
#include "broker_client.h"

using namespace core;
using namespace std::chrono;
using namespace google::protobuf;

#define BUFFER_SIZE 10000000    // 0.01 Hz resolution at a sampling frequency of 100000
//#define AVERAGE_COUNT 5        // Number of FFTs to average and send

int AVERAGE_COUNT = 5;

unique_ptr<Bundle> publishing_bundle;
unique_ptr<PublisherClient> publisher_client;
//unique_ptr<SubscriberClient> subscriber_client;

fftw_plan plan = nullptr;
double input[BUFFER_SIZE], output[BUFFER_SIZE], fft_magnitudes[BUFFER_SIZE / 2 + 1], fft_magnitudes_average[BUFFER_SIZE / 2 + 1];
boost::circular_buffer<double> samples(BUFFER_SIZE);
int fft_count = 0;

bool exit_flag = false;
mutex signal_mutex;
condition_variable signal_cv;

void SendToBroker(const Timestamp &timestamp) {
  cout << "Publishing data" << endl;

  publishing_bundle->clear_value();

  for (const double &kElem : fft_magnitudes_average)
	publishing_bundle->add_value(kElem);

//  publisher_client->Publish(*publishing_bundle, timestamp);
  publisher_client->Publish(*publishing_bundle, timestamp);
}

void ProcessBundle(const Bundle &bundle) {
  auto start = high_resolution_clock::now();

  const auto &kValue = bundle.value();

  // Append received data. Note that we will process the last BUFFER_SIZE samples,
  // so if value and BUFFER_SIZE are not aligned, some old data could be lost
  samples.insert(samples.end(), kValue.begin(), kValue.end());

  cout << "\33[2K\r" << ((float) samples.size() / BUFFER_SIZE) * 100.0 << " % samples" << flush;

  // Check if we have enough data to proceed
  if (samples.size() < BUFFER_SIZE)
	return;

  cout << endl;

  // Create input
  for (int i = 0; i < BUFFER_SIZE; i++)
	input[i] = samples.at(i);

  // Create plan if needed
  if (plan == nullptr) {
	cout << "Creating plan" << endl;
	// Save input data since a new plan erases the input
	//	double input_backup[BUFFER_SIZE];
	auto *input_backup = new double[BUFFER_SIZE];

	//	copy(begin(input), end(input), begin(input_backup));
	memcpy(input_backup, input, BUFFER_SIZE * sizeof(double));

	// Create plan
	plan = fftw_plan_r2r_1d(BUFFER_SIZE, input, output, FFTW_R2HC, FFTW_ESTIMATE);    // FFTW_MEASURE for a better estimation, but a longer execution time

	// Restore input
	//	copy(begin(input_backup), end(input_backup), begin(input));
	memcpy(input, input_backup, BUFFER_SIZE * sizeof(double));

	// Delete backup
	delete[] input_backup;
  }

  cout << "Executing plan" << endl;

  // Execute FFTW plan
  fftw_execute(plan);

  // Remove DC component
  fft_magnitudes[0] = 0;

  // Calculate magnitudes
  for (int i = 1; i < (BUFFER_SIZE + 1) / 2; i++)  // (i < N/2 rounded up)
	fft_magnitudes[i] = sqrt(output[i] * output[i] + output[BUFFER_SIZE - i] * output[BUFFER_SIZE - i]);

  if (BUFFER_SIZE % 2 == 0) // Only if N is even. NOLINT: BUFFER_SIZE could be odd too
	fft_magnitudes[BUFFER_SIZE / 2] = abs(output[BUFFER_SIZE / 2]);  // Nyquist frequency

  // Aggregate fft_magnitudes_average
  if (fft_count == 0) {
	memcpy(fft_magnitudes_average, fft_magnitudes, sizeof(fft_magnitudes));
  } else {
	for (int i = 0; i < (BUFFER_SIZE / 2 + 1); i++)
	  fft_magnitudes_average[i] += fft_magnitudes[i];
  }

  fft_count++;

  // Ready to send FFT
  if (fft_count == AVERAGE_COUNT) {
	double max_fft;

	// Calculate average
	for (auto &elem : fft_magnitudes_average)
	  elem /= AVERAGE_COUNT;

	// Find max magnitude
	max_fft = 0;
	for (const auto &kElem : fft_magnitudes_average) {
	  if (kElem > max_fft)
		max_fft = kElem;
	}

	// Normalize magnitudes vector
	for (auto &elem : fft_magnitudes_average)
	  elem /= max_fft;

	SendToBroker(bundle.timestamp());

	fft_count = 0;
  }

  // Clear samples. This is a fixed (no sliding) window approach
  samples.clear();

  auto stop = high_resolution_clock::now();
  auto duration = duration_cast<microseconds>(stop - start);
  cout << "FFT Calculation Time: " << duration.count() << " us" << endl;
}

void HandleSignal(int) {
  unique_lock<mutex> slck(signal_mutex);

  cout << "Exiting..." << endl;

  exit_flag = true;

  signal_cv.notify_one();
}

int main(int argc, char *argv[]) {
  
    if (argc != 2) {
        std::cerr << "Uso: " << argv[0] << " <AVERAGE_COUNT>" << std::endl;
        return 1;     }

    AVERAGE_COUNT = std::atoi(argv[1]);

    if (AVERAGE_COUNT < 1) {
        std::cerr << "AVERAGE_COUNT must be a positive integer." << std::endl;
        return 1; 
    }
    
  
  
  unique_lock<mutex> slck(signal_mutex);

//  publisher_client = new PublisherClient();
  publisher_client = make_unique<PublisherClient>();
//  request_.mutable_types()->Assign(request.begin(), request.end());
//  subscriber_client = new SubscriberClient(&ProcessBundle, vector<int>{DATA_APD_FULL});
//  subscriber_client = make_unique<SubscriberClient>(&ProcessBundle, vector<int>{DATA_APD_FULL});
  SubscriberClient subscriber_client(&ProcessBundle, vector<int>{DATA_APD_FULL});
//  subscriber_client = new SubscriberClient(&ProcessBundle, &interests);
//  publishing_bundle = new Bundle();
  publishing_bundle = make_unique<Bundle>();

  publishing_bundle->set_type(DATA_FFT_FULL);
  
  // Register handler
  std::signal(SIGINT, HandleSignal);

  // Initialize FFT threads
  fftw_init_threads();
  fftw_plan_with_nthreads(8);

  // Wait fot CTRL-C signal
  signal_cv.wait(slck, [] { return exit_flag; });

  // Destroy FFTW plan
  if (plan != nullptr)
	fftw_destroy_plan(plan);

  fftw_cleanup_threads();

//  delete subscriber_client;
//  delete publisher_client;
//  delete publishing_bundle;

  return 0;
}
