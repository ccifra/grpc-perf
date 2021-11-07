import grpc
import time
import perftest_pb2 as perftest
import perftest_pb2_grpc as perftest_rpc

server_address = "localhost"
server_port = "50051"

if __name__ == "__main__":
    channel = grpc.insecure_channel(f"{server_address}:{server_port}")
    client = perftest_rpc.niPerfTestServiceStub(channel)
    iterations = 10000
    iteration = 0
    #samples = 131072
    samples = 10000

    print("Start Test")
    # Open session to NI-SCOPE module with options.
    client.Init(perftest.InitParameters(command="", id=0))
        
    for x in client.ReadContinuously(perftest.ReadContinuouslyParameters(numIterations = iterations, numSamples = samples)):
        iteration += 1

    start_time = time.perf_counter()
    for x in client.ReadContinuously(perftest.ReadContinuouslyParameters(numIterations = iterations, numSamples = samples)):
        iteration += 1
    stop_time = time.perf_counter()
    total_seconds = (stop_time - start_time)
    total_time = (stop_time - start_time) * 1000
    bytesPerSecond = (8.0 * samples * iterations) / total_seconds
    MBPerSecond = bytesPerSecond / (1024.0 * 1024)

    print("TEST Complete: {:.3f}ms total or {:.3f}ms per iteration, {:.3f} MB/s".format(total_time,total_time/iterations, MBPerSecond))