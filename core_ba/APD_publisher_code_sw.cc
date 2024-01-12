#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <csignal>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include "broker_client.h"
#include "core.grpc.pb.h"

#define serialportpath "/dev/serial/by-id/"

#pragma clang diagnostic ignored "-Wimplicit-const-int-float-conversion"
#pragma ide diagnostic ignored "cppcoreguidelines-narrowing-conversions"
#pragma ide diagnostic ignored "cert-msc50-cpp"

using namespace core;
using google::protobuf::Timestamp;

// Global variables
int fd; // File descriptor
bool exit_flag = false;  // Used to signal the program to exit
std::mutex signal_mutex; // Mutex for synchronization
std::condition_variable signal_cv; // Condition variable for synchronization

// Function to handle the interrupt signal (SIGINT)
void HandleSignal(int) {
  std::unique_lock<std::mutex> slck(signal_mutex);

  std::cout << "Exiting..." << std::endl;

  exit_flag = true;

  signal_cv.notify_one();
}

// Function to configure the serial interface attributes
int set_interface_attribs(int fd, int speed) {
    struct termios tty;
    if (tcgetattr(fd, &tty) < 0) {
        printf("Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }
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
        return -1;
    }
    return 0;
}

// Function to set the minimum count for reading data from the serial interface
void set_mincount(int fd, int mcount) {
    struct termios tty;
    if (tcgetattr(fd, &tty) < 0) {
        printf("Error tcgetattr: %s\n", strerror(errno));
        return;
    }
    tty.c_cc[VMIN] = mcount ? 1 : 0;
    tty.c_cc[VTIME] = 5;       
    if (tcsetattr(fd, TCSANOW, &tty) < 0)
        printf("Error tcsetattr: %s\n", strerror(errno));
}

int main(int argc, char* argv[]) {
    int s;
    const char *portname;
    
    // Check if the program is provided with the correct command-line argument (FPGA serial port, sended from the GUI)
    if (argc < 2) {
        printf("Usage: %s <portname>\n", argv[0]);
        return -1;
    } 
    portname = argv[1];
    
    // Open the serial port for communication
    char fullPortPath[512];

    strcpy(fullPortPath, serialportpath);
    strcat(fullPortPath, portname);

    fd = open(fullPortPath, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        printf("Error opening %s: %s\n", fullPortPath, strerror(errno));
        return -1;
    }
    
    // Configure serial interface attributes and minimum count
    set_interface_attribs(fd, B2000000);
    set_mincount(fd, 0);

    // Create a data bundle and a publisher client for communication
    Bundle bundle;
    PublisherClient publisher_client;
    Timestamp timestamp; 

    // Register the signal handler for SIGINT (Ctrl+C)
    std::signal(SIGINT, HandleSignal);

    // Set the data bundle type to DATA_APD_FULL
    bundle.set_type(DATA_APD_FULL);
    
    struct timeval tv;
    int c = 0;
    int amount_data = 1000; // Bundle size

    // Main loop: continuously read data from the FPGA
    while (!exit_flag) { 
        while (fd > 0 and c <= amount_data) {      
            unsigned char buf[2];
            int rdlen;
            
            // Read data from the serial port into the buffer
            rdlen = read(fd, buf, sizeof(buf) - 1); 
            c++;
            
            if (rdlen > 0) {
                unsigned char *p;
                
                // Process the received data and add it to the data bundle to be sended to the broker
                for (p = buf; rdlen-- > 0; p++)
                    bundle.add_value(*p);
                // If enough data has been collected, publish the bundle to the broker
                if (c >= amount_data) {
                    publisher_client.Publish(bundle);
                    //std::this_thread::sleep_for(std::chrono::microseconds(2));
                    
                    // Clear the data bundle and reset the count
                    bundle.clear_value(); 
                    c = 0;
                }
            }	
        }   
    }
    
    return 0;
}

