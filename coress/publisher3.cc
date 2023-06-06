#include <condition_variable>
#include <grpcpp/grpcpp.h>
#include <queue>
#include <thread>

#include "core.grpc.pb.h"

using namespace core;

using core::Broker;
using google::protobuf::Empty;
using google::protobuf::Timestamp;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientWriteReactor;
using grpc::Status;
using std::queue;

#define SERVER_ADDRESS "localhost:50051"

std::queue<Bundle> Queue_;
std::mutex queue_mutex;

bool terminated = false;
//std::mutex publisher_mutex;

void OnPublisherOnDone();
void Init();
void Publish(Bundle &bundle);
void Terminate();

class PublisherClientReactor : public ClientWriteReactor<Bundle> {
public:
	explicit PublisherClientReactor() {
		std::unique_ptr<Broker::Stub> stub;

		stub = Broker::NewStub(grpc::CreateChannel(SERVER_ADDRESS, grpc::InsecureChannelCredentials()));

		std::cout << "Publisher Reactor: Starting a new instance" << std::endl;

		// Register reactor
		stub->async()->Publish(&context, &empty, this);

		// No OnDone until RemoveHold()
		AddHold();

		// Start RPC operations, now or later
		StartCall();
	}

	void OnDone(const Status &s) override {
		std::unique_lock<std::mutex> tlck(done_mutex);

		std::cout << "Publisher Reactor: OnDone" << std::endl;

		done_ = true;

		done_cv.notify_one();

		OnPublisherOnDone();
	}

	void OnWriteDone(bool ok) override {
		std::unique_lock<std::mutex> qlck(queue_mutex);
		//		std::unique_lock<std::mutex> tlck(done_mutex);

		if (ok) {
			std::cout << "Publisher Reactor: A value has been written" << std::endl;

			// Erase front message
			Queue_.pop();

			std::cout << "Message: Queue size: " << Queue_.size() << std::endl;

			// If there is more, send front now
			//			if (!Queue_.empty()) {
			//				StartWrite(&Queue_.front());
			//			} else {
			//				if (running_) {
			//					StartWritesDone();
			//
			//					RemoveHold();
			//				}
			//			}

			// Do something if we are still operational
			if (running_) {
				if (Queue_.empty()) {
					// If they are waiting for me, shutdown now
					if (stopping_)
						StopNow();
				} else {
					// If there are more messages, send front now
					StartWrite(&Queue_.front());
				}
			}
		} else {
			std::cout << "Publisher Reactor: Writing failure" << std::endl;

			StopNow();
		}
	}

	/*		void Enqueue(const Bundle &bundle) {
			std::unique_lock<std::mutex> mlck(queue_mutex);
			std::unique_lock<std::mutex> tlck(done_mutex);

			// No more enqueueing after termination
			if (running_)
				return;

			std::cout << "Publisher Reactor: Enqueueing new message" << std::endl;

			Queue_.push(bundle);

			std::cout << "Publisher Reactor: Queue size: " << Queue_.size() << std::endl;

			// If this is the first element, we should send it now
			if (Queue_.size() == 1)
				StartWrite(&Queue_.front());
		}
		*/

	void StopNow() {
		std::unique_lock<std::mutex> tlck(running_mutex);

		//		if (Queue_.empty()) {
		if (running_) {
			std::cout << "Publisher Reactor: Shutting down" << std::endl;

			StartWritesDone();

			RemoveHold();

			running_ = false;
		}
	}

	void Stop() {
		stopping_ = true;
	}

	void Wait() {
		std::unique_lock<std::mutex> tlck(done_mutex);

		std::cout << "Publisher Reactor: Waiting for Done" << std::endl;

		done_cv.wait(tlck, [this] { return done_; });

		std::cout << "Publisher Reactor: Done" << std::endl;
	}

private:
	ClientContext context;
	Empty empty;
	//	queue<Bundle> Queue_;
	std::mutex done_mutex;
	std::mutex running_mutex;
	std::condition_variable done_cv;
	bool done_ = false;
	bool running_ = true;
	bool stopping_ = false;
};

PublisherClientReactor *publisherreactor;

void OnPublisherOnDone() {
	std::cout << "OnPublisherOnDone" << std::endl;

	//	std::this_thread::sleep_for(std::chrono::milliseconds(1000));

	if (!terminated) {

		Init();

		// If there are elements in queue, start sending them now
		if (!Queue_.empty())
			publisherreactor->StartWrite(&Queue_.front());
	}
}

void Init() {
	//	std::unique_lock<std::mutex> plck(publisher_mutex);

	std::cout << "Initiating" << std::endl;

	publisherreactor = new PublisherClientReactor();
}


void Publish(Bundle &bundle) {
	std::unique_lock<std::mutex> qlck(queue_mutex);

	Timestamp *timestamp;

	if (terminated) {
		std::cout << "Trying to Publish on a terminated client, no message has been published" << std::endl;

		return;
	}

	//	std::unique_lock<std::mutex> plck(publisher_mutex);


	std::cout << "Publishing new message" << std::endl;

	struct timeval tv {};
	gettimeofday(&tv, nullptr);

	timestamp = bundle.mutable_timestamp();

	timestamp->set_seconds((std::int64_t) tv.tv_sec);
	timestamp->set_nanos((std::int32_t) tv.tv_usec * 1000);

	Queue_.push(bundle);

	std::cout << "Message: Queue size: " << Queue_.size() << std::endl;

	// If this is the only element in queue, we should send it now
	if (Queue_.size() == 1)
		publisherreactor->StartWrite(&Queue_.front());
}

void Terminate() {
	//	std::unique_lock<std::mutex> plck(publisher_mutex);

	std::cout << "Terminating" << std::endl;

	terminated = true;

	//	publisherreactor->StopNow();
	publisherreactor->Stop();

	publisherreactor->Wait();
}

int main(__attribute__((unused)) int argc, __attribute__((unused)) char **argv) {
	Bundle bundle;

	Init();

	for (int i = 0; i < 10; i++) {
		Publish(bundle);

		std::cout << "Value " << i << " written" << std::endl;

		//		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(2000));

	for (int i = 0; i < 100; i++) {
		Publish(bundle);

		std::cout << "Value " << i << " written" << std::endl;

		//		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	//	std::this_thread::sleep_for(std::chrono::milliseconds(20000));

	Terminate();

	return 0;
}