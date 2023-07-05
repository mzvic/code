#include "client.h"

#include <utility>

SubscriberClient::SubscriberClientReactor::SubscriberClientReactor(SubscriberClient *subscriber_client) {
  unique_ptr<Broker::Stub> stub;
  shared_ptr<Channel> channel;
  ChannelArguments channel_arguments;

  subscriber_client_ = subscriber_client;

  channel = CreateChannel(SERVER_ADDRESS, InsecureChannelCredentials());

  stub = Broker::NewStub(channel);

  cout << "SubscriberClientReactor: Starting a new instance" << endl;

  // Register reactor
  stub->async()->Subscribe(&context_, &subscriber_client->interests_, this);

  // No OnDone until RemoveHold()
  AddHold();

  // Start reading now
  StartRead(&bundle_);

  // Start RPC operations
  StartCall();
}

void SubscriberClient::SubscriberClientReactor::OnDone(const Status &s) {
  cout << "SubscriberClientReactor: OnDone" << endl;

  subscriber_client_->OnSubscriberOnDone();
}

void SubscriberClient::SubscriberClientReactor::OnReadDone(bool ok) {
  if (ok) {
//	cout << "Subscriber Reactor: A value has been read" << endl;

	if (subscriber_client_ != nullptr)
	  subscriber_client_->callback_(bundle_);
	else
	  cout << "SubscriberClientReactor: No registered callback" << endl;

	StartRead(&bundle_);
  } else {
	cout << "SubscriberClientReactor: Reading failure" << endl;

	RemoveHold();
  }
}

SubscriberClient::SubscriberClient(function<void(const Bundle &)> callback, const vector<int> &interests) {
  cout << "SubscriberClient: Constructing instance" << endl;

  callback_ = std::move(callback);

  interests_.mutable_types()->Assign(interests.begin(), interests.end());

  // Create new reactor
  new SubscriberClientReactor(this);
}

SubscriberClient::~SubscriberClient() {
  cout << "SubscriberClient: Destroying instance" << endl;
}

void SubscriberClient::OnSubscriberOnDone() {
  cout << "SubscriberClient: OnSubscriberOnDone" << endl;

  new SubscriberClientReactor(this);
}