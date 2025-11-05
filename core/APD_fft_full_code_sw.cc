#include <algorithm>
#include <csignal>
#include <fftw3.h>
#include <boost/circular_buffer.hpp>
#include <cstdlib> 
#include <iostream>

#include "core.grpc.pb.h"
#include "broker_client.h"

using namespace core;
using namespace std::chrono;
using namespace google::protobuf;

int BUFFER_SIZE = 10000000;
int AVERAGE_COUNT = 5;

unique_ptr<Bundle> publishing_bundle;
unique_ptr<PublisherClient> publisher_client;

fftw_plan plan = nullptr;
std::vector<double> input;
std::vector<double> output;
std::vector<double> fft_magnitudes;
std::vector<double> fft_magnitudes_average;
unique_ptr<boost::circular_buffer<double>> samples;
int fft_count = 0;

bool exit_flag = false;
mutex signal_mutex;
condition_variable signal_cv;

void destroy_plan() {
    if (plan != nullptr) {
        fftw_destroy_plan(plan);
        plan = nullptr;
    }
}

void initialize_buffers(int size) {
    BUFFER_SIZE = size;

    // Destroy existing plan first
    destroy_plan();

    // Resize vectors
    input.resize(BUFFER_SIZE);
    output.resize(BUFFER_SIZE);
    fft_magnitudes.resize(BUFFER_SIZE / 2 + 1);
    fft_magnitudes_average.resize(BUFFER_SIZE / 2 + 1);
    
    // Recreate circular buffer
    samples = make_unique<boost::circular_buffer<double>>(BUFFER_SIZE);

    // Initialize with zeros
    std::fill(input.begin(), input.end(), 0.0);
    std::fill(output.begin(), output.end(), 0.0);
    std::fill(fft_magnitudes.begin(), fft_magnitudes.end(), 0.0);
    std::fill(fft_magnitudes_average.begin(), fft_magnitudes_average.end(), 0.0);

    // Create new plan - use FFTW_MEASURE for better performance
    // Note: This will overwrite the input array during planning
    plan = fftw_plan_r2r_1d(BUFFER_SIZE,
                            input.data(),
                            output.data(),
                            FFTW_R2HC,
                            FFTW_MEASURE);  // Changed to MEASURE for better optimization

    cout << "Plan created for buffer size: " << BUFFER_SIZE << endl;
}

void SendToBroker(const Timestamp &timestamp) {
  cout << "Publishing data" << endl;

  publishing_bundle->clear_value();

  // Calcular cuántos puntos tenemos
  size_t our_points = fft_magnitudes_average.size();
  size_t storage_expected_points = 5000001;  // FFT_FULL_SIZE del storage
  
  cout << "Sending " << our_points << " points, storage expects " << storage_expected_points << endl;

  // Enviar los puntos que tenemos
  for (size_t i = 0; i < std::min(our_points, storage_expected_points); i++)
    publishing_bundle->add_value(fft_magnitudes_average[i]);
    
  // Si tenemos menos puntos de los esperados, rellenar con ceros
  for (size_t i = our_points; i < storage_expected_points; i++)
    publishing_bundle->add_value(0.0);

  publisher_client->Publish(*publishing_bundle, timestamp);
}

void ProcessBundle(const Bundle &bundle) {
  auto start = high_resolution_clock::now();

  const auto &kValue = bundle.value();

  // Check if samples buffer is properly initialized
  if (!samples || samples->capacity() != BUFFER_SIZE) {
      cerr << "Error: samples buffer not properly initialized!" << endl;
      return;
  }

  // Append received data
  samples->insert(samples->end(), kValue.begin(), kValue.end());

  cout << "\33[2K\r" << ((float) samples->size() / BUFFER_SIZE) * 100.0 << " % samples" << flush;

  // Check if we have enough data to proceed
  if (samples->size() < BUFFER_SIZE)
    return;

  cout << endl;

  // Verify plan exists
  if (plan == nullptr) {
      cerr << "Error: FFTW plan not initialized!" << endl;
      samples->clear();
      return;
  }

  // Create input - ensure we don't access out of bounds
  for (int i = 0; i < BUFFER_SIZE && i < samples->size(); i++)
    input[i] = samples->at(i);

  cout << "Executing plan for size: " << BUFFER_SIZE << endl;

  // Execute FFTW plan
  fftw_execute(plan);

  // Remove DC component
  if (fft_magnitudes.size() > 0)
    fft_magnitudes[0] = 0;

  // Calculate magnitudes - with bounds checking
  int max_i = std::min((BUFFER_SIZE + 1) / 2, static_cast<int>(fft_magnitudes.size()));
  for (int i = 1; i < max_i; i++) {
      if (i < BUFFER_SIZE && (BUFFER_SIZE - i) < BUFFER_SIZE) {
          fft_magnitudes[i] = sqrt(output[i] * output[i] + output[BUFFER_SIZE - i] * output[BUFFER_SIZE - i]);
      }
  }

  if (BUFFER_SIZE % 2 == 0 && (BUFFER_SIZE / 2) < fft_magnitudes.size())
    fft_magnitudes[BUFFER_SIZE / 2] = abs(output[BUFFER_SIZE / 2]);  // Nyquist frequency

  // Aggregate fft_magnitudes_average
  if (fft_count == 0) {
    std::copy(fft_magnitudes.begin(), fft_magnitudes.end(), fft_magnitudes_average.begin());
  } else {
    int max_avg_i = std::min(static_cast<int>(fft_magnitudes.size()), 
                             static_cast<int>(fft_magnitudes_average.size()));
    for (int i = 0; i < max_avg_i; i++)
      fft_magnitudes_average[i] += fft_magnitudes[i];
  }

  fft_count++;

  // Ready to send FFT
  if (fft_count == AVERAGE_COUNT) {
    double max_fft = 0;

    // Calculate average
    for (auto &elem : fft_magnitudes_average)
      elem /= AVERAGE_COUNT;

    // Find max magnitude
    for (const auto &kElem : fft_magnitudes_average) {
      if (kElem > max_fft)
        max_fft = kElem;
    }

    // Normalize magnitudes vector (avoid division by zero)
    if (max_fft > 0) {
        for (auto &elem : fft_magnitudes_average)
          elem /= max_fft;
    }

    SendToBroker(bundle.timestamp());

    fft_count = 0;
    // Reset average array
    std::fill(fft_magnitudes_average.begin(), fft_magnitudes_average.end(), 0.0);
  }

  // Clear samples (fixed window approach)
  samples->clear();

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
  
    if (argc != 3) {  
        std::cerr << "Usage: " << argv[0] << " <AVERAGE_COUNT> <BUFFER_SIZE>" << std::endl;
        return 1;
    }

    AVERAGE_COUNT = std::atoi(argv[1]);
    BUFFER_SIZE = std::atoi(argv[2]);

    if (AVERAGE_COUNT < 1) {
        std::cerr << "AVERAGE_COUNT must be a positive integer." << std::endl;
        return 1; 
    }
    
    if (BUFFER_SIZE < 100000) {
        std::cerr << "BUFFER_SIZE must be greater than 100000." << std::endl;
        return 1; 
    }
    
    // Initialize FFT threads FIRST, before any FFTW calls
    fftw_init_threads();
    fftw_plan_with_nthreads(8);
    
    initialize_buffers(BUFFER_SIZE);

    cout << "BUFFER_SIZE = " << BUFFER_SIZE << endl;
    cout << "FFT initialized correctly." << endl;
  
    unique_lock<mutex> slck(signal_mutex);

    publisher_client = make_unique<PublisherClient>();
    SubscriberClient subscriber_client(&ProcessBundle, vector<int>{DATA_APD_FULL});
    publishing_bundle = make_unique<Bundle>();

    publishing_bundle->set_type(DATA_FFT_FULL);
    
    // Register handler
    std::signal(SIGINT, HandleSignal);

    // Wait for CTRL-C signal
    signal_cv.wait(slck, [] { return exit_flag; });

    // Cleanup
    destroy_plan();
    fftw_cleanup_threads();

    return 0;
}
