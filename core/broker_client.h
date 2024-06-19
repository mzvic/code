#include <condition_variable>
#include <grpcpp/grpcpp.h>
#include <queue>
#include <thread>

#include "client.h"
#include "core.grpc.pb.h"
#include "core.pb.h"

#define BROKER_SERVER_ADDRESS "localhost:50051"

class PublisherClient : public ClientUpstream<Bundle, Empty> {
 public:
  explicit PublisherClient();
  ~PublisherClient();
  void Initialize(ClientUpstreamReactor<Bundle> *) override;
  void Publish(Bundle &);
  void Publish(Bundle &, const Timestamp &);
};

class SubscriberClient : public ClientDownstream<Bundle, Interests> {
 public:
  explicit SubscriberClient(const function<void(const Bundle &)> &, const vector<int> & = {});
  ~SubscriberClient();
  void Initialize(ClientDownstreamReactor<Bundle> *) override;
};