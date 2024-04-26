#include <cstring>
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

bool exit_flag = false;  // Used to signal the program to exit
std::mutex signal_mutex; // Mutex for synchronization
std::condition_variable signal_cv; // Condition variable for synchronization
int pserial;
// Function to handle the interrupt signal (SIGINT)
void HandleSignal(int) {
  std::unique_lock<std::mutex> slck(signal_mutex);
  std::cout << "Exiting..." << std::endl;
  exit_flag = true;
  signal_cv.notify_one();
}

struct Response {
    unsigned char* data;
    unsigned char data_length;
};

Response read_response(int pserial) {
    unsigned char response[512];
    int bytesRead = read(pserial, response, sizeof(response)); 
    if (bytesRead > 0) {
        unsigned char read_function = response[0];
        unsigned char function_code = response[1];
        unsigned char data_length = response[2];
        unsigned char* data = new unsigned char[data_length]; 
        
        std::memcpy(data, &response[3], data_length);
        return {data, data_length}; 
    } 
    return {nullptr, 0}; 
}

void send_data(unsigned char add, unsigned char function, unsigned char word_0, unsigned char word_1, unsigned char windows_0, unsigned char windows_1, unsigned char crc_2, unsigned char crc_1) {
    write(pserial, &add, 1);
    write(pserial, &function, 1);
    write(pserial, &word_0, 1);
    write(pserial, &word_1, 1);
    write(pserial, &windows_0, 1);
    write(pserial, &windows_1, 1);
    write(pserial, &crc_2, 1);
    write(pserial, &crc_1, 1);

}

short int hexToInt16(unsigned char* bytes) {
    if (bytes == nullptr) {
        //std::cerr << "Error: nullptr passed to hexToInt16" << std::endl;
        return 0; 
    }
    if (bytes[0] == '\0' || bytes[1] == '\0') {
        //std::cerr << "Error: bytes array too short in hexToInt16" << std::endl;
        return 0; 
    }
    int intValue = (bytes[0] << 8) | bytes[1];
    intValue = intValue * 0.1; 
    return intValue;
}


int main() {
    // Main loop: continuously read data from Prevac (electron gun)
    while (!exit_flag) {  
        pserial = open(serialport, O_RDWR | O_NOCTTY | O_NDELAY);
        if (pserial == -1) {
            std::cerr << "Error opening serial port." << std::endl;
            return 1;
        }

        struct termios options;
        tcgetattr(pserial, &options);
        cfsetispeed(&options, B9600);
        cfsetospeed(&options, B9600);
        options.c_cflag |= (CLOCAL | CREAD);
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSTOPB;
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS8;
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        options.c_oflag &= ~OPOST;
        options.c_cc[VMIN] = 0;
        options.c_cc[VTIME] = 10;
        tcsetattr(pserial, TCSANOW, &options);

        Response response;
        send_data(4, 3, 0, 56, 0, 1, 5, 146);
        usleep(50000);
        response = read_response(pserial); 
        short int intValue = hexToInt16(response.data);
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::cout << "Timestamp: " << std::put_time(std::localtime(&now_c), "%F %T")  << "  -  Battery status: " << intValue << "%..." << std::endl;

        close(pserial);
        sleep(1);
    }
    
    return 0;
}

