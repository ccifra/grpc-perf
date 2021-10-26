//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include <grpcpp/grpcpp.h>
#include <perftest.grpc.pb.h>
#include <memory>

//---------------------------------------------------------------------
//---------------------------------------------------------------------
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using namespace std;
using namespace niPerfTest;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
using timeVector = vector<chrono::microseconds>;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
struct AsyncInitResults
{
    Status status;
    ClientContext context;
    InitResult reply;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class NIPerfTestClient
{
public:
    NIPerfTestClient(shared_ptr<Channel> channel);

public:
    int Init(int id);
    int Init(int id, string command);
    int InitAsync(int id, string command, grpc::CompletionQueue& cq,  AsyncInitResults* results);
    int Read(double timeout, int numSamples, double* samples);
    int TestWrite(int numSamples, double* samples);
    unique_ptr<grpc::ClientReader<niPerfTest::ReadContinuouslyResult>> ReadContinuously(grpc::ClientContext* context, double timeout, int numSamples);
public:
    unique_ptr<niPerfTestService::Stub> m_Stub;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class NIMonikerClient
{
public:
    NIMonikerClient(shared_ptr<Channel> channel);
public:
    unique_ptr<MonikerService::Stub> m_Stub;

};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void WriteLatencyData(timeVector times, const string& fileName);
void ReadSamples(NIPerfTestClient* client, int numSamples);
void ReportMBPerSecond(chrono::high_resolution_clock::time_point start, chrono::high_resolution_clock::time_point end, int numSamples);
void EnableTracing();
void DisableTracing();
void TracingOff();
void TracingOn();
void TraceMarker(const char* marker);