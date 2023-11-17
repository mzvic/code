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
#include <iomanip> 
#include <algorithm>
#include <fstream>
#include <boost/asio/serial_port.hpp>
#include <boost/asio/serial_port_base.hpp>

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

char xor_STX = 2;
char xor_ADDR = 128;
char xor_ETX = 3;

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
        for (int i = 0; i < rdlen; i++) {
            printf("%02X ", static_cast<unsigned char>(buf[i]));
        }
        printf("\n");
    }
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
        
        std::string xor_WIN;
        char xor_OnOff = '1';
        char xor_WR = '1';
        char xor_checksum;
        if (subs_values[1] == 0){ // para pressure, 162 para setear
            xor_WIN = "162";
        }else if (subs_values[1] == 1){ // para motor 117/120 (low/high)
            xor_WIN = "117";
        }else if (subs_values[1] == 2){ // para valve 122
            xor_WIN = "122";  
        }
        
        char xor_WIN1 = xor_WIN[0];
        char xor_WIN2 = xor_WIN[1];
        char xor_WIN3 = xor_WIN[2];
        
        xor_checksum = xor_ADDR ^ xor_WIN1 ^ xor_WIN2 ^ xor_WIN3 ^ xor_WR ^ xor_OnOff ^ xor_ETX;
        char crc_1 = ((xor_checksum >> 4) & 0XF);
        char crc_2 = (xor_checksum & 0xF);
        char ascii_crc_1[3];
        char ascii_crc_2[3];
        snprintf(ascii_crc_1, sizeof(ascii_crc_1), "%X", crc_1);
        snprintf(ascii_crc_2, sizeof(ascii_crc_2), "%X", crc_2);

        std::cout << xor_STX << "\n";
        std::cout << xor_ADDR << "\n";
        std::cout << xor_WIN1 << "\n";
        std::cout << xor_WIN2 << "\n";
        std::cout << xor_WIN3 << "\n";
        std::cout << xor_WR << "\n";
        std::cout << xor_OnOff << "\n";           
        std::cout << xor_ETX << "\n";        
        std::cout << ascii_crc_1 << "\n";
        std::cout << ascii_crc_2 << "\n";
        std::ifstream serial("/dev/ttyACM0", std::ios::binary);                
        if (xor_checksum){

            std::ofstream serial_out("/dev/ttyACM0", std::ios::binary);
            serial_out.write(&xor_STX, 1);
            serial_out.write(&xor_ADDR, 1);
            serial_out.write(&xor_WIN1, 1);
            serial_out.write(&xor_WIN2, 1);
            serial_out.write(&xor_WIN3, 1);
            serial_out.write(&xor_WR, 1);
            serial_out.write(&xor_OnOff, 1);
            serial_out.write(&xor_ETX, 1);
            serial_out.write(ascii_crc_1, 1);
            serial_out.write(ascii_crc_2, 1);
        }
        
        // lectura temperatura de la bomba: 204
        // lectura estado valvula: 122
        // lectura presión: 224
        // lectura potencia bomba: 202 
        // lectura rpm motor bomba: 226
        
        char ANS;
        while (serial.read(&ANS, 1)) {
            std::cout << ANS;
        }
        
        slck.lock();
        exit_reading = false;
        slck.unlock();
    }
}


int main(int argc, char **argv) {
    try {
        boost::asio::io_service io;
        boost::asio::serial_port serial(io, "/dev/ttyACM0"); 
        serial.set_option(boost::asio::serial_port_base::baud_rate(2000000));
        serial.set_option(boost::asio::serial_port_base::character_size(8));
        serial.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));
        serial.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
    } catch (const std::exception& e) {
        std::cerr << "Error al configurar el puerto serie: " << e.what() << std::endl;
        return 1;
    }    

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
