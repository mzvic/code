//#include <utility>

//#include "client_reactor.h"

using namespace client_reactor;

template<class Outbound>
ClientUpstreamReactor<Outbound>::ClientUpstreamReactor(queue<Outbound> *queue, recursive_mutex *queue_mutex, const function<void(bool)> &done_callback) {
  queue_ = queue;

  queue_mutex_ = queue_mutex;

  done_callback_ = done_callback;

//  cout << "T:" << this_thread::get_id() << " ClientUpstream Reactor: Instance created" << endl;
  LOG("Upstream reactor created");
}

template<class Outbound>
void ClientUpstreamReactor<Outbound>::OnDone(const Status &status) {
//  cout << "T:" << this_thread::get_id() << " PublisherClientReactor: OnDone: " << status.ok() << endl;
  LOG("Upstream reactor OnDone: " << status.ok());

  if (done_callback_ != nullptr)
	done_callback_(status.ok());
}

template<class Outbound>
void ClientUpstreamReactor<Outbound>::OnWriteDone(bool ok) {
  unique_lock<recursive_mutex> qlck(*queue_mutex_);
  unique_lock<recursive_mutex> slck(state_mutex_);

//  cout << "T:" << this_thread::get_id() << " Publisher Reactor: OnWriteDone" << endl;
  LOG("Upstream reactor OnWriteDone: " << ok);

  if (ok) {
	// Erase front message, if any
	if (!queue_->empty())
	  queue_->pop();

	switch (state_) {
	  case RUNNING:
		// If there are more messages, send front now
		if (!queue_->empty())
		  this->StartWrite(&queue_->front());

		break;

	  case STOPPING:
		// If all messages were sent, stop now
		if (queue_->empty())
		  Stop();
		else
		  // If there are pending messages, send them now
		  this->StartWrite(&queue_->front());

		break;

	  case STOPPED:
		// Do nothing if we are stopped
		break;
	}
  } else {
//	cout << "T:" << this_thread::get_id() << " PublisherClientReactor: Writing failure" << endl;

	Stop();
  }
}

template<class Outbound>
void ClientUpstreamReactor<Outbound>::Start() {
  // No OnDone until RemoveHold()
  this->AddHold();

  // If there are message, start sending them now
  if (!queue_->empty())
	this->StartWrite(&queue_->front());

  // Start RPC operations
  this->StartCall();

//  cout << "T:" << this_thread::get_id() << " PublisherClientReactor: Started" << endl;
  LOG("Upstream reactor started");
}

template<class Outbound>
void ClientUpstreamReactor<Outbound>::Stop() {
  unique_lock<recursive_mutex> slck(state_mutex_);

  if (state_ != STOPPED) {
//	cout << "T:" << this_thread::get_id() << " PublisherClientReactor: Stopping" << endl;
	LOG("Upstream reactor stopping");

	// If we are connected, make a graceful shutdown
	if (channel_->GetState(false) == GRPC_CHANNEL_READY) {
	  this->StartWritesDone();

	  this->RemoveHold();
	} else {
	  this->RemoveHold();

	  context_.TryCancel();
	}

	state_ = STOPPED;
  }
}

template<class Outbound>
void ClientUpstreamReactor<Outbound>::BeginStop() {
  unique_lock<recursive_mutex> qlck(*queue_mutex_);
  unique_lock<recursive_mutex> slck(state_mutex_);

//  cout << "T:" << this_thread::get_id() << " PublisherClientReactor: Beginning to stop" << endl;
  LOG("Upstream reactor beginning to stop");

  if (state_ == RUNNING) {
	// If we are empty, stop now
	if (queue_->empty())
	  Stop();
	else
	  state_ = STOPPING;
  }
}

template<class Outbound>
ClientContext *ClientUpstreamReactor<Outbound>::GetContext() {
  return &context_;
}

template<class Outbound>
shared_ptr<Channel> &ClientUpstreamReactor<Outbound>::GetChannel() {
  return channel_;
}

//template<class Outbound>
//bool ClientUpstreamReactor<Outbound>::IsRunning() {
//  return state_ == RUNNING;
//}

template<class Inbound>
ClientDownstreamReactor<Inbound>::ClientDownstreamReactor(const function<void(Inbound &)> &enqueue_callback, const function<void(bool)> &done_callback) {
  enqueue_callback_ = enqueue_callback;

  done_callback_ = done_callback;

  /*ChannelArguments channel_arguments;

  client_downstream_ = client_downstream;

  channel_arguments.SetMaxReceiveMessageSize(-1);

  channel_ = CreateCustomChannel(SERVER_ADDRESS, InsecureChannelCredentials(), channel_arguments);*/

//  cout << "T:" << this_thread::get_id() << " ClientDownstreamReactor: Starting a new instance" << endl;
  LOG("Downstream reactor created");
}

template<class Inbound>
void ClientDownstreamReactor<Inbound>::OnDone(const Status &status) {
//  cout << "T:" << this_thread::get_id() << " ClientDownstreamReactor: OnDone: " << status.ok() << endl;
  LOG("Downstream reactor OnDone: " << status.ok());

  if (done_callback_ != nullptr)
	done_callback_(status.ok());
}

template<class Inbound>
void ClientDownstreamReactor<Inbound>::OnReadDone(bool ok) {
  unique_lock<recursive_mutex> slck(state_mutex_);

  LOG("Downstream reactor OnReadDone: " << ok);

  if (ok) {
	if (state_ == RUNNING) {
	  enqueue_callback_(inbound_);

	  this->StartRead(&inbound_);
	}
  } else {
//	cout << "T:" << this_thread::get_id() << " ClientDownstreamReactor: Reading failure" << endl;

	Stop();
  }
}

template<class Inbound>
void ClientDownstreamReactor<Inbound>::Start() {
  this->AddHold();

  // Start reading now
  this->StartRead(&inbound_);

  // Start RPC operations
  this->StartCall();

  LOG("Downstream reactor started");
}

template<class Inbound>
void ClientDownstreamReactor<Inbound>::Stop() {
  unique_lock<recursive_mutex> slck(state_mutex_);

  if (state_ != STOPPED) {
//	cout << "T:" << this_thread::get_id() << " ClientDownstreamReactor: Stopping" << endl;
	LOG("Downstream reactor stopping");

	this->RemoveHold();

	context_.TryCancel();

	state_ = STOPPED;
  }
}

template<class Inbound>
ClientContext *ClientDownstreamReactor<Inbound>::GetContext() {
  return &context_;
}

template<class Inbound>
shared_ptr<Channel> &ClientDownstreamReactor<Inbound>::GetChannel() {
  return channel_;
}

//template<class Inbound>
//bool ClientDownstreamReactor<Inbound>::IsRunning() {
//  return state_ == RUNNING;
//}

