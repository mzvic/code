#include <condition_variable>
#include <grpcpp/grpcpp.h>
#include <queue>
#include <thread>

#include "core.grpc.pb.h"

#define SERVER_ADDRESS "localhost:50051"

using namespace core;
using namespace grpc;
using namespace std;
using namespace google::protobuf;

class PublisherClient {
 public:
  explicit PublisherClient();
  ~PublisherClient();
  void Publish(Bundle &);
  void Publish(Bundle &, const timeval &);

 private:
  class PublisherClientReactor : public ClientWriteReactor<Bundle> {
   public:
	explicit PublisherClientReactor(PublisherClient *);
	void OnDone(const Status &) override;
	void OnWriteDone(bool ok) override;
	void StopNow();
	void Stop();

   private:
	PublisherClient *publisher_client_;
	ClientContext context_;
	Empty empty_;
	mutex running_mutex_;
	bool running_ = true;
	bool stopping_ = false;
  };

  PublisherClientReactor *publisher_client_reactor_;
  queue <Bundle> queue_;
  mutex queue_mutex_;
  mutex running_mutex_;
  condition_variable running_cv_;
  bool running_ = true;
  bool stopping_ = false;

  void Wait();
  void OnPublisherOnDone();
};

class SubscriberClient {
 public:
  explicit SubscriberClient(function<void(const Bundle &)>, const vector<int> &);
  ~SubscriberClient();

 private:
  class SubscriberClientReactor : public ClientReadReactor<Bundle> {
   public:
	explicit SubscriberClientReactor(SubscriberClient *);
	void OnDone(const Status &) override;
	void OnReadDone(bool ok) override;

   private:
	SubscriberClient *subscriber_client_;
	ClientContext context_;
	Bundle bundle_;
  };

  function<void(const Bundle &)> callback_;
  Interests interests_;

  void OnSubscriberOnDone();
};
