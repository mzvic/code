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

    void Publish(int fd) {
        ClientContext context;
        Empty empty;
        Bundle bundle;
        Timestamp timestamp;
        struct timeval tv;
        std::unique_ptr<ClientWriter<Bundle>> writer(stub->Publish(&context, &empty));
        int c = 0;
        int amount_data = 1000;
        while (fd > 0 and c <= amount_data){
            gettimeofday(&tv, nullptr);
            unsigned char buf[2];
            int rdlen;
            rdlen = read(fd, buf, sizeof(buf) - 1); 
            c++;
            if (rdlen > 0) {
	            unsigned char   *p;
                for (p = buf; rdlen-- > 0; p++)
          	        bundle.add_apd(*p);
      	        //google::protobuf::Timestamp* timestamp = new google::protobuf::Timestamp;
             	if (c >= amount_data){
                    timestamp.set_seconds(tv.tv_sec);
                    timestamp.set_nanos(tv.tv_usec * 1000);
                    bundle.mutable_timestamp()->CopyFrom(timestamp);
                    writer->Write(bundle);
                    bundle.clear_apd();
                    c = 0;
                    					//printf("Counts: %d - Ts: %ld.%06ld\n", s, tv.tv_sec, tv.tv_usec);	
				}
      	        //printf("Counts: %d - Ts: %ld.%06ld\n", s, tv.tv_sec, tv.tv_usec);
 	    	}	
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
int main(int argc, char* argv[]) {
    int s;
    const char *portname;
    if (argc < 2) {
        printf("Usage: %s <portname>\n", argv[0]);
        return -1;
    } 
    portname = argv[1];
    fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
    printf("Error opening %s: %s\n", portname, strerror(errno));
    	return -1;}
    set_interface_attribs(fd, B2000000);
    set_mincount(fd, 0);               
    std::string server_address("localhost:50051");
    BrokerClient broker(grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials()));
    broker.Publish(fd);} 

