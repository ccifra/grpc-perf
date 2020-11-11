using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using Grpc.Core;
using NiScope;

namespace PerfTestServer
{
    public class PerftestServiceImpl : NiScope.niScopeService.niScopeServiceBase
    {
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

        static List<double> data = new List<double>();

        public override async Task ReadContinuously(ReadContinuouslyParameters request, IServerStreamWriter<ReadContinuouslyResult> responseStream, ServerCallContext context)
        {
            var result = new ReadContinuouslyResult();

            if (data.Count != request.NumSamples)
            {
                data = new List<double>(request.NumSamples);
                for (int x = 0; x < request.NumSamples; ++x)
                {
                    data.Add(1.1 + x);
                }
            }
            result.Wfm.AddRange(data);

            for (int x=0; x<10000; ++x)
            {
                await responseStream.WriteAsync(result);
            }
        }

        public override Task<ReadResult> Read(ReadParameters request, ServerCallContext context)
        {
            if (data.Count != request.NumSamples)
            {
                data = new List<double>(request.NumSamples);
                for (int x = 0; x < request.NumSamples; ++x)
                {
                    data.Add(1.1 + x);
                }
            }

            var response = new ReadResult();
            response.Wfm.AddRange(data);
            response.Status = 0;

            return Task.FromResult(response);
        }
    }
}
