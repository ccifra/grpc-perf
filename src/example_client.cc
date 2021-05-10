//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <niScope.grpc.pb.h>
#include <sstream>
#include <fstream>
#include <iostream>
#include <thread>
#include <sched.h>

//---------------------------------------------------------------------
//---------------------------------------------------------------------
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using namespace std;
using namespace niScope;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class NIScope
{
public:
    NIScope(shared_ptr<Channel> channel);

public:
    int Init(int id);
    int InitWithOptions(string resourceName, bool idQuery, bool resetDevice, string options, ViSession* session);
    int Read(ViSession session, string channels, double timeout, int numSamples, double* samples, ScopeWaveformInfo* waveformInfo);
    int TestWrite(int numSamples, double* samples);
    unique_ptr<grpc::ClientReader<niScope::ReadContinuouslyResult>> ReadContinuously(grpc::ClientContext* context, ViSession session, string channels, double timeout, int numSamples);
public:
    unique_ptr<niScopeService::Stub> m_Stub;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
static NIScope* client1;
static NIScope* client2;
static NIScope* client3;
static NIScope* client4;

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
static void ReadSamples(NIScope* client, int numSamples)
{    
    ViSession session;
    int index = 0;
    grpc::ClientContext context;
    auto readResult = client->ReadContinuously(&context, session, (char*)"0", 5.0, numSamples);
    ReadContinuouslyResult cresult;
    while(readResult->Read(&cresult))
    {
        cresult.wfm().size();
        index += 1;
    }
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformMessagePerformanceTest(NIScope& client)
{
    cout << "Start Messages per second test" << endl;

    auto start = chrono::steady_clock::now();
    for (int x=0; x<50000; ++x)
    {
        client.Init(42);
    }
    auto end = chrono::steady_clock::now();
    auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
    double msgsPerSecond = (50000.0 * 1000.0 * 1000.0) / (double)elapsed.count();

    cout << "Result: " << msgsPerSecond << " messages/s" << endl << endl;
}

void EnableTracing()
{
    // std::ofstream fout;
    // fout.open("/sys/kernel/debug/tracing/events/enable");
    // fout << "1";
    // fout.close();
}

void DisableTracing()
{
    // std::ofstream fout;
    // fout.open("/sys/kernel/debug/tracing/events/enable");
    // fout << "0";
    // fout.close();
}

void TracingOff()
{
    // std::ofstream fout;
    // fout.open("/sys/kernel/debug/tracing/tracing_on");
    // fout << "0";
    // fout.close();
}

void TracingOn()
{
    // std::ofstream fout;
    // fout.open("/sys/kernel/debug/tracing/tracing_on");
    // fout << "1";
    // fout.close();
}

void TraceMarker(const char* marker)
{
    // std::ofstream fout;
    // fout.open("/sys/kernel/debug/tracing/trace_marker");
    // fout << marker;
    // fout.close();
}

using timeVector = vector<chrono::microseconds>;


//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformLatencyStreamTest(NIScope& client, std::string fileName)
{    
    int iterations = 100000;

    cout << "Start RPC Stream latency test, iterations=" << iterations << endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    timeVector times;
    times.reserve(iterations);

	niScope::StreamLatencyClient clientData;
	niScope::StreamLatencyServer serverData;

    ClientContext context;
    auto stream = client.m_Stub->StreamLatencyTest(&context);
    for (int x=0; x<10; ++x)
    {
        stream->Write(clientData);
        stream->Read(&serverData);
    }

    EnableTracing();
    for (int x=0; x<iterations; ++x)
    {
        TraceMarker("Start iteration");
        auto start = chrono::steady_clock::now();
        stream->Write(clientData);
        stream->Read(&serverData);
        auto end = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
        times.emplace_back(elapsed);
        if (elapsed.count() > 200)
        {
            TraceMarker("High Latency");
            DisableTracing();
            cout << "HIGH Latency: " << x << "iterations" << endl;
            break;
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    {
        std::ofstream fout;
        fout.open("xaxis");
        for (int x=0; x<iterations; ++x)
        {
            fout << (x+1) << std::endl;
        }
        fout.close();
    }

    {
        std::ofstream fout;
        fout.open(fileName);
        for (auto i : times)
        {
            fout << i.count() << std::endl;
        }
        fout.close();
    }

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
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformLatencyStreamTest2(NIScope& client, NIScope& client2, std::string fileName)
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
void PerformMessageLatencyTest(NIScope& client, std::string fileName)
{
    int iterations = 100000;

    cout << "Start RPC latency test, iterations=" << iterations << endl;

    timeVector times;
    times.reserve(iterations);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    InitParameters request;
    request.set_id(123);

    InitResult reply;

    for (int x=0; x<10; ++x)
    {
        ClientContext context;
        client.m_Stub->Init(&context, request, &reply);
    }
    for (int x=0; x<iterations; ++x)
    {
        auto start = chrono::steady_clock::now();
        ClientContext context;
        client.m_Stub->Init(&context, request, &reply);
        auto end = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
        times.emplace_back(elapsed);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

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
void PerformReadTest(NIScope& client, int numSamples)
{    
    cout << "Start " << numSamples << " Read Test" << endl;

    ViSession session;
    ScopeWaveformInfo info;
    int index = 0;
    double* samples = new double[numSamples];

    auto start = chrono::steady_clock::now();
    for (int x=0; x<1000; ++x)
    {
        client.Read(session, "", 1000, numSamples, samples, &info);
    }
    auto end = chrono::steady_clock::now();
    auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
    double msgsPerSecond = (1000.0 * 1000.0 * 1000.0) / (double)elapsed.count();

    delete [] samples;
    cout << "Result: " << msgsPerSecond << " reads per second" << endl << endl;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformWriteTest(NIScope& client, int numSamples)
{   
    cout << "Start " << numSamples << " Write Test" << endl;

    ViSession session;
    ScopeWaveformInfo info;
    int index = 0;
    double* samples = new double[numSamples];

    auto start = chrono::steady_clock::now();
    for (int x=0; x<1000; ++x)
    {
        client.TestWrite(numSamples, samples);
        if (x == 0 || x == 2)
        {
            cout << ",";
        }
        if ((x % 20) == 0)
        {
            cout << ".";
        }
    }
    cout << endl;
    auto end = chrono::steady_clock::now();
    auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
    double msgsPerSecond = (1000.0 * 1000.0 * 1000.0) / (double)elapsed.count();

    delete [] samples;
    cout << "Result: " << msgsPerSecond << " reads per second" << endl << endl;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void ReportMBPerSecond(chrono::steady_clock::time_point start, chrono::steady_clock::time_point end, int numSamples)
{
    int64_t elapsed = chrono::duration_cast<chrono::microseconds>(end - start).count();
    double elapsedSeconds = elapsed / (1000.0 * 1000.0);
    double bytesPerSecond = (8.0 * (double)numSamples * 10000) / elapsedSeconds;
    double MBPerSecond = bytesPerSecond / (1024.0 * 1024);

    cout << numSamples << " Samples: " << MBPerSecond << " MB/s, " << elapsed << " total microseconds" << endl;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformStreamingTest(NIScope& client, int numSamples)
{
    auto start = chrono::steady_clock::now();
    ReadSamples(&client, numSamples);
    auto end = chrono::steady_clock::now();
    ReportMBPerSecond(start, end, numSamples);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformTwoStreamTest(NIScope& client, NIScope& client2, int numSamples)
{
    auto start = chrono::steady_clock::now();

    auto thread1 = new thread(ReadSamples, &client, numSamples);
    auto thread2 = new thread(ReadSamples, &client2, numSamples);

    thread1->join();
    thread2->join();

    auto end = chrono::steady_clock::now();
    ReportMBPerSecond(start, end, numSamples * 2);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformFourStreamTest(NIScope& client, NIScope& client2, NIScope& client3, NIScope& client4, int numSamples)
{
    auto start = chrono::steady_clock::now();

    auto thread1 = new thread(ReadSamples, &client, numSamples);
    auto thread2 = new thread(ReadSamples, &client2, numSamples);
    auto thread3 = new thread(ReadSamples, &client3, numSamples);
    auto thread4 = new thread(ReadSamples, &client4, numSamples);

    thread1->join();
    thread2->join();
    thread3->join();
    thread4->join();

    auto end = chrono::steady_clock::now();
    ReportMBPerSecond(start, end, numSamples * 4);
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

    auto target_str = GetServerAddress(argc, argv);
    auto creds = CreateCredentials(argc, argv);

    auto port = ":50051";
    
    //client1 = new NIScope(grpc::CreateChannel("unix:///home/chrisc/test.sock", creds));
    //client2 = new NIScope(grpc::CreateChannel("unix:///home/chrisc/test2.sock", creds));
    //client2 = new NIScope(grpc::CreateChannel(target_str + port, creds));
    client1 = new NIScope(grpc::CreateChannel(target_str + port, creds));

    ViSession session;
    auto result = client1->Init(42);

    cout << "Init result: " << result << endl;

    EnableTracing();
    PerformLatencyStreamTest(*client1, "streamlatency1.txt");
    DisableTracing();

    // PerformLatencyStreamTest(*client1, "streamlatency2.txt");
    // PerformLatencyStreamTest(*client1, "streamlatency3.txt");
    // PerformLatencyStreamTest(*client1, "streamlatency4.txt");
    // PerformLatencyStreamTest(*client1, "streamlatency5.txt");

    //PerformLatencyStreamTest2(*client1, *client2, "streamlatency2Channel.txt");

    // PerformMessageLatencyTest(*client1, "latency1.txt");
    // PerformMessageLatencyTest(*client1, "latency2.txt");
    // PerformMessageLatencyTest(*client1, "latency3.txt");
    // PerformMessageLatencyTest(*client1, "latency4.txt");
    // PerformMessageLatencyTest(*client1, "latency5.txt");

    // PerformMessagePerformanceTest(*client1);

    // cout << "Start streaming tests" << endl;

    // PerformStreamingTest(*client1, 10);
    // PerformStreamingTest(*client1, 100);
    // PerformStreamingTest(*client1, 1000);
    // PerformStreamingTest(*client1, 10000);
    // PerformStreamingTest(*client1, 100000);
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
