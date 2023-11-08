#include <queue>

#include <grpcpp/grpcpp.h>

#include "core.grpc.pb.h"

using namespace grpc;
using namespace std;

namespace server_reactor {
template<class Inbound, class Response>
class ServerUpstreamReactor : public ServerReadReactor<Inbound> {
 public:
  explicit ServerUpstreamReactor(Response *);
  void OnDone() override;
  void OnCancel() override;
  void OnReadDone(bool) override;
  Response *GetResponse();
  void SetInboundCallback(const function<void(const Inbound &)> &);
  void SetReadyCallback(const function<void(void *)> &);
  void SetDoneCallback(const function<void(void *)> &);

 private:
  Inbound inbound_;
  Response *response_;    // This is a reference to the response allocated by GRPC library
//  ServerUpstreamReactorInterface<Inbound> *interface_;
  bool finished_;
  bool done_;
  mutex mutex_;
  function<void(const Inbound &)> inbound_callback_;
  function<void(void *)> ready_callback_; // Signal before finish call to be sure the response is ready to be sent back
  function<void(void *)> done_callback_;
};

template<class Outbound, class Request>
class ServerDownstreamReactor : public ServerWriteReactor<Outbound> {
 public:
  explicit ServerDownstreamReactor(const Request *);
  void OnDone() override;
  void OnCancel() override;
  void OnWriteDone(bool) override;
  const Request *GetRequest();
  void SetDoneCallback(const function<void(void *)> &);
  void EnqueueOutboundMessage(const Outbound &);
  void Terminate();                        // Function to call to signal the client about termination of an outbound operation

 private:
  queue<Outbound> queue_;
  const Request *request_;        // This is a reference to the request allocated by GRPC library
  bool finished_;
  bool done_;
  mutex mutex_;
  function<void(void *)> done_callback_;
};

}// namespace server_reactor

#include "server_reactor.tpp" // The only portable way of using templates at the moment is to implement them in header files by using inline functions.