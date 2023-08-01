#include <grpcpp/grpcpp.h>
#include <iostream>
#include <memory>
#include <string>
#include <google/protobuf/timestamp.pb.h>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <cstdio>
#include "client.h"
#include "core.grpc.pb.h"
#include <csignal>
#include <thread>
#include <vector>

using namespace google::protobuf;
using namespace core;
using namespace std::chrono;

bool exit_flag = false;
std::mutex signal_mutex;
std::condition_variable signal_cv;

int64_t sec_0 = 9999999999;
int32_t nsec_0 = 999999999;
int64_t sec = 0;
int32_t nsec = 0;
int64_t sec_i = 0;
int32_t nsec_i = 0;
int accum = 0;

int apd_size = 0;
vector<float> subs_values(apd_size);
char filename[70];

int c = 0;
long int t_0 = 0;


void HandleSignal(int)
{
    std::unique_lock<std::mutex> slck(signal_mutex);
    std::cout << "Exiting..." << std::endl;
    exit_flag = true;
    signal_cv.notify_one();
}
// registro procesado (1khz)
void save_reg(vector<float> subs_values)
{
    FILE *file = fopen(filename, "a");

    if (sec < sec_0)
    {
        sec_0 = sec;
        nsec_0 = nsec;
    }

    sec_i = sec;
    nsec_i = nsec;

    for (int i = 0; i < subs_values.size(); ++i)
    {
        accum = accum + subs_values[i];
        if (i == (subs_values.size() / 10) - 1 ||
            i == (subs_values.size() / 5) - 1 ||
            i == (subs_values.size() * 3 / 10) - 1 ||
            i == (subs_values.size() * 2 / 5) - 1 ||
            i == (subs_values.size() / 2) - 1 ||
            i == (subs_values.size() * 3 / 5) - 1 ||
            i == (subs_values.size() * 7 / 10) - 1 ||
            i == (subs_values.size() * 4 / 5) - 1 ||
            i == (subs_values.size() * 9 / 10) - 1 ||
            i == subs_values.size() - 1)
        {
            nsec_i = nsec + i * 9700;
            if (nsec_i > 999999999)
            {
                sec_i = sec + 1;
                nsec_i = nsec_i % 1000000000;
            }
            int64_t sec_diff = sec_i - sec_0;
            int32_t nsec_diff = nsec_i - nsec_0;
            if (nsec_diff < 0)
            {
                sec_diff--;
                nsec_diff += 1000000000;
            }
            fprintf(file, "%ld.%09d;%d\n", sec_diff, nsec_diff % 1000000000, accum);
            //printf("%ld.%09d;%d\n", sec_diff, nsec_diff % 1000000000, accum);
            accum = 0;
        }
    }
    fclose(file);
    
}


void ProcessSub(const Bundle &bundle)
{
    // cout << "Starting ProcessSub..." << endl;
    apd_size = 0;
    while (apd_size == 0)
    {
        apd_size = bundle.value().size();
        sec = bundle.timestamp().seconds();
        nsec = bundle.timestamp().nanos();
        //cout << "ESPERANDO BUNDLE... " << endl;
    }

    // cout << apd_size << endl;
    subs_values.assign(apd_size, 0.0);

    for (int i = 0; i < apd_size; i++)
    {
        subs_values[i] = bundle.value(i);
        // cout << "   " << bundle.value(i);
    }
    save_reg(subs_values);
}
int main()
{
    std::unique_lock<std::mutex> slck(signal_mutex);

    struct timespec ts;
    timespec_get(&ts, TIME_UTC);
    ts.tv_sec -= 14400;
    struct tm t;
    gmtime_r(&ts.tv_sec, &t);
    char buf[40];
    strftime(buf, sizeof(buf), "[1kHz]APD_logs_%d-%m-%Y_%H:%M:%S", &t);
    printf("Creating APD log file...");
    snprintf(filename, sizeof(filename), "%s.txt", buf);
    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        printf("Error creating APD log file...\n");
    }
    printf("Created file '%s'...\n", filename);

    SubscriberClient subscriber_client(&ProcessSub, vector<int>{DATA_APD_FULL});

    std::signal(SIGINT, HandleSignal);

    signal_cv.wait(slck, [] { return exit_flag; });
    return 0;
}
