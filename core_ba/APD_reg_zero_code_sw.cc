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

using namespace core;
using namespace std::chrono;
using namespace google::protobuf;

bool exit_flag = false;
std::mutex signal_mutex;
std::condition_variable signal_cv;

char filename[70];
int apd_size = 0;
vector<int> subs_values(apd_size);
int64_t sec_0 = 9999999999;
int32_t usec_0 = 999999999;
int64_t sec_value;
int32_t nsec_value;

void HandleSignal(int)
{
    std::unique_lock<std::mutex> slck(signal_mutex);
    std::cout << "Exiting..." << std::endl;
    exit_flag = true;
    signal_cv.notify_one();
}

void Subscribe(int64_t sec_value, int32_t nsec_value, std::vector<int> subs_values, int apd_size)
{
    FILE *file = fopen(filename, "a");
    int64_t sec_i = 0;
    int32_t usec_i = 0;

    if (sec_value < sec_0)
    {
        sec_0 = sec_value;
        usec_0 = nsec_value;
    }

    for (int i = 0; i < subs_values.size(); ++i)
    {
        sec_i = sec_value;
        usec_i = nsec_value + i * 9700;
        int64_t sec_diff = sec_i - sec_0;
        int32_t usec_diff = usec_i - usec_0;

        if (usec_diff < 0)
        {
            sec_diff--;
            usec_diff += 1000000000;
        }
        fprintf(file, "%ld.%09d;%d\n", sec_diff, usec_diff % 1000000000, subs_values.at(i));
        // printf("%ld.%09d;%d\n", sec_diff, usec_diff % 1000000000, subs_values.at(i));
    }
    fclose(file);
}

void ProcessSub(const Bundle &bundle)
{
    // cout << "Starting ProcessSub..." << endl;
    // apd_size = bundle.value().size();
    apd_size = 0;
    while (apd_size == 0)
    {
        apd_size = bundle.value().size();
        sec_value = bundle.timestamp().seconds();
        nsec_value = bundle.timestamp().nanos();
    }

    // cout << apd_size << endl;
    subs_values.assign(apd_size, 0.0);

    for (int i = 0; i < apd_size; i++)
    {
        subs_values[i] = bundle.value(i);
        // cout << "   " << bundle.value(i);
    }
    Subscribe(sec_value, nsec_value, subs_values, apd_size);
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
    strftime(buf, sizeof(buf), "[100kHz]APD_logs_%d-%m-%Y_%H:%M:%S", &t);
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
