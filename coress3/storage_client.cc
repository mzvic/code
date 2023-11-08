#include "storage_client.h"

PusherClient::PusherClient() : ClientUpstream(true) {
  Start();
}

void PusherClient::Initialize(ClientUpstreamReactor<Bundle> *client_upstream_reactor) {
  unique_ptr<Storage::Stub> stub;

  client_upstream_reactor->GetChannel() = CreateChannel(SERVER_ADDRESS, InsecureChannelCredentials());

  stub = Storage::NewStub(client_upstream_reactor->GetChannel());

  stub->async()->Push(client_upstream_reactor->GetContext(), &response_, client_upstream_reactor);

  LOG("Broker client has initialized an upstream reactor");
}

PullerClient::PullerClient(const function<void(const Bundle &)> &inbound_callback) : ClientDownstream(true, inbound_callback) {
  Start();
}

void PullerClient::Initialize(ClientDownstreamReactor<Bundle> *client_downstream_reactor) {
  unique_ptr<Storage::Stub> stub;

  ChannelArguments channel_arguments;

  channel_arguments.SetMaxReceiveMessageSize(-1);

  client_downstream_reactor->GetChannel() = CreateCustomChannel(SERVER_ADDRESS, InsecureChannelCredentials(), channel_arguments);

  stub = Storage::NewStub(client_downstream_reactor->GetChannel());

  stub->async()->Pull(client_downstream_reactor->GetContext(), &request_, client_downstream_reactor);

  LOG("Broker client has initialized a downstream reactor");
}
