#ifndef CORE_CLIENT_H_
#define CORE_CLIENT_H_

#include <condition_variable>
#include <grpcpp/grpcpp.h>
#include <queue>
#include <thread>

#include "core.grpc.pb.h"
#include "reactor/client_reactor.h"
#include "log.h"

using namespace core;
using namespace client_reactor;
using namespace grpc;
using namespace std;
using namespace google::protobuf;

namespace client {
typedef enum {
  RUNNING,
  STOPPING,
  STOPPED
} State;

template<class Outbound, class Response>
class ClientUpstream {
 public:
  explicit ClientUpstream(bool);
  ~ClientUpstream();
  void SetDoneCallback(const function<void(bool)> &);
//  void EnqueueOutboundMessage(const Outbound &);
//  void EnqueueOutboundMessage(Outbound &, const Timestamp &);
//  void Flush();
//  void Terminate();
  const Response &GetResponse();

 protected:
  Response response_;        // Here the server response will be stored
  void Start();
  void Stop();
  void EnqueueOutboundMessage(const Outbound &);

 private:
  ClientUpstreamReactor<Outbound> *client_upstream_reactor_;
  bool persistent_;
  queue<Outbound> queue_;
  recursive_mutex queue_mutex_;
  function<void(bool)> done_callback_;    // Callback once underlying reactor is done. Called on non-persistent mode only and is a good place to get response if ok

  mutex state_mutex_;
  condition_variable wait_cv_;
  State state_ = RUNNING;

  void OnClientUpstreamReactorOnDone(bool);
  virtual void Initialize(ClientUpstreamReactor<Outbound> *) = 0;
};

template<class Inbound, class Request>
class ClientDownstream {
 public:
  explicit ClientDownstream(bool, const function<void(const Inbound &)> &);
  ~ClientDownstream();
  void SetDoneCallback(const function<void(bool)> &);

 protected:
  Request request_;    // Here we store the request to be sent to the server
  void Start();
  void Stop();

 private:
  ClientDownstreamReactor<Inbound> *client_downstream_reactor_;
  bool persistent_;
  queue<Inbound> queue_;
  mutex queue_mutex_;
  condition_variable queue_cv_;
  thread queue_thread_;
  function<void(const Inbound &)> inbound_callback_;
  function<void(bool)> done_callback_;

  mutex state_mutex_;
  condition_variable wait_cv_;
  State state_ = RUNNING;

  void OnClientDownstreamReactorOnDone(bool);
  void EnqueueInboundMessage(const Inbound &);
  void QueueProcessing();
  virtual void Initialize(ClientDownstreamReactor<Inbound> *) = 0;
};
} //namespace client

#include "client.tpp" // The only portable way of using templates is to implement them in header files

#endif //CORE_CLIENT_H_