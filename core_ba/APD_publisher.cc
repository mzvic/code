#include <errno.h>
#include <fcntl.h> 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <stdbool.h>
#include <csignal>
//#include <grpcpp/grpcpp.h>
#include <chrono>
#include <iostream>
#include <memory>
#include <thread>
#include "client.h"
#include "core.grpc.pb.h"

using namespace core;
using google::protobuf::Timestamp;

bool exit_flag = false;
mutex signal_mutex;
condition_variable signal_cv;

void HandleSignal(int) {
  unique_lock<mutex> slck(signal_mutex);

  cout << "Exiting..." << endl;

  exit_flag = true;

  signal_cv.notify_one();
}



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
  Bundle bundle;
  PublisherClient publisher_client;
  Timestamp timestamp;        
  std::signal(SIGINT, HandleSignal);
  bundle.set_type(DATA_APD_FULL);
  struct timeval tv;
  int c = 0;
  int amount_data = 1000;
  while (!exit_flag) { 
    while (fd > 0 and c <= amount_data){      
      unsigned char buf[2];
      int rdlen;
      rdlen = read(fd, buf, sizeof(buf) - 1); 
      c++;
      if (rdlen > 0) {
        unsigned char *p;
        for (p = buf; rdlen-- > 0; p++)
          bundle.add_value(*p);
        if (c >= amount_data){
          publisher_client.Publish(bundle);
          //std::this_thread::sleep_for(std::chrono::microseconds(2));
          bundle.clear_value(); 
          c = 0;
        }
      }	
    }   
  }
  return 0;
} 

