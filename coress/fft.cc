#include <algorithm>
#include <csignal>
#include <iterator>
#include <fftw3.h>
#include <memory>
#include <boost/asio.hpp>

#include "core.grpc.pb.h"
#include "client.h"

using namespace core;
using namespace std::chrono;
using namespace google::protobuf;

#define BUFFER_SIZE 4096
#define SAMPLING_FREQUENCY 100000

int sub_sampling_frequency;
int window_type;

fftw_plan plan = nullptr;
vector<double> samples, sub_samples, fft(BUFFER_SIZE / 2 + 1);
//input(BUFFER_SIZE)
double input[BUFFER_SIZE], output[BUFFER_SIZE];

bool exit_flag = false;
mutex signal_mutex;
condition_variable signal_cv;

std::shared_ptr<boost::asio::ip::tcp::socket> socket_;
//boost::asio::socket_base::reuse_address option(true);

void Send2Socket() {
  double freq_i, fft_i;
  std::string data = "";
  for (int i = 0; i < fft.size(); ++i) {
	if (i == 0) {
	  freq_i = 0.001;
	} else {
	  freq_i = i * ((double) sub_sampling_frequency / BUFFER_SIZE);
	}

	fft_i = fft[i];
	//data += std::to_string(freq_i) + " " + std::to_string(fft_i) + " ";
	data += std::to_string(fft_i) + " ";

  }
  //std::cout << "FFT data: " << data <<  std::endl;
  boost::asio::write(*socket_, boost::asio::buffer(data + "\n"));
  //this_thread::sleep_for(std::chrono::milliseconds(100));
  //socket_->set_option(option);  
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
	//std::cout << "111------------------------------------------------------" <<  std::endl;

  double sample, mean, max_fft;
  int sample_size;

//  auto start = high_resolution_clock::now();

  const auto &kApd = bundle.apd();

  sample_size = SAMPLING_FREQUENCY / sub_sampling_frequency;
//  double sub_samples[kApd.size() / sample_size];

  // Subsample incoming data. Here we just calculate a simple mean per group sample
  sub_samples.clear();
  for (int j = 0; j < kApd.size() / sample_size; j++) {
	sample = 0;

	for (int i = 0; i < sample_size; i++)
	  sample += kApd[i + j * sample_size];
	//std::cout << "sample: "<< sample <<  std::endl;
	sub_samples.push_back(sample / sample_size);
  }

  // Make room for incoming data if needed
  if ((BUFFER_SIZE - samples.size()) < sub_samples.size())
	// We need to free sub_samples size, minus the already available space
	samples.erase(samples.end() + 1 - ((int) sub_samples.size() - (BUFFER_SIZE - (int) samples.size())), samples.end() + 1);

  // Put received data in samples head
  samples.insert(samples.begin(), sub_samples.begin(), sub_samples.end());

  // Check if we have enough data to proceed
  if (samples.size() < BUFFER_SIZE)
	return;

  // Create input and apply window
  for (int i = 0; i < BUFFER_SIZE; i++)
	input[i] = samples.at(i) * GetWindow(i, BUFFER_SIZE);

  // Remove DC Component
  mean = 0;
  for (auto &elem : input)
	mean += elem;

  mean /= BUFFER_SIZE;

  for (auto &elem : input)
	elem -= mean;
    	

  // Create plan if needed
  if (plan == nullptr) {
	//Save input data since a new plan erases the input
//	auto input_backup = input;
	double input_backup[BUFFER_SIZE];
	copy(begin(input), end(input), begin(input_backup));

	// Create plan
	plan = fftw_plan_r2r_1d(BUFFER_SIZE, input, output, FFTW_R2HC, FFTW_MEASURE);

	// Restore input
	copy(begin(input_backup), end(input_backup), begin(input));
  }

  // Execute FFTW plan
  fftw_execute(plan);

  // Calculate FFT
  fft[0] = abs(output[0]);

  for (int i = 1; i < (BUFFER_SIZE + 1) / 2; i++) { // (i < N/2 rounded up)
	fft[i] = sqrt(output[i] * output[i] + output[BUFFER_SIZE - i] * output[BUFFER_SIZE - i]);
	//std::cout << "FFT["<< i << "]: " << fft[i] <<  std::endl;
	}

  if (BUFFER_SIZE % 2 == 0) // Only if N is even. NOLINT: BUFFER_SIZE could be odd too
	fft[BUFFER_SIZE / 2] = output[BUFFER_SIZE / 2];  // Nyquist frequency

  // Normalize FFT
  max_fft = *max_element(fft.begin(), fft.end());

//  if (max_fft > 0) {
  for (auto &elem : fft)
	elem /= max_fft;
//  }

//  cout << "----NORMALIZED----" << endl;
//  cout << "Max: " << max_fft << ", Size: " << fft.size() << endl;
//  for (int i = 0; i < 10; ++i) {
//	cout << i << "," << i * ((double) sub_sampling_frequency / BUFFER_SIZE) << "," << fft[i] << endl;
//  }

  // Ready to send FFT
  Send2Socket();

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

int main(int argc, char *argv[]) {
  //if (argc < 2) {
    //cout << "Not enough arguments" << endl;
	//return 1;
  //}

  window_type = stoi(argv[1]);
  sub_sampling_frequency = stoi(argv[2]);

  unique_lock<mutex> slck(signal_mutex);
  // Connect to GUI
  boost::asio::io_service io_service;
  socket_ = std::make_shared<boost::asio::ip::tcp::socket>(io_service);
  socket_->connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 12352));

  SubscriberClient subscriber_client(&ProcessBundle);

  // Register handler
  std::signal(SIGINT, HandleSignal);

  // Wait fot CTRL-C signal
  signal_cv.wait(slck, [] { return exit_flag; });

  // Destroy FFTW plan
  if (plan != nullptr)
	fftw_destroy_plan(plan);

  return 0;
}

