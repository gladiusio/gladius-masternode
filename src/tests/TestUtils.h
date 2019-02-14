#pragma once

#include <boost/thread.hpp>
#include <proxygen/httpserver/HTTPServer.h>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

#include "Masternode.h"

using namespace proxygen;

class ServerThread {
 private:
  boost::barrier barrier_{2};
  std::thread t_;
  HTTPServer* server_{nullptr};

 public:

  explicit ServerThread(HTTPServer* server) : server_(server) {}
  ~ServerThread() {
    if (server_) {
      server_->stop();
    }
    t_.join();
  }

  bool start() {
    bool throws = false;
    t_ = std::thread([&]() {
      server_->start([&]() { barrier_.wait(); },
                     [&](std::exception_ptr /*ex*/) {
                       throws = true;
                       server_ = nullptr;
                       barrier_.wait();
                     });
    });
    barrier_.wait();
    return !throws;
  }
};

// Wrapper/helper class around an httplib Server object
// so it can run in another thread.
class OriginThread {
 private:
  std::thread t_;
  httplib::Server& server_;

 public:
  explicit OriginThread(httplib::Server& server) : server_(server) {}
  ~OriginThread() {
    // Stop the server and join the thread back in
    // automatically when a unit test finishes.
    server_.stop();
    t_.join();
  }

  void start() {
    t_ = std::thread([&]() {
      server_.listen("0.0.0.0", 8085);
    });
  }
};

class MasternodeThread {
  private:
    boost::barrier barrier_{2}; // barrier so we can "wait" for the server to start
    std::thread t_;
    masternode::Masternode* master_{nullptr};
  public:
    explicit MasternodeThread(masternode::Masternode* master) : master_(master){}
    ~MasternodeThread() {

      if (master_) {
        master_->stop();
      }
      t_.join();
    }

    bool start() {
      bool throws = false;
      t_ = std::thread([&]() {
        master_->start([&]() { barrier_.wait(); },
                        [&](std::exception_ptr ep) {
                          try {
                            if (ep) {
                              std::rethrow_exception(ep);
                            }
                          } catch(const std::exception& e) {
                            LOG(ERROR) << "Caught exception: " << e.what() << "\n";
                          }
                          throws = true;
                          master_ = nullptr;
                          barrier_.wait();
                        });
      });
      barrier_.wait();
      return !throws;
    }
};