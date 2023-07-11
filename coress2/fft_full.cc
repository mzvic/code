#include <algorithm>
#include <csignal>
#include <fftw3.h>
#include <memory>
#include <boost/asio.hpp>
#include <boost/circular_buffer.hpp>

#include "core.grpc.pb.h"
#include "client.h"

using namespace core;
using namespace std::chrono;
using namespace google::protobuf;

//#define SAMPLING_FREQUENCY 100000
#define BUFFER_SIZE 1000000
#define AVERAGE_COUNT 6        // Number of FFTs to average and send

Bundle *publishing_bundle;
PublisherClient *publisher_client;

fftw_plan plan = nullptr;
double input[BUFFER_SIZE], output[BUFFER_SIZE], fft_magnitudes[BUFFER_SIZE / 2 + 1], fft_magnitudes_average[BUFFER_SIZE / 2 + 1];
boost::circular_buffer<double> samples(BUFFER_SIZE);
int fft_count = 0;

bool exit_flag = false;
mutex signal_mutex;
condition_variable signal_cv;

void Send2Broker(const Timestamp &timestamp) {
  publishing_bundle->clear_value();

  for (const double &kElem : fft_magnitudes_average)
	publishing_bundle->add_value(kElem);

  publisher_client->Publish(*publishing_bundle, timestamp);
}

/*double GetWindow(int n, int N) {
  switch (window_type) {
	case 1: {
	  const double a0 = 0.27105140069342;
	  const double a1 = 0.43329793923448;
	  const double a2 = 0.21812299954311;
	  const double a3 = 0.06592544638803;
	  const double a4 = 0.01081174209837;
	  const double a5 = 0.00077658482522;
	  const double a6 = 0.00001388721735;
	  return a0 - a1 * cos(2.0 * M_PI * n / (N - 1)) + a2 * cos(4.0 * M_PI * n / (N - 1)) - a3 * cos(6.0 * M_PI * n / (N - 1)) + a4 * cos(8.0 * M_PI * n / (N - 1)) - a5 * cos(10.0 * M_PI * n / (N - 1)) + a6 * cos(12.0 * M_PI * n / (N - 1));
	}

	case 2: {
	  const double a0 = 0.35875;
	  const double a1 = 0.48829;
	  const double a2 = 0.14128;
	  const double a3 = 0.01168;
	  return a0 - a1 * cos(2.0 * M_PI * n / (N - 1)) + a2 * cos(4.0 * M_PI * n / (N - 1)) - a3 * cos(6.0 * M_PI * n / (N - 1));
	}

	case 3:
	  return 0.54 - 0.46 * cos(2.0 * M_PI * n / (N - 1));

	case 4:
	  return 0.5 * (1 - cos(2.0 * M_PI * n / (N - 1)));

	default:
	  return 1;
  }
}*/

void ProcessBundle(const Bundle &bundle) {
  double max_fft;

//  auto start = high_resolution_clock::now();

  const auto &value = bundle.value();

  // Append received data. Note that we will process the last BUFFER_SIZE samples,
  // so if value and BUFFER_SIZE are not aligned, some old data could be lost
  samples.insert(samples.end(), value.begin(), value.end());

  // Check if we have enough data to proceed
  if (samples.size() < BUFFER_SIZE)
	return;

  // Create input and apply window (window is now commented out until strictly needed)
  for (int i = 0; i < BUFFER_SIZE; i++)
//	input[i] = samples.at(i) * GetWindow(i, BUFFER_SIZE);
	input[i] = samples.at(i);

  // Create plan if needed
  if (plan == nullptr) {
	//Save input data since a new plan erases the input
	double input_backup[BUFFER_SIZE];

	copy(begin(input), end(input), begin(input_backup));

	// Create plan
	plan = fftw_plan_r2r_1d(BUFFER_SIZE, input, output, FFTW_R2HC, FFTW_MEASURE);

	// Restore input
	copy(begin(input_backup), end(input_backup), begin(input));
  }

  // Execute FFTW plan
  fftw_execute(plan);

  // Remove DC component
  fft_magnitudes[0] = 0;

  // Calculate magnitudes
  for (int i = 1; i < (BUFFER_SIZE + 1) / 2; i++)  // (i < N/2 rounded up)
	fft_magnitudes[i] = sqrt(output[i] * output[i] + output[BUFFER_SIZE - i] * output[BUFFER_SIZE - i]);

  if (BUFFER_SIZE % 2 == 0) // Only if N is even. NOLINT: BUFFER_SIZE could be odd too
	fft_magnitudes[BUFFER_SIZE / 2] = abs(output[BUFFER_SIZE / 2]);  // Nyquist frequency

  // Find max magnitude
  max_fft = 0;
  for (const auto &kElem : fft_magnitudes) {
	if (kElem > max_fft)
	  max_fft = kElem;
  }

  // Normalize magnitudes vector
  for (auto &elem : fft_magnitudes)
	elem /= max_fft;

  // Aggregate fft_magnitudes_average, for now
  if (fft_count == 0) {
	memcpy(fft_magnitudes_average, fft_magnitudes, sizeof(fft_magnitudes));
  } else {
	for (int i = 0; i < (BUFFER_SIZE / 2 + 1); i++)
	  fft_magnitudes_average[i] += fft_magnitudes[i];
  }

  fft_count++;

  // Ready to send FFT
  if (fft_count == AVERAGE_COUNT) {
	// Calculate average
	for (auto &elem : fft_magnitudes_average)
	  elem /= AVERAGE_COUNT;

	Send2Broker(bundle.timestamp());

	fft_count = 0;
  }

  // Clear samples. This is a fixed (no sliding) window approach
  samples.clear();

//  auto stop = high_resolution_clock::now();
//  auto duration = duration_cast<microseconds>(stop - start);
//  cout << "FFT Calculation Time: " << duration.count() << " us" << endl;
}

void HandleSignal(int) {
  unique_lock<mutex> slck(signal_mutex);

  cout << "Exiting..." << endl;

  exit_flag = true;

  signal_cv.notify_one();
}

int main() {
//  if (argc < 4) {
//	cout << "Not enough arguments" << endl;
//
//	return 1;
//  }
//
//  window_type = stoi(argv[2]);
//  stop_view_frequency = stoi(argv[3]);

  unique_lock<mutex> slck(signal_mutex);
  SubscriberClient subscriber_client(&ProcessBundle, vector<int>{DATA_APD_FULL});
  publisher_client = new PublisherClient();
  publishing_bundle = new Bundle();
  publishing_bundle->set_type(DATA_FFT_FULL);

  // Register handler
  std::signal(SIGINT, HandleSignal);

  // Wait fot CTRL-C signal
  signal_cv.wait(slck, [] { return exit_flag; });

  // Destroy FFTW plan
  if (plan != nullptr)
	fftw_destroy_plan(plan);

  free(publisher_client);
  free(publishing_bundle);

  return 0;
}