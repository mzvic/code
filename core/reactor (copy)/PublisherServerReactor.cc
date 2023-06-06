#include "reactor.h"

using namespace reactor;
using core::Broker;
using grpc::ServerReadReactor;
using grpc::Status;

PublisherServerReactor::PublisherServerReactor(ServerReactorInterface *pInterface) {
	//	std::cout << "Publisher Reactor: Starting a new instance" << std::endl;

	Interface_ = pInterface;

	// Block until next read op
	StartRead(&Request_);

	Finished_ = false;
}

void PublisherServerReactor::OnDone() {
	//	ShutdownNow();

	//	std::cout << "Publisher Reactor: Done" << std::endl;

	Interface_->OnPublisherServerReactorFinish(this);
}

void PublisherServerReactor::OnCancel() {
	//	std::cout << "Publisher Reactor: Cancelling" << std::endl;

	if (!Finished_) {
		Finished_ = true;

		Finish(Status::CANCELLED);
	}
	//	ShutdownNow();
}

void PublisherServerReactor::OnReadDone(bool ok) {
	if (ok) {
		//		std::cout << "Publisher Reactor: Reading a value" << std::endl;

		// Fresh value is inside Request_
		Interface_->ProcessMessage(Request_);

		// Block until next read op
		StartRead(&Request_);
	} else {
		//		std::cout << "Publisher Reactor: Reading done" << std::endl;

		if (!Finished_) {
			Finished_ = true;

			Finish(Status::OK);
		}
	}
}

//void PublisherServerReactor::ShutdownNow() {
//	std::cout << "Publisher Reactor: Terminating" << std::endl;
//
//	Interface_->OnPublisherServerReactorFinish(this);
//
//	delete this;
//}