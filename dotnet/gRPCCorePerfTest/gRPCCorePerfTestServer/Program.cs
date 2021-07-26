using Grpc.Core;
using PerfTestServer;
using System;
using System.Threading.Tasks;

namespace gRPCCorePerfTest
{
    class Program
    {
        static async Task Main(string[] args)
        {
            Server server = new Server
            {
                Services = { NiPerfTest.niPerfTestService.BindService(new PerftestServiceImpl()) },
                Ports = { new ServerPort("localhost", 50051, ServerCredentials.Insecure) }
            };
            server.Start();

            Console.WriteLine("Server listening on port 50052");
            Console.WriteLine("Press any key to stop the server...");
            Console.ReadKey();

            await server.ShutdownAsync();
        }
    }
}
