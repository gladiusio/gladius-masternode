#include <unistd.h>

#include <folly/Memory.h>
#include <folly/executors/IOThreadPoolExecutor.h>
#include <folly/io/async/EventBaseManager.h>

#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/RequestHandlerFactory.h>

#include "ProxyHandler.h"

using namespace proxygen;
using namespace masternode;

using folly::HHWheelTimer;

class ProxyHandlerFactory : public RequestHandlerFactory {
  public:
    ProxyHandlerFactory(size_t size)
        : cpuPool_(std::make_unique<folly::CPUThreadPoolExecutor>(size)) {}

    ~ProxyHandlerFactory() {}

    void onServerStart(folly::EventBase *evb) noexcept override {
      timer_->timer = HHWheelTimer::newTimer(
        evb,
        std::chrono::milliseconds(HHWheelTimer::DEFAULT_TICK_INTERVAL),
        folly::AsyncTimeout::InternalEnum::NORMAL,
        std::chrono::seconds(60000));
    }

    void onServerStop() noexcept override {
      timer_->timer.reset();
    }

    RequestHandler *onRequest(RequestHandler *, HTTPMessage *) noexcept override {
      return new ProxyHandler(cpuPool_.get(), timer_->timer.get());
    }

  protected:
    std::unique_ptr<folly::CPUThreadPoolExecutor> cpuPool_;

  private:
    struct TimerWrapper {
      HHWheelTimer::UniquePtr timer;
    };
    folly::ThreadLocal<TimerWrapper> timer_;
};

int main(int argc, char *argv[]) {
  size_t threads = sysconf(_SC_NPROCESSORS_ONLN);

  std::vector<HTTPServer::IPConfig> IPs = {
      {folly::SocketAddress("0.0.0.0", 80, true),
       HTTPServer::Protocol::HTTP}};

  HTTPServerOptions options;
  options.threads = threads;
  options.idleTimeout = std::chrono::milliseconds(60000);
  options.shutdownOn = {SIGINT, SIGTERM};
  options.enableContentCompression = false;
  options.handlerFactories =
      RequestHandlerChain()
          .addThen<ProxyHandlerFactory>(threads)
          .build();

  HTTPServer server(std::move(options));
  server.bind(IPs);

  std::thread t([&]() { server.start(); });

  t.join();

  return 0;
}