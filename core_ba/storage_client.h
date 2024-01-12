#include <condition_variable>
#include <grpcpp/grpcpp.h>
#include <queue>
#include <thread>

#include "client.h"
#include "core.grpc.pb.h"
#include "core.pb.h"

#define STORAGE_SERVER_ADDRESS "localhost:50052"

class PusherClient : public ClientUpstream<Bundle, Empty> {
 public:
  explicit PusherClient();
  ~PusherClient();
  void Initialize(ClientUpstreamReactor<Bundle> *) override;
  void Push(Bundle &);
  void Push(Bundle &, const Timestamp &);
};

class PullerClient : public ClientDownstream<Bundle, Query> {
 public:
  explicit PullerClient(const function<void(const Bundle &)> &);
  ~PullerClient();
  void Initialize(ClientDownstreamReactor<Bundle> *) override;
};