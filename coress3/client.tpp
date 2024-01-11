//#include "client.h"

using namespace client;

template<class Outbound, class Response>
ClientUpstream<Outbound, Response>::ClientUpstream(bool persistent) {
  persistent_ = persistent;

  LOG("Upstream client created");
}

template<class Outbound, class Response>
ClientUpstream<Outbound, Response>::~ClientUpstream() {
//
//
//  Stop();

//  cout << "T:" << this_thread::get_id() << " PublisherClient: Instance destroyed" << endl;
  LOG("Upstream client destroyed");
}

template<class Outbound, class Response>
void ClientUpstream<Outbound, Response>::SetDoneCallback(const function<void(bool)> &done_callback) {
  done_callback_ = done_callback;
}

template<class Outbound, class Response>
void ClientUpstream<Outbound, Response>::EnqueueOutboundMessage(const Outbound &outbound) {
  unique_lock<recursive_mutex> qlck(queue_mutex_);
  unique_lock<mutex> slck(state_mutex_);

  if (state_ != RUNNING) {
	LOG("Upstream client is not running, ignoring message");

	return;
  }

  queue_.push(outbound);

  // If this is the first element in inbound_queue, we should send it now
  if (queue_.size() == 1)
	client_upstream_reactor_->StartWrite(&queue_.front());
}

//template<class Outbound, class Response>
//void ClientUpstream<Outbound, Response>::Flush() {
//  unique_lock<recursive_mutex> qlck(queue_mutex_);
//
//  while (!queue_.empty())
//	queue_.pop();
//
////  cout << "T:" << this_thread::get_id() << " PublisherClient: Queue has been flushed" << endl;
//  LOG("Upstream client queue has been flushed");
//}

//template<class Outbound, class Response>
//void ClientUpstream<Outbound, Response>::Terminate() {
//  Stop();
//}

template<class Outbound, class Response>
const Response &ClientUpstream<Outbound, Response>::GetResponse() {
  return response_;
}

template<class Outbound, class Response>
void ClientUpstream<Outbound, Response>::Start() {
  // Create new reactor. It will delete itself on completion
  client_upstream_reactor_ = new ClientUpstreamReactor<Outbound>(&queue_, &queue_mutex_, [this](bool ok) { OnClientUpstreamReactorOnDone(ok); });

  Initialize(client_upstream_reactor_);

  client_upstream_reactor_->Start();
}

template<class Outbound, class Response>
void ClientUpstream<Outbound, Response>::Stop() {
  unique_lock<mutex> slck(state_mutex_);

  if (state_ == RUNNING) {
//	cout << "T:" << this_thread::get_id() << " PublisherClient: Destroying instance" << endl;

	state_ = STOPPING;

	client_upstream_reactor_->BeginStop();

//	cout << "T:" << this_thread::get_id() << " PublisherClient: Waiting for reactors" << endl;
	LOG("Upstream client is stopping and waiting for reactors");

	// Now wait for signal
	wait_cv_.wait(slck, [this] { return (state_ == STOPPED); });
  }
}

//template<class Outbound, class Response>
//void ClientUpstream<Outbound, Response>::EnqueueOutboundMessage(Outbound &outbound) {
//  Timestamp timestamp;
//  struct timeval tv{};
//  gettimeofday(&tv, nullptr);
//
//  timestamp.set_seconds((int64_t) tv.tv_sec);
//  timestamp.set_nanos((int32_t) tv.tv_usec * 1000);
//
//  EnqueueOutboundMessage(outbound, timestamp);
//}

//template<class Outbound, class Response>
//void ClientUpstream<Outbound, Response>::EnqueueOutboundMessage(Outbound &outbound, const Timestamp &timestamp) {
//  unique_lock<recursive_mutex> qlck(queue_mutex_);
//  unique_lock<mutex> slck(state_mutex_);
//
//  if (state_ != RUNNING) {
//	LOG("Upstream client is not running, message is not processed");
//
//	return;
//  }
//
//  outbound.mutable_timestamp()->CopyFrom(timestamp);
//
//  queue_.push(outbound);
//
//  // If this is the first element in inbound_queue, we should send it now
//  if (queue_.size() == 1)
//	client_upstream_reactor_->StartWrite(&queue_.front());
//}

template<class Outbound, class Response>
void ClientUpstream<Outbound, Response>::OnClientUpstreamReactorOnDone(bool ok) {
  unique_lock<recursive_mutex> qlck(queue_mutex_);
  unique_lock<mutex> slck(state_mutex_);

  LOG("Upstream client OnClientUpstreamReactorOnDone: " << ok);

  // Notify on non-persistent mode only
  if (!persistent_ && (done_callback_ != nullptr))
	done_callback_(ok);

  switch (state_) {
	case RUNNING:
	  // Restart on persistent mode only
	  if (persistent_)
		Start();
	  else
		state_ = STOPPED;

	  break;

	case STOPPING:
	  state_ = STOPPED;

	  wait_cv_.notify_one();

	  break;

	case STOPPED:
	  // Do nothing
	  break;
  }
}

//template<class Outbound, class Response>
//void ClientUpstream<Outbound, Response>::OnClientUpstreamReactorOnDone(bool ok) {
//  unique_lock<recursive_mutex> qlck(queue_mutex_);
//  unique_lock<mutex> slck(state_mutex_);
//
////  cout << "T:" << this_thread::get_id() << " PublisherClient: OnClientUpstreamReactorOnDone: " << ok << endl;
//  LOG("Upstream client OnClientUpstreamReactorOnDone: " << ok);
//
//  if (persistent_) {
//	// On persistent mode, act according to current state
//	switch (state_) {
//	  case RUNNING:
//		Start();
//
////		// If there are elements in inbound_queue, start sending them now
////		if (!queue_.empty())
////		  client_upstream_reactor_->StartWrite(&queue_.front());
//
////		cout << "T:" << this_thread::get_id() << " PublisherClient: New instance constructed" << endl;
//
//		break;
//
//	  case STOPPING:
//		state_ = STOPPED;
//
//		wait_cv_.notify_one();
////		if (queue_.empty()) {
////		  state_ = STOPPED;
////
////		  wait_cv_.notify_one();
////		} else {
////		  Start();
////
//////		  // Since there are elements in inbound_queue, start sending them now
//////		  client_upstream_reactor_->StartWrite(&queue_.front());
////
////		  // Signal this reactor we are terminating
////		  client_upstream_reactor_->BeginStop();
////		}
//
//		break;
//
//	  case STOPPED:
//		// Do nothing
//		break;
//	}
//  } else {
//	// On non-persistent mode, we will stop immediately. There are no running reactors at this point
//	if (done_callback_ != nullptr)
//	  done_callback_(ok);
//
//	switch (state_) {
//	  case RUNNING:
//		state_ = STOPPED;
//
//		break;
//
//	  case STOPPING:
//		state_ = STOPPED;
//
//		wait_cv_.notify_one();
//
//		break;
//
//	  case STOPPED:
//		// Do nothing
//		break;
//	}
//
////	// Check if there is a stop paused operation and notify
////	if (state_ == STOPPING) {
////	  state_ = STOPPED;
////
////	  wait_cv_.notify_one();
////	} else {
////	  state_ = STOPPED;
////	}
//  }
//}

template<class Inbound, class Request>
ClientDownstream<Inbound, Request>::ClientDownstream(bool persistent, const function<void(const Inbound &)> &inbound_callback) {
//  cout << "T:" << this_thread::get_id() << " ClientDownstream: Constructing instance" << endl;

//  assert(inbound_callback != nullptr);

  persistent_ = persistent;

  inbound_callback_ = inbound_callback;

  queue_thread_ = thread(&ClientDownstream::QueueProcessing, this);

  LOG("Downstream client created");
}

template<class Inbound, class Request>
ClientDownstream<Inbound, Request>::~ClientDownstream() {
//  Stop();

//  cout << "T:" << this_thread::get_id() << " SubscriberClient: Instance destroyed" << endl;

  // Join queue thread
  queue_thread_.join();

  LOG("Downstream client destroyed");
}

template<class Inbound, class Request>
void ClientDownstream<Inbound, Request>::SetDoneCallback(const function<void(bool)> &done_callback) {
  done_callback_ = done_callback;
}

template<class Inbound, class Request>
void ClientDownstream<Inbound, Request>::Start() {
  // Create new reactor. It will delete itself on completion
  client_downstream_reactor_ = new ClientDownstreamReactor<Inbound>([this](const Inbound &inbound) { EnqueueInboundMessage(inbound); }, [this](bool ok) { OnClientDownstreamReactorOnDone(ok); });

  Initialize(client_downstream_reactor_);

  client_downstream_reactor_->Start();
}

template<class Inbound, class Request>
void ClientDownstream<Inbound, Request>::Stop() {
  unique_lock<mutex> slck(state_mutex_);

  if (state_ == RUNNING) {
//	cout << "T:" << this_thread::get_id() << " SubscriberClient: Destroying instance" << endl;

	state_ = STOPPING;

	// Notify queue thread that we are not RUNNING anymore
	queue_cv_.notify_one();

	client_downstream_reactor_->Stop();

//	cout << "T:" << this_thread::get_id() << " SubscriberClient: Waiting for reactors" << endl;
	LOG("Downstream client is stopping and waiting for reactors");

	wait_cv_.wait(slck, [this] {
	  return (state_ == STOPPED);
	});

//	// Join queue thread
//	queue_thread_.join();
  }
}

template<class Inbound, class Request>
void ClientDownstream<Inbound, Request>::OnClientDownstreamReactorOnDone(bool ok) {
  unique_lock<mutex> slck(state_mutex_);

  LOG("Downstream client OnClientDownstreamReactorOnDone: " << ok);

  // Notify on non-persistent mode only
  if (!persistent_ && (done_callback_ != nullptr))
	done_callback_(ok);

  switch (state_) {
	case RUNNING:
	  // Restart on persistent mode only
	  if (persistent_) {
		Start();
	  } else {
		state_ = STOPPED;

		// Notify queue thread that we are not RUNNING anymore
		queue_cv_.notify_one();
	  }

	  break;

	case STOPPING:
	  state_ = STOPPED;

	  wait_cv_.notify_one();

	  break;

	case STOPPED:
	  // Do nothing
	  break;
  }
}

//template<class Inbound, class Request>
//void ClientDownstream<Inbound, Request>::OnClientDownstreamReactorOnDone(bool ok) {
//  unique_lock<mutex> slck(state_mutex_);
//
////  cout << "T:" << this_thread::get_id() << " SubscriberClient: OnClientDownstreamReactorOnDone: " << ok << endl;
//  LOG("Downstream client OnClientDownstreamReactorOnDone: " << ok);
//
//  if (persistent_) {
//	// On persistent mode, act according to current state
//	switch (state_) {
//	  case RUNNING:
//		// Start new reactor
//		Start();
//
//		break;
//
//	  case STOPPING:
//		state_ = STOPPED;
//
//		wait_cv_.notify_one();
//
//		break;
//
//	  case STOPPED:
//		// Do nothing
//		break;
//	}
//  } else {
//	// On non-persistent mode, stop immediately. There are no running reactors at this point
//	if (done_callback_ != nullptr)
//	  done_callback_(ok);
//
//	switch (state_) {
//	  case RUNNING:
//		state_ = STOPPED;
//
//		// Notify queue thread that we are not RUNNING anymore
//		queue_cv_.notify_one();
//
//		break;
//
//	  case STOPPING:
//		state_ = STOPPED;
//
//		wait_cv_.notify_one();
//
//		break;
//
//	  case STOPPED:
//		// Do nothing
//		break;
//	}
//
////	// Check if there is a stop paused operation and notify
////	if (state_ == STOPPING) {
////	  state_ = STOPPED;
////
////	  wait_cv_.notify_one();
////	} else {
////	  state_ = STOPPED;
////
////	  // Notify queue thread that we are not RUNNING anymore
////	  queue_cv_.notify_one();
////
//////	  // Join queue thread
//////	  queue_thread_.join();
////	}
//  }
//}

template<class Inbound, class Request>
void ClientDownstream<Inbound, Request>::EnqueueInboundMessage(const Inbound &inbound) {
  unique_lock<mutex> qlck(queue_mutex_);

  queue_.push(inbound);

  // Notify reading thread of this new element
  if (queue_.size() == 1)
	queue_cv_.notify_one();
}

template<class Inbound, class Request>
void ClientDownstream<Inbound, Request>::QueueProcessing() {
  while (state_ == RUNNING) {
	{
	  unique_lock<mutex> qlck(queue_mutex_);

	  queue_cv_.wait(qlck, [this] {
		return ((state_ != RUNNING) || !queue_.empty());
	  });
	}

	if ((state_ == RUNNING) && !queue_.empty() && (inbound_callback_ != nullptr))
	  inbound_callback_(queue_.front());

	if ((state_ == RUNNING) && !queue_.empty()) {
	  unique_lock<mutex> qlck(queue_mutex_);

	  queue_.pop();
	}
  }
}

