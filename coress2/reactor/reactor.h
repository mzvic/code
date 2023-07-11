#include <queue>

#include <grpcpp/grpcpp.h>

#include "core.grpc.pb.h"

using namespace core;
using namespace grpc;
using namespace google::protobuf;
using namespace std;

namespace reactor {
class ServerReactorInterface {
 public:
  virtual void ProcessMessage(Bundle &) = 0;

  virtual void OnPublisherServerReactorFinish(void *) = 0;

  virtual void OnSubscriberServerReactorFinish(void *) = 0;
};

class PublisherServerReactor : public ServerReadReactor<Bundle> {
 public:
  explicit PublisherServerReactor(ServerReactorInterface *);

  void OnDone() override;

  void OnCancel() override;

  void OnReadDone(bool) override;

 private:
  Bundle request_;
  ServerReactorInterface *interface_;
  bool finished_;
  bool done_;
  mutex mutex_;

  //		void Now();
};

class SubscriberServerReactor : public ServerWriteReactor<Bundle> {
 public:
  explicit SubscriberServerReactor(ServerReactorInterface *, const Interests *);

  //		~SubscriberServerReactor() override;

  void OnDone() override;

  void OnCancel() override;

  void OnWriteDone(bool) override;

  void EnqueueMessage(const Bundle &);

 private:
  queue <Bundle> queue_;
  ServerReactorInterface *interface_;
  vector<int> interests_;
  bool finished_;
  bool done_;
  mutex mutex_;

  //		void Now();
};

//	class PublisherClientReactor : public ClientWriteReactor<Bundle> {
//	public:
//		//		explicit PublisherClientReactor(ClientReactorInterface *);
//		//		explicit PublisherClientReactor(Broker::Stub *, std::mutex_);
//		explicit PublisherClientReactor(Broker::Stub *);
//
//		void OnDone(const Status &) override;
//
//		void OnWriteDone(bool) override;
//
//		void Enqueue(const Bundle &);
//
//	private:
//		vector<Bundle> queue_;
//		//		mutex_ subscriber_mutex;
//
//		//		std::mutex_ &mutex_;
//		//		ClientReactorInterface *interface_;
//
//		//		void Now();
//	};


}// namespace reactor
