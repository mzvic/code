#include "reactor.h"

using namespace reactor;
using core::Broker;
using grpc::ServerReadReactor;
using grpc::Status;

PublisherReactor::PublisherReactor(ReactorInterface *pInterface) {
	std::cout << "Publisher Reactor: Starting a new instance" << std::endl;

	Interface_ = pInterface;

	// Block until next read op
	StartRead(&Request_);
}

void PublisherReactor::OnDone() {
	//	Terminate();

	std::cout << "Publisher Reactor: Done" << std::endl;

	Interface_->OnPublisherReactorFinish(this);
}

void PublisherReactor::OnCancel() {
	std::cout << "Publisher Reactor: Cancelling" << std::endl;

	//	Terminate();
}

void PublisherReactor::OnReadDone(bool ok) {
	if (ok) {
		std::cout << "Publisher Reactor: Reading a value" << std::endl;

		// Fresh value is inside Request_
		Interface_->ProcessMessage(Request_);

		// Block until next read op
		StartRead(&Request_);
	} else {
		std::cout << "Publisher Reactor: Reading done" << std::endl;

		Finish(Status::OK);
	}
}

void PublisherReactor::Terminate() {
	std::cout << "Publisher Reactor: Terminating" << std::endl;

	Interface_->OnPublisherReactorFinish(this);

	delete this;
}