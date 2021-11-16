//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include <client_utilities.h>
#include <performance_tests.h>
#include <thread>
#include <sstream>
#include <fstream>
#include <iostream>
#include <perftest_server.h>
#include <perftest.grpc.pb.h>
#include <src/core/lib/iomgr/executor.h>
#include <src/core/lib/iomgr/timer_manager.h>
#include <thread>

#ifndef _WIN32
#include <sched.h>
#endif

//---------------------------------------------------------------------
//---------------------------------------------------------------------
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerWriter;

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
using namespace niPerfTest;
using namespace google::protobuf;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
using timeVector = vector<chrono::microseconds>;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
static ::grpc::ServerWriter<niPerfTest::StreamLatencyServer>* _writers [32];

//---------------------------------------------------------------------
//---------------------------------------------------------------------
static bool useAnyType;
static std::shared_ptr<grpc::Channel> _inProcServer;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
grpc::Status NIPerfTestServer::StreamLatencyTestClient(grpc::ServerContext* context, grpc::ServerReader<niPerfTest::StreamLatencyClient>* reader, niPerfTest::StreamLatencyServer* response)
{	
	niPerfTest::StreamLatencyClient client;
	niPerfTest::StreamLatencyServer server;
    uint32_t slot;
    bool first = true;
	while (reader->Read(&client))
	{
        slot = client.message();
        if (first)
        {
            first = false;
#ifndef _WIN32    
            cpu_set_t cpuSet;
            CPU_ZERO(&cpuSet);
            CPU_SET(slot, &cpuSet);
            sched_setaffinity(0, sizeof(cpu_set_t), &cpuSet);
#endif
        }
        if (_writers[slot] != nullptr)
        {
            _writers[slot]->Write(server);
        }
 	}
     _writers[slot] = nullptr;
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
grpc::Status NIPerfTestServer::StreamLatencyTestServer(grpc::ServerContext* context, const niPerfTest::StreamLatencyClient* request, grpc::ServerWriter<niPerfTest::StreamLatencyServer>* writer)
{
    auto slot = request->message();
    _writers[slot] = writer;
    while (_writers[slot] == writer)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestServer::StreamLatencyTest(ServerContext* context, grpc::ServerReaderWriter<niPerfTest::StreamLatencyServer, niPerfTest::StreamLatencyClient>* stream)
{
	niPerfTest::StreamLatencyClient client;
	niPerfTest::StreamLatencyServer server;
	while (stream->Read(&client))
	{
        //server.set_message(client.message());
		stream->Write(server);
	}
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestServer::Init(ServerContext* context, const niPerfTest::InitParameters* request, niPerfTest::InitResult* response)
{
	response->set_status(request->id());
	return Status::OK;	
}


//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestServer::ConfigureVertical(grpc::ServerContext* context, const niPerfTest::ConfigureVerticalRequest* request, niPerfTest::ConfigureVerticalResponse* response)
{    
	return Status::OK;	
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestServer::ConfigureHorizontalTiming(grpc::ServerContext* context, const niPerfTest::ConfigureHorizontalTimingRequest* request, niPerfTest::ConfigureHorizontalTimingResponse* response)
{    
	return Status::OK;	
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestServer::InitiateAcquisition(grpc::ServerContext* context, const niPerfTest::InitiateAcquisitionRequest* request, niPerfTest::InitiateAcquisitionResponse* response)
{    
	return Status::OK;	
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestServer::Read(ServerContext* context, const niPerfTest::ReadParameters* request, niPerfTest::ReadResult* response)
{	
	response->mutable_samples()->Reserve(request->numsamples());
	response->mutable_samples()->Resize(request->numsamples(), 8.325793493);
	response->set_status(0);
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestServer::TestWrite(ServerContext* context, const niPerfTest::TestWriteParameters* request, niPerfTest::TestWriteResult* response)
{
	response->set_status(0);
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestServer::TestWriteContinuously(ServerContext* context, grpc::ServerReaderWriter<niPerfTest::TestWriteResult, niPerfTest::TestWriteParameters>* stream)
{
    niPerfTest::TestWriteParameters readParameters;
    niPerfTest::TestWriteResult response;    
    response.set_status(0);
    while (stream->Read(&readParameters))
    {
        stream->Write(response);
    }
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIPerfTestServer::ReadContinuously(ServerContext* context, const niPerfTest::ReadContinuouslyParameters* request, grpc::ServerWriter<niPerfTest::ReadContinuouslyResult>* writer)
{			
	niPerfTest::ReadContinuouslyResult response;
	response.mutable_wfm()->Reserve(request->numsamples());
	response.mutable_wfm()->Resize(request->numsamples(), 0.0);
    auto iterations = request->numiterations();
    if (iterations == 0)
    {
        iterations = 10000;
    }
	for (int x=0; x<iterations; ++x)
	{
		writer->Write(response);
	}
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status MonikerServer::InitiateMonikerStream(grpc::ServerContext* context, const niPerfTest::MonikerList* request, niPerfTest::MonikerStreamId* response)
{
    useAnyType = request->useanytype();    
    response->set_streamid(1);
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status MonikerServer::StreamReadWrite(grpc::ServerContext* context, grpc::ServerReaderWriter<MonikerReadResult, MonikerWriteRequest>* stream)
{    
	MonikerWriteRequest client;
    Arena areana;
	while (stream->Read(&client))
	{
        auto server = Arena::CreateMessage<MonikerReadResult>(&areana);
        for (int x=0; x<client.values().size(); ++x)
        {
            auto newValue = server->add_values();
            if (useAnyType)
            {
                auto writeValue = Arena::CreateMessage<StreamLatencyClient>(&areana);
                client.values()[x].values().UnpackTo(writeValue);

                auto readValue = Arena::CreateMessage<StreamLatencyServer>(&areana);
                readValue->set_message(writeValue->message());

                auto anyValue = Arena::CreateMessage<Any>(&areana);
                anyValue->PackFrom(*readValue);
                newValue->set_allocated_values(anyValue);
            }
            else
            {                
                auto doubleValue = client.values()[x].doublearray().values(0);

                auto doubleArray = Arena::CreateMessage<DoubleArray>(&areana);
                doubleArray->add_values(doubleValue);
                newValue->set_allocated_doublearray(doubleArray);
            }
        }
		stream->Write(*server);
        areana.Reset();
	}
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
string GetServerAddress(int argc, char** argv)
{
    string target_str = "0.0.0.0:50051";
    string arg_str("--address");
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
                cout << "The only correct argument syntax is --address=" << endl;
            }
        }
        else
        {
            cout << "The only acceptable argument is --address=" << endl;
        }
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
std::string read_keycert( const std::string& filename)
{	
	std::string data;
	std::ifstream file(filename.c_str(), std::ios::in);
	if (file.is_open())
	{
		std::stringstream ss;
		ss << file.rdbuf();
		file.close();
		data = ss.str();
	}
	return data;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
std::shared_ptr<grpc::ServerCredentials> CreateCredentials(int argc, char **argv)
{
	auto certPath = GetCertPath(argc, argv);

	std::shared_ptr<grpc::ServerCredentials> creds;
	if (!certPath.empty())
	{
		std::string servercert = read_keycert(certPath + ".crt");
		std::string serverkey = read_keycert(certPath + ".key");

		grpc::SslServerCredentialsOptions::PemKeyCertPair pkcp;
		pkcp.private_key = serverkey;
		pkcp.cert_chain = servercert;

		grpc::SslServerCredentialsOptions ssl_opts;
		ssl_opts.pem_root_certs="";
		ssl_opts.pem_key_cert_pairs.push_back(pkcp);

		creds = grpc::SslServerCredentials(ssl_opts);
	}
	else
	{
		creds = grpc::InsecureServerCredentials();
	}
	return creds;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunServer(int argc, char **argv, const char* server_address)
{
    // Init gRPC
    grpc_init();
    grpc_timer_manager_set_threading(false);
    // grpc_core::Executor::SetThreadingDefault(false);
    // grpc_core::Executor::SetThreadingAll(false);

	auto creds = CreateCredentials(argc, argv);

	NIPerfTestServer service;
    MonikerServer monikerService;
	ServerBuilder builder;
	builder.AddListeningPort(server_address, creds);
    builder.AddChannelArgument(GRPC_ARG_MINIMAL_STACK, 1);
	builder.SetDefaultCompressionAlgorithm(GRPC_COMPRESS_NONE);
	builder.SetMaxMessageSize(1 * 1024 * 1024);
	builder.SetMaxReceiveMessageSize(1 * 1024 * 1024);
    // GRPC_ARG_ENABLE_CHANNELZ
    // GRPC_ARG_ENABLE_CENSUS
    // GRPC_ARG_DISABLE_CLIENT_AUTHORITY_FILTER
    // GRPC_ARG_ENABLE_DEADLINE_CHECKS
    // GRPC_ARG_ENABLE_LOAD_REPORTING
    // GRPC_ARG_MINIMAL_STACK
    // GRPC_ARG_ENABLE_PER_MESSAGE_DECOMPRESSION
    // GRPC_ARG_ENABLE_RETRIES
    // GRPC_ARG_HTTP2_BDP_PROBE
    // GRPC_ARG_INHIBIT_HEALTH_CHECKING
    // GRPC_ARG_TCP_TX_ZEROCOPY_ENABLED
    // builder.AddChannelArgument();
	builder.RegisterService(&service);
    builder.RegisterService(&monikerService);

	// Assemble the server.
	auto server = builder.BuildAndStart();
	_inProcServer = server->InProcessChannel(grpc_impl::ChannelArguments());
	cout << "Server listening on " << server_address << endl;
	server->Wait();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int main(int argc, char **argv)
{
    //grpc_init();
    //grpc_timer_manager_set_threading(false);
    //grpc_core::Executor::SetThreadingDefault(false);
    //grpc_core::Executor::SetThreadingAll(false);

#ifndef _WIN32    
    sched_param schedParam;
    schedParam.sched_priority = 95;
    sched_setscheduler(0, SCHED_FIFO, &schedParam);

    cpu_set_t cpuSet;
    CPU_ZERO(&cpuSet);
    CPU_SET(2, &cpuSet);
    sched_setaffinity(1, sizeof(cpu_set_t), &cpuSet);
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

    std::vector<thread*> threads;
    std::vector<string> ports;
    for (int x=0; x<1; ++x)
    {
        auto port = 50051 + x;
        auto portStr = string("0.0.0.0:") + to_string(port);
        ports.push_back(portStr);
    } 
    for (auto port: ports)
    {
        auto p = new string(port.c_str());
        auto t = new std::thread(RunServer, 0, argv, p->c_str());
        threads.push_back(t);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(2000));

    // localhost testing
    auto target_str = std::string("localhost");
    auto creds = grpc::InsecureChannelCredentials();
    auto port = ":50051";
    ::grpc::ChannelArguments args;
    args.SetInt(GRPC_ARG_MINIMAL_STACK, 1);
    //auto client = new NIPerfTestClient(grpc::CreateCustomChannel(target_str + port, creds, args));
    // auto client = new NIPerfTestClient(grpc::CreateCustomChannel(target_str + port, creds, args));
    
    // inprocess server
    //auto client = new NIPerfTestClient(_inProcServer);

    // auto result = client->Init(42);
    // cout << "Init result: " << result << endl;
    // result = client->Init(43);
    // cout << "Init result: " << result << endl;
    // result = client->Init(44);
    // cout << "Init result: " << result << endl;

    // cout << "Start streaming tests" << endl;
    //PerformStreamingTest(*client, 100000);

    //PerformLatencyStreamTest(*client, "streamlatency1.txt");
    //cout << "Performing streaming test" << endl;
    //PerformStreamingTest(*client, 100000);
    
    for (auto t: threads)
    {
        t->join();
    }
	return 0;
}
