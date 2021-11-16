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
    int ConfigureVertical(string vi, string channelList, double range, double offset, VerticalCoupling coupling, double probe_attenuation, bool enabled);
    int ConfigureHorizontalTiming(string vi, double min_sample_rate, int min_num_pts, double ref_position, int num_records, bool enforce_realtime);
    int InitiateAcquisition(string vi);
    int Read(double timeout, int numSamples, double* samples);
    int TestWrite(int numSamples, double* samples);
    unique_ptr<grpc::ClientReader<niPerfTest::ReadContinuouslyResult>> ReadContinuously(grpc::ClientContext* context, double timeout, int numSamples, int numIterations);
public:
    unique_ptr<niPerfTestService::Stub> m_Stub;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void WriteLatencyData(timeVector times, const string& fileName);
void ReadSamples(NIPerfTestClient* client, int numSamples, int numIterations);
void ReportMBPerSecond(chrono::high_resolution_clock::time_point start, chrono::high_resolution_clock::time_point end, int numSamples, int numIterations);
void EnableTracing();
void DisableTracing();
void TracingOff();
void TracingOn();
void TraceMarker(const char* marker);