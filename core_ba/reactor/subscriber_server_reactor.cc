#include "reactor.h"

using namespace reactor;

SubscriberServerReactor::SubscriberServerReactor(ServerReactorInterface *interface, const Interests *interests) {
//  std::cout << "Subscriber Reactor: Starting a new instance" << std::endl;

  interface_ = interface;

  interests_.assign(interests->types().begin(), interests->types().end());

  done_ = false;

  finished_ = false;
}

//SubscriberServerReactor::~SubscriberServerReactor() {
//	//	interface_->OnSubscriberServerReactorFinish(this);
//}

void SubscriberServerReactor::OnDone() {
  unique_lock<mutex> lck(mutex_);

  if (!done_) {
	done_ = true;

//	std::cout << "Subscriber Reactor: OnDone" << std::endl;

	interface_->OnSubscriberServerReactorFinish(this);
  }
}

void SubscriberServerReactor::OnCancel() {
  unique_lock<mutex> lck(mutex_);

//  std::cout << "Subscriber Reactor: OnCancel" << std::endl;

  if (!finished_) {
	finished_ = true;

	Finish(Status::CANCELLED);
  }
}

void SubscriberServerReactor::OnWriteDone(bool ok) {
  unique_lock<mutex> lck(mutex_);

  if (ok) {
	//		std::cout << "Subscriber Reactor: A value has been written" << std::endl;

	//		std::cout << "Queue size: " << queue_.size() << std::endl;

	// Erase first message
	//		queue_.erase(queue_.begin());
	queue_.pop();

	// If we are not finished and there is more, send first now
	if (!finished_ && !queue_.empty())
	  StartWrite(&queue_.front());
  } else {
//	std::cout << "Subscriber Reactor: Writing done" << std::endl;

	if (!finished_) {
	  finished_ = true;

	  Finish(Status::OK);
	}
  }
}

void SubscriberServerReactor::EnqueueMessage(const Bundle &bundle) {
  unique_lock<mutex> lck(mutex_);

  // If there is no particular interest, or explicit ones, enqueue incoming message
  if (interests_.empty() || (find(interests_.begin(), interests_.end(), bundle.type()) != interests_.end())) {

	//	std::cout << "Subscriber Reactor: Enqueueing new message" << std::endl;

	//	queue_.emplace_back(bundle);
	queue_.push(bundle);

	//	std::cout << "Queue size: " << queue_.size() << std::endl;

	// If we are not finished and this is the first element, we should send it now
	if (!finished_ && queue_.size() == 1)
	  StartWrite(&queue_.front());
  }
}

//void SubscriberServerReactor::Now() {
//	std::cout << "Subscriber Reactor: Terminating" << std::endl;
//
//	interface_->OnSubscriberServerReactorFinish(this);
//
//	delete this;
//}
