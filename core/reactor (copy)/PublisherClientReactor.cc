#include "reactor.h"

using namespace reactor;

using core::Broker;
using google::protobuf::Empty;
using grpc::ClientContext;
using grpc::Status;

//PublisherClientReactor::PublisherClientReactor(ServerReactorInterface *pInterface) {
//PublisherClientReactor::PublisherClientReactor(Broker::Stub *stub, std::Mutex_ mu) {
PublisherClientReactor::PublisherClientReactor(Broker::Stub *stub) {
	ClientContext context;
	Empty empty;

	std::cout << "Publisher Reactor: Starting a new instance" << std::endl;

	// Interface_ = pInterface;

	//	Mutex_ = mu;

	// Register reactor
	stub->async()->Publish(&context, &empty, this);
	//
	AddHold();
	//
	//	StartCall();
}

//SubscriberServerReactor::~SubscriberServerReactor() {
//	//	Interface_->OnSubscriberServerReactorFinish(this);
//}

void PublisherClientReactor::OnDone(const Status &s) {
	//	Terminate();

	std::cout << "Publisher Reactor: Done" << std::endl;

	//	Interface_->OnSubscriberServerReactorFinish(this);
}

void PublisherClientReactor::OnWriteDone(bool ok) {
	if (ok) {
		std::cout << "Publisher Reactor: A value has been written" << std::endl;

		// Erase first message
		Queue_.erase(Queue_.begin());

		// If there is more, send first now
		if (!Queue_.empty())
			StartWrite(&Queue_.front());
	} else {
		std::cout << "Publisher Reactor: Writing done" << std::endl;

		StartWritesDone();

		//		RemoveHold();
	}
}

void PublisherClientReactor::Enqueue(const Bundle &Bundle) {
	std::cout << "Publisher Reactor: Enqueueing new message" << std::endl;

	Queue_.emplace_back(Bundle);

	// If this is the first element, we should send it now
	if (Queue_.size() == 1)
		StartWrite(&Queue_.front());
}

//void SubscriberServerReactor::Terminate() {
//	std::cout << "Subscriber Reactor: Terminating" << std::endl;
//
//	Interface_->OnSubscriberServerReactorFinish(this);
//
//	delete this;
//}
