import time
import threading
import grpc
from google.protobuf.timestamp_pb2 import Timestamp
import core_pb2 as core
import core_pb2_grpc as core_grpc
from google.protobuf.empty_pb2 import Empty

bundle = None

def process(bundle):
    #print(bundle)    
    return bundle

def main():
    global bundle
    bundle = core.Bundle()
    channel = grpc.insecure_channel('localhost:50051')
    stub = core_grpc.BrokerStub(channel)
    response_stream = stub.Subscribe(Empty())

    for bundle in response_stream:
        process(bundle)
       
if __name__ == '__main__':
    main()

