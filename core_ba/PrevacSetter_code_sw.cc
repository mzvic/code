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
int STx = 1;
int ETx = 0;
int write_function = 16;
int w1, w2;
int w3 = 0;
int w4 = 2;
int w5 = 4;
int val1, val2;
std::vector<int> crc_data, crc;


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

std::vector<int> calculate_crc(const std::vector<int>& data) {
    unsigned short CRC = 0xFFFF;
    for (int byte : data) {
        CRC ^= byte;
        for (int i = 0; i < 8; ++i) {
            unsigned short carry = CRC & 1;
            CRC >>= 1;
            if (carry) {
                CRC ^= 0xA001;
            }
        }
    }
    return {(int)(CRC >> 8), (int)(CRC & 0xFF)};
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <w1> <w2> <val1> <val2>" << std::endl;
        return 1;
    }

    w1 = std::stoi(argv[1]);
    w2 = std::stoi(argv[2]);
    val1 = std::stoi(argv[3]);
    val2 = std::stoi(argv[4]);

        
    std::cout << "w1: " << std::hex << static_cast<int>(w1) << std::endl;
    std::cout << "w2: " << std::hex << static_cast<int>(w2) << std::endl;
    std::cout << "val1: " << std::uppercase << std::setw(4) << std::setfill('0') << std::hex << (val1 & 0xFFFF) << std::endl;
    std::cout << "val2: " << std::uppercase << std::setw(4) << std::setfill('0') << std::hex << (val2 & 0xFFFF) << std::endl;
        
    int val1_byte0 = (val1 >> 8) & 0xFF;
    int val1_byte1 = val1 & 0xFF;
    
    int val2_byte0 = (val2 >> 8) & 0xFF;
    int val2_byte1 = val2 & 0xFF;
            
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
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    close(pserial);
    // Register the signal handler for SIGINT (Ctrl+C)
    std::signal(SIGINT, HandleSignal);

    std::fstream serial(serialport, std::ios::in | std::ios::out | std::ios::binary);
    if (!serial.is_open()) {
        std::cerr << "Error opening serial port." << std::endl;
        return 1;
    }

    crc_data = {STx, write_function, w1, w2, w3, w4, w5, val1_byte0, val1_byte1, val2_byte0, val2_byte1};
    crc = calculate_crc(crc_data);

    std::cout << "CRC 1: " << static_cast<int>(crc[0]) << std::endl;
    std::cout << "CRC 2: " << static_cast<int>(crc[1]) << std::endl;    



    std::cout << "Sending STx: " << STx << std::endl;
    write(pserial, &STx, 1);

    std::cout << "Sending write_function: " << write_function << std::endl;
    write(pserial, &write_function, 1);

    std::cout << "Sending w1: " << w1 << std::endl;
    write(pserial, &w1, 1);

    std::cout << "Sending w2: " << w2 << std::endl;
    write(pserial, &w2, 1);

    std::cout << "Sending w3: " << w3 << std::endl;
    write(pserial, &w3, 1);

    std::cout << "Sending w4: " << w4 << std::endl;
    write(pserial, &w4, 1);

    std::cout << "Sending w5: " << w5 << std::endl;
    write(pserial, &w5, 1);

    std::cout << "Sending val1_b0: " << val1_byte0 << std::endl;
    write(pserial, &val1_byte0, 1);
    
    std::cout << "Sending val1_b1: " << val1_byte1 << std::endl;
    write(pserial, &val1_byte1, 1);    

    std::cout << "Sending val2_b0: " << val2_byte0 << std::endl;
    write(pserial, &val2_byte0, 1);
    
    std::cout << "Sending val2_b1: " << val2_byte1 << std::endl;
    write(pserial, &val2_byte1, 1);   

    std::cout << "Sending CRC[1]: " << crc[1] << std::endl;
    write(pserial, &crc[1], 1);

    std::cout << "Sending CRC[0]: " << crc[0] << std::endl;
    write(pserial, &crc[0], 1);

    std::cout << "Sending ETx: " << ETx << std::endl;
    write(pserial, &ETx, 1);


    serial.flush();

    std::this_thread::sleep_for(std::chrono::milliseconds(150));
	   
    return 0;
}

