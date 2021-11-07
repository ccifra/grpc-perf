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
    samples = 10

    # Open session to NI-SCOPE module with options.
    client.Init(perftest.InitParameters(command="Scope0", id=0))
        
    start_time = time.perf_counter()
    client.ConfigureVertical(perftest.ConfigureVerticalRequest(
        vi = "Scope0",
        channel_list = "0",
        range = 10.0,
        offset = 0.0,
        coupling = perftest.VerticalCoupling.VERTICAL_COUPLING_NISCOPE_VAL_AC,
        probe_attenuation = 1.0,
        enabled = True
        ))
    client.ConfigureHorizontalTiming(perftest.ConfigureHorizontalTimingRequest(
        vi = "Scope0",
        min_sample_rate = 50000000,
        min_num_pts = samples,
        ref_position = 50.0,
        num_records = 1,
        enforce_realtime = True
        ))
    client.InitiateAcquisition(perftest.InitiateAcquisitionRequest(vi = "Scope0"))
    fetch_response = client.Read(perftest.ReadParameters(
        timeout = 10000,
        numSamples = samples
        ))
    waveforms = fetch_response.samples
    stop_time = time.perf_counter()
    print("First Sample: {:.3f}".format(waveforms[0]))
    total_time = (stop_time - start_time) * 1000
    print("TEST Complete: {:.3f}ms total or {:.3f}ms per iteration".format(total_time,total_time/iterations))