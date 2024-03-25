#include <vector>
#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

namespace asio = boost::asio;
using namespace std;

asio::io_service io;
asio::serial_port serial(io);

int RS485;

vector<unsigned char> calculate_crc(const vector<unsigned char>& data) {
    unsigned short CRC = 0xFFFF;
    for (unsigned char byte : data) {
        CRC ^= byte;
        for (int i = 0; i < 8; ++i) {
            bool carry = CRC & 1;
            CRC >>= 1;
            if (carry) {
                CRC ^= 0xA001;
            }
        }
    }
    vector<unsigned char> result;
    result.push_back((CRC >> 8) & 0xFF);
    result.push_back(CRC & 0xFF);
    return result;
}

tuple<int, int, int, vector<unsigned char>> read_response() {
    vector<unsigned char> A(1), B(1), C_data(1), CRC_received(2);
    read(RS485, &A, 1);
    read(RS485, &B, 1);
    read(RS485, &C_data, 1);
    std::cerr << &A << &B << &C_data << std::endl;
    int address = static_cast<int>(A[0]);
    int function_code = static_cast<int>(B[0]);
    int C = static_cast<int>(C_data[0]);
    vector<unsigned char> data;
    data.resize(C);
    read(RS485, data.data(), C);
    read(RS485, CRC_received.data(), 4);
    return make_tuple(address, function_code, C, data);
}

int main() {
    RS485 = open("/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0", O_RDWR | O_NOCTTY | O_NDELAY);
    if (RS485 == -1) {
        std::cerr << "Error al abrir el puerto serial." << std::endl;
        return 1;
    }

    struct termios options;
    tcgetattr(RS485, &options);
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
    tcsetattr(RS485, TCSANOW, &options);

    // Obtener la entrada del usuario
    int strt_mem, win_amount;
    cout << "First word address: ";
    cin >> strt_mem;
    cout << "Amount of windows to read: ";
    cin >> win_amount;

    // Calcular los valores a enviar
    int chr3 = (strt_mem >> 8) & 0xFF;
    int chr4 = strt_mem & 0xFF;
    int chr5 = (win_amount >> 8) & 0xFF;
    int chr6 = win_amount & 0xFF;

    vector<unsigned char> data = {1, 3, static_cast<unsigned char>(chr3), static_cast<unsigned char>(chr4),
                                   static_cast<unsigned char>(chr5), static_cast<unsigned char>(chr6)};
    auto crc = calculate_crc(data);

    // Enviar datos al puerto serial
    write(RS485, data.data(), data.size());
    write(RS485, &crc, sizeof(crc));

    // Leer la respuesta del puerto serial
    auto response = read_response();

    // Imprimir la respuesta
    if (get<3>(response).size() > 0) {
        cout << "Address: " << get<0>(response) << endl;
        cout << "Function Code: " << get<1>(response) << endl;
        cout << "Data Length: " << get<2>(response) << endl;
        cout << "Data: ";
        for (unsigned char byte : get<3>(response)) {
            cout << static_cast<int>(byte) << " ";
        }
        cout << endl;
    } else {
        cout << "Error al leer la respuesta." << endl;
    }

    // Cerrar el puerto serial
    close(RS485);

    return 0;
}

