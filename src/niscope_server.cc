//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include <thread>
#include <sstream>
#include <fstream>
#include <iostream>
#include <niscope_server.h>
#include <niScope.grpc.pb.h>
#include <thread>
#include <sched.h>

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
using namespace std;
using namespace niScope;


static ::grpc::ServerWriter< ::niScope::StreamLatencyServer>* _writer;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class NIScope
{
public:
    NIScope(shared_ptr<Channel> channel);

public:
    int Init(int id);
    int InitWithOptions(string resourceName, bool idQuery, bool resetDevice, string options, ViSession* session);
    int ConfigureHorizontalTiming(ViSession session, double minSampleRate, int numPoints, double refPosition, int numRecords, bool enforceRealtime);
    int AutoSetup(ViSession session);
    int Read(ViSession session, string channels, double timeout, int numSamples, double* samples, ScopeWaveformInfo* waveformInfo);
    int TestWrite(int numSamples, double* samples);
    unique_ptr<grpc::ClientReader<niScope::ReadContinuouslyResult>> ReadContinuously(grpc::ClientContext* context, ViSession session, string channels, double timeout, int numSamples);
public:
    unique_ptr<niScopeService::Stub> m_Stub;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
NIScope::NIScope(shared_ptr<Channel> channel)
    : m_Stub(niScopeService::NewStub(channel))
{        
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int NIScope::Init(int id)
{
    InitParameters request;
    request.set_id(id);

    ClientContext context;
    InitResult reply;
    Status status = m_Stub->Init(&context, request, &reply);
    if (!status.ok())
    {
        cout << status.error_code() << ": " << status.error_message() << endl;
    }
    return reply.status();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int NIScope::InitWithOptions(string resourceName, bool idQuery, bool resetDevice, string options, ViSession* session)
{
    InitWithOptionsParameters request;
    request.set_resourcename(resourceName.c_str());
    request.set_idquery(idQuery);
    request.set_resetdevice(resetDevice);
    request.set_optionstring(options.c_str());

    ClientContext context;
    InitWithOptionsResult reply;
    Status status = m_Stub->InitWithOptions(&context, request, &reply);
    if (!status.ok())
    {
        cout << status.error_code() << ": " << status.error_message() << endl;
    }
    *session = reply.newvi();
    return reply.status();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int NIScope::ConfigureHorizontalTiming(ViSession session, double minSampleRate, int numPoints, double refPosition, int numRecords, bool enforceRealtime)
{    
    ConfigureHorizontalTimingParameters request;
    auto requestSession = new ViSession;
    requestSession->set_id(session.id());
    request.set_allocated_vi(requestSession);
    request.set_minsamplerate(minSampleRate);
    request.set_minnumpts(numPoints);
    request.set_refposition(refPosition);
    request.set_numrecords(numRecords);
    request.set_enforcerealtime(enforceRealtime);

    ClientContext context;
    ConfigureHorizontalTimingResult reply;
    Status status = m_Stub->ConfigureHorizontalTiming(&context, request, &reply);
    if (!status.ok())
    {
        cout << status.error_code() << ": " << status.error_message() << endl;
    }
    return reply.status();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int NIScope::AutoSetup(ViSession session)
{
    AutoSetupParameters request;
    auto requestSession = new ViSession;
    requestSession->set_id(session.id());
    request.set_allocated_vi(requestSession);

    ClientContext context;
    AutoSetupResult reply;
    Status status = m_Stub->AutoSetup(&context, request, &reply);
    if (!status.ok())
    {
        cout << status.error_code() << ": " << status.error_message() << endl;
    }
    return reply.status();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int NIScope::Read(ViSession session, string channels, double timeout, int numSamples, double* samples, ScopeWaveformInfo* waveformInfo)
{
    ReadParameters request;
    auto requestSession = new ViSession;
    requestSession->set_id(session.id());
    request.set_allocated_vi(requestSession);
    request.set_channellist(channels.c_str());
    request.set_timeout(timeout);
    request.set_numsamples(numSamples);

    ClientContext context;
    ReadResult reply;
    Status status = m_Stub->Read(&context, request, &reply);    
    if (!status.ok())
    {
        cout << status.error_code() << ": " << status.error_message() << endl;
    }
    memcpy(samples, reply.wfm().data(), numSamples * sizeof(double));
    //*waveformInfo = reply.wfminfo()[0];
    return reply.status();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int NIScope::TestWrite(int numSamples, double* samples)
{   
    TestWriteParameters request;
    request.mutable_wfm()->Reserve(numSamples);
    request.mutable_wfm()->Resize(numSamples, 0);

    ClientContext context;
    TestWriteResult reply;
    auto status = m_Stub->TestWrite(&context, request, &reply);
    if (!status.ok())
    {
        cout << status.error_code() << ": " << status.error_message() << endl;
    }
    return reply.status();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
unique_ptr<grpc::ClientReader<niScope::ReadContinuouslyResult>> NIScope::ReadContinuously(grpc::ClientContext* context, ViSession session, string channels, double timeout, int numSamples)
{    
    ReadContinuouslyParameters request;
    auto requestSession = new ViSession;
    requestSession->set_id(session.id());
    request.set_allocated_vi(requestSession);
    request.set_channellist(channels.c_str());
    request.set_timeout(timeout);
    request.set_numsamples(numSamples);

    return m_Stub->ReadContinuously(context, request);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
::grpc::Status NIScopeServer::StreamLatencyTestClient(::grpc::ServerContext* context, ::grpc::ServerReader<::niScope::StreamLatencyClient>* reader, ::niScope::StreamLatencyServer* response)
{	
	niScope::StreamLatencyClient client;
	niScope::StreamLatencyServer server;
	while (reader->Read(&client))
	{
		_writer->Write(server);
	}
	_writer = nullptr;
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
::grpc::Status NIScopeServer::StreamLatencyTestServer(::grpc::ServerContext* context, const ::niScope::StreamLatencyClient* request, ::grpc::ServerWriter< ::niScope::StreamLatencyServer>* writer)
{
	_writer = writer;
	while (_writer == writer)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIScopeServer::StreamLatencyTest(ServerContext* context, grpc::ServerReaderWriter<niScope::StreamLatencyServer, niScope::StreamLatencyClient>* stream)
{
	niScope::StreamLatencyClient client;
	niScope::StreamLatencyServer server;
	while (stream->Read(&client))
	{
		stream->Write(server);
	}
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIScopeServer::Init(ServerContext* context, const niScope::InitParameters* request, niScope::InitResult* response)
{
	response->set_status(request->id());
	return Status::OK;	
}


//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIScopeServer::InitWithOptions(ServerContext* context, const niScope::InitWithOptionsParameters* request, niScope::InitWithOptionsResult* response)
{	
	response->set_status(0);
	niScope::ViSession* session = new niScope::ViSession();
	session->set_id(1);
	response->set_allocated_newvi(session);
	response->set_status(0);
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIScopeServer::Read(ServerContext* context, const niScope::ReadParameters* request, niScope::ReadResult* response)
{	
	response->mutable_wfm()->Reserve(request->numsamples());
	response->mutable_wfm()->Resize(request->numsamples(), 0.0);
	response->set_status(0);
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIScopeServer::TestWrite(ServerContext* context, const niScope::TestWriteParameters* request, niScope::TestWriteResult* response)
{
	response->set_status(0);
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIScopeServer::ReadContinuously(ServerContext* context, const niScope::ReadContinuouslyParameters* request, grpc::ServerWriter<niScope::ReadContinuouslyResult>* writer)
{			
	niScope::ReadContinuouslyResult response;
	response.mutable_wfm()->Reserve(request->numsamples());
	response.mutable_wfm()->Resize(request->numsamples(), 0.0);
	cout << "Writing " << request->numsamples() << " 10000 times" << endl;
	for (int x=0; x<10000; ++x)
	{
		writer->Write(response);
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

std::shared_ptr<grpc::Channel> _inProcServer;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunServer(int argc, char **argv, const char* saddress)
{
	auto server_address = saddress; //GetServerAddress(argc, argv);
	auto creds = CreateCredentials(argc, argv);

	NIScopeServer service;
	grpc::EnableDefaultHealthCheckService(true);
	grpc::reflection::InitProtoReflectionServerBuilderPlugin();

	ServerBuilder builder;
	builder.SetDefaultCompressionAlgorithm(GRPC_COMPRESS_NONE);
	// Listen on the given address without any authentication mechanism.
	builder.AddListeningPort(server_address, creds);
	builder.SetMaxMessageSize(4 * 1024 * 1024);
	builder.SetMaxReceiveMessageSize(4 * 1024 * 1024);
	// Register "service" as the instance through which we'll communicate with
	// clients. In this case it corresponds to an *synchronous* service.
	builder.RegisterService(&service);

	// Finally assemble the server.
	auto server = builder.BuildAndStart();
	cout << "Server listening on " << server_address << endl;

	_inProcServer = server->InProcessChannel(grpc_impl::ChannelArguments());

	server->Wait();
}

using timeVector = vector<chrono::microseconds>;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformLatencyStreamTest2(NIScope& client, std::string fileName)
{    
    int iterations = 100000;

    cout << "Start RPC Stream latency test, iterations=" << iterations << endl;

    timeVector times;
    times.reserve(iterations);

	niScope::StreamLatencyClient clientData;
	niScope::StreamLatencyServer serverData;
	niScope::StreamLatencyServer serverResponseData;

    ClientContext context;
    auto rstream = client.m_Stub->StreamLatencyTestServer(&context, clientData);

    std::this_thread::sleep_for(std::chrono::milliseconds(1500));

    ClientContext context2;
    auto wstream = client.m_Stub->StreamLatencyTestClient(&context2, &serverResponseData);

    for (int x=0; x<iterations; ++x)
    {
        auto start = chrono::steady_clock::now();
        wstream->Write(clientData);
        rstream->Read(&serverData);
        auto end = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
        times.emplace_back(elapsed);
    }

    {
    std::ofstream fout;
    fout.open("xaxis");
    for (int x=0; x<iterations; ++x)
        fout << (x+1) << std::endl;
    fout.close();
    }

    std::ofstream fout;
    fout.open(fileName);
    for (auto i : times)
        fout << i.count() << std::endl;
    fout.close();

    std::sort(times.begin(), times.end());
    auto min = times.front();
    auto max = times.back();
    auto median = *(times.begin() + iterations / 2);
    
    double average = times.front().count();
    for (auto i : times)
        average += (double)i.count();
    average = average / iterations;

    cout << "End Test" << endl;
    cout << "Min: " << min.count() << endl;
    cout << "Max: " << max.count() << endl;
    cout << "Median: " << median.count() << endl;
    cout << "Average: " << average << endl;
    cout << endl;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int main(int argc, char **argv)
{
    sched_param schedParam;
    schedParam.sched_priority = 99;
    sched_setscheduler(0, SCHED_FIFO, &schedParam);

    cpu_set_t cpuSet;
    CPU_ZERO(&cpuSet);
    CPU_SET(1, &cpuSet);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuSet);

	//auto thread1 = new std::thread(RunServer, argc, argv, "unix:///home/chrisc/test.sock");
	auto thread1 = new std::thread(RunServer, argc, argv, "0.0.0.0:50051");
	//auto thread2 = new std::thread(RunServer, argc, argv, "0.0.0.0:50052");
	//auto thread2 = new std::thread(RunServer, argc, argv, "unix:///home/chrisc/test2.sock");
//	auto thread3 = new std::thread(RunServer, argc, argv, "0.0.0.0:50053");
	//auto thread4 = new std::thread(RunServer, argc, argv, "0.0.0.0:50054");
	thread1->join();

	// std::this_thread::sleep_for(std::chrono::milliseconds(500));
	// auto scope = NIScope(_inProcServer);
	// PerformLatencyStreamTest2(scope, "inprocess.txt");
	//thread2->join();
	//thread3->join();
	//thread4->join();
	return 0;
}
