#include "client.h"

PublisherClient::PublisherClientReactor::PublisherClientReactor(PublisherClient *publisher_client) {
  unique_ptr<Broker::Stub> stub;

  publisher_client_ = publisher_client;

  stub = Broker::NewStub(CreateChannel(SERVER_ADDRESS, InsecureChannelCredentials()));

  cout << "PublisherClientReactor: Starting a new instance" << endl;

  // Register reactor
  stub->async()->Publish(&context_, &empty_, this);

  // No OnDone until RemoveHold()
  AddHold();

  // Start RPC operations, now or later
  StartCall();
}

void PublisherClient::PublisherClientReactor::OnDone(const Status &s) {
  cout << "PublisherClientReactor: OnDone" << endl;

  publisher_client_->OnPublisherOnDone();
}

void PublisherClient::PublisherClientReactor::OnWriteDone(bool ok) {
  unique_lock<mutex> qlck(publisher_client_->queue_mutex_);
  if (ok) {
	//	cout << "Publisher Reactor: A value has been written" << endl;

	// Erase front message
	publisher_client_->queue_.pop();

	//	cout << "Message: Queue size: " << publisher_client_->queue_.size() << endl;

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
	cout << "PublisherClientReactor: Writing failure" << endl;

	StopNow();
  }
}

void PublisherClient::PublisherClientReactor::StopNow() {
  unique_lock<mutex> rlck(running_mutex_);

  if (running_) {
	cout << "PublisherClientReactor: Stopping down" << endl;

	StartWritesDone();

	RemoveHold();

	running_ = false;
  }
}

void PublisherClient::PublisherClientReactor::Stop() {
  cout << "PublisherClientReactor: Stopping" << endl;

  // If we are empty, stop now
  if (publisher_client_->queue_.empty())
	StopNow();

  stopping_ = true;
}

PublisherClient::PublisherClient() {
  cout << "PublisherClient: Constructing instance" << endl;

  // Create new reactor
  publisher_client_reactor_ = new PublisherClientReactor(this);
}

PublisherClient::~PublisherClient() {
  cout << "PublisherClient: Destroying instance" << endl;

  stopping_ = true;

  publisher_client_reactor_->Stop();

  Wait();
}

void PublisherClient::Publish(Bundle &bundle) {
  Timestamp timestamp;
  struct timeval tv{};
  gettimeofday(&tv, nullptr);

  timestamp.set_seconds((int64_t) tv.tv_sec);
  timestamp.set_nanos((int32_t) tv.tv_usec * 1000);

  Publish(bundle, timestamp);
}

void PublisherClient::Publish(Bundle &bundle, const Timestamp &timestamp) {
  unique_lock<mutex> qlck(queue_mutex_);

//  Timestamp *timestamp;

  if (stopping_) {
	cout << "PublisherClient: Trying to publish on a stopping_ client, no message has been published" << endl;

	return;
  }

//  cout << "Publishing new message" << endl;

//  struct timeval timestamp{};
//  gettimeofday(&timestamp, nullptr);

//  timestamp = bundle.mutable_timestamp();

//  timestamp->set_seconds((int64_t) timestamp.tv_sec);
//  timestamp->set_nanos((int32_t) timestamp.tv_usec * 1000);

  bundle.mutable_timestamp()->CopyFrom(timestamp);

  queue_.push(bundle);

//  cout << "Message: Queue size: " << queue_.size() << endl;

  // If this is the first element in queue, we should send it now
  if (queue_.size() == 1)
	publisher_client_reactor_->StartWrite(&queue_.front());
}

void PublisherClient::Wait() {
  unique_lock<mutex> rlck(running_mutex_);

  cout << "PublisherClient: Waiting for Done" << endl;

  running_cv_.wait(rlck, [this] { return !running_; });

  cout << "PublisherClient: Done" << endl;
}

void PublisherClient::OnPublisherOnDone() {
  unique_lock<mutex> qlck(queue_mutex_);
  unique_lock<mutex> rlck(running_mutex_);

  cout << "PublisherClient: OnSubscriberOnDone" << endl;

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
