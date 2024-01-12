#include "broker_client.h"

PublisherClient::PublisherClient() : ClientUpstream(true) {
  Start();
}

PublisherClient::~PublisherClient() {
  Stop();
}

void PublisherClient::Initialize(ClientUpstreamReactor<Bundle> *client_upstream_reactor) {
  unique_ptr<Broker::Stub> stub;

  client_upstream_reactor->GetChannel() = CreateChannel(BROKER_SERVER_ADDRESS, InsecureChannelCredentials());

  stub = Broker::NewStub(client_upstream_reactor->GetChannel());

  stub->async()->Publish(client_upstream_reactor->GetContext(), &response_, client_upstream_reactor);

  LOG("Broker client has initialized an upstream reactor");
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
  bundle.mutable_timestamp()->CopyFrom(timestamp);

  EnqueueOutboundMessage(bundle);
}

SubscriberClient::SubscriberClient(const function<void(const Bundle &)> &inbound_callback, const vector<int> &interests) : ClientDownstream(true, inbound_callback) {
  request_.mutable_types()->Assign(interests.begin(), interests.end());

  Start();
}

SubscriberClient::~SubscriberClient() {
  Stop();
}

void SubscriberClient::Initialize(ClientDownstreamReactor<Bundle> *client_downstream_reactor) {
  unique_ptr<Broker::Stub> stub;

  ChannelArguments channel_arguments;

  channel_arguments.SetMaxReceiveMessageSize(-1);

  client_downstream_reactor->GetChannel() = CreateCustomChannel(BROKER_SERVER_ADDRESS, InsecureChannelCredentials(), channel_arguments);

  stub = Broker::NewStub(client_downstream_reactor->GetChannel());

  stub->async()->Subscribe(client_downstream_reactor->GetContext(), &request_, client_downstream_reactor);

//  cout << "T:" << this_thread::get_id() << " SubscriberClient: Instance started" << std::endl;
  LOG("Broker client has initialized a downstream reactor");
}
