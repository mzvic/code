#include <algorithm>
#include <csignal>
#include <fftw3.h>
#include <memory>
#include <boost/asio.hpp>
#include <boost/circular_buffer.hpp>

#include "core.grpc.pb.h"
#include "broker_client.h"

using namespace core;
using namespace std::chrono;
using namespace google::protobuf;
int samples_count = 0;
#define SAMPLING_FREQUENCY 100000
#define BUFFER_SIZE 1000000
#define MAX_BIN_COUNT 100

Bundle *publishing_bundle;
PublisherClient *publisher_client;

int window_type = 0;
#define PI 3.14159265358979323846
std::vector<float> window;

fftw_plan plan = nullptr;
double input[BUFFER_SIZE], output[BUFFER_SIZE], fft_magnitudes[BUFFER_SIZE / 2 + 1], fft_bin_frequencies[MAX_BIN_COUNT], fft_bin_magnitudes[MAX_BIN_COUNT];
boost::circular_buffer<double> samples(BUFFER_SIZE);
int bin_size, bin_count;
int start_idx, stop_idx;
double frequency_resolution;

bool exit_flag = false;
std::mutex signal_mutex;
std::condition_variable signal_cv;

void Send2Broker(const Timestamp &timestamp) { // Function that sends the values to the broker
  int i;
  publishing_bundle->clear_value(); // Cleans the DATA_FFT_PARTIAL variable of the bundle
  for (i = 0; i < bin_count; i++) {
	publishing_bundle->add_value(fft_bin_frequencies[i]);  // Add frequency values to be sended to the broker 
  }
  for (i = 0; i < bin_count; i++) {
	publishing_bundle->add_value(fft_bin_magnitudes[i]);  // Add magnitude values to be sended to the broker 
  }
  publisher_client->Publish(*publishing_bundle, timestamp); // Sends values to the broker (freq and magn)
}

void GenerateWindow(int N, int window_type) { // Function that generates the window to apply to the FFT (only once at the beggining of the process)
    window.clear(); // Cleans the window
    window.resize(N); // Assign the size of the window
    switch (window_type) { // Switch case depending on the window selected in the GUI (window_type)
        case 4: {  // If window_type = 4, generates the Blackman Harris 7 parameters window (in vector 'window')
            const double a0 = 0.27105140069342;
            const double a1 = 0.43329793923448;
            const double a2 = 0.21812299954311;
            const double a3 = 0.06592544638803;
            const double a4 = 0.01081174209837;
            const double a5 = 0.00077658482522;
            const double a6 = 0.00001388721735;
            for (int n = 0; n < N; ++n) {
                window[n] = static_cast<float>(a0 - a1 * cos(2.0 * PI * n / (N - 1)) +
                                               a2 * cos(4.0 * PI * n / (N - 1)) -
                                               a3 * cos(6.0 * PI * n / (N - 1)) +
                                               a4 * cos(8.0 * PI * n / (N - 1)) -
                                               a5 * cos(10.0 * PI * n / (N - 1)) +
                                               a6 * cos(12.0 * PI * n / (N - 1)));
            }
            break;
        }
        case 3: {  // If window_type = 3, generates the Blackman Harris 4 parameters window (in vector 'window')
            const double b0 = 0.35875;
            const double b1 = 0.48829;
            const double b2 = 0.14128;
            const double b3 = 0.01168;
            for (int n = 0; n < N; ++n) {
                window[n] = static_cast<float>(b0 - b1 * cos(2.0 * PI * n / (N - 1)) +
                                               b2 * cos(4.0 * PI * n / (N - 1)) -
                                               b3 * cos(6.0 * PI * n / (N - 1)));
            }
            break;
        }
        case 1: {  // If window_type = 1, generates the Hamming window (in vector 'window')
            for (int n = 0; n < N; ++n) {
                window[n] = static_cast<float>(0.54 - 0.46 * cos(2.0 * PI * n / (N - 1)));
            }
            break;
        }
        case 2: {  // If window_type = 2, generates the Hann window (in vector 'window')
            for (int n = 0; n < N; ++n) {
                window[n] = static_cast<float>(0.5 * (1 - cos(2.0 * PI * n / (N - 1))));
            }
            break;
        }
        case 5: {  // If window_type = 5, generates a '1' window (in vector 'window')
            for (int n = 0; n < N; ++n) {
                window[n] = 1.0f;
            }
            break;
        }
        default: {  // By default, generates a '1' window (in vector 'window')
            for (int n = 0; n < N; ++n) {
                window[n] = 1.0f;
            }
            break;
        }
    }
}

void ProcessBundle(const Bundle &bundle) { // Function that receives the values from the broker and process the data to obtain the FFT
  double max_fft;
  int i, j, k;
  // Assigns the incoming data to the 'kValue' variable
  const auto &kValue = bundle.value();
  // Append received data. Note that we will process the last BUFFER_SIZE samples,
  // so if kValue and BUFFER_SIZE are not aligned, some old data could be lost
  samples.insert(samples.end(), kValue.begin(), kValue.end());
  // Only proceed if we have 1000*5 = 5.000 values on 'samples', e.i. once each 50 milliseconds
  if (samples_count < 5){
	samples_count++;
	return;  
	}
  // Check if we have enough data to proceed
  if (samples.size() < BUFFER_SIZE)
    return;
  // Create input and apply window
  for (int i = 0; i < BUFFER_SIZE; i++)
	input[i] = samples.at(i) * window[i];
  // If the vector size that contains the samples is bigger than the buffer, we erase the last 'BUFFER_SIZE' values of 'samples'
  if (samples.size() > BUFFER_SIZE) {
    samples.erase(samples.begin(), samples.begin() + (samples.size() - BUFFER_SIZE));
  }
  // Create plan if needed
  if (plan == nullptr) {
    // Save input data since a new plan erases the input
    double input_backup[BUFFER_SIZE];
    std::copy(std::begin(input), std::end(input), std::begin(input_backup));
    // Create plan
    plan = fftw_plan_r2r_1d(BUFFER_SIZE, input, output, FFTW_R2HC, FFTW_MEASURE);
    // Restore input
    std::copy(std::begin(input_backup), std::end(input_backup), std::begin(input));
  }
  // Execute FFTW plan
  fftw_execute(plan);
  // Remove DC component
  fft_magnitudes[0] = 0;
  // Calculate magnitudes
  for (i = 1; i < (BUFFER_SIZE + 1) / 2; i++)  // (i < N/2 rounded up)
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
  // Fill bins
  std::vector<std::pair<double, double>> magnitudes_frecuencias;
  for (i = start_idx; i <= stop_idx; i++) {
    double frecuencia = i * frequency_resolution;
    double magnitud = fft_magnitudes[i];
    magnitudes_frecuencias.emplace_back(magnitud, frecuencia);
  }
  // Sort the data from greater to smaller
  std::partial_sort(magnitudes_frecuencias.begin(), magnitudes_frecuencias.begin() + bin_count, magnitudes_frecuencias.end(), std::greater<std::pair<double, double>>());
  // Assign the 'MAX_BIN_COUNT' (100) greater magnitudes and it's frequencies to the 'fft_bin_freq/magn' vectors
  bin_count = std::min(static_cast<int>(magnitudes_frecuencias.size()), MAX_BIN_COUNT);
  for (i = 0; i < bin_count; i++) {
    fft_bin_frequencies[i] = magnitudes_frecuencias[i].second;
    fft_bin_magnitudes[i] = magnitudes_frecuencias[i].first;
  }
  // Ready to send FFT
  Send2Broker(bundle.timestamp());
  // Cleans the count started in line 119
  samples_count = 0; 
}

void HandleSignal(int) { // Function to exit process
  std::unique_lock<std::mutex> slck(signal_mutex);
  std::cout << "Exiting..." << std::endl;
  exit_flag = true;
  signal_cv.notify_one();
}

int main(int argc, char *argv[]) {
  int start_frequency, stop_frequency;
  start_frequency = std::max(0, std::stoi(argv[1])); // Argument that sets the start of the frequency of interest
  stop_frequency = std::min(std::stoi(argv[2]), SAMPLING_FREQUENCY / 2) + 1; // Argument that sets the end of the frequency of interest
  window_type = stoi(argv[3]); // Argument that sets the FFT window
  // If 'window' is empty, generates the window, if not, it goes ahead with the existing window
  if (window.empty()) {
    GenerateWindow(BUFFER_SIZE, window_type);
    cout << "Creating window..." << endl;
  }  
  // Calculate bin info
  frequency_resolution = (double) SAMPLING_FREQUENCY / BUFFER_SIZE;
  // Check if we can fill all bins
  bin_count = std::min((int) ((stop_frequency - start_frequency) / frequency_resolution), MAX_BIN_COUNT);
  bin_size = std::floor(((stop_frequency - start_frequency) / frequency_resolution) / bin_count);
  start_idx = std::floor(start_frequency / frequency_resolution); // Start of the frequency of interest
  stop_idx = std::min(start_idx + bin_count * bin_size, BUFFER_SIZE / 2 + 1 - bin_size); // End of the frequency of interest
  // Locks the thread
  std::unique_lock<std::mutex> slck(signal_mutex);
  // Indicates the interest variable for the susbscribe method (to get the data from the broker)
  SubscriberClient subscriber_client(&ProcessBundle, std::vector<int>{DATA_APD_FULL});
  // Indicates the broker publishing variable
  publisher_client = new PublisherClient();
  publishing_bundle = new Bundle();
  publishing_bundle->set_type(DATA_FFT_PARTIAL);
  // Register handler
  std::signal(SIGINT, HandleSignal);
  // Wait for CTRL-C signal
  signal_cv.wait(slck, [] { return exit_flag; });
  // Destroy FFTW plan
  if (plan != nullptr)
    fftw_destroy_plan(plan);
  delete publisher_client;
  delete publishing_bundle;
  return 0;
}
