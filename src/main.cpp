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

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>

static const std::string urls[] = {
  #include "1000URLs.h"
};

static const int ports[] = {
  #include "1000Ports.h"
};

static std::vector<std::string> ips = {};

class NotificationQueue {
  private:
    std::deque<std::function<void()>> _q;
    bool _done{false};
    std::mutex _mutex;
    std::condition_variable _ready;
  public:
    void done() {
      {
        std::unique_lock<std::mutex> lock{_mutex};
        _done = true;
      }
      _ready.notify_all();
    }

    bool pop(std::function<void()>& x) {
      std::unique_lock<std::mutex> lock{_mutex};
      while (_q.empty() && !_done) {
        _ready.wait(lock);
      }
      if (_q.empty()) {
        return false;
      }
      x = move(_q.front());
      _q.pop_front();
      return true;
    }

    template<typename F>
    void push(F&& f) {
      {
        std::unique_lock<std::mutex> lock{_mutex};
        _q.emplace_back(std::forward<F>(f));
      }
      _ready.notify_one();
    }
};

class TaskSystem {
  private:
    const unsigned _count{std::thread::hardware_concurrency()};
    std::vector<std::thread> _threads;
    NotificationQueue _q;
    void run(unsigned i) {
      while (true) {
        std::function<void()> f;
        _q.pop(f);
        f();
      }
    }
  public:
    TaskSystem() {
      for (unsigned n = 0; n < _count; ++n) {
        _threads.emplace_back([&, n] { run(n); });
      }

    }

    ~TaskSystem() {
      for (auto& e : _threads) {
        e.join();
      }
    }

    template <typename F>
    void async_(F&& f) {
      _q.push(std::forward<F>(f));
    }
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

void portScan(std::string ip, TaskSystem& taskSystem) {
  for (auto port : ports) {
    taskSystem.async_([=](){
      if (isPortOpen(ip, port)) {
        std::cout << ip << ":" << port << " is open" << std::endl;
      }
    });
  }
}

void taskDnsLookup(std::string url, TaskSystem& taskSystem) {
  std::string ip = dnsLookup(url);
  if (!ip.empty()) {
    ips.emplace_back(ip);
    std::cout << ip << std::endl;
    portScan(ip, taskSystem);
  }
}

int main() {
  TaskSystem taskSystem;
  for (auto url : urls) {
    taskSystem.async_([&](){
      taskDnsLookup(url, taskSystem);
    });
  }
}
