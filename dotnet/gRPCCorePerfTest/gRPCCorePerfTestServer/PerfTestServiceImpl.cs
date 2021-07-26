using System.Collections.Generic;
using System.Threading.Tasks;
using Grpc.Core;
using NiPerfTest;

namespace PerfTestServer
{
    public class PerftestServiceImpl : NiPerfTest.niPerfTestService.niPerfTestServiceBase
    {
        private static List<double> _data = new List<double>();

        public override Task<InitResult> Init(InitParameters request, ServerCallContext context)
        {
            InitResult response = new InitResult();
            response.Status = request.Id;
            return Task.FromResult(response);
        }

        public override Task<InitWithOptionsResult> InitWithOptions(InitWithOptionsParameters request, ServerCallContext context)
        {
            var response = new InitWithOptionsResult();
            response.Status = 0;
            response.NewVi = new ViSession() { Id = 1 };
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
            if (_data.Count != numSamples)
            {
                _data = new List<double>(numSamples);
                for (int x = 0; x < numSamples; ++x)
                {
                    _data.Add(1.1 + x);
                }
            }
        }

        public override async Task ReadContinuously(ReadContinuouslyParameters request, IServerStreamWriter<ReadContinuouslyResult> responseStream, ServerCallContext context)
        {
            var result = new ReadContinuouslyResult();

            InitData(request.NumSamples);
            result.Wfm.AddRange(_data);

            for (int x=0; x<10000; ++x)
            {
                await responseStream.WriteAsync(result);
            }
        }

        public override Task<ReadResult> Read(ReadParameters request, ServerCallContext context)
        {
            InitData(request.NumSamples);
            var response = new ReadResult();
            response.Wfm.AddRange(_data);
            response.Status = 0;

            return Task.FromResult(response);
        }
    }
}
