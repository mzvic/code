#include <queue>

#include <grpcpp/grpcpp.h>

#include "core.grpc.pb.h"

using core::Broker;
using core::Bundle;
using grpc::ClientReadReactor;
using grpc::ClientWriteReactor;
using grpc::ServerReadReactor;
using grpc::ServerWriteReactor;
using grpc::Status;
using std::mutex;
using std::queue;
using std::vector;

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
		Bundle Request_;
		ServerReactorInterface *Interface_;
		bool Finished_;

		//		void ShutdownNow();
	};

	class SubscriberServerReactor : public ServerWriteReactor<Bundle> {
	public:
		explicit SubscriberServerReactor(ServerReactorInterface *);

		//		~SubscriberServerReactor() override;

		void OnDone() override;

		void OnCancel() override;

		void OnWriteDone(bool) override;

		void EnqueueMessage(const Bundle &);

	private:
		queue<Bundle> Queue_;
		ServerReactorInterface *Interface_;
		bool Finished_;
		mutex Mutex_;

		//		void ShutdownNow();
	};

	class PublisherClientReactor : public ClientWriteReactor<Bundle> {
	public:
		//		explicit PublisherClientReactor(ClientReactorInterface *);
		//		explicit PublisherClientReactor(Broker::Stub *, std::Mutex_);
		explicit PublisherClientReactor(Broker::Stub *);

		void OnDone(const Status &) override;

		void OnWriteDone(bool) override;

		void Enqueue(const Bundle &);

	private:
		vector<Bundle> Queue_;
		//		Mutex_ subscriber_mutex;

		//		std::Mutex_ &Mutex_;
		//		ClientReactorInterface *Interface_;

		//		void ShutdownNow();
	};


}// namespace reactor
