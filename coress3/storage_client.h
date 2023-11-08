#include <condition_variable>
#include <grpcpp/grpcpp.h>
#include <queue>
#include <thread>

#include "client.h"
#include "core.grpc.pb.h"
#include "core.pb.h"

#define SERVER_ADDRESS "localhost:50052"

class PusherClient : public ClientUpstream<Bundle, Empty> {
 public:
  explicit PusherClient();
  void Initialize(ClientUpstreamReactor<Bundle> *) override;
};

class PullerClient : public ClientDownstream<Bundle, Query> {
 public:
  explicit PullerClient(const function<void(const Bundle &)> &);
  void Initialize(ClientDownstreamReactor<Bundle> *) override;
};