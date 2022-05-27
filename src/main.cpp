#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string>
#include <iostream>
#include <thread>
#include <memory>
#include <vector>

bool isPortOpen(std::string ip, int port) {
  struct sockaddr_in address;
  struct timeval tv;
  short int sock;
  fd_set fdset;

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = inet_addr(ip.c_str());
  address.sin_port = htons(port);

  sock = socket(AF_INET, SOCK_STREAM, 0);
  fcntl(sock, F_SETFL, O_NONBLOCK);
  connect(sock, (struct sockaddr *)&address, sizeof(address));

  FD_ZERO(&fdset);
  FD_SET(sock, &fdset);
  tv.tv_sec = 5;             /* timeout */
  tv.tv_usec = 0;

  if (select(sock + 1, NULL, &fdset, NULL, &tv) == 1) {
      int so_error;
      socklen_t len = sizeof so_error;
      getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
      close(sock);
      return so_error == 0 ? true : false;
  }
  return false;
}

void taskFunction(std::string ip, int port) {
  if (isPortOpen(ip, port)) {
    std::cout << ip << ":" << port << " is open" << std::endl;
  }
}

int main() {
    static const int ports[] = {
        #include "1000Ports.h"
    };

    std::vector<std::unique_ptr<std::thread>> tasks;

    for (auto port : ports) {
      tasks.push_back(std::make_unique<std::thread>(std::thread(taskFunction, "45.33.32.156", port)));
    }
    for (auto& task : tasks) {
      task->join();
    }
}
