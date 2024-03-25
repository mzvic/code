#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

int main() {
    int RS485 = open("/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0", O_RDWR | O_NOCTTY | O_NDELAY);
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

    unsigned char STx = 35;
    unsigned char _0 = 48;
    unsigned char _1 = 49;
    unsigned char _2 = 50;
    unsigned char _3 = 51;
    unsigned char _4 = 52;
    unsigned char _5 = 53;
    unsigned char _6 = 54;
    unsigned char _7 = 55;
    unsigned char _8 = 56;
    unsigned char _9 = 57;
    unsigned char _A = 65;
    unsigned char _U = 85;
    unsigned char _X = 88;
    unsigned char _I = 73;
    unsigned char _F = 70;
    unsigned char _T = 84;
    unsigned char _E = 69;
    unsigned char _CR = 13;

    while(true) {
        write(RS485, &STx, 1);
        write(RS485, &_0, 1);
        write(RS485, &_0, 1);
        //write(RS485, &_0);
        //write(RS485, &_F);
        //write(RS485, &_5);
        //write(RS485, &_E);
        //write(RS485, &_1);
        //write(RS485, &_1);

        //write(RS485, &_5);
        //write(RS485, &_F);
        //write(RS485, &_1);

        write(RS485, &_0, 1);
        write(RS485, &_F, 1);

        //write(RS485, &_A);
        //write(RS485, &_1);
        //write(RS485, &_A);
        //write(RS485, &_U);
        //write(RS485, &_X);
        //write(RS485, &_2);

        write(RS485, &_CR, 1);

        std::cout << "XGS600 response: " << std::endl;

        char ANS;
        std::string response;
        while (true) {
            read(RS485, &ANS, 1);
            if (ANS == '\r') {
                break;
            }
            response += ANS;
        }

        std::size_t start_index = response.find('>');
        std::size_t comma_index = response.find(',');

        std::string AUX1 = response.substr(start_index + 1, comma_index - start_index - 1);
        std::string AUX2 = response.substr(comma_index + 1);

        std::cout << "AUX1: " << AUX1 << std::endl;
        std::cout << "AUX2: " << AUX2 << std::endl;

        usleep(1000000); // Espera 1 segundo
        std::cout << std::endl;
    }

    close(RS485);
    return 0;
}

