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

#define SAMPLING_FREQUENCY 100000
#define BUFFER_SIZE 1000000

int samples_count = 0;

Bundle *publishing_bundle;
PublisherClient *publisher_client;

int window_type = 0;
#define PI 3.14159265358979323846
std::vector<float> window;

fftw_plan plan = nullptr;
double input[BUFFER_SIZE], output[BUFFER_SIZE], fft_magnitudes[BUFFER_SIZE / 2 + 1], fft_magnitudes_average[BUFFER_SIZE / 2 + 1];
boost::circular_buffer<double> samples(BUFFER_SIZE);
int fft_count = 0;

bool exit_flag = false;
mutex signal_mutex;
condition_variable signal_cv;

void Send2Broker(const Timestamp &timestamp) { // Function that sends the values to the broker
  int i;
  publishing_bundle->clear_value(); // Cleans the DATA_FFT_FULL variable of the bundle
  for (i = 0; i < 500000; i++) {
	publishing_bundle->add_value(fft_magnitudes[i]); // Add magnitude values to be sended to the broker (only magnitudes, because as we send all the values, the index of the vector is used to know the corresponding frequency)
  }
  publisher_client->Publish(*publishing_bundle, timestamp); // Sends values to the broker
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
  int i, j, k;
  double max_fft;
  // Assigns the incoming data to the 'kValue' variable
  const auto &kValue = bundle.value(); 
  // Append received data. Note that we will process the last BUFFER_SIZE samples,
  // so if value and BUFFER_SIZE are not aligned, some old data could be lost
  samples.insert(samples.end(), kValue.begin(), kValue.end());
  // Only proceed if we have 1000*1000 = 1.000.000 values on 'samples', e.i. once each 10 seconds
  if (samples_count < 1000){
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
	//Save input data since a new plan erases the input
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
  // Sends data to broker
  Send2Broker(bundle.timestamp());
  samples_count = 0;
}

void HandleSignal(int) { // Function to exit process
  unique_lock<mutex> slck(signal_mutex);
  cout << "Exiting..." << endl;
  exit_flag = true;
  signal_cv.notify_one();
}

int main(int argc, char *argv[]) {
  window_type = stoi(argv[1]); // This is the only argument that is sended from the GUI
  // If 'window' is empty, generates the window, if not, it goes ahead with the existing window
  if (window.empty()) { 
    GenerateWindow(BUFFER_SIZE, window_type);
    cout << "Creating window..." << endl;
  }
  // Locks the thread
  unique_lock<mutex> slck(signal_mutex);
  // Indicates the interest variable for the susbscribe method (to get the data from the broker)
  SubscriberClient subscriber_client(&ProcessBundle, vector<int>{DATA_APD_FULL});
  // Indicates the broker publishing variable 
  publisher_client = new PublisherClient();
  publishing_bundle = new Bundle();
  publishing_bundle->set_type(DATA_FFT_FULL);
  // Register handler
  std::signal(SIGINT, HandleSignal);
  // Initialize FFT threads, doesn't work very well, so it's commented
  //fftw_init_threads();
  //fftw_plan_with_nthreads(4);
  // Wait fot CTRL-C signal
  signal_cv.wait(slck, [] { return exit_flag; });
  // Destroy FFTW plan
  if (plan != nullptr)
	fftw_destroy_plan(plan);
  //fftw_cleanup_threads();
  free(publisher_client);
  free(publishing_bundle);
  return 0;
}
