#include "client.h"

PublisherClient::PublisherClientReactor::PublisherClientReactor(PublisherClient *publisher_client) {
  unique_ptr<Broker::Stub> stub;

  publisher_client_ = publisher_client;

  stub = Broker::NewStub(CreateChannel(SERVER_ADDRESS, InsecureChannelCredentials()));

  cout << "Publisher Reactor: Starting a new instance" << endl;

  // Register reactor
  stub->async()->Publish(&context_, &empty_, this);

  // No OnDone until RemoveHold()
  AddHold();

  // Start RPC operations, now or later
  StartCall();
}

void PublisherClient::PublisherClientReactor::OnDone(const Status &s) {
  cout << "Publisher Reactor: OnDone" << endl;

  publisher_client_->OnPublisherOnDone();
}

void PublisherClient::PublisherClientReactor::OnWriteDone(bool ok) {
  unique_lock<mutex> qlck(publisher_client_->queue_mutex_);

  if (ok) {
	cout << "Publisher Reactor: A value has been written" << endl;

	// Erase front message
	publisher_client_->queue_.pop();

	cout << "Message: Queue size: " << publisher_client_->queue_.size() << endl;

	// Do something if we are still running
	if (running_) {
	  if (publisher_client_->queue_.empty()) {
		// If they are waiting for me, stop now
		if (stopping_)
		  StopNow();
	  } else {
		// If there are more messages, send front now
		StartWrite(&publisher_client_->queue_.front());
	  }
	}
  } else {
	cout << "Publisher Reactor: Writing failure" << endl;

	StopNow();
  }
}

void PublisherClient::PublisherClientReactor::StopNow() {
  unique_lock<mutex> rlck(running_mutex_);

  if (running_) {
	cout << "Publisher Reactor: Shutting down" << endl;

	StartWritesDone();

	RemoveHold();

	running_ = false;
  }
}

void PublisherClient::PublisherClientReactor::Stop() {
  // If we are empty, stop now
  if (publisher_client_->queue_.empty())
	StopNow();

  stopping_ = true;
}

PublisherClient::PublisherClient() {
  cout << "Constructing PublisherClient instance" << endl;

  // Create new reactor
  publisher_client_reactor_ = new PublisherClientReactor(this);
}

PublisherClient::~PublisherClient() {
  cout << "Destroying PublisherClient instance" << endl;

  stopping_ = true;

  publisher_client_reactor_->Stop();

  Wait();
}

void PublisherClient::Publish(Bundle &bundle) {
  unique_lock<mutex> qlck(queue_mutex_);

  Timestamp *timestamp;

  if (stopping_) {
	cout << "Trying to Publish on a stopping_ client, no message has been published" << endl;

	return;
  }

  cout << "Publishing new message" << endl;

  struct timeval tv{};
  gettimeofday(&tv, nullptr);

  timestamp = bundle.mutable_timestamp();

  timestamp->set_seconds((int64_t) tv.tv_sec);
  timestamp->set_nanos((int32_t) tv.tv_usec * 1000);

  queue_.push(bundle);

  cout << "Message: Queue size: " << queue_.size() << endl;

  // If this is the first element in queue, we should send it now
  if (queue_.size() == 1)
	publisher_client_reactor_->StartWrite(&queue_.front());
}

void PublisherClient::Wait() {
  unique_lock<mutex> rlck(running_mutex_);

  cout << "Publisher Reactor: Waiting for Done" << endl;

  running_cv_.wait(rlck, [this] { return !running_; });

  cout << "Publisher Reactor: Done" << endl;
}

void PublisherClient::OnPublisherOnDone() {
  unique_lock<mutex> rlck(running_mutex_);

  cout << "OnSubscriberOnDone" << endl;

  if (stopping_ && queue_.empty()) {
	running_ = false;

	running_cv_.notify_one();
  } else {
	// Create new reactor
	publisher_client_reactor_ = new PublisherClientReactor(this);

	// If there are elements in queue, start sending them now
	if (!queue_.empty())
	  publisher_client_reactor_->StartWrite(&queue_.front());

	// Stop it if we are terminating
	if (stopping_)
	  publisher_client_reactor_->Stop();
  }
}