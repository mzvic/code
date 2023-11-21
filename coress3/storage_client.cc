#include "storage_client.h"

PusherClient::PusherClient() : ClientUpstream(true) {
  Start();
}

PusherClient::~PusherClient() {
  Stop();
}

void PusherClient::Initialize(ClientUpstreamReactor<Bundle> *client_upstream_reactor) {
  unique_ptr<Storage::Stub> stub;
//  ChannelArguments args;
//
//  args.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, 300);
//  args.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 300);
//  args.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
//  args.SetInt(GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA, 0);
//  args.SetInt(GRPC_ARG_MAX_CONNECTION_IDLE_MS, 1000);
//  args.SetInt(GRPC_ARG_SERVER_HANDSHAKE_TIMEOUT_MS, 1000);
//  args.SetInt(GRPC_ARG_CLIENT_IDLE_TIMEOUT_MS, 1000);

//  client_upstream_reactor->GetChannel() = CreateCustomChannel(STORAGE_SERVER_ADDRESS, InsecureChannelCredentials(), args);
  client_upstream_reactor->GetChannel() = CreateChannel(STORAGE_SERVER_ADDRESS, InsecureChannelCredentials());

  stub = Storage::NewStub(client_upstream_reactor->GetChannel());

  stub->async()->Push(client_upstream_reactor->GetContext(), &response_, client_upstream_reactor);

  LOG("Storage client has initialized an upstream reactor");
}

PullerClient::PullerClient(const function<void(const Bundle &)> &inbound_callback) : ClientDownstream(true, inbound_callback) {
  Start();
}
PullerClient::~PullerClient() {
  Stop();
}

void PullerClient::Initialize(ClientDownstreamReactor<Bundle> *client_downstream_reactor) {
  unique_ptr<Storage::Stub> stub;

  ChannelArguments channel_arguments;

  channel_arguments.SetMaxReceiveMessageSize(-1);

  client_downstream_reactor->GetChannel() = CreateCustomChannel(STORAGE_SERVER_ADDRESS, InsecureChannelCredentials(), channel_arguments);

  stub = Storage::NewStub(client_downstream_reactor->GetChannel());

  stub->async()->Pull(client_downstream_reactor->GetContext(), &request_, client_downstream_reactor);

  LOG("Storage client has initialized a downstream reactor");
}
