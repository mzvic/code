#include "broker_client.h"

PublisherClient::PublisherClient() : ClientUpstream(true) {
  Start();
}

void PublisherClient::Initialize(ClientUpstreamReactor<Bundle> *client_upstream_reactor) {
  unique_ptr<Broker::Stub> stub;

//  cout << "T:" << this_thread::get_id() << " ClientUpstream Reactor: Starting instance" << std::endl;

  client_upstream_reactor->GetChannel() = CreateChannel(SERVER_ADDRESS, InsecureChannelCredentials());

  stub = Broker::NewStub(client_upstream_reactor->GetChannel());

  stub->async()->Publish(client_upstream_reactor->GetContext(), &response_, client_upstream_reactor);

//  cout << "T:" << this_thread::get_id() << "ClientUpstream Reactor: Instance started" << std::endl;

  LOG("Broker client has initialized an upstream reactor");
}

SubscriberClient::SubscriberClient(const function<void(const Bundle &)> &inbound_callback, const vector<int> &interests) : ClientDownstream(true, inbound_callback) {
  request_.mutable_types()->Assign(interests.begin(), interests.end());

  Start();
}

void SubscriberClient::Initialize(ClientDownstreamReactor<Bundle> *client_downstream_reactor) {
  unique_ptr<Broker::Stub> stub;

//  cout << "T:" << this_thread::get_id() << " SubscriberClient: Starting instance" << std::endl;

  ChannelArguments channel_arguments;

  channel_arguments.SetMaxReceiveMessageSize(-1);

  client_downstream_reactor->GetChannel() = CreateCustomChannel(SERVER_ADDRESS, InsecureChannelCredentials(), channel_arguments);

  stub = Broker::NewStub(client_downstream_reactor->GetChannel());

  stub->async()->Subscribe(client_downstream_reactor->GetContext(), &request_, client_downstream_reactor);

//  cout << "T:" << this_thread::get_id() << " SubscriberClient: Instance started" << std::endl;
  LOG("Broker client has initialized a downstream reactor");
}
