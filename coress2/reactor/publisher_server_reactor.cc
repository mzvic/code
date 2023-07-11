#include "reactor.h"

using namespace reactor;

PublisherServerReactor::PublisherServerReactor(ServerReactorInterface *interface) {
  //	std::cout << "Publisher Reactor: Starting a new instance" << std::endl;

  interface_ = interface;

  // Block until next read op
  StartRead(&request_);

  done_ = false;

  finished_ = false;
}

void PublisherServerReactor::OnDone() {
  unique_lock<mutex> lck(mutex_);

  if (!done_) {
	done_ = true;

//	std::cout << "Publisher Reactor: OnDone" << std::endl;

	interface_->OnPublisherServerReactorFinish(this);
  }
}

void PublisherServerReactor::OnCancel() {
  unique_lock<mutex> lck(mutex_);

  //	std::cout << "Publisher Reactor: OnCancel" << std::endl;

  if (!finished_) {
	finished_ = true;

	Finish(Status::CANCELLED);
  }
}

void PublisherServerReactor::OnReadDone(bool ok) {
  unique_lock<mutex> lck(mutex_);

  if (ok) {
	//		std::cout << "Publisher Reactor: Reading a value" << std::endl;

	// Fresh value is inside request_
	interface_->ProcessMessage(request_);

	// Block until next read op if we are not finished yet
	if (!finished_)
	  StartRead(&request_);
  } else {
	//		std::cout << "Publisher Reactor: Reading done" << std::endl;

	if (!finished_) {
	  finished_ = true;

	  Finish(Status::OK);
	}
  }
}
