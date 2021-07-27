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

#ifndef _WIN32
#include <sched.h>
#endif

//---------------------------------------------------------------------
//---------------------------------------------------------------------
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using namespace std;
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
string GetCertPath(int argc, char** argv)
{
    string cert_str;
    string arg_str("--cert");
    if (argc > 2)
    {
        string arg_val = argv[2];
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
int main(int argc, char **argv)
{
    grpc_init();
    grpc_timer_manager_set_threading(false);
    ::grpc_core::Executor::SetThreadingDefault(false);
    ::grpc_core::Executor::SetThreadingAll(false);

#ifndef _WIN32    
    sched_param schedParam;
    schedParam.sched_priority = 95;
    sched_setscheduler(0, SCHED_FIFO, &schedParam);

    cpu_set_t cpuSet;
    CPU_ZERO(&cpuSet);
    CPU_SET(4, &cpuSet);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuSet);
#endif

    auto target_str = GetServerAddress(argc, argv);
    auto creds = CreateCredentials(argc, argv);
    auto port = ":50051";   

    //client1 = new NIScope(grpc::CreateChannel("unix:///home/chrisc/test.sock", creds));
    //client2 = new NIScope(grpc::CreateChannel("unix:///home/chrisc/test2.sock", creds));
    //client2 = new NIScope(grpc::CreateChannel(target_str + port, creds));

    ::grpc::ChannelArguments args;
    // Set a dummy (but distinct) channel arg on each channel so that
    // every channel gets its own connection
    args.SetInt(GRPC_ARG_MINIMAL_STACK, 1);
    auto client1 = new NIPerfTestClient(grpc::CreateCustomChannel(target_str + port, creds, args));
    //client2 = new NIScope(grpc::CreateCustomChannel(target_str + port, creds, args));

    auto result = client1->Init(42);
    cout << "Init result: " << result << endl;

    // EnableTracing();
    //PerformLatencyStreamTest(*client1, "streamlatency1.txt");
    // PerformLatencyStreamTest(*client1, "streamlatency2.txt");
    // PerformLatencyStreamTest(*client1, "streamlatency3.txt");
    // PerformLatencyStreamTest(*client1, "streamlatency4.txt");
    // PerformLatencyStreamTest(*client1, "streamlatency5.txt");
    // DisableTracing();

    // cout << "Start moniker latency read write tests" << endl;
    // auto monikerClient = new NIMonikerClient(grpc::CreateCustomChannel(target_str + port, creds, args));
    // PerformMonikerLatencyReadWriteTest(*monikerClient, 1, false, "monikerlatency1.txt");
    // PerformMonikerLatencyReadWriteTest(*monikerClient, 2, false, "monikerlatency2.txt");
    // PerformMonikerLatencyReadWriteTest(*monikerClient, 3, false, "monikerlatency3.txt");
    // PerformMonikerLatencyReadWriteTest(*monikerClient, 4, false, "monikerlatency4.txt");
    // PerformMonikerLatencyReadWriteTest(*monikerClient, 5, false, "monikerlatency5.txt");
    // PerformMonikerLatencyReadWriteTest(*monikerClient, 5, true, "monikerlatencyany5.txt");

    // PerformMessageLatencyTest(*client1, "latency1.txt");
    // PerformMessageLatencyTest(*client1, "latency2.txt");
    // PerformMessageLatencyTest(*client1, "latency3.txt");
    // PerformMessageLatencyTest(*client1, "latency4.txt");
    // PerformMessageLatencyTest(*client1, "latency5.txt");

    // cout << "Start latency payload write tests" << endl;
    // PerformLatencyPayloadWriteTest(*client1, 1, "payloadlatency1.txt");
    // PerformLatencyPayloadWriteTest(*client1, 8, "payloadlatency8.txt");
    // PerformLatencyPayloadWriteTest(*client1, 16, "payloadlatency16.txt");
    // PerformLatencyPayloadWriteTest(*client1, 32, "payloadlatency32.txt");
    // PerformLatencyPayloadWriteTest(*client1, 64, "payloadlatency64.txt");
    // PerformLatencyPayloadWriteTest(*client1, 128, "payloadlatency128.txt");
    // PerformLatencyPayloadWriteTest(*client1, 1024, "payloadlatency1024.txt");
    // PerformLatencyPayloadWriteTest(*client1, 32768, "payloadlatency32768.txt");

    // PerformLatencyPayloadWriteStreamTest(*client1, 1, "payloadstreamlatency1.txt");
    // PerformLatencyPayloadWriteStreamTest(*client1, 8, "payloadstreamlatency8.txt");
    // PerformLatencyPayloadWriteStreamTest(*client1, 16, "payloadstreamlatency16.txt");
    // PerformLatencyPayloadWriteStreamTest(*client1, 32, "payloadstreamlatency32.txt");
    // PerformLatencyPayloadWriteStreamTest(*client1, 64, "payloadstreamlatency64.txt");
    // PerformLatencyPayloadWriteStreamTest(*client1, 128, "payloadstreamlatency128.txt");
    // PerformLatencyPayloadWriteStreamTest(*client1, 1024, "payloadstreamlatency1024.txt");
    // PerformLatencyPayloadWriteStreamTest(*client1, 32768, "payloadstreamlatency32768.txt");

    //PerformMessagePerformanceTest(*client1);

    // client2 = new NIScope(grpc::CreateChannel(target_str + ":50052", creds));
    // result = client2->Init(42);
    // cout << endl << "Start 2 stream streaming tests" << endl;
    // PerformTwoStreamTest(*client1, *client2, 1000);
    // PerformTwoStreamTest(*client1, *client2, 10000);
    // PerformTwoStreamTest(*client1, *client2, 100000);
    // PerformTwoStreamTest(*client1, *client2, 200000);


    // client3 = new NIScope(grpc::CreateChannel(target_str + ":50053", creds));
    // client4 = new NIScope(grpc::CreateChannel(target_str + ":50054", creds));
    // result = client3->Init(32);
    // result = client4->Init(42);
    // cout << endl << "Start 4 stream streaming tests" << endl;
    // PerformFourStreamTest(*client1, *client2, *client3, *client4, 1000);
    // PerformFourStreamTest(*client1, *client2, *client3, *client4, 10000);
    // PerformFourStreamTest(*client1, *client2, *client3, *client4, 100000);
    // PerformFourStreamTest(*client1, *client2, *client3, *client4, 200000);
    
    // std::vector<NIScope*> clients;
    // for (int x=0; x<20; ++x)
    // {
    //     auto port = 50051 + x;        
    //     auto client = new NIScope(grpc::CreateChannel(target_str + string(":") + to_string(port), creds));
    //     clients.push_back(client);
    // }
    // cout << endl << "Start 20 stream streaming tests" << endl;
    // PerformNStreamTest(clients, 1000);
    // PerformNStreamTest(clients, 10000);
    // PerformNStreamTest(clients, 100000);


    // cout << "Start parallel stream latency test" << endl;
    // PerformLatencyStreamTest2(*client1, *client1, 1, "streamlatency1Stream.txt");
    // PerformLatencyStreamTest2(*client1, *client1, 2, "streamlatency1Stream.txt");
    // PerformLatencyStreamTest2(*client1, *client1, 3, "streamlatency1Stream.txt");
    // PerformLatencyStreamTest2(*client1, *client1, 4, "streamlatency4Stream.txt");
    // PerformLatencyStreamTest2(*client1, *client1, 5, "streamlatency4Stream.txt");
    
    // PerformMessagePerformanceTest(*client1);
    
    cout << "Start streaming tests" << endl;
    // PerformStreamingTest(*client1, 10);
    // PerformStreamingTest(*client1, 100);
    // PerformStreamingTest(*client1, 1000);
    // PerformStreamingTest(*client1, 10000);
    // PerformStreamingTest(*client1, 100000);
    PerformStreamingTest(*client1, 100000);
    PerformStreamingTest(*client1, 100000);
    PerformStreamingTest(*client1, 100000);
    // PerformStreamingTest(*client1, 200000);

    // PerformReadTest(*client1, 100);
    // PerformReadTest(*client1, 1000);
    // PerformReadTest(*client1, 10000);
    // PerformReadTest(*client1, 100000);
    // PerformReadTest(*client1, 200000);
    // PerformReadTest(*client1, 393216);
    
    // PerformWriteTest(*client1, 100);
    // PerformWriteTest(*client1, 1000);
    // PerformWriteTest(*client1, 10000);
    // PerformWriteTest(*client1, 100000);
    // PerformWriteTest(*client1, 200000);
    // PerformWriteTest(*client1, 393216);
}
