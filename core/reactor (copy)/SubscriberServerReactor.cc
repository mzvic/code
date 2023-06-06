#include "reactor.h"

using namespace reactor;
using grpc::ServerWriteReactor;
using grpc::Status;

SubscriberServerReactor::SubscriberServerReactor(ServerReactorInterface *pInterface) {
	std::cout << "Subscriber Reactor: Starting a new instance" << std::endl;

	Interface_ = pInterface;

	Finished_ = false;
}

//SubscriberServerReactor::~SubscriberServerReactor() {
//	//	Interface_->OnSubscriberServerReactorFinish(this);
//}

void SubscriberServerReactor::OnDone() {
	//	ShutdownNow();

	std::cout << "Subscriber Reactor: Done" << std::endl;

	Interface_->OnSubscriberServerReactorFinish(this);
}

void SubscriberServerReactor::OnCancel() {
	std::cout << "Subscriber Reactor: Cancelling" << std::endl;
	//
	if (!Finished_) {
		Finished_ = true;

		Finish(Status::CANCELLED);
	}
	//	//	ShutdownNow();
}

void SubscriberServerReactor::OnWriteDone(bool ok) {
	std::unique_lock<std::mutex> lck(Mutex_);

	if (ok) {
		//		std::cout << "Subscriber Reactor: A value has been written" << std::endl;

		//		std::cout << "Queue size: " << Queue_.size() << std::endl;

		// Erase first message
		//		Queue_.erase(Queue_.begin());
		Queue_.pop();

		// If there is more, send first now
		if (!Queue_.empty())
			StartWrite(&Queue_.front());
	} else {
		std::cout << "Subscriber Reactor: Writing done" << std::endl;

		if (!Finished_) {
			Finished_ = true;

			Finish(Status::OK);
		}
	}
}

void SubscriberServerReactor::EnqueueMessage(const Bundle &bundle) {
	std::unique_lock<std::mutex> lck(Mutex_);

	//	std::cout << "Subscriber Reactor: Enqueueing new message" << std::endl;

	//	Queue_.emplace_back(bundle);
	Queue_.push(bundle);

	//	std::cout << "Queue size: " << Queue_.size() << std::endl;

	// If this is the first element, we should send it now
	if (Queue_.size() == 1)
		StartWrite(&Queue_.front());
}

//void SubscriberServerReactor::ShutdownNow() {
//	std::cout << "Subscriber Reactor: Terminating" << std::endl;
//
//	Interface_->OnSubscriberServerReactorFinish(this);
//
//	delete this;
//}
