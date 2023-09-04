#include <iostream>
#include <memory>
#include <string>
#include <google/protobuf/timestamp.pb.h>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <cstdio>
#include <boost/asio.hpp>
#include <iomanip>
#include <thread>
#include <csignal>
#include <vector>
#include "core.grpc.pb.h"
#include "client.h"
#include <sstream>

using namespace core;
using namespace std::chrono;
using namespace google::protobuf;

Bundle *publishing_bundle;
PublisherClient *publisher_client;

bool exit_flag = false;
std::mutex signal_mutex;
std::condition_variable signal_cv;

std::vector<float> subs_values(3);
const char *portname;
int fd;
bool exit_reading = false;

float a, b, c, d, e;

void HandleSignal(int) {
    std::unique_lock<std::mutex> slck(signal_mutex);
    std::cout << "Exiting..." << std::endl;
    exit_flag = true;
    signal_cv.notify_one();
}

void Send2Broker(const Timestamp &timestamp) {
    publishing_bundle->clear_value();
    char buf[70];
    int rdlen;
    rdlen = read(fd, buf, sizeof(buf) - 1);
    if (rdlen > 0) {
        buf[rdlen] = '\0';
        std::istringstream iss(buf);
        std::string value;
        char variable;
        while (iss >> variable >> value) {
            try {
                float parsed_value = std::stof(value);
                switch (variable) {
                    case 'a':
                        a = parsed_value;
                        break;
                    case 'b':
                        b = parsed_value;
                        break;
                    case 'c':
                        c = parsed_value;
                        break;
                    case 'd':
                        d = parsed_value;
                        break;
                    case 'e':
                        e = parsed_value;
                        break;
                    default:
                        //std::cout << "Unknown variable identifier: " << variable << std::endl;
                        break;
                }
            } catch (std::exception &e) {
                //std::cout << "Error parsing value: " << e.what() << std::endl;
            }
        }
    }
    //std::cout << a << " " << b << " " << c << " " << d << " " << e << std::endl;
    publishing_bundle->add_value(a);
    publishing_bundle->add_value(b);
    publishing_bundle->add_value(c);
    publishing_bundle->add_value(d);
    publishing_bundle->add_value(e);
    publisher_client->Publish(*publishing_bundle, timestamp);
}

void Send2BrokerLoop(const Timestamp &timestamp) {
    while (!exit_flag) {
        std::unique_lock<std::mutex> lck(signal_mutex);
        if (!exit_reading) {
            lck.unlock();
            Send2Broker(timestamp);
        } else {
            lck.unlock();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void ProcessSub(const Bundle &bundle) {
    if (bundle.value().size() > 0) {
        std::unique_lock<std::mutex> slck(signal_mutex);
        exit_reading = true;
        slck.unlock();

        for (int i = 0; i < bundle.value().size(); i++) {
            subs_values[i] = bundle.value(i);
        }
        std::stringstream ss;
        for (size_t i = 0; i < subs_values.size(); ++i) {
            ss << subs_values[i];
            if (i != subs_values.size() - 1) {
                ss << " ";
            }
        }

        std::string result = ss.str();

        std::cout << result << std::endl;
        if (result.length() > 0) {
            int bytes_written = write(fd, result.c_str(), result.length());
            (void)bytes_written;
        }
        slck.lock();
        exit_reading = false;
        slck.unlock();
    }
}

int set_interface_attribs(int fd, int speed){
    struct termios tty;
    if (tcgetattr(fd, &tty) < 0) {
	printf("Error from tcgetattr: %s\n", strerror(errno));
	return -1;}
    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);
    tty.c_cflag |= (CLOCAL | CREAD);    
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         
    tty.c_cflag &= ~PARENB;     
    tty.c_cflag &= ~CSTOPB;     
    tty.c_cflag &= ~CRTSCTS;   
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
	printf("Error from tcsetattr: %s\n", strerror(errno));
	return -1;}
    return 0;
}

void set_mincount(int fd, int mcount){
    struct termios tty;
    if (tcgetattr(fd, &tty) < 0) {
	printf("Error tcgetattr: %s\n", strerror(errno));
	return;}
    tty.c_cc[VMIN] = mcount ? 1 : 0;
    tty.c_cc[VTIME] = 5;       
    if (tcsetattr(fd, TCSANOW, &tty) < 0)
	printf("Error tcsetattr: %s\n", strerror(errno));
}  

int main(int argc, char **argv) {
    portname = "/dev/ttyACM0";
    fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);

    if (fd < 0) {
        printf("Error opening %s: %s\n", portname, strerror(errno));
        return -1;
    }

    set_interface_attribs(fd, B2000000);
    set_mincount(fd, 0);

    std::unique_lock<std::mutex> slck(signal_mutex);
    SubscriberClient subscriber_client(&ProcessSub, std::vector<int>{DATA_TT_SET});

    publisher_client = new PublisherClient();
    publishing_bundle = new Bundle();
    publishing_bundle->set_type(DATA_TT_MON);

    std::signal(SIGINT, HandleSignal);

    std::thread send_thread(Send2BrokerLoop, publishing_bundle->timestamp());

    signal_cv.wait(slck, [] { return exit_flag; });

    delete publisher_client;
    delete publishing_bundle;

    send_thread.join();

    return 0;
}
