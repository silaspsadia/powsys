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

static const std::string urls[] = {
  #include "1000URLs.h"
};

static const int ports[] = {
  #include "1000Ports.h"
};

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

std::string dnsLookup(std::string url) {
  struct addrinfo hints, *res, *p;
  char ipstr[INET6_ADDRSTRLEN];
  int status;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  if ((status = getaddrinfo(url.c_str(), NULL, &hints, &res)) != 0) {
    return "";
  }

  for (p = res; p != NULL; p = p->ai_next) {
    void *addr;
    if (p->ai_family == AF_INET) { // IPv4
      struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
      addr = &(ipv4->sin_addr);
      inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
    }
  }
  return std::string(ipstr);
}

void taskFunction(std::string ip, int port) {
  if (isPortOpen(ip, port)) {
    std::cout << ip << ":" << port << " is open" << std::endl;
  }
}

void taskDnsLookup(std::string url) {
  std::string ip = dnsLookup(url);
  std::cout << ip << std::endl;
}

void runPortScanDemo() {
  std::vector<std::unique_ptr<std::thread>> tasks;

  for (auto port : ports) {
    tasks.push_back(
      std::make_unique<std::thread>(
        std::thread(taskFunction, "45.33.32.156", port)
      )
    );
  }

  for (auto url: urls) {
    taskDnsLookup(url);
  }
}

void runDnsLookupDemo() {
  std::vector<std::unique_ptr<std::thread>> tasks;

  for (auto url : urls) {
    tasks.push_back(
      std::make_unique<std::thread>(
        std::thread(taskDnsLookup, url)
      )
    );
  }

  for (auto& task : tasks) {
    task->join();
  }
}

int main() {
  runPortScanDemo();
  runDnsLookupDemo();
}
