#include <algorithm>
#include <csignal>
#include <fftw3.h>
#include <memory>
#include <numeric>
#include <boost/asio.hpp>
#include <boost/circular_buffer.hpp>
#include <omp.h>

#include "core.grpc.pb.h"
#include "client.h"

using namespace core;
using namespace std::chrono;
using namespace google::protobuf;

#define SAMPLING_FREQUENCY 100000
#define FFT_PEAKS_SIZE 10000
#define BUFFER_SIZE 1000000
int samples_count = 0;
//#define BUFFER_SIZE 131072
//#define BUFFER_SIZE 262144


int window_type = 0;

Bundle *publishing_bundle;
PublisherClient *publisher_client;

// More accumulation before report. Some kind of data accumulation gate
fftw_plan plan = nullptr;
vector<int> idx(BUFFER_SIZE / 2 + 1);
double input[BUFFER_SIZE], output[BUFFER_SIZE], fft_magnitudes[BUFFER_SIZE / 2 + 1], fft_peaks_frequencies[FFT_PEAKS_SIZE], fft_peaks_magnitudes[FFT_PEAKS_SIZE];
boost::circular_buffer<double> samples(BUFFER_SIZE);

bool exit_flag = false;
mutex signal_mutex;
condition_variable signal_cv;


void Send2Broker() {
//  auto start = high_resolution_clock::now();

  publishing_bundle->clear_value();

  for (const double &fft_peaks_magnitude : fft_peaks_magnitudes)
	publishing_bundle->add_value(fft_peaks_magnitude);

  for (const double &fft_peaks_frequencie : fft_peaks_frequencies)
	publishing_bundle->add_value(fft_peaks_frequencie);

  publisher_client->Publish(*publishing_bundle);

//  auto stop = high_resolution_clock::now();
//  auto duration = duration_cast<microseconds>(stop - start);
//  cout << "FFT Calculation Time: " << duration.count() << " us" << endl;
}

double GetWindow(int n, int N) {
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
}

void ProcessBundle(const Bundle &bundle) {
  double max_fft;

  auto start = high_resolution_clock::now();

  const auto &kApd = bundle.value();

  // Put received data at the tail

  samples.insert(samples.end(), kApd.begin(), kApd.end());
  if (samples_count < 5){
	samples_count++;
	return;  
	}
  // Check if we have enough data to proceed
  if (samples.size() < BUFFER_SIZE)
	return;

  // Create input and apply window
  for (int i = 0; i < BUFFER_SIZE; i++)
	input[i] = samples.at(i) * GetWindow(i, BUFFER_SIZE);
	
  // Create plan if needed
  if (plan == nullptr) {
    // Create plan
    double* input_copy = input;
    plan = fftw_plan_r2r_1d(BUFFER_SIZE, input_copy, output, FFTW_R2HC, FFTW_MEASURE);
  }

  // Execute FFTW plan
  fftw_execute(plan);

  fft_magnitudes[0] = 0;

  for (int i = 1; i < (BUFFER_SIZE + 1) / 2; i++)  // (i < N/2 rounded up)
	fft_magnitudes[i] = sqrt(output[i] * output[i] + output[BUFFER_SIZE - i] * output[BUFFER_SIZE - i]);

  // Find top 10 elements
  // Initialize index vector
  
  for (int i = 0; i < BUFFER_SIZE / 2 + 1; ++i) {
    idx[i] = i;
  }
  
  // Sort
  partial_sort(idx.begin(), idx.begin() + 100, idx.end(), [](size_t i1, size_t i2) { return fft_magnitudes[i1] > fft_magnitudes[i2]; });

  // Fill peaks information
  max_fft = 0;
  for (int i = 0; i < FFT_PEAKS_SIZE; i++) {
    fft_peaks_magnitudes[i] = fft_magnitudes[idx[i]];
    fft_peaks_frequencies[i] = idx[i] * (static_cast<double>(SAMPLING_FREQUENCY) / BUFFER_SIZE);
    max_fft = std::max(max_fft, fft_peaks_magnitudes[i]);
  }

  // Normalize magnitudes vector
  for (auto &elem : fft_peaks_magnitudes)
    elem /= max_fft;

  // Ready to send FFT
  Send2Broker();
  samples_count = 0;
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

int main() {
//  if (argc < 4) {
//	cout << "Not enough arguments" << endl;
//
//	return 1;
//  }
//
//  window_type = stoi(argv[2]);
//  stop_view_frequency = stoi(argv[3]);
  cout << "Threads: " << omp_get_max_threads() << endl;
  unique_lock<mutex> slck(signal_mutex);
  SubscriberClient subscriber_client(&ProcessBundle, vector<int>{DATA_APD_FULL});
  publisher_client = new PublisherClient();
  publishing_bundle = new Bundle();
  publishing_bundle->set_type(DATA_FFT_FULL);


  // Register handler
  std::signal(SIGINT, HandleSignal);

//  // Connect to GUI
//  boost::asio::io_service io_service;
//  socket_ = std::make_shared<boost::asio::ip::tcp::socket>(io_service);
//  socket_->connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 12352));


  fftw_init_threads();
  fftw_plan_with_nthreads(omp_get_max_threads());
  // Wait fot CTRL-C signal
  signal_cv.wait(slck, [] { return exit_flag; });

  // Destroy FFTW plan
  if (plan != nullptr)
	fftw_destroy_plan(plan);

  free(publisher_client);
  free(publishing_bundle);

  return 0;
}
