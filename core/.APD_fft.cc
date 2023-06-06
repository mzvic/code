#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>
#include <cstdio>
#include <vector>
#include <fftw3.h>
#include <gnuplot-iostream.h>
#include <algorithm>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <numeric>
#include <chrono>
#include <algorithm>
#include <cmath>

#include "core.grpc.pb.h"

using core::Broker;
using core::Bundle;
using google::protobuf::Empty;
using google::protobuf::Timestamp;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;

std::mutex mtx;
std::mutex output_fft_mtx;
std::mutex output_mtx;
std::mutex data_mtx;
std::condition_variable cv;
std::condition_variable plot_cv;
std::queue<std::vector<int32_t>> data_queue;
bool finished = false;
int BUFFER_SIZE = 0;
std::vector<double> output_fft;
double f_dt_rx = 8560;//713; //720.72072072072072
std::vector<double> magnitudes_fft;


class BrokerClient {
public:
    BrokerClient(std::shared_ptr<Channel> channel) : stub_(Broker::NewStub(channel)) {}

    void Subscribe() {
        ClientContext context;
        Empty empty;
        Bundle bundle;
        std::unique_ptr<ClientReader<Bundle>> reader(stub_->Subscribe(&context, empty));
		while (reader->Read(&bundle)) {
		    const google::protobuf::RepeatedField<int32_t>& apd_list = bundle.apd();
		    std::vector<int32_t> segment_sums(48);
		    process_apd_data(apd_list, segment_sums);
		    for (int i = 0; i < 48; ++i) {
            	push_data({segment_sums[i]});
        	}
		    
		}
        set_finished(true);
        const Status status = reader->Finish();
        if (!status.ok()) {
            std::cout << "Writing finished with error" << std::endl;
        }
    }

private:
    std::unique_ptr<Broker::Stub> stub_;

	void push_data(std::vector<int32_t> data) {
		std::unique_lock<std::mutex> lock(data_mtx);
		data_queue.push(std::move(data));
		lock.unlock();
		cv.notify_all();
	}

    void set_finished(bool status) {
        std::unique_lock<std::mutex> lock(data_mtx);
        finished = status;
        lock.unlock();
        cv.notify_all();
    }
    
    void process_apd_data(const google::protobuf::RepeatedField<int32_t>& apd_list, std::vector<int32_t>& segment_sums) {
        const int num_segments = 48;
        const int segment_size = apd_list.size() / num_segments;

        for (int i = 0; i < num_segments; ++i) {
            segment_sums[i] = std::accumulate(apd_list.begin() + i * segment_size,
                                          apd_list.begin() + (i + 1) * segment_size, 0);
        }
    }
};
void receive_data(std::shared_ptr<Channel> channel) {
    BrokerClient broker(channel);
    broker.Subscribe();
}

void process_data() {
    std::vector<int32_t> data;
    std::vector<double> input_fft(BUFFER_SIZE, 0);
    std::vector<double> input_fft_windowed(BUFFER_SIZE);
	std::vector<double> bh7_values(BUFFER_SIZE);
	auto blackman_harris_7_window = [](int n, int N) {
		const double a0 = 0.27105140069342;
		const double a1 = 0.43329793923448;
		const double a2 = 0.21812299954311;
		const double a3 = 0.06592544638803;
		const double a4 = 0.01081174209837;
		const double a5 = 0.00077658482522;
		const double a6 = 0.00001388721735;
		return a0 - a1 * std::cos(2.0 * M_PI * n / (N - 1)) + a2 * std::cos(4.0 * M_PI * n / (N - 1)) - a3 * std::cos(6.0 * M_PI * n / (N - 1)) + a4 * std::cos(8.0 * M_PI * n / (N - 1)) - a5 * std::cos(10.0 * M_PI * n / (N - 1)) + a6 * std::cos(12.0 * M_PI * n / (N - 1));
	};

	for (int i = 0; i < BUFFER_SIZE; ++i) {
		bh7_values[i] = blackman_harris_7_window(i, BUFFER_SIZE);
	}

    fftw_plan plan;

    while (!finished || !data_queue.empty()) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [] { return !data_queue.empty() || finished; });

            if (data_queue.empty() && finished) {
                break;
            }
            data = std::move(data_queue.front());
            data_queue.pop();
            lock.unlock();
        }
        std::unique_lock<std::mutex> output_lock(output_mtx);
        input_fft.erase(input_fft.begin());
        input_fft.push_back(data[0]);

		for (size_t i = 0; i < BUFFER_SIZE; ++i) {
		    input_fft_windowed[i] = input_fft[i] * bh7_values[i];
		}

        plan = fftw_plan_r2r_1d(BUFFER_SIZE, input_fft_windowed.data(), output_fft.data(), FFTW_R2HC, FFTW_ESTIMATE);
        fftw_execute(plan);
        fftw_destroy_plan(plan);

        std::vector<double> magnitudes(BUFFER_SIZE / 2 + 1);
        magnitudes[0] = 0;//std::abs(output_fft[0]);
        for (size_t i = 1; i <= BUFFER_SIZE / 2; ++i) {
            double real_part = output_fft[i];
            double imag_part;

            if (i < BUFFER_SIZE / 2) {
                imag_part = output_fft[BUFFER_SIZE - i];
            } else {
                imag_part = 0.0;
            }

            magnitudes[i] = std::sqrt(real_part * real_part + imag_part * imag_part);
        }

        double max_magnitude = *std::max_element(magnitudes.begin(), magnitudes.end());

        if (max_magnitude > 0) {
            for (double& magnitude : magnitudes) {
                magnitude /= max_magnitude;
            }
        }

        magnitudes_fft = magnitudes;

        output_lock.unlock();
        cv.notify_all();
    }
}


void plot_data() {
    auto update_interval = std::chrono::microseconds(50); 
    Gnuplot gp;
    gp << "set term x11 title 'CoDE - APD Data FFT'\n";
	gp << "set object 1 rectangle from screen 0,0 to screen 1,1 fillcolor rgb '#000000' behind\n";
	gp << "set border linecolor rgb 'green'\n";
	gp << "set key textcolor rgb '#ffffff'\n";
	gp << "set xlabel textcolor rgb '#ffffff'\n";
	gp << "set ylabel textcolor rgb '#ffffff'\n";
	gp << "set xtics tc rgb 'white'\n";
	gp << "set ytics tc rgb 'white'\n";
	gp << "set grid linecolor rgb 'green'\n";
	gp << "set xlabel 'Frequency [Hz]'\n";
	gp << "set ylabel '|Power|'\n";
	gp << "set grid\n";
	gp << "set logscale x\n";
	gp << "unset key\n";
	//gp << "unset mouse\n";
	gp << "set mxtics 10\n";
	gp << "set mytics 2\n";
	gp << "set style line 101 lw 0.5 lc rgb 'green' dt '..-'\n";
	gp << "set grid xtics ytics mxtics mytics ls 101, ls 101\n";
    double freq, amplitude;
    while (!finished) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [] { return !data_queue.empty() || finished; });
            if (data_queue.empty() && finished) {
                break;
            }
        }
        std::unique_lock<std::mutex> output_lock(output_mtx);
        if (!magnitudes_fft.empty()) {
            gp << "plot [1:"<< f_dt_rx/2<<"] [-0.1:1.1] '-' with lines lc rgb 'red' linewidth 3 \n";
            for (int i = 0; i < BUFFER_SIZE / 2; ++i) {
                freq = (i < 1) ? 0.001 * (f_dt_rx/BUFFER_SIZE) : i * (f_dt_rx/BUFFER_SIZE);
                amplitude = magnitudes_fft[i];
                gp << freq << " " << amplitude << "\n";
            }
            gp << "e\n";
            gp.flush();
        }
        output_lock.unlock();
        std::this_thread::sleep_for(update_interval);
    }
}

int main() {
    printf("FFT samples: ");
    scanf("%d", &BUFFER_SIZE);
    output_fft.resize(BUFFER_SIZE, 0);

    const std::string server_address("localhost:50051");
    std::shared_ptr<Channel> channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());

    std::thread receiver_thread(receive_data, channel);
    std::thread processor_thread(process_data);
    std::thread plotter_thread(plot_data);

    receiver_thread.join();
    processor_thread.join();
    plotter_thread.join();

    return 0;
}


