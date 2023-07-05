#include "reactor.h"

using namespace reactor;

PublisherServerReactor::PublisherServerReactor(ServerReactorInterface *interface) {
  //	std::cout << "Publisher Reactor: Starting a new instance" << std::endl;

  interface_ = interface;

  // Block until next read op
  StartRead(&request_);

  finished_ = false;
}

void PublisherServerReactor::OnDone() {
  //	Now();

  //	std::cout << "Publisher Reactor: Done" << std::endl;

  interface_->OnPublisherServerReactorFinish(this);
}

void PublisherServerReactor::OnCancel() {
  unique_lock<std::mutex> lck(mutex_);
  //	std::cout << "Publisher Reactor: Cancelling" << std::endl;

  if (!finished_) {
	finished_ = true;

	Finish(Status::CANCELLED);
  }
  //	Now();
}

void PublisherServerReactor::OnReadDone(bool ok) {
  unique_lock<std::mutex> lck(mutex_);
  
  if (ok) {
	//		std::cout << "Publisher Reactor: Reading a value" << std::endl;

	// Fresh value is inside request_
	interface_->ProcessMessage(request_);

	// Block until next read op
	StartRead(&request_);
  } else {
	//		std::cout << "Publisher Reactor: Reading done" << std::endl;

	if (!finished_) {
	  finished_ = true;

	  Finish(Status::OK);
	}
  }
}

//void PublisherServerReactor::Now() {
//	std::cout << "Publisher Reactor: Terminating" << std::endl;
//
//	interface_->OnPublisherServerReactorFinish(this);
//
//	delete this;
//}