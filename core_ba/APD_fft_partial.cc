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
int samples_count = 0;
#define SAMPLING_FREQUENCY 100000
#define BUFFER_SIZE 100000
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

void Send2Broker(const Timestamp &timestamp) {
  int i;

  publishing_bundle->clear_value();

  for (i = 0; i < bin_count; i++) {
	publishing_bundle->add_value(fft_bin_frequencies[i]);
  }

  for (i = 0; i < bin_count; i++) {
	publishing_bundle->add_value(fft_bin_magnitudes[i]);
  }

  publisher_client->Publish(*publishing_bundle, timestamp);
}


void GenerateWindow(int N, int window_type) {
    window.clear();
    window.resize(N);

    switch (window_type) {
        case 4: {
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

        case 3: {
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

        case 1: {
            for (int n = 0; n < N; ++n) {
                window[n] = static_cast<float>(0.54 - 0.46 * cos(2.0 * PI * n / (N - 1)));
            }
            break;
        }

        case 2: {
            for (int n = 0; n < N; ++n) {
                window[n] = static_cast<float>(0.5 * (1 - cos(2.0 * PI * n / (N - 1))));
            }
            break;
        }

        case 5: {
            for (int n = 0; n < N; ++n) {
                window[n] = 1.0f;
            }
            break;
        }

        default: {
            for (int n = 0; n < N; ++n) {
                window[n] = 1.0f;
            }
            break;
        }
    }
}


void ProcessBundle(const Bundle &bundle) {
  double max_fft;
  int i, j, k;

  //auto start = high_resolution_clock::now();

  const auto &kValue = bundle.value();

  // Append received data. Note that we will process the last BUFFER_SIZE samples,
  // so if kValue and BUFFER_SIZE are not aligned, some old data could be lost
  samples.insert(samples.end(), kValue.begin(), kValue.end());
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

  //std::sort(magnitudes_frecuencias.rbegin(), magnitudes_frecuencias.rend());
  std::partial_sort(magnitudes_frecuencias.begin(), magnitudes_frecuencias.begin() + bin_count, magnitudes_frecuencias.end(), std::greater<std::pair<double, double>>());

  bin_count = std::min(static_cast<int>(magnitudes_frecuencias.size()), MAX_BIN_COUNT);

  for (i = 0; i < bin_count; i++) {
    fft_bin_frequencies[i] = magnitudes_frecuencias[i].second;
    fft_bin_magnitudes[i] = magnitudes_frecuencias[i].first;
    //std::cout << "Freq[" << i << "] = " << fft_bin_frequencies[i] << "Hz       Magn[" << i << "] = " << fft_bin_magnitudes[i] << std::endl;
  }


  // Ready to send FFT
  Send2Broker(bundle.timestamp());
  samples_count = 0;
  //auto stop = high_resolution_clock::now();
  //auto duration = duration_cast<microseconds>(stop - start);
  //cout << "FFT Calculation Time: " << duration.count() << " us" << endl;
}

void HandleSignal(int) {
  std::unique_lock<std::mutex> slck(signal_mutex);

  std::cout << "Exiting..." << std::endl;

  exit_flag = true;

  signal_cv.notify_one();
}

int main(int argc, char *argv[]) {
  int start_frequency, stop_frequency;



  start_frequency = std::max(0, std::stoi(argv[1]));
  stop_frequency = std::min(std::stoi(argv[2]), SAMPLING_FREQUENCY / 2) + 1;   
  window_type = stoi(argv[3]);
  if (window.empty()) {
    GenerateWindow(BUFFER_SIZE, window_type);
    cout << "Creating window..." << endl;
  }  
  
  // Calculate bin info
  frequency_resolution = (double) SAMPLING_FREQUENCY / BUFFER_SIZE;

  // Check if we can fill all bins
  bin_count = std::min((int) ((stop_frequency - start_frequency) / frequency_resolution), MAX_BIN_COUNT);

  bin_size = std::floor(((stop_frequency - start_frequency) / frequency_resolution) / bin_count);

  start_idx = std::floor(start_frequency / frequency_resolution);
  stop_idx = std::min(start_idx + bin_count * bin_size, BUFFER_SIZE / 2 + 1 - bin_size);

  std::unique_lock<std::mutex> slck(signal_mutex);
  SubscriberClient subscriber_client(&ProcessBundle, std::vector<int>{DATA_APD_FULL});
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
