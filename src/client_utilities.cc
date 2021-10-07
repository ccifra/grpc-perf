//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include "client_utilities.h"
#include <fstream>
#include <WinSock2.h>
#include <in6addr.h>
#include <ws2ipdef.h>
#include <Mswsock.h>

//---------------------------------------------------------------------
//---------------------------------------------------------------------
NIPerfTestClient::NIPerfTestClient(shared_ptr<Channel> channel)
    : m_Stub(niPerfTestService::NewStub(channel))
{        
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int NIPerfTestClient::Init(int id)
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
int NIPerfTestClient::Init(int id, string command)
{
    InitParameters request;
    request.set_id(id);
    request.set_command(command);

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
int NIPerfTestClient::InitAsync(int id, string command, grpc::CompletionQueue& cq,  AsyncInitResults* results)
{
    InitParameters request;
    request.set_id(id);
    request.set_command(command);
    
    auto rpc = m_Stub->PrepareAsyncInit(&results->context, request, &cq);
    rpc->StartCall();
    rpc->Finish(&results->reply, &results->status, (void*)results);
    return 0;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int NIPerfTestClient::Read(double timeout, int numSamples, double* samples)
{
    ReadParameters request;
    request.set_timeout(timeout);
    request.set_numsamples(numSamples);

    ClientContext context;
    ReadResult reply;
    Status status = m_Stub->Read(&context, request, &reply);    
    if (!status.ok())
    {
        cout << status.error_code() << ": " << status.error_message() << endl;
    }
    if (reply.samples().size() != numSamples)
    {
        cout << "ERROR, wrong number of samples";
    }
    memcpy(samples, reply.samples().data(), numSamples * sizeof(double));
    return reply.status();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int NIPerfTestClient::TestWrite(int numSamples, double* samples)
{   
    TestWriteParameters request;
    request.mutable_samples()->Reserve(numSamples);
    request.mutable_samples()->Resize(numSamples, 0);

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
unique_ptr<grpc::ClientReader<niPerfTest::ReadContinuouslyResult>> NIPerfTestClient::ReadContinuously(grpc::ClientContext* context, double timeout, int numSamples)
{    
    ReadContinuouslyParameters request;
    request.set_numsamples(numSamples);
    request.set_numiterations(10000);

    return m_Stub->ReadContinuously(context, request);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
NIMonikerClient::NIMonikerClient(shared_ptr<Channel> channel)
    : m_Stub(MonikerService::NewStub(channel))
{
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void WriteLatencyData(timeVector times, const string& fileName)
{
    auto iterations = times.size();

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
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void ReadSamples(NIPerfTestClient* client, int numSamples)
{    
    int index = 0;
    grpc::ClientContext context;
    auto readResult = client->ReadContinuously(&context, 5.0, numSamples);
    ReadContinuouslyResult cresult;
    while(readResult->Read(&cresult))
    {
        cresult.wfm().size();
        index += 1;
    }
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
void EnableTracing()
{
    // std::ofstream fout;
    // fout.open("/sys/kernel/debug/tracing/events/enable");
    // fout << "1";
    // fout.close();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void DisableTracing()
{
    // std::ofstream fout;
    // fout.open("/sys/kernel/debug/tracing/events/enable");
    // fout << "0";
    // fout.close();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void TracingOff()
{
    // std::ofstream fout;
    // fout.open("/sys/kernel/debug/tracing/tracing_on");
    // fout << "0";
    // fout.close();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void TracingOn()
{
    // std::ofstream fout;
    // fout.open("/sys/kernel/debug/tracing/tracing_on");
    // fout << "1";
    // fout.close();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void TraceMarker(const char* marker)
{
    // std::ofstream fout;
    // fout.open("/sys/kernel/debug/tracing/trace_marker");
    // fout << marker;
    // fout.close();
}
