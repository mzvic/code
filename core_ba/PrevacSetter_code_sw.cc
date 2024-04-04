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
#include <iostream>
#include <sstream>

#define serialport "/dev/serial/by-id/usb-1a86_USB2.0-Ser_-if00-port0"
#define baudrate 9600

// Global variables
const unsigned char STx = 1;
const unsigned char ETx = 0;
const unsigned char write_function = 6;
unsigned char w1, w2;
uint16_t val1, val2;
std::vector<unsigned char> crc_data, crc;


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

std::vector<unsigned char> calculate_crc(const std::vector<unsigned char>& data) {
    unsigned short CRC = 0xFFFF;
    for (unsigned char byte : data) {
        CRC ^= byte;
        for (int i = 0; i < 8; ++i) {
            unsigned short carry = CRC & 1;
            CRC >>= 1;
            if (carry) {
                CRC ^= 0xA001;
            }
        }
    }
    return {(unsigned char)(CRC >> 8), (unsigned char)(CRC & 0xFF)};
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <w1> <w2> <val1> <val2>" << std::endl;
        return 1;
    }

    w1 = std::stoi(argv[1], nullptr, 10);
    w2 = std::stoi(argv[2], nullptr, 10);
    val1 = std::stoi(argv[3], nullptr, 10);
    val2 = std::stoi(argv[4], nullptr, 10);
        
    std::cout << "w1: " << static_cast<int>(w1) << std::endl;
    std::cout << "w2: " << static_cast<int>(w2) << std::endl;
    std::cout << "val1: " << static_cast<int>(val1) << std::endl;
    std::cout << "val2: " << static_cast<int>(val2) << std::endl;

    std::cout << "w1: 0x" << std::hex << static_cast<int>(w1) << std::endl;
    std::cout << "w2: 0x" << std::hex << static_cast<int>(w2) << std::endl;
    std::cout << "val1: 0x" << std::setw(4) << std::setfill('0') << std::hex << val1 << std::endl;
    std::cout << "val2: 0x" << std::setw(4) << std::setfill('0') << std::hex << val2 << std::endl;


    
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
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    close(pserial);
    // Register the signal handler for SIGINT (Ctrl+C)
    std::signal(SIGINT, HandleSignal);

    std::fstream serial(serialport, std::ios::in | std::ios::out | std::ios::binary);
    if (!serial.is_open()) {
        std::cerr << "Error opening serial port." << std::endl;
        return 1;
    }

    crc_data = {STx, write_function, w1, w2, val1, val2};
    crc = calculate_crc(crc_data);

    std::cout << "CRC 1: " << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(crc[0]) << std::endl;
    std::cout << "CRC 2: " << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(crc[1]) << std::endl;    
    serial.write(reinterpret_cast<const char*>(&STx), 1);
    serial.write(reinterpret_cast<const char*>(&write_function), 1);
    serial.write(reinterpret_cast<const char*>(&w1), 1);
    serial.write(reinterpret_cast<const char*>(&w2), 1);
    serial.write(reinterpret_cast<const char*>(&val1), 1);
    serial.write(reinterpret_cast<const char*>(&val2), 1);
    serial.write(reinterpret_cast<const char*>(&crc[0]), 1);
    serial.write(reinterpret_cast<const char*>(&crc[1]), 1);
    serial.write(reinterpret_cast<const char*>(&ETx), 1);

    serial.flush();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
	   
    return 0;
}

