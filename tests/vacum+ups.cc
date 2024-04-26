#include <iostream>
#include <cstring>
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

int pserial;

bool exit_flag = false;  // Used to signal the program to exit
std::mutex signal_mutex; // Mutex for synchronization
std::condition_variable signal_cv; // Condition variable for synchronization

const char xor_STX = 2;
const char xor_ADDR1 = 128;
const char xor_ADDR2 = 129;
const char xor_WR = 48;
const char xor_OnOff = 48;
const char xor_ETX = 3;


volatile sig_atomic_t received_signal = 0;
// Function to handle the interrupt signal (SIGINT)
void HandleSignal(int signal) {
    if (signal == SIGINT) {
        if (exit_flag) {
            std::exit(0);
        } else {
            std::unique_lock<std::mutex> slck(signal_mutex);
            std::cout << "Exiting..." << std::endl;
            exit_flag = true;
            signal_cv.notify_one();
        }
    }
}

struct ups_Response {
    unsigned char* data;
    unsigned char data_length;
};

struct baro_Response {
    float pressure1;
    float pressure2;
};

struct pump_Response {
    float pump_data;
};

ups_Response read_ups_response(int pserial) {
    unsigned char ups_response[512];
    int bytesRead = read(pserial, ups_response, sizeof(ups_response)); 
    if (bytesRead > 0) {
        unsigned char read_function = ups_response[0];
        unsigned char function_code = ups_response[1];
        unsigned char data_length = ups_response[2];
        unsigned char* data = new unsigned char[data_length]; 
        
        std::memcpy(data, &ups_response[3], data_length);
        return {data, data_length}; 
    } 
    return {nullptr, 0}; 
}

float stringToFloat(const std::string& str) {
    std::istringstream iss(str);
    float f;
    iss >> f;
    if (iss.fail()) {
        throw std::invalid_argument("Invalid float format");
    }
    return f;
}

baro_Response read_baro_response(int pserial) {
    unsigned char baro_response[512];
    int bytesRead = read(pserial, baro_response, sizeof(baro_response)); 
    if (bytesRead > 0) {
        std::string response_str(reinterpret_cast<char*>(baro_response), bytesRead);
        //std::cout << "baro_Response: " << response_str << std::endl;
        size_t comma_pos = response_str.find(',');
        if (comma_pos != std::string::npos) {
            std::string pressure1_str = response_str.substr(1, comma_pos - 1); // Ignorar el símbolo '>'
            std::string pressure2_str = response_str.substr(comma_pos + 1);
            float pressure1 = stringToFloat(pressure1_str);
            float pressure2 = stringToFloat(pressure2_str);
            return {pressure1, pressure2};
        } else {
            std::cerr << "Error: Comma not found in response." << std::endl;
            return {0, 0}; 
        }
    } else {
        std::cerr << "Error reading from serial port." << std::endl;
        return {0, 0}; 
    } 
}


pump_Response read_pump_response(int pserial) {
    unsigned char pump_response[512];
    int bytesRead = read(pserial, pump_response, sizeof(pump_response));
    if (bytesRead > 0) {
        //std::cout << "pump_Response: ";
        //for (int i = 0; i < bytesRead; ++i) {
        //    std::cout << pump_response[i];
        //}
        //std::cout << std::endl;
        
        float rx_value = stringToFloat(std::string(pump_response + 6, pump_response + bytesRead));
        return {rx_value};
    } else {
        std::cerr << "Error reading from serial port." << std::endl;
    }
    return {0}; 
}


void send_data_pump(unsigned char pump_st, unsigned char pump_add, unsigned char pump_w1, unsigned char pump_w2, unsigned char pump_w3, unsigned char pump_wr, unsigned char pump_val, unsigned char pump_end, unsigned char pump_crc1, unsigned char pump_crc2) {
    write(pserial, &pump_st, 1);
    write(pserial, &pump_add, 1);
    write(pserial, &pump_w1, 1);
    write(pserial, &pump_w2, 1);
    write(pserial, &pump_w3, 1);
    write(pserial, &pump_wr, 1);
    write(pserial, &pump_val, 1);
    write(pserial, &pump_end, 1);
    write(pserial, &pump_crc1, 1);
    write(pserial, &pump_crc2, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

}

void send_data_baro(unsigned char baro_add, unsigned char baro_w1, unsigned char baro_w2, unsigned char baro_cr) {
    write(pserial, &baro_add, 1);
    write(pserial, &baro_w1, 1);
    write(pserial, &baro_w1, 1);
    write(pserial, &baro_w1, 1);
    write(pserial, &baro_w2, 1);
    write(pserial, &baro_cr, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

}

void send_data_ups(unsigned char add, unsigned char function, unsigned char word_0, unsigned char word_1, unsigned char windows_0, unsigned char windows_1, unsigned char crc_2, unsigned char crc_1) {
    write(pserial, &add, 1);
    write(pserial, &function, 1);
    write(pserial, &word_0, 1);
    write(pserial, &word_1, 1);
    write(pserial, &windows_0, 1);
    write(pserial, &windows_1, 1);
    write(pserial, &crc_2, 1);
    write(pserial, &crc_1, 1);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

}

short int hexToInt16(unsigned char* bytes) {
    if (bytes == nullptr) {
        return 0; 
    }
    if (bytes[0] == '\0' || bytes[1] == '\0') {
        return 0; 
    }
    int intValue = (bytes[0] << 8) | bytes[1];
    intValue = intValue * 0.1; 
    return intValue;
}

int main(int argc, char* argv[]) {
    // Register the signal handler for SIGINT (Ctrl+C)
    std::signal(SIGINT, HandleSignal);
        // Main loop: continuously read data from twistorr
        while (!exit_flag) { 
            std::cout << "---------------------------------------------------------------------------------" << std::endl;
            pserial = open(serialport, O_RDWR | O_NOCTTY);
            if (pserial == -1) {
                std::cerr << "Error opening serial port (first try)." << std::endl;
                pserial = open(serialport, O_RDWR | O_NOCTTY);
                if (pserial == -1) {
                    std::cerr << "Error opening serial port (second try)." << std::endl;
                    return 1;}
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

            pump_Response pump_response;
            baro_Response baro_response;
            ups_Response ups_response;
            for (int device_count = 1; device_count <= 2; device_count++) { // ADDR1 and ADDR2
                for (int xor_WIN = 199; xor_WIN <= 204; ++xor_WIN) {
                    std::string WIN = std::to_string(xor_WIN);
                    char xor_WIN1 = WIN[0];
                    char xor_WIN2 = WIN[1];
                    char xor_WIN3 = WIN[2];
                    if (xor_WIN == 199){
                        xor_WIN1 = 48;
                        xor_WIN2 = 48;
                        xor_WIN3 = 48;                
                    }
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
                    if (device_count == 1){
                        send_data_pump(2, xor_ADDR1, xor_WIN1, xor_WIN2, xor_WIN3, 48, 48, 3, ascii_crc_1[0], ascii_crc_2[0]);
                    }else{
                        send_data_pump(2, xor_ADDR2, xor_WIN1, xor_WIN2, xor_WIN3, 48, 48, 3, ascii_crc_1[0], ascii_crc_2[0]);}
                    pump_response = read_pump_response(pserial); 
                    std::cout << "Pump " << device_count << " - Window " << xor_WIN  << " value: " << pump_response.pump_data << std::endl;
                    std::cout << "---------------------------------------------------------------------------------" << std::endl;
                    
                }
            }
            
            while (baro_response.pressure1 < 0.000000001){
                send_data_baro(35, 48, 70, 13);  
                baro_response = read_baro_response(pserial);
            }
            std::cout << "Pressure 1: " << baro_response.pressure1 << " [Torr] - Pressure2: " << baro_response.pressure2 << " [Torr]" << std::endl;
            // Asking for UPS battery status
            std::cout << "---------------------------------------------------------------------------------" << std::endl;
            int intValue = 0;
            while (intValue > 100 || intValue < 1){
                send_data_ups(4, 3, 0, 56, 0, 1, 5, 146);
                ups_response = read_ups_response(pserial); 
                intValue = hexToInt16(ups_response.data);
            }    
            auto now = std::chrono::system_clock::now();
            std::time_t now_c = std::chrono::system_clock::to_time_t(now);
            std::cout << "Timestamp: " << std::put_time(std::localtime(&now_c), "%F %T")  << "  -  Battery status: " << intValue << "%..." << std::endl;
            std::cout << "---------------------------------------------------------------------------------" << std::endl;
            close(pserial);
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

        }

    return 0;
}

