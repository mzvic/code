using namespace server_reactor;

template<class Inbound, class Response>
ServerUpstreamReactor<Inbound, Response>::ServerUpstreamReactor(Response *response) {
  response_ = response;

  // Block until next read op
  this->StartRead(&inbound_);

  done_ = false;

  finished_ = false;
}

template<class Inbound, class Response>
void ServerUpstreamReactor<Inbound, Response>::OnDone() {
  unique_lock<mutex> lck(mutex_);

  if (!done_) {
	done_ = true;

	if (done_callback_ != nullptr)
	  done_callback_(this);
  }
}

template<class Inbound, class Response>
void ServerUpstreamReactor<Inbound, Response>::OnCancel() {
  unique_lock<mutex> lck(mutex_);

  if (!finished_) {
	finished_ = true;

	this->Finish(Status::CANCELLED);
  }
}

template<class Inbound, class Response>
void ServerUpstreamReactor<Inbound, Response>::OnReadDone(bool ok) {
  unique_lock<mutex> lck(mutex_);

  if (ok) {
	// Fresh value is inside inbound_
	if (inbound_callback_ != nullptr)
	  inbound_callback_(inbound_);

	// Block until next read op if we are not finished yet
	if (!finished_)
	  this->StartRead(&inbound_);
  } else {
	if (!finished_) {
	  finished_ = true;

	  if (ready_callback_ != nullptr)
		ready_callback_(this);

	  this->Finish(Status::OK);
	}
  }
}

template<class Inbound, class Response>
Response *ServerUpstreamReactor<Inbound, Response>::GetResponse() {
  return response_;
}

template<class Inbound, class Response>
void ServerUpstreamReactor<Inbound, Response>::SetInboundCallback(const function<void(const Inbound &)> &inbound_callback) {
  inbound_callback_ = inbound_callback;
}

template<class Inbound, class Response>
void ServerUpstreamReactor<Inbound, Response>::SetReadyCallback(const function<void(void *)> &ready_callback) {
  ready_callback_ = ready_callback;
}

template<class Inbound, class Response>
void ServerUpstreamReactor<Inbound, Response>::SetDoneCallback(const function<void(void *)> &done_callback) {
  done_callback_ = done_callback;
}

template<class Outbound, class Request>
ServerDownstreamReactor<Outbound, Request>::ServerDownstreamReactor(const Request *request) {
  request_ = request;

  done_ = false;

  finished_ = false;
}

template<class Outbound, class Request>
void ServerDownstreamReactor<Outbound, Request>::OnDone() {
  unique_lock<mutex> lck(mutex_);

  if (!done_) {
	done_ = true;

	if (done_callback_ != nullptr)
	  done_callback_(this);
  }
}

template<class Outbound, class Request>
void ServerDownstreamReactor<Outbound, Request>::OnCancel() {
  unique_lock<mutex> lck(mutex_);

  if (!finished_) {
	finished_ = true;

	this->Finish(Status::CANCELLED);
  }
}

template<class Outbound, class Request>
void ServerDownstreamReactor<Outbound, Request>::OnWriteDone(bool ok) {
  unique_lock<mutex> lck(mutex_);

  if (ok) {
	// Erase first message
	queue_.pop();

	// If we are not finished and there is more, send first now
	if (!finished_ && !queue_.empty())
	  this->StartWrite(&queue_.front());
  } else {
	if (!finished_) {
	  finished_ = true;

	  this->Finish(Status::CANCELLED);
	}
  }
}

template<class Outbound, class Request>
const Request *ServerDownstreamReactor<Outbound, Request>::GetRequest() {
  return request_;
}

template<class Outbound, class Request>
void ServerDownstreamReactor<Outbound, Request>::SetDoneCallback(const function<void(void *)> &done_callback) {
  done_callback_ = done_callback;
}

template<class Outbound, class Request>
void ServerDownstreamReactor<Outbound, Request>::EnqueueOutboundMessage(const Outbound &outbound) {
  unique_lock<mutex> lck(mutex_);

  queue_.push(outbound);

  // If we are not finished and this is the first element, we should send it now
  if (!finished_ && queue_.size() == 1)
	this->StartWrite(&queue_.front());
}

template<class Outbound, class Request>
void ServerDownstreamReactor<Outbound, Request>::Terminate() {
  unique_lock<mutex> lck(mutex_);

  if (!finished_) {
	finished_ = true;

	this->Finish(Status::OK);
  }
}
