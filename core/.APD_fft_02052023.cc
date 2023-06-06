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
std::mutex output_mtx;
std::condition_variable cv;
std::queue<std::vector<int32_t>> data_queue;
bool finished = false;
int BUFFER_SIZE = 0;
std::vector<double> output_fft;
double f_dt_rx = 540.540540540540/3.04;

class BrokerClient {
public:
    BrokerClient(std::shared_ptr<Channel> channel) : stub_(Broker::NewStub(channel)) {}
    
    void Subscribe() {
        int accum = 0;
        ClientContext context;
        Empty empty;
        Bundle bundle;
        std::unique_ptr<ClientReader<Bundle>> reader(stub_->Subscribe(&context, empty));
        while (reader->Read(&bundle)) {
            const google::protobuf::RepeatedField<int32_t>& apd_list = bundle.apd();
            accum = std::accumulate(apd_list.begin(), apd_list.end(), 0);
            push_data({accum});
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
        std::unique_lock<std::mutex> lock(mtx);
        data_queue.push(std::move(data));
        lock.unlock();
        cv.notify_all();
    }

    void set_finished(bool status) {
        std::unique_lock<std::mutex> lock(mtx);
        finished = status;
        lock.unlock();
        cv.notify_all();
    }
};

void receive_data(std::shared_ptr<Channel> channel) {
    BrokerClient broker(channel);
    broker.Subscribe();
}

void process_data() {
    std::vector<int32_t> data;
    std::vector<double> input_fft(BUFFER_SIZE, 0);
    fftw_plan plan = fftw_plan_r2r_1d(BUFFER_SIZE, input_fft.data(), output_fft.data(), FFTW_R2HC, FFTW_ESTIMATE);
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
        fftw_execute(plan);
        auto max = *std::max_element(output_fft.begin(), output_fft.end());
        if (max > 0) {
            for (double& val : output_fft) {
                val = abs(val / max);
            }
        } else {
            std::fill(output_fft.begin(), output_fft.end(), 0.0);
        }
        output_lock.unlock();
        cv.notify_all(); 
    }
}


void plot_data() {
	auto update_interval = std::chrono::milliseconds(10); 
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
        if (!output_fft.empty()) {
            gp << "plot [1:"<< f_dt_rx/2<<"] [-0.1:1.1] '-' with lines lc rgb 'blue' linewidth 3 \n";
            for (int i = 0; i < BUFFER_SIZE / 2; ++i) {
                freq = (i < 1) ? 0.001 * (f_dt_rx/BUFFER_SIZE) : i * (f_dt_rx/BUFFER_SIZE);
                amplitude = static_cast<double>(output_fft[i]);
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


