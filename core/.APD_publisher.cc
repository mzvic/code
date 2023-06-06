#define terminal    "/dev/ttyUSB1"

#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <grpcpp/grpcpp.h>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include "core.grpc.pb.h"

using core::Broker;
using core::Bundle;
using google::protobuf::Empty;
using google::protobuf::Timestamp;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientWriter;
using grpc::Status;

class BrokerClient {
public:
    BrokerClient(std::shared_ptr<Channel> channel) : stub(Broker::NewStub(channel)) {}

    void Publish(int fd, int s) {
        ClientContext context;
        Empty empty;
        Bundle bundle;
        Timestamp timestamp;
        
        struct timeval tv;
        std::unique_ptr<ClientWriter<Bundle>> writer(stub->Publish(&context, &empty));
        int c = 0;
        int amount_data = 768;
        while (fd > 0 and c <= amount_data){
            unsigned char buf[2];
            int rdlen;
            rdlen = read(fd, buf, sizeof(buf) - 1); 
            if (rdlen > 0) {
                unsigned char   *p;
                for (p = buf; rdlen-- > 0; p++)
                    s = *p;
      	        bundle.add_apd(s);
      	        //google::protobuf::Timestamp* timestamp = new google::protobuf::Timestamp;
		gettimeofday(&tv, nullptr);
      	        bundle.add_sec(tv.tv_sec);
      	        bundle.add_nan(tv.tv_usec);
             	if (c >= amount_data){
			writer->Write(bundle);
			c = 0;
			//printf("Counts: %d - Ts: %ld.%06ld\n", s, tv.tv_sec, tv.tv_usec);
			bundle.Clear();
		}
      	        //printf("Counts: %d - Ts: %ld.%06ld\n", s, tv.tv_sec, tv.tv_usec);
 	    	}
 	    	c++;
      	}
        writer->WritesDone();
        Status status = writer->Finish();
    }
private:
    std::unique_ptr<Broker::Stub> stub;
};

int set_interface_attribs(int fd, int speed){
    struct termios tty;
    if (tcgetattr(fd, &tty) < 0) {
	printf("Error from tcgetattr: %s\n", strerror(errno));
	return -1;}
    cfsetospeed(&tty, (speed_t)speed);
    cfsetispeed(&tty, (speed_t)speed);
    tty.c_cflag |= (CLOCAL | CREAD);    
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         
    tty.c_cflag &= ~PARENB;     
    tty.c_cflag &= ~CSTOPB;     
    tty.c_cflag &= ~CRTSCTS;   
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;
    tty.c_cc[VMIN] = 1;
    tty.c_cc[VTIME] = 1;
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
	printf("Error from tcsetattr: %s\n", strerror(errno));
	return -1;}
    return 0;}

void set_mincount(int fd, int mcount){
    struct termios tty;
    if (tcgetattr(fd, &tty) < 0) {
	printf("Error tcgetattr: %s\n", strerror(errno));
	return;}
    tty.c_cc[VMIN] = mcount ? 1 : 0;
    tty.c_cc[VTIME] = 5;       
    if (tcsetattr(fd, TCSANOW, &tty) < 0)
	printf("Error tcsetattr: %s\n", strerror(errno));}    

int wlen, fd;
int main(int argc, char **argv) {
    int s;
    const char *portname = terminal;    
    fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
    printf("Error opening %s: %s\n", portname, strerror(errno));
    	return -1;}
    set_interface_attribs(fd, B4000000);
    set_mincount(fd, 0);               
    std::string server_address("localhost:50051");
    BrokerClient broker(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));
    broker.Publish(fd,s);} 

