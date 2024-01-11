#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <csignal>
#include <mutex>
#include <condition_variable>
#include <memory>
#include "core.grpc.pb.h"
#include "broker_client.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

//#define serialport "/dev/serial/by-id/usb-1a86_USB2.0-Ser_-if00-port0"
#define serialport "/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0"
#define baudrate 9600

using namespace core;
using google::protobuf::Timestamp;

// Global variables
const char xor_STX = 2;
//const char xor_ADDR = 128;
const char xor_ADDR1 = 128;
const char xor_ADDR2 = 129;
const char xor_WR = 48;
const char xor_OnOff = 48;
const char xor_ETX = 3;

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

int main(int argc, char* argv[]) {
    int pserial = open(serialport, O_RDWR | O_NOCTTY);
    if (pserial == -1) {
        std::cerr << "Error opening serial port." << std::endl;
        return 1;
    }
    struct termios options;
    tcgetattr(pserial, &options);
    cfsetispeed(&options, baudrate);
    cfsetospeed(&options, baudrate);
    options.c_cflag |= (CLOCAL | CREAD);    
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;         
    options.c_cflag &= ~PARENB;     
    options.c_cflag &= ~CSTOPB;     
    options.c_cflag &= ~CRTSCTS;   
    options.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    options.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    options.c_oflag &= ~OPOST;
    options.c_cc[VMIN] = 1;
    options.c_cc[VTIME] = 1;
    tcsetattr(pserial, TCSANOW, &options);
    char data = '0';
    write(pserial, &data, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    close(pserial);
    // Create a data bundle and a publisher client for communication
    Bundle bundle;
    PublisherClient publisher_client;
    Timestamp timestamp; 

    // Register the signal handler for SIGINT (Ctrl+C)
    std::signal(SIGINT, HandleSignal);

    // Set the data bundle type to DATA_TT_MON
    bundle.set_type(DATA_TT_MON);
    // Main loop: continuously read data from twistorr
    while (!exit_flag) { 
        std::fstream serial(serialport, std::ios::in | std::ios::out | std::ios::binary);
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
        bundle.clear_value(); 
        for (int device_count = 1; device_count <= 2; device_count++) { // ADDR1 and ADDR2
            for (int xor_WIN = 200; xor_WIN <= 204; ++xor_WIN) {
                std::fstream serial(serialport, std::ios::in | std::ios::out | std::ios::binary);
                if (!serial.is_open()) {
                    std::cerr << "Error opening serial port." << std::endl;
                    return 1;
                }
                //std::cout << "--------------------------------------------------------------------\n";

                std::string WIN = std::to_string(xor_WIN);

                char xor_WIN1 = WIN[0];
                char xor_WIN2 = WIN[1];
                char xor_WIN3 = WIN[2];

                char xor_checksum = 30;
                if (device_count == 1){
                    xor_checksum = xor_ADDR1 ^ xor_WIN1 ^ xor_WIN2 ^ xor_WIN3 ^ xor_WR ^ xor_OnOff ^ xor_ETX;
                }else{
                    xor_checksum = xor_ADDR2 ^ xor_WIN1 ^ xor_WIN2 ^ xor_WIN3 ^ xor_WR ^ xor_OnOff ^ xor_ETX;}
                char crc_1 = ((xor_checksum >> 4) & 0xF);
                char crc_2 = (xor_checksum & 0xF);
                char ascii_crc_1[3];
                char ascii_crc_2[3];
                snprintf(ascii_crc_1, sizeof(ascii_crc_1), "%X", crc_1);
                snprintf(ascii_crc_2, sizeof(ascii_crc_2), "%X", crc_2);

                serial.write(&xor_STX, 1);
                if (device_count == 1){
                    serial.write(&xor_ADDR1, 1);
                }else{
                    serial.write(&xor_ADDR2, 1);}
                serial.write(&xor_WIN1, 1);
                serial.write(&xor_WIN2, 1);
                serial.write(&xor_WIN3, 1);
                serial.write(&xor_WR, 1);
                serial.write(&xor_OnOff, 1);
                serial.write(&xor_ETX, 1);
                serial.write(ascii_crc_1, 1);
                serial.write(ascii_crc_2, 1);

                serial.flush();

                std::this_thread::sleep_for(std::chrono::milliseconds(50));

                std::vector<char> response;
                char currentChar;
                int responseSize = 0;
                while (serial.get(currentChar)) {
                    response.push_back(currentChar);
                    if (currentChar == 3) {
                        break;
                    }
                }
                //std::cout << "Size of response vector: " << response.size() << std::endl;
                float rx_value = std::stof(std::string(response.begin() + 6, response.end()));
                bundle.add_value(rx_value);
                //std::cout << "Rx window " << xor_WIN << ": " << std::fixed << std::setprecision(2) << rx_value << "\n";
            }
        }
        publisher_client.Publish(bundle);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }	   
    return 0;
}

