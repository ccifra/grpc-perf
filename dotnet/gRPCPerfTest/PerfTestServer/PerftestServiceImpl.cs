using System.Collections.Generic;
using System.Threading.Tasks;
using Grpc.Core;
using NiPerfTest;

namespace PerfTestServer
{
    public class PerftestServiceImpl : NiPerfTest.niPerfTestService.niPerfTestServiceBase
    {
        static List<double> data = new List<double>();

        public override Task<InitResult> Init(InitParameters request, ServerCallContext context)
        {
            InitResult response = new InitResult();
            response.Status = request.Id;
            return Task.FromResult(response);
        }

        public override async Task StreamLatencyTest(IAsyncStreamReader<StreamLatencyClient> requestStream, IServerStreamWriter<StreamLatencyServer> responseStream, ServerCallContext context)
        {
            StreamLatencyServer response = new StreamLatencyServer();
            while (await requestStream.MoveNext())
            {
                response.Message = requestStream.Current.Message;
                await responseStream.WriteAsync(response);
            }
        }

        private void InitData(int numSamples)
        {
            if (data.Count != numSamples)
            {
                data = new List<double>(numSamples);
                for (int x = 0; x < numSamples; ++x)
                {
                    data.Add(1.1 + x);
                }
            }
        }

        public override async Task ReadContinuously(ReadContinuouslyParameters request, IServerStreamWriter<ReadContinuouslyResult> responseStream, ServerCallContext context)
        {
            var result = new ReadContinuouslyResult();

            InitData(request.NumSamples);
            result.Wfm.AddRange(data);

            for (int x=0; x<10000; ++x)
            {
                await responseStream.WriteAsync(result);
            }
        }

        public override Task<ReadResult> Read(ReadParameters request, ServerCallContext context)
        {
            InitData(request.NumSamples);

            var response = new ReadResult();
            response.Samples.AddRange(data);
            response.Status = 0;

            return Task.FromResult(response);
        }
    }
}
