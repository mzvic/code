#include "reactor.h"

using namespace reactor;
using grpc::ServerWriteReactor;
using grpc::Status;

SubscriberReactor::SubscriberReactor(ReactorInterface *pInterface) {
	std::cout << "Subscriber Reactor: Starting a new instance" << std::endl;

	Interface_ = pInterface;
}

SubscriberReactor::~SubscriberReactor() {
	Interface_->OnSubscriberReactorFinish(this);
}

void SubscriberReactor::OnDone() {
	//	Terminate();

	std::cout << "Subscriber Reactor: Done" << std::endl;

	Interface_->OnSubscriberReactorFinish(this);
}

void SubscriberReactor::OnCancel() {
	std::cout << "Subscriber Reactor: Cancelling" << std::endl;

	Finish(grpc::Status::CANCELLED);
	//	Terminate();
}

void SubscriberReactor::OnWriteDone(bool ok) {
	if (ok) {
		std::cout << "Subscriber Reactor: A value has been written" << std::endl;

		// Erase first message
		Queue_.erase(Queue_.begin());

		// If there is more, send first now
		if (!Queue_.empty())
			StartWrite(&Queue_.front());
	} else {
		std::cout << "Subscriber Reactor: Writing done" << std::endl;

		Finish(Status::OK);
	}
}

void SubscriberReactor::Enqueue(const Bundle &Bundle) {
	Queue_.emplace_back(Bundle);

	// If this is the first element, we should send it now
	if (Queue_.size() == 1)
		StartWrite(&Queue_.front());
}

void SubscriberReactor::Terminate() {
	std::cout << "Subscriber Reactor: Terminating" << std::endl;

	Interface_->OnSubscriberReactorFinish(this);

	delete this;
}
