#include <algorithm>
#include <csignal>
#include <iterator>
#include <fftw3.h>
#include <memory>
#include <numeric>
#include <boost/asio.hpp>
#include <boost/circular_buffer.hpp>

#include "core.grpc.pb.h"
#include "client.h"

using namespace core;
using namespace std::chrono;
using namespace google::protobuf;

#define SAMPLING_FREQUENCY 100000
#define FFT_PEAKS_SIZE 100
#define BUFFER_SIZE 100000
//#define BUFFER_SIZE 131072
//#define BUFFER_SIZE 262144

//int stop_view_frequency;
int window_type;

// More accumulation before report. Some kind of data accumulation gate
fftw_plan plan = nullptr;
//vector<double> samples;
vector<int> idx(BUFFER_SIZE / 2 + 1);
//input(BUFFER_SIZE)
double input[BUFFER_SIZE], output[BUFFER_SIZE], fft_magnitudes[BUFFER_SIZE / 2 + 1], fft_peaks_frequencies[FFT_PEAKS_SIZE], fft_peaks_magnitudes[FFT_PEAKS_SIZE];
boost::circular_buffer<double> samples(BUFFER_SIZE);

bool exit_flag = false;
mutex signal_mutex;
condition_variable signal_cv;

std::shared_ptr<boost::asio::ip::tcp::socket> socket_;
void Send2Socket() {
  std::string data;

  for (int i = 0; i < FFT_PEAKS_SIZE; i++)
	data += " " + to_string(fft_peaks_frequencies[i]) + " " + to_string(fft_peaks_magnitudes[i]) + "\n";

  boost::asio::write(*socket_, boost::asio::buffer(data + "\n"));
}

double GetWindow(int n, int N) {
  switch (window_type) {
	case 4: {
	  const double a0 = 0.27105140069342;
	  const double a1 = 0.43329793923448;
	  const double a2 = 0.21812299954311;
	  const double a3 = 0.06592544638803;
	  const double a4 = 0.01081174209837;
	  const double a5 = 0.00077658482522;
	  const double a6 = 0.00001388721735;
	  return a0 - a1 * cos(2.0 * M_PI * n / (N - 1)) + a2 * cos(4.0 * M_PI * n / (N - 1)) - a3 * cos(6.0 * M_PI * n / (N - 1)) + a4 * cos(8.0 * M_PI * n / (N - 1)) - a5 * cos(10.0 * M_PI * n / (N - 1)) + a6 * cos(12.0 * M_PI * n / (N - 1));
	}

	case 3: {
	  const double a0 = 0.35875;
	  const double a1 = 0.48829;
	  const double a2 = 0.14128;
	  const double a3 = 0.01168;
	  return a0 - a1 * cos(2.0 * M_PI * n / (N - 1)) + a2 * cos(4.0 * M_PI * n / (N - 1)) - a3 * cos(6.0 * M_PI * n / (N - 1));
	}

	case 1:
	  return 0.54 - 0.46 * cos(2.0 * M_PI * n / (N - 1));

	case 2:
	  return 0.5 * (1 - cos(2.0 * M_PI * n / (N - 1)));
	  
	case 5:
	  return 1;	  

	default:
	  return 1;
  }
}

void ProcessBundle(const Bundle &bundle) {
  double max_fft;

//  auto start = high_resolution_clock::now();

  const auto &kApd = bundle.apd();

  // Put received data at the tail
  samples.insert(samples.end(), kApd.begin(), kApd.end());
std::cout << "------------------------------------------> Samples size: " << samples.size() <<  std::endl;
  // Check if we have enough data to proceed
  if (samples.size() < BUFFER_SIZE)
	return;

  // Create input and apply window
  for (int i = 0; i < BUFFER_SIZE; i++)
	input[i] = samples.at(i) * GetWindow(i, BUFFER_SIZE);

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

  fft_magnitudes[0] = 0;

  for (int i = 1; i < (BUFFER_SIZE + 1) / 2; i++)  // (i < N/2 rounded up)
	fft_magnitudes[i] = sqrt(output[i] * output[i] + output[BUFFER_SIZE - i] * output[BUFFER_SIZE - i]);

  if (BUFFER_SIZE % 2 == 0) // Only if N is even. NOLINT: BUFFER_SIZE could be odd too
	fft_magnitudes[BUFFER_SIZE / 2] = abs(output[BUFFER_SIZE / 2]);  // Nyquist frequency

  // Find top 10 elements
  // Initialize index vector
  iota(idx.begin(), idx.end(), 0);

  // Sort
  partial_sort(idx.begin(), idx.begin() + 100, idx.end(), [](size_t i1, size_t i2) { return fft_magnitudes[i1] > fft_magnitudes[i2]; });

  // Fill peaks information
  max_fft = 0;
  for (int i = 0; i < FFT_PEAKS_SIZE; i++) {
	fft_peaks_magnitudes[i] = fft_magnitudes[idx[i]];
	fft_peaks_frequencies[i] = idx[i] * ((double) SAMPLING_FREQUENCY / BUFFER_SIZE);

	if (fft_peaks_magnitudes[i] > max_fft)
	  max_fft = fft_peaks_magnitudes[i];
  }

  // Normalize magnitudes vector
  for (auto &elem : fft_peaks_magnitudes)
	elem /= max_fft;

  // Ready to send FFT
  Send2Socket();

//  auto stop = high_resolution_clock::now();
//  auto duration = duration_cast<microseconds>(stop - start);
//  cout << "FFT Calculation Time: " << duration.count() << " us" << endl;

//  for (int i = 0; i < FFT_PEAKS_SIZE; i++)
//	cout << fft_peaks_frequencies[i] << "," << fft_peaks_magnitudes[i] << endl;
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
  SubscriberClient subscriber_client(&ProcessBundle);

  // Register handler
  std::signal(SIGINT, HandleSignal);

  // Connect to GUI
  boost::asio::io_service io_service;
  socket_ = std::make_shared<boost::asio::ip::tcp::socket>(io_service);
  socket_->connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 12352));

  // Wait fot CTRL-C signal
  signal_cv.wait(slck, [] { return exit_flag; });

  // Destroy FFTW plan
  if (plan != nullptr)
	fftw_destroy_plan(plan);

  return 0;
}
