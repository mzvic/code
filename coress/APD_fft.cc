#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>
#include <cstdio>
#include <vector>
#include <fftw3.h>
#include <algorithm>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <numeric>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <boost/asio.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>

#include "core.grpc.pb.h"

using core::Broker;
using core::Bundle;
using google::protobuf::Empty;
using google::protobuf::Timestamp;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientWriter;
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
int tx_freq_fpga = 100000; 
std::vector<double> magnitudes_fft;
int window_type;
std::shared_ptr<boost::asio::ip::tcp::socket> socket_;
boost::asio::socket_base::reuse_address option(true);


class BrokerClient {
public:

    BrokerClient(std::shared_ptr<Channel> channel) : stub(Broker::NewStub(channel)) {

    }

    void Subscribe(int segments) {
        ClientContext context;
        Empty empty;
        Bundle bundle;
        struct timeval tv;
        std::unique_ptr<ClientReader<Bundle>> reader(stub->Subscribe(&context, empty));
		while (reader->Read(&bundle)) {
		    const google::protobuf::RepeatedField<int32_t>& apd_list = bundle.apd();
		    std::vector<int32_t> segment_sums(segments);
		    process_apd_data(apd_list, segment_sums, segments);
		    for (int i = 0; i < segments; ++i) {
            	push_data({segment_sums[i]});
        	}
		}
        set_finished(true);
        //const Status status = reader->Finish();
        //if (!status.ok()) {
        //    std::cout << "Writing finished with error" << std::endl;
        //}
    }



private:
    std::unique_ptr<Broker::Stub> stub;
    

	void push_data(std::vector<int32_t> data) {
		std::unique_lock<std::mutex> lock(data_mtx);
		//std::cout << "-----------> Tamaño cola: " << data_queue.size() << std::endl;
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
    
    void process_apd_data(const google::protobuf::RepeatedField<int32_t>& apd_list, std::vector<int32_t>& segment_sums, int segments) {
        const int num_segments = segments;
        const int segment_size = apd_list.size() / num_segments;

        for (int i = 0; i < num_segments; ++i) {
            segment_sums[i] = std::accumulate(apd_list.begin() + i * segment_size,
                                          apd_list.begin() + (i + 1) * segment_size, 0);
        }
    }
};

void receive_data(int segments, std::shared_ptr<Channel> channel) {
    BrokerClient broker(channel);
    broker.Subscribe(segments);
}

void send2Socket(std::vector<double> magnitudes_fft, double f_dt_rx, int BUFFER_SIZE) {
    double freq_i, fft_i;
    std::string data;
    for (int i = 0; i < magnitudes_fft.size(); ++i) {
        if (i == 0){
            freq_i = 0.001;
        } else {
            freq_i = i*(f_dt_rx/BUFFER_SIZE);
        }
        fft_i = magnitudes_fft[i];
        data = data + " " + std::to_string(freq_i) + " " + std::to_string(fft_i);
        }
        boost::asio::write(*socket_, boost::asio::buffer(data+"\n"));
        data ="";
        socket_->set_option(option);
        //socket_->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
}

void process_data() {
    std::vector<int32_t> data;
    std::vector<double> input_fft(BUFFER_SIZE, 0);
    std::vector<double> input_fft_windowed(BUFFER_SIZE);
	std::vector<double> window_values(BUFFER_SIZE);
	
	// Different windows
	auto BH7 = [](int n, int N) {
		const double a0 = 0.27105140069342;
		const double a1 = 0.43329793923448;
		const double a2 = 0.21812299954311;
		const double a3 = 0.06592544638803;
		const double a4 = 0.01081174209837;
		const double a5 = 0.00077658482522;
		const double a6 = 0.00001388721735;
		return a0 - a1 * std::cos(2.0 * M_PI * n / (N - 1)) + a2 * std::cos(4.0 * M_PI * n / (N - 1)) - a3 * std::cos(6.0 * M_PI * n / (N - 1)) + a4 * std::cos(8.0 * M_PI * n / (N - 1)) - a5 * std::cos(10.0 * M_PI * n / (N - 1)) + a6 * std::cos(12.0 * M_PI * n / (N - 1));
	};

	auto BH4 = [](int n, int N) {
		const double a0 = 0.35875;
		const double a1 = 0.48829;
		const double a2 = 0.14128;
		const double a3 = 0.01168;
		return a0 - a1 * std::cos(2.0 * M_PI * n / (N - 1)) + a2 * std::cos(4.0 * M_PI * n / (N - 1)) - a3 * std::cos(6.0 * M_PI * n / (N - 1));
	};
	
	auto hamming = [](int n, int N) {
        return 0.54 - 0.46 * std::cos(2.0 * M_PI * n / (N - 1));
    };

    auto hann = [](int n, int N) {
        return 0.5 * (1 - std::cos(2.0 * M_PI * n / (N - 1)));
    };    
	
	auto nw = [](int n, int N) {
        return 1;
    }; 

    fftw_plan plan = fftw_plan_r2r_1d(BUFFER_SIZE, input_fft_windowed.data(), output_fft.data(), FFTW_R2HC, FFTW_MEASURE);


    while (!finished || !data_queue.empty()) {
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [] { return !data_queue.empty() || finished; });

            if (data_queue.empty() && finished) {
                break;
            }
            if (!data_queue.empty()) {
                data = std::move(data_queue.front());
                data_queue.pop();
                lock.unlock();
            }
        }
        std::unique_lock<std::mutex> output_lock(output_mtx);
        
        while (!data.empty()) {
            input_fft.erase(input_fft.begin());
            input_fft.emplace_back(data.front());
            data.erase(data.begin());
        }

        //input_fft.erase(input_fft.begin());
        //input_fft.emplace_back(data[0]);
        std::cout << "-----------> Tamaño data: " << data.size() << std::endl;
        std::cout << "-----------> Tamaño data_queue: " << data_queue.size() << std::endl;
		double mean = 0;
		for (int i = 0; i < BUFFER_SIZE; i++) {
			mean += input_fft[i];
		}
		
		mean /= BUFFER_SIZE;
		for (int i = 0; i < BUFFER_SIZE; i++) {
			input_fft[i] -= mean;					// Substracting average to input_fft to reduce DC component (uncomment to substract)
			if (window_type == 1) {
                window_values[i] = hamming(i, BUFFER_SIZE);
            } else if (window_type == 2){
                window_values[i] = hann(i, BUFFER_SIZE);
            } else if (window_type == 3){
                window_values[i] = BH4(i, BUFFER_SIZE);
            } else if (window_type == 4){
                window_values[i] = BH7(i, BUFFER_SIZE);
            } else if (window_type == 5){
                window_values[i] = nw(i, BUFFER_SIZE);
            }
		    input_fft_windowed[i] = input_fft[i] * window_values[i];
		}
		
        fftw_execute(plan);

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
		plot_cv.notify_one();
        output_lock.unlock();
        cv.notify_one();
    }
    fftw_destroy_plan(plan);
}


void send_fft(double f_dt_rx) {
    while (!finished) {
        {
            std::unique_lock<std::mutex> lock(output_mtx);
            plot_cv.wait(lock, [] { return !magnitudes_fft.empty() || finished; });
            if (magnitudes_fft.empty() && finished) {
                break;
            }
        }
        std::unique_lock<std::mutex> output_lock(output_mtx);
        if (!magnitudes_fft.empty()) {
            send2Socket(magnitudes_fft, f_dt_rx, BUFFER_SIZE);
            magnitudes_fft.clear();
        }
        output_lock.unlock();
        plot_cv.notify_all();
    }
}

int main(int argc, char* argv[]) {
    boost::asio::io_service io_service;
    socket_ = std::make_shared<boost::asio::ip::tcp::socket>(io_service);
    socket_->connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 12352));

    BUFFER_SIZE = std::stoi(argv[1]);
    window_type = std::stoi(argv[2]);
    const int segments = std::stoi(argv[3]);
    const double f_dt_rx = (tx_freq_fpga/(1000/segments));
    output_fft.resize(BUFFER_SIZE, 0);
    const std::string server_address("localhost:50051");
    std::shared_ptr<Channel> channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());

    std::thread subscriber_thread(receive_data, segments, channel);
    std::thread processor_thread(process_data);
    std::thread socket_thread(send_fft, f_dt_rx);

    subscriber_thread.join();
    processor_thread.join();
    socket_thread.join();

    return 0;
}

