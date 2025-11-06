#include <cstring>
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

#define serialport "/dev/serial/by-id/usb-1a86_USB2.0-Ser_-if00-port0"
//#define serialport "/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0"

using namespace core;
using google::protobuf::Timestamp;

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

struct Response {
    unsigned char* data;
    unsigned char data_length;
};

int pserial;

unsigned char STx = 1;
unsigned char ETx = 0;
unsigned char read_function = 3;

Response read_response() {
    unsigned char response[512];
    int bytesRead = read(pserial, response, sizeof(response)); 
    if (bytesRead > 0) {
        if (bytesRead < 5) {
            std::cerr << "Respuesta incompleta." << std::endl;
            return {nullptr, 0}; 
        }
        unsigned char read_function = response[0];
        unsigned char function_code = response[1];
        unsigned char data_length = response[2];
        unsigned char* data = new unsigned char[data_length]; 
        
        std::memcpy(data, &response[3], data_length);

        return {data, data_length}; 
    } 
    return {nullptr, 0}; 
}

void send_data(unsigned char function, unsigned char word_0, unsigned char word_1, unsigned char windows_0, unsigned char windows_1, unsigned char crc_2, unsigned char crc_1) {
    write(pserial, &STx, 1);
    write(pserial, &read_function, 1);
    write(pserial, &word_0, 1);
    write(pserial, &word_1, 1);
    write(pserial, &windows_0, 1);
    write(pserial, &windows_1, 1);
    write(pserial, &crc_2, 1);
    write(pserial, &crc_1, 1);
    write(pserial, &ETx, 1);
    write(pserial, &ETx, 1);
    write(pserial, &ETx, 1);
}

void clear_responses() {
    while (read_response().data != nullptr) {} 
}

float hexToFloat(unsigned char* bytes) {
    unsigned int uintValue = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
    float floatValue;
    std::memcpy(&floatValue, &uintValue, sizeof(float));
    return floatValue;
}

short int hexToInt16(unsigned char* bytes) {
    short int intValue = (bytes[0] << 8) | bytes[1];
    return intValue;
}

uint8_t hexToUint8(unsigned char* bytes) {
    uint8_t value = bytes[0]; 
    return value;
}

int main() {
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

    // Create a data bundle and a publisher client for communication
    Bundle bundle;
    PublisherClient publisher_client;
    Timestamp timestamp; 

    // Register the signal handler for SIGINT (Ctrl+C)
    std::signal(SIGINT, HandleSignal);

    // Set the data bundle type to DATA_EG_MON
    bundle.set_type(DATA_EG_MON);
    
    // Main loop: continuously read data from Prevac (electron gun)
    while (!exit_flag) {  
        bundle.clear_value(); 
        Response responses[13];
        
        for (int param = 0; param < 13; ++param) {
            if (param == 0) { //Operate
                send_data(read_function, 0, 0, 0, 1, 132, 10);
            } else if (param == 1) { //Stand by
                send_data(read_function, 0, 1, 0, 1, 213, 202);
            } else if (param == 2) { //Energy voltage set
                send_data(read_function, 0, 15, 0, 2, 244, 8);
            } else if (param == 3) { //Focus voltage set
                send_data(read_function, 0, 17, 0, 2, 148, 14);
            } else if (param == 4) { //Wehnelt voltage set
                send_data(read_function, 0, 19, 0, 2, 53, 206);
            } else if (param == 5) { //Emission current set
                send_data(read_function, 0, 21, 0, 2, 213, 207);
            } else if (param == 6) { //Scan position X
                send_data(read_function, 0, 23, 0, 2, 116, 15);
            } else if (param == 7) { //Scan position Y
                send_data(read_function, 0, 25, 0, 2, 21, 204);
            } else if (param == 8) { //Scan area X
                send_data(read_function, 0, 27, 0, 2, 180, 12);
            } else if (param == 9) { //Scan area Y
                send_data(read_function, 0, 29, 0, 2, 84, 13);
            } else if (param == 10) { //Scan grid X
                send_data(read_function, 0, 31, 0, 2, 245, 205);
            } else if (param == 11) { //Scan grid Y
                send_data(read_function, 0, 33, 0, 2, 148, 1);
            } else if (param == 12) { //Time per dot
                send_data(read_function, 0, 37, 0, 1, 149, 193);
            }
            usleep(50000);
            responses[param] = read_response(); 
        }

        //std::cout << "----------------------------------------------------------" << std::endl;  
        const char* param_names[13] = {"Operate", "Status flags", "Energy voltage set", "Focus voltage set", "Wehnelt voltage set", "Emission current set", "Scan position X", "Scan position Y", "Scan area X", "Scan area Y", "Scan grid X", "Scan grid Y", "Time Per Dot"};
        for (int i = 0; i < 13; ++i) {
            //std::cout << param_names[i] << ": ";
            for (unsigned char j = 0; j < responses[i].data_length; j += 4) {
                if (i == 0 || i == 1 || i == 12) { // INT16
                    short int intValue = hexToInt16(&responses[i].data[j]);
                    //std::cout << intValue << " ";
                    bundle.add_value(intValue);
                } else { // FLOAT
                    float floatValue = hexToFloat(&responses[i].data[j]);
                    //std::cout << floatValue << " ";
                    bundle.add_value(floatValue);
                }
            }    
            //std::cout << std::endl;
        }
        publisher_client.Publish(bundle);
        clear_responses(); 

        sleep(0.8);
    }

    close(pserial);
    return 0;
}

