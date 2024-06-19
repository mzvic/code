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

#define serialport "/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0"
#define baudrate 9600


// Global variables
const char xor_STX = 2;
const char xor_ADDR = 128;
const char xor_WIN1 = 48;
const char xor_WIN2 = 48;
const char xor_WIN3 = 48;
const char xor_WR = 49;
char xor_OnOff = 48;
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
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <value for xor_OnOff>" << std::endl;
        return 1;
    }

    xor_OnOff = argv[1][0];

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
    char xor_checksum = xor_ADDR ^ xor_WIN1 ^ xor_WIN2 ^ xor_WIN3 ^ xor_WR ^ xor_OnOff ^ xor_ETX;
    char crc_1 = ((xor_checksum >> 4) & 0xF);
    char crc_2 = (xor_checksum & 0xF);
    char ascii_crc_1[3];
    char ascii_crc_2[3];
    snprintf(ascii_crc_1, sizeof(ascii_crc_1), "%X", crc_1);
    snprintf(ascii_crc_2, sizeof(ascii_crc_2), "%X", crc_2);

    serial.write(&xor_STX, 1);
    serial.write(&xor_ADDR, 1);
    serial.write(&xor_WIN1, 1);
    serial.write(&xor_WIN2, 1);
    serial.write(&xor_WIN3, 1);
    serial.write(&xor_WR, 1);
    serial.write(&xor_OnOff, 1);
    serial.write(&xor_ETX, 1);
    serial.write(ascii_crc_1, 1);
    serial.write(ascii_crc_2, 1);

    serial.flush();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    std::vector<char> response;
    char currentChar;
    int responseSize = 0;
    while (serial.get(currentChar)) {
        response.push_back(currentChar);
        if (currentChar == 3) {
            break;
        }
    }	   
    return 0;
}

