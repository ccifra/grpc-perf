import grpc
import time
import perftest_pb2 as perftest
import perftest_pb2_grpc as perftest_rpc

server_address = "localhost"
server_port = "50051"

if __name__ == "__main__":
    channel = grpc.insecure_channel(f"{server_address}:{server_port}")
    client = perftest_rpc.niPerfTestServiceStub(channel)
    iterations = 1000
    samples = 131072

    print("Start Test")
    # Open session to NI-SCOPE module with options.
    client.Init(perftest.InitParameters(command="", id=0))
        
    start_time = time.perf_counter()
    for x in range(iterations):
        client.Read(perftest.ReadParameters(timeout = 1000, numSamples = samples))
    stop_time = time.perf_counter()
    total_time = (stop_time - start_time) * 1000
    print("TEST Complete: {:.3f}ms total or {:.3f}ms per iteration".format(total_time,total_time/iterations))