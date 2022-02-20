//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include <client_utilities.h>
#include <performance_tests.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <perftest.grpc.pb.h>
#include <sstream>
#include <iostream>
#include <fstream>
#include <thread>
#include <src/core/lib/iomgr/executor.h>
#include <src/core/lib/iomgr/timer_manager.h>

//---------------------------------------------------------------------
//---------------------------------------------------------------------
#ifndef _WIN32
#include <sched.h>
#endif

//---------------------------------------------------------------------
//---------------------------------------------------------------------
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using std::cout;
using std::endl;
using namespace niPerfTest;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
string GetServerAddress(int argc, char** argv)
{
    string target_str;
    string arg_str("--target");
    if (argc > 1)
    {
        string arg_val = argv[1];
        size_t start_pos = arg_val.find(arg_str);
        if (start_pos != string::npos)
        {
            start_pos += arg_str.size();
            if (arg_val[start_pos] == '=')
            {
                target_str = arg_val.substr(start_pos + 1);
            }
            else
            {
                cout << "The only correct argument syntax is --target=" << endl;
                return 0;
            }
        }
        else
        {
            cout << "The only acceptable argument is --target=" << endl;
            return 0;
        }
    }
    else
    {
        target_str = "localhost";
    }
    return target_str;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
string GetServerPort(int argc, char** argv)
{
    string target_str;
    string arg_str("--port");
    if (argc > 2)
    {
        string arg_val = argv[2];
        size_t start_pos = arg_val.find(arg_str);
        if (start_pos != string::npos)
        {
            start_pos += arg_str.size();
            if (arg_val[start_pos] == '=')
            {
                target_str = arg_val.substr(start_pos + 1);
            }
            else
            {
                cout << "The only correct argument syntax is --port=" << endl;
                return 0;
            }
        }
        else
        {
            cout << "The only acceptable argument is --port=" << endl;
            return 0;
        }
    }
    else
    {
        target_str = "50051";
    }
    return target_str;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
string GetCertPath(int argc, char** argv)
{
    string cert_str;
    string arg_str("--cert");
    if (argc > 3)
    {
        string arg_val = argv[3];
        size_t start_pos = arg_val.find(arg_str);
        if (start_pos != string::npos)
        {
            start_pos += arg_str.size();
            if (arg_val[start_pos] == '=')
            {
                cert_str = arg_val.substr(start_pos + 1);
            }
            else
            {
                cout << "The only correct argument syntax is --cert=" << endl;
                return 0;
            }
        }
        else
        {
            cout << "The only acceptable argument is --cert=" << endl;
            return 0;
        }
    }
    return cert_str;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
string read_keycert( const string& filename)
{	
	string data;
	ifstream file(filename.c_str(), ios::in);
	if (file.is_open())
	{
		stringstream ss;
		ss << file.rdbuf();
		file.close();
		data = ss.str();
	}
	return data;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
shared_ptr<grpc::ChannelCredentials> CreateCredentials(int argc, char **argv)
{
    shared_ptr<grpc::ChannelCredentials> creds;
    auto certificatePath = GetCertPath(argc, argv);
    if (!certificatePath.empty())
    {
        string cacert = read_keycert(certificatePath);
        grpc::SslCredentialsOptions ssl_opts;
        ssl_opts.pem_root_certs=cacert;
        creds = grpc::SslCredentials(ssl_opts);
    }
    else
    {
        creds = grpc::InsecureChannelCredentials();
    }
    return creds;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunLatencyStreamTestSuite(NIPerfTestClient& client)
{
    cout << "Start Latency Stream Test Suite" << endl;
    EnableTracing();
    PerformLatencyStreamTest(client, "streamlatency1.txt");
    PerformLatencyStreamTest(client, "streamlatency2.txt");
    PerformLatencyStreamTest(client, "streamlatency3.txt");
    PerformLatencyStreamTest(client, "streamlatency4.txt");
    PerformLatencyStreamTest(client, "streamlatency5.txt");
    DisableTracing();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunMessageLatencyTestSuite(NIPerfTestClient& client)
{
    cout << "Start Message Latency Test Suite" << endl;
    PerformMessageLatencyTest(client, "latency1.txt");
    PerformMessageLatencyTest(client, "latency2.txt");
    PerformMessageLatencyTest(client, "latency3.txt");
    PerformMessageLatencyTest(client, "latency4.txt");
    PerformMessageLatencyTest(client, "latency5.txt");
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunLatencyPayloadWriteTestSuite(NIPerfTestClient& client)
{
    cout << "Start Latency Payload Write Test Suite" << endl;
    PerformLatencyPayloadWriteTest(client, 1, "payloadlatency1.txt");
    PerformLatencyPayloadWriteTest(client, 8, "payloadlatency8.txt");
    PerformLatencyPayloadWriteTest(client, 16, "payloadlatency16.txt");
    PerformLatencyPayloadWriteTest(client, 32, "payloadlatency32.txt");
    PerformLatencyPayloadWriteTest(client, 64, "payloadlatency64.txt");
    PerformLatencyPayloadWriteTest(client, 128, "payloadlatency128.txt");
    PerformLatencyPayloadWriteTest(client, 1024, "payloadlatency1024.txt");
    PerformLatencyPayloadWriteTest(client, 32768, "payloadlatency32768.txt");
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunLatencyPayloadWriteStreamTestSuite(NIPerfTestClient& client)
{
    cout << "Start Latency Payload Write Stream Test Suite" << endl;
    PerformLatencyPayloadWriteStreamTest(client, 1, "payloadstreamlatency1.txt");
    PerformLatencyPayloadWriteStreamTest(client, 8, "payloadstreamlatency8.txt");
    PerformLatencyPayloadWriteStreamTest(client, 16, "payloadstreamlatency16.txt");
    PerformLatencyPayloadWriteStreamTest(client, 32, "payloadstreamlatency32.txt");
    PerformLatencyPayloadWriteStreamTest(client, 64, "payloadstreamlatency64.txt");
    PerformLatencyPayloadWriteStreamTest(client, 128, "payloadstreamlatency128.txt");
    PerformLatencyPayloadWriteStreamTest(client, 1024, "payloadstreamlatency1024.txt");
    PerformLatencyPayloadWriteStreamTest(client, 32768, "payloadstreamlatency32768.txt");
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunParallelStreamTest(int numClients, std::string targetStr, std::string& port, std::shared_ptr<grpc::ChannelCredentials> creds)
{
    cout << "Start Parallel Stream Test Suite" << endl;
    std::vector<NIPerfTestClient*> clients;
    for (int x=0; x<numClients; ++x)
    {
        // Set a dummy (but distinct) channel arg on each channel so that
        // every channel gets its own connection
        grpc::ChannelArguments args;
        args.SetInt(GRPC_ARG_MINIMAL_STACK, 1);
        args.SetInt("ClientIndex", x);
        auto client = new NIPerfTestClient(grpc::CreateCustomChannel(targetStr + port, creds, args));
        clients.push_back(client);
    }
    cout << endl << "Start " << numClients << " streaming tests" << endl;
    PerformNStreamTest(clients, 1000);
    PerformNStreamTest(clients, 10000);
    PerformNStreamTest(clients, 100000);
    PerformNStreamTest(clients, 200000);
    PerformNStreamTest(clients, 400000);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunParallelStreamTestSuite(std::string targetStr, std::string& port, std::shared_ptr<grpc::ChannelCredentials> creds)
{
    RunParallelStreamTest(2, targetStr, port, creds);
    RunParallelStreamTest(4, targetStr, port, creds);
    RunParallelStreamTest(8, targetStr, port, creds);
    RunParallelStreamTest(16, targetStr, port, creds);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunParallelatencyStreamTestSuite(NIPerfTestClient& client)
{
    cout << "Start Parallel Stream Latency Test Suite" << endl;
    PerformLatencyStreamTest2(client, client, 1, "streamlatency1Stream.txt");
    PerformLatencyStreamTest2(client, client, 2, "streamlatency1Stream.txt");
    PerformLatencyStreamTest2(client, client, 3, "streamlatency1Stream.txt");
    PerformLatencyStreamTest2(client, client, 4, "streamlatency4Stream.txt");
    PerformLatencyStreamTest2(client, client, 5, "streamlatency4Stream.txt");
}
    
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunMessagePerformanceTestSuite(NIPerfTestClient& client)
{
    cout << "Start Message Performance Test Suite" << endl;
    PerformMessagePerformanceTest(client);
}
    
//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunSteamingTestSuite(NIPerfTestClient& client)
{
    cout << "Start Streaming Test Suite" << endl;
    PerformStreamingTest(client, 10);
    PerformStreamingTest(client, 100);
    PerformStreamingTest(client, 1000);
    PerformStreamingTest(client, 10000);
    PerformStreamingTest(client, 100000);
    // PerformStreamingTest(client, 100000);
    // PerformStreamingTest(client, 100000);
    // PerformStreamingTest(client, 100000);
     PerformStreamingTest(client, 200000);
     PerformStreamingTest(client, 400000);
     PerformStreamingTest(client, 1000000);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunReadTestSuite(NIPerfTestClient& client)
{
    cout << "Start Read Test Suite" << endl;
    PerformReadTest(client, 100, 100000);
    PerformReadTest(client, 1000, 100000);
    PerformReadTest(client, 10000, 100000);
    PerformReadTest(client, 100000, 100000);
    PerformReadTest(client, 200000, 100000);
    PerformReadTest(client, 393216, 100000);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunWriteTestSuite(NIPerfTestClient& client)
{
    cout << "Start Write Test Suite" << endl;
    PerformWriteTest(client, 100);
    PerformWriteTest(client, 1000);
    PerformWriteTest(client, 10000);
    PerformWriteTest(client, 100000);
    PerformWriteTest(client, 200000);
    PerformWriteTest(client, 393216);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunScpiCompareTestSuite(NIPerfTestClient& client)
{
    cout << "Start SCPI Compare Test Suite" << endl;
    // PerformReadTest(client, 10, 100000);
    // PerformAsyncInitTest(client, 10, 10000);
    PerformScopeLikeRead(client);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int main(int argc, char **argv)
{
    // Configure gRPC
    grpc_init();
    grpc_timer_manager_set_threading(false);
    ::grpc_core::Executor::SetThreadingDefault(false);
    ::grpc_core::Executor::SetThreadingAll(false);

    // Configure enviornment
#ifndef _WIN32    
    sched_param schedParam;
    schedParam.sched_priority = 95;
    sched_setscheduler(0, SCHED_FIFO, &schedParam);

    // cpu_set_t cpuSet;
    // CPU_ZERO(&cpuSet);
    // CPU_SET(4, &cpuSet);
    // sched_setaffinity(0, sizeof(cpu_set_t), &cpuSet);
#else
    DWORD dwError, dwPriClass;
    if(!SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS))
    {
        dwError = GetLastError();
        if( ERROR_PROCESS_MODE_ALREADY_BACKGROUND == dwError)
            cout << "Already in background mode" << endl;
        else
            cout << "Failed change priority: " << dwError << endl;
   } 
#endif

    // Get server information and channel credentials
    auto target_str = GetServerAddress(argc, argv);
    auto creds = CreateCredentials(argc, argv);
    std::string port = ":" + GetServerPort(argc, argv);

    cout << "Target: " << target_str << " Port: " << port << endl;

    // Create the connection to the server
    grpc::ChannelArguments args;
    args.SetInt(GRPC_ARG_MINIMAL_STACK, 1);
    args.SetMaxReceiveMessageSize(10 * 100 * 1024 * 1024);
    args.SetMaxSendMessageSize(10 * 100 * 1024 * 1024);
    auto client = new NIPerfTestClient(grpc::CreateCustomChannel(target_str + port, creds, args));

    // Verify the client is working correctly
    auto result = client->Init(42);
    if (result != 42)
    {
        cout << "Server communication failure!" << endl;
        return -1;
    }
    else
    {
        cout << "Init result: " << result << endl;
    }

    // Run desired test suites
    RunMessageLatencyTestSuite(*client);
    return 0;   
}
