// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: core.proto
#ifndef GRPC_core_2eproto__INCLUDED
#define GRPC_core_2eproto__INCLUDED

#include "core.pb.h"

#include <functional>
#include <grpcpp/generic/async_generic_service.h>
#include <grpcpp/support/async_stream.h>
#include <grpcpp/support/async_unary_call.h>
#include <grpcpp/support/client_callback.h>
#include <grpcpp/client_context.h>
#include <grpcpp/completion_queue.h>
#include <grpcpp/support/message_allocator.h>
#include <grpcpp/support/method_handler.h>
#include <grpcpp/impl/proto_utils.h>
#include <grpcpp/impl/rpc_method.h>
#include <grpcpp/support/server_callback.h>
#include <grpcpp/impl/server_callback_handlers.h>
#include <grpcpp/server_context.h>
#include <grpcpp/impl/service_type.h>
#include <grpcpp/support/status.h>
#include <grpcpp/support/stub_options.h>
#include <grpcpp/support/sync_stream.h>

namespace core {

class Broker final {
 public:
  static constexpr char const* service_full_name() {
    return "core.Broker";
  }
  class StubInterface {
   public:
    virtual ~StubInterface() {}
    std::unique_ptr< ::grpc::ClientWriterInterface< ::core::Bundle>> Publish(::grpc::ClientContext* context, ::google::protobuf::Empty* response) {
      return std::unique_ptr< ::grpc::ClientWriterInterface< ::core::Bundle>>(PublishRaw(context, response));
    }
    std::unique_ptr< ::grpc::ClientAsyncWriterInterface< ::core::Bundle>> AsyncPublish(::grpc::ClientContext* context, ::google::protobuf::Empty* response, ::grpc::CompletionQueue* cq, void* tag) {
      return std::unique_ptr< ::grpc::ClientAsyncWriterInterface< ::core::Bundle>>(AsyncPublishRaw(context, response, cq, tag));
    }
    std::unique_ptr< ::grpc::ClientAsyncWriterInterface< ::core::Bundle>> PrepareAsyncPublish(::grpc::ClientContext* context, ::google::protobuf::Empty* response, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncWriterInterface< ::core::Bundle>>(PrepareAsyncPublishRaw(context, response, cq));
    }
    std::unique_ptr< ::grpc::ClientReaderInterface< ::core::Bundle>> Subscribe(::grpc::ClientContext* context, const ::google::protobuf::Empty& request) {
      return std::unique_ptr< ::grpc::ClientReaderInterface< ::core::Bundle>>(SubscribeRaw(context, request));
    }
    std::unique_ptr< ::grpc::ClientAsyncReaderInterface< ::core::Bundle>> AsyncSubscribe(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq, void* tag) {
      return std::unique_ptr< ::grpc::ClientAsyncReaderInterface< ::core::Bundle>>(AsyncSubscribeRaw(context, request, cq, tag));
    }
    std::unique_ptr< ::grpc::ClientAsyncReaderInterface< ::core::Bundle>> PrepareAsyncSubscribe(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncReaderInterface< ::core::Bundle>>(PrepareAsyncSubscribeRaw(context, request, cq));
    }
    class async_interface {
     public:
      virtual ~async_interface() {}
      virtual void Publish(::grpc::ClientContext* context, ::google::protobuf::Empty* response, ::grpc::ClientWriteReactor< ::core::Bundle>* reactor) = 0;
      virtual void Subscribe(::grpc::ClientContext* context, const ::google::protobuf::Empty* request, ::grpc::ClientReadReactor< ::core::Bundle>* reactor) = 0;
    };
    typedef class async_interface experimental_async_interface;
    virtual class async_interface* async() { return nullptr; }
    class async_interface* experimental_async() { return async(); }
   private:
    virtual ::grpc::ClientWriterInterface< ::core::Bundle>* PublishRaw(::grpc::ClientContext* context, ::google::protobuf::Empty* response) = 0;
    virtual ::grpc::ClientAsyncWriterInterface< ::core::Bundle>* AsyncPublishRaw(::grpc::ClientContext* context, ::google::protobuf::Empty* response, ::grpc::CompletionQueue* cq, void* tag) = 0;
    virtual ::grpc::ClientAsyncWriterInterface< ::core::Bundle>* PrepareAsyncPublishRaw(::grpc::ClientContext* context, ::google::protobuf::Empty* response, ::grpc::CompletionQueue* cq) = 0;
    virtual ::grpc::ClientReaderInterface< ::core::Bundle>* SubscribeRaw(::grpc::ClientContext* context, const ::google::protobuf::Empty& request) = 0;
    virtual ::grpc::ClientAsyncReaderInterface< ::core::Bundle>* AsyncSubscribeRaw(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq, void* tag) = 0;
    virtual ::grpc::ClientAsyncReaderInterface< ::core::Bundle>* PrepareAsyncSubscribeRaw(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq) = 0;
  };
  class Stub final : public StubInterface {
   public:
    Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());
    std::unique_ptr< ::grpc::ClientWriter< ::core::Bundle>> Publish(::grpc::ClientContext* context, ::google::protobuf::Empty* response) {
      return std::unique_ptr< ::grpc::ClientWriter< ::core::Bundle>>(PublishRaw(context, response));
    }
    std::unique_ptr< ::grpc::ClientAsyncWriter< ::core::Bundle>> AsyncPublish(::grpc::ClientContext* context, ::google::protobuf::Empty* response, ::grpc::CompletionQueue* cq, void* tag) {
      return std::unique_ptr< ::grpc::ClientAsyncWriter< ::core::Bundle>>(AsyncPublishRaw(context, response, cq, tag));
    }
    std::unique_ptr< ::grpc::ClientAsyncWriter< ::core::Bundle>> PrepareAsyncPublish(::grpc::ClientContext* context, ::google::protobuf::Empty* response, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncWriter< ::core::Bundle>>(PrepareAsyncPublishRaw(context, response, cq));
    }
    std::unique_ptr< ::grpc::ClientReader< ::core::Bundle>> Subscribe(::grpc::ClientContext* context, const ::google::protobuf::Empty& request) {
      return std::unique_ptr< ::grpc::ClientReader< ::core::Bundle>>(SubscribeRaw(context, request));
    }
    std::unique_ptr< ::grpc::ClientAsyncReader< ::core::Bundle>> AsyncSubscribe(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq, void* tag) {
      return std::unique_ptr< ::grpc::ClientAsyncReader< ::core::Bundle>>(AsyncSubscribeRaw(context, request, cq, tag));
    }
    std::unique_ptr< ::grpc::ClientAsyncReader< ::core::Bundle>> PrepareAsyncSubscribe(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq) {
      return std::unique_ptr< ::grpc::ClientAsyncReader< ::core::Bundle>>(PrepareAsyncSubscribeRaw(context, request, cq));
    }
    class async final :
      public StubInterface::async_interface {
     public:
      void Publish(::grpc::ClientContext* context, ::google::protobuf::Empty* response, ::grpc::ClientWriteReactor< ::core::Bundle>* reactor) override;
      void Subscribe(::grpc::ClientContext* context, const ::google::protobuf::Empty* request, ::grpc::ClientReadReactor< ::core::Bundle>* reactor) override;
     private:
      friend class Stub;
      explicit async(Stub* stub): stub_(stub) { }
      Stub* stub() { return stub_; }
      Stub* stub_;
    };
    class async* async() override { return &async_stub_; }

   private:
    std::shared_ptr< ::grpc::ChannelInterface> channel_;
    class async async_stub_{this};
    ::grpc::ClientWriter< ::core::Bundle>* PublishRaw(::grpc::ClientContext* context, ::google::protobuf::Empty* response) override;
    ::grpc::ClientAsyncWriter< ::core::Bundle>* AsyncPublishRaw(::grpc::ClientContext* context, ::google::protobuf::Empty* response, ::grpc::CompletionQueue* cq, void* tag) override;
    ::grpc::ClientAsyncWriter< ::core::Bundle>* PrepareAsyncPublishRaw(::grpc::ClientContext* context, ::google::protobuf::Empty* response, ::grpc::CompletionQueue* cq) override;
    ::grpc::ClientReader< ::core::Bundle>* SubscribeRaw(::grpc::ClientContext* context, const ::google::protobuf::Empty& request) override;
    ::grpc::ClientAsyncReader< ::core::Bundle>* AsyncSubscribeRaw(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq, void* tag) override;
    ::grpc::ClientAsyncReader< ::core::Bundle>* PrepareAsyncSubscribeRaw(::grpc::ClientContext* context, const ::google::protobuf::Empty& request, ::grpc::CompletionQueue* cq) override;
    const ::grpc::internal::RpcMethod rpcmethod_Publish_;
    const ::grpc::internal::RpcMethod rpcmethod_Subscribe_;
  };
  static std::unique_ptr<Stub> NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options = ::grpc::StubOptions());

  class Service : public ::grpc::Service {
   public:
    Service();
    virtual ~Service();
    virtual ::grpc::Status Publish(::grpc::ServerContext* context, ::grpc::ServerReader< ::core::Bundle>* reader, ::google::protobuf::Empty* response);
    virtual ::grpc::Status Subscribe(::grpc::ServerContext* context, const ::google::protobuf::Empty* request, ::grpc::ServerWriter< ::core::Bundle>* writer);
  };
  template <class BaseClass>
  class WithAsyncMethod_Publish : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithAsyncMethod_Publish() {
      ::grpc::Service::MarkMethodAsync(0);
    }
    ~WithAsyncMethod_Publish() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Publish(::grpc::ServerContext* /*context*/, ::grpc::ServerReader< ::core::Bundle>* /*reader*/, ::google::protobuf::Empty* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestPublish(::grpc::ServerContext* context, ::grpc::ServerAsyncReader< ::google::protobuf::Empty, ::core::Bundle>* reader, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncClientStreaming(0, context, reader, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithAsyncMethod_Subscribe : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithAsyncMethod_Subscribe() {
      ::grpc::Service::MarkMethodAsync(1);
    }
    ~WithAsyncMethod_Subscribe() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Subscribe(::grpc::ServerContext* /*context*/, const ::google::protobuf::Empty* /*request*/, ::grpc::ServerWriter< ::core::Bundle>* /*writer*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestSubscribe(::grpc::ServerContext* context, ::google::protobuf::Empty* request, ::grpc::ServerAsyncWriter< ::core::Bundle>* writer, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncServerStreaming(1, context, request, writer, new_call_cq, notification_cq, tag);
    }
  };
  typedef WithAsyncMethod_Publish<WithAsyncMethod_Subscribe<Service > > AsyncService;
  template <class BaseClass>
  class WithCallbackMethod_Publish : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithCallbackMethod_Publish() {
      ::grpc::Service::MarkMethodCallback(0,
          new ::grpc::internal::CallbackClientStreamingHandler< ::core::Bundle, ::google::protobuf::Empty>(
            [this](
                   ::grpc::CallbackServerContext* context, ::google::protobuf::Empty* response) { return this->Publish(context, response); }));
    }
    ~WithCallbackMethod_Publish() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Publish(::grpc::ServerContext* /*context*/, ::grpc::ServerReader< ::core::Bundle>* /*reader*/, ::google::protobuf::Empty* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual ::grpc::ServerReadReactor< ::core::Bundle>* Publish(
      ::grpc::CallbackServerContext* /*context*/, ::google::protobuf::Empty* /*response*/)  { return nullptr; }
  };
  template <class BaseClass>
  class WithCallbackMethod_Subscribe : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithCallbackMethod_Subscribe() {
      ::grpc::Service::MarkMethodCallback(1,
          new ::grpc::internal::CallbackServerStreamingHandler< ::google::protobuf::Empty, ::core::Bundle>(
            [this](
                   ::grpc::CallbackServerContext* context, const ::google::protobuf::Empty* request) { return this->Subscribe(context, request); }));
    }
    ~WithCallbackMethod_Subscribe() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Subscribe(::grpc::ServerContext* /*context*/, const ::google::protobuf::Empty* /*request*/, ::grpc::ServerWriter< ::core::Bundle>* /*writer*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual ::grpc::ServerWriteReactor< ::core::Bundle>* Subscribe(
      ::grpc::CallbackServerContext* /*context*/, const ::google::protobuf::Empty* /*request*/)  { return nullptr; }
  };
  typedef WithCallbackMethod_Publish<WithCallbackMethod_Subscribe<Service > > CallbackService;
  typedef CallbackService ExperimentalCallbackService;
  template <class BaseClass>
  class WithGenericMethod_Publish : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithGenericMethod_Publish() {
      ::grpc::Service::MarkMethodGeneric(0);
    }
    ~WithGenericMethod_Publish() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Publish(::grpc::ServerContext* /*context*/, ::grpc::ServerReader< ::core::Bundle>* /*reader*/, ::google::protobuf::Empty* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithGenericMethod_Subscribe : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithGenericMethod_Subscribe() {
      ::grpc::Service::MarkMethodGeneric(1);
    }
    ~WithGenericMethod_Subscribe() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Subscribe(::grpc::ServerContext* /*context*/, const ::google::protobuf::Empty* /*request*/, ::grpc::ServerWriter< ::core::Bundle>* /*writer*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
  };
  template <class BaseClass>
  class WithRawMethod_Publish : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawMethod_Publish() {
      ::grpc::Service::MarkMethodRaw(0);
    }
    ~WithRawMethod_Publish() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Publish(::grpc::ServerContext* /*context*/, ::grpc::ServerReader< ::core::Bundle>* /*reader*/, ::google::protobuf::Empty* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestPublish(::grpc::ServerContext* context, ::grpc::ServerAsyncReader< ::grpc::ByteBuffer, ::grpc::ByteBuffer>* reader, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncClientStreaming(0, context, reader, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithRawMethod_Subscribe : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawMethod_Subscribe() {
      ::grpc::Service::MarkMethodRaw(1);
    }
    ~WithRawMethod_Subscribe() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Subscribe(::grpc::ServerContext* /*context*/, const ::google::protobuf::Empty* /*request*/, ::grpc::ServerWriter< ::core::Bundle>* /*writer*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    void RequestSubscribe(::grpc::ServerContext* context, ::grpc::ByteBuffer* request, ::grpc::ServerAsyncWriter< ::grpc::ByteBuffer>* writer, ::grpc::CompletionQueue* new_call_cq, ::grpc::ServerCompletionQueue* notification_cq, void *tag) {
      ::grpc::Service::RequestAsyncServerStreaming(1, context, request, writer, new_call_cq, notification_cq, tag);
    }
  };
  template <class BaseClass>
  class WithRawCallbackMethod_Publish : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawCallbackMethod_Publish() {
      ::grpc::Service::MarkMethodRawCallback(0,
          new ::grpc::internal::CallbackClientStreamingHandler< ::grpc::ByteBuffer, ::grpc::ByteBuffer>(
            [this](
                   ::grpc::CallbackServerContext* context, ::grpc::ByteBuffer* response) { return this->Publish(context, response); }));
    }
    ~WithRawCallbackMethod_Publish() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Publish(::grpc::ServerContext* /*context*/, ::grpc::ServerReader< ::core::Bundle>* /*reader*/, ::google::protobuf::Empty* /*response*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual ::grpc::ServerReadReactor< ::grpc::ByteBuffer>* Publish(
      ::grpc::CallbackServerContext* /*context*/, ::grpc::ByteBuffer* /*response*/)  { return nullptr; }
  };
  template <class BaseClass>
  class WithRawCallbackMethod_Subscribe : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithRawCallbackMethod_Subscribe() {
      ::grpc::Service::MarkMethodRawCallback(1,
          new ::grpc::internal::CallbackServerStreamingHandler< ::grpc::ByteBuffer, ::grpc::ByteBuffer>(
            [this](
                   ::grpc::CallbackServerContext* context, const::grpc::ByteBuffer* request) { return this->Subscribe(context, request); }));
    }
    ~WithRawCallbackMethod_Subscribe() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable synchronous version of this method
    ::grpc::Status Subscribe(::grpc::ServerContext* /*context*/, const ::google::protobuf::Empty* /*request*/, ::grpc::ServerWriter< ::core::Bundle>* /*writer*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    virtual ::grpc::ServerWriteReactor< ::grpc::ByteBuffer>* Subscribe(
      ::grpc::CallbackServerContext* /*context*/, const ::grpc::ByteBuffer* /*request*/)  { return nullptr; }
  };
  typedef Service StreamedUnaryService;
  template <class BaseClass>
  class WithSplitStreamingMethod_Subscribe : public BaseClass {
   private:
    void BaseClassMustBeDerivedFromService(const Service* /*service*/) {}
   public:
    WithSplitStreamingMethod_Subscribe() {
      ::grpc::Service::MarkMethodStreamed(1,
        new ::grpc::internal::SplitServerStreamingHandler<
          ::google::protobuf::Empty, ::core::Bundle>(
            [this](::grpc::ServerContext* context,
                   ::grpc::ServerSplitStreamer<
                     ::google::protobuf::Empty, ::core::Bundle>* streamer) {
                       return this->StreamedSubscribe(context,
                         streamer);
                  }));
    }
    ~WithSplitStreamingMethod_Subscribe() override {
      BaseClassMustBeDerivedFromService(this);
    }
    // disable regular version of this method
    ::grpc::Status Subscribe(::grpc::ServerContext* /*context*/, const ::google::protobuf::Empty* /*request*/, ::grpc::ServerWriter< ::core::Bundle>* /*writer*/) override {
      abort();
      return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
    }
    // replace default version of method with split streamed
    virtual ::grpc::Status StreamedSubscribe(::grpc::ServerContext* context, ::grpc::ServerSplitStreamer< ::google::protobuf::Empty,::core::Bundle>* server_split_streamer) = 0;
  };
  typedef WithSplitStreamingMethod_Subscribe<Service > SplitStreamedService;
  typedef WithSplitStreamingMethod_Subscribe<Service > StreamedService;
};

}  // namespace core


#endif  // GRPC_core_2eproto__INCLUDED
