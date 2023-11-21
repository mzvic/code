#include <condition_variable>
#include <grpcpp/grpcpp.h>
#include <queue>

#include "core.grpc.pb.h"
#include "../log.h"

using namespace core;
using namespace grpc;
using namespace std;
using namespace google::protobuf;

namespace client_reactor {
typedef enum {
  RUNNING,
  STOPPING,
  STOPPED
} State;

template<class Outbound>
class ClientUpstreamReactor final : public ClientWriteReactor<Outbound> {
 public:
  explicit ClientUpstreamReactor(queue<Outbound> *, recursive_mutex *, const function<void(bool)> &);
  void OnDone(const Status &) override;
  void OnWriteDone(bool) override;
  void Start();
  void BeginStop();
  ClientContext *GetContext();
  shared_ptr<Channel> &GetChannel();
//  bool IsRunning();

 private:
  ClientContext context_;
  shared_ptr<Channel> channel_;
  queue<Outbound> *queue_;
  recursive_mutex *queue_mutex_;
  recursive_mutex state_mutex_;
  function<void(bool)> done_callback_;
  State state_ = RUNNING;

  void Stop();
};

template<class Inbound>
class ClientDownstreamReactor final : public ClientReadReactor<Inbound> {
 public:
  explicit ClientDownstreamReactor(const function<void(const Inbound &)> &, const function<void(bool)> &);
  void OnDone(const Status &) override;
  void OnReadDone(bool) override;
  void Start();
  void Stop();
  ClientContext *GetContext();
  shared_ptr<Channel> &GetChannel();
//  bool IsRunning();

 private:
  ClientContext context_;
  shared_ptr<Channel> channel_;
  Inbound inbound_;
  recursive_mutex state_mutex_;
  function<void(const Inbound &)> enqueue_callback_;
  function<void(bool)> done_callback_;
  State state_ = RUNNING;
};
} //namespace client_reactor

#include "client_reactor.tpp" // The only portable way of using templates at the moment is to implement them in header files by using inline functions.