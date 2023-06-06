#include <grpcpp/grpcpp.h>

#include "core.grpc.pb.h"

using core::Bundle;
using grpc::ServerReadReactor;
using grpc::ServerWriteReactor;
using std::vector;

namespace reactor {
	class ReactorInterface {
	public:
		virtual void ProcessMessage(Bundle) = 0;

		virtual void OnPublisherReactorFinish(void *) = 0;

		virtual void OnSubscriberReactorFinish(void *) = 0;
	};

	class PublisherReactor : public ServerReadReactor<Bundle> {
	public:
		explicit PublisherReactor(ReactorInterface *);

		void OnDone() override;

		void OnCancel() override;

		void OnReadDone(bool) override;

	private:
		Bundle Request_;
		ReactorInterface *Interface_;

		void Terminate();
	};

	class SubscriberReactor : public ServerWriteReactor<Bundle> {
	public:
		explicit SubscriberReactor(ReactorInterface *);

		~SubscriberReactor() override;

		void OnDone() override;

		void OnCancel() override;

		void OnWriteDone(bool) override;

		void Enqueue(const Bundle &);

	private:
		vector<Bundle> Queue_;
		ReactorInterface *Interface_;

		void Terminate();
	};
}// namespace reactor
