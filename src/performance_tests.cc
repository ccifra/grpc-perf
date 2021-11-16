//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include <client_utilities.h>
#include <performance_tests.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <perftest.grpc.pb.h>
#include <sstream>
#include <fstream>
#include <iostream>
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
using std::cout;
using std::endl;
using namespace niPerfTest;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
static int LatencyTestIterations = 300000;
static int DefaultTestIterations = 20000;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformAsyncInitTest(NIPerfTestClient& client, int numCommands, int numIterations)
{
    cout << "Start async init test, num commands: " << numCommands << endl;

    auto start = chrono::high_resolution_clock::now();
    for (int i=0; i<numIterations; ++i)
    {
        AsyncInitResults* results = new AsyncInitResults[numCommands];
        grpc::CompletionQueue cq;
        for (int x=0; x<numCommands; ++x)
        {
            client.InitAsync(42+x, "This is a command", cq, &results[x]);
        }
        std::vector<AsyncInitResults*> completedList;
        while (completedList.size() != numCommands)
        {
            AsyncInitResults* completed;
            bool ok;
            cq.Next((void**)&completed, &ok);
            completedList.push_back(completed);
            if (!ok)
            {
                cout << "Completion Queue returned error!" << endl;
                return;
            }
            if (completed->reply.status() < 42 || completed->reply.status() > (42+numCommands))
            {
                cout << "BAD REPLY!" << endl;
            }
            if (!completed->status.ok())
            {
                cout << completed->status.error_code() << ": " << completed->status.error_message() << endl;
            }
        }
        delete[] results;
    }
    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);

    double msgsPerSecond = (numCommands * numIterations * 1000.0 * 1000.0) / (double)elapsed.count();
    double timePerMessage = elapsed.count() / (numCommands * numIterations);
    cout << "Result: " << msgsPerSecond << " reads per second, Microseconds per read: " << timePerMessage << endl << endl;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformScopeLikeRead(NIPerfTestClient& client)
{
    int samples = 10;
    double* buffer = new double[samples];

    cout << "Start Scope Like Read test" << endl;
    auto start = chrono::high_resolution_clock::now();
    for (int x=0; x<10000; ++x)
    {
        client.ConfigureVertical("", "0",10.0, 0.0, VERTICAL_COUPLING_NISCOPE_VAL_AC, 1.0, true);
        client.ConfigureHorizontalTiming("", 50000000, samples, 50.0, 1, true);
        client.InitiateAcquisition("");
        client.Read(1000, samples, buffer);
    }
    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
    double timePerTest = (double)elapsed.count() / 10000.0;
    delete [] buffer;

    cout << "Result: " << timePerTest << " us Per iteration" << endl << endl;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformMessagePerformanceTest(NIPerfTestClient& client)
{
    cout << "Start Messages per second test" << endl;

    auto start = chrono::high_resolution_clock::now();
    for (int x=0; x<50000; ++x)
    {
        client.Init(42);
    }
    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
    double msgsPerSecond = (50000.0 * 1000.0 * 1000.0) / (double)elapsed.count();

    cout << "Result: " << msgsPerSecond << " messages/s" << endl << endl;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformLatencyStreamTest(NIPerfTestClient& client, std::string fileName)
{    
    cout << "Start RPC Stream latency test, iterations=" << LatencyTestIterations << endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    timeVector times;
    times.reserve(LatencyTestIterations);

	niPerfTest::StreamLatencyClient clientData;
	niPerfTest::StreamLatencyServer serverData;

    ClientContext context;
    auto stream = client.m_Stub->StreamLatencyTest(&context);
    for (int x=0; x<10; ++x)
    {
        stream->Write(clientData);
        stream->Read(&serverData);
    }

    EnableTracing();
    for (int x=0; x<LatencyTestIterations; ++x)
    {
        TraceMarker("Start iteration");
        //clientData.set_message(x);
        auto start = chrono::high_resolution_clock::now();
        stream->Write(clientData);
        stream->Read(&serverData);
        auto end = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
        times.emplace_back(elapsed);
        // if (elapsed.count() > 200)
        // {
        //     TraceMarker("High Latency");
        //     DisableTracing();
        //     cout << "HIGH Latency: " << x << "iterations" << endl;
        //     break;
        // }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    WriteLatencyData(times, fileName);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
}

std::unique_ptr<grpc::ByteBuffer> SerializeToByteBuffer(
    const grpc::protobuf::Message& message)
{
    std::string buf;
    message.SerializeToString(&buf);
    grpc::Slice slice(buf);
    return std::unique_ptr<grpc::ByteBuffer>(new grpc::ByteBuffer(&slice, 1));
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformLatencyPayloadWriteTest(NIPerfTestClient& client, int numSamples, std::string fileName)
{    
    cout << "Start RPC Latency payload write test, iterations=" << LatencyTestIterations << " numSamples=" << numSamples << endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    timeVector times;
    times.reserve(LatencyTestIterations);

    TestWriteParameters request;
    request.mutable_samples()->Reserve(numSamples);
    request.mutable_samples()->Resize(numSamples, 0);
    TestWriteResult reply;

    for (int x=0; x<100; ++x)
    {
        ClientContext context;
        client.m_Stub->TestWrite(&context, request, &reply);
    }
    EnableTracing();
    for (int x=0; x<LatencyTestIterations; ++x)
    {
        TraceMarker("Start iteration");
        auto start = chrono::high_resolution_clock::now();
        ClientContext context;
        client.m_Stub->TestWrite(&context, request, &reply);
        auto end = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
        times.emplace_back(elapsed);
    }
    WriteLatencyData(times, fileName);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformLatencyPayloadWriteStreamTest(NIPerfTestClient& client, int numSamples, std::string fileName)
{    
    cout << "Start RPC Latency payload write stream test, iterations=" << LatencyTestIterations << " numSamples=" << numSamples << endl;

    timeVector times;
    times.reserve(LatencyTestIterations);

    TestWriteParameters request;
    request.mutable_samples()->Reserve(numSamples);
    request.mutable_samples()->Resize(numSamples, 0);
    TestWriteResult reply;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    ClientContext context;
    auto stream = client.m_Stub->TestWriteContinuously(&context);
    for (int x=0; x<100; ++x)
    {
        stream->Write(request);
        stream->Read(&reply);
    }
    EnableTracing();
    for (int x=0; x<LatencyTestIterations; ++x)
    {
        TraceMarker("Start iteration");
        auto start = chrono::high_resolution_clock::now();
        stream->Write(request);
        stream->Read(&reply);
        auto end = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
        times.emplace_back(elapsed);
    }
    WriteLatencyData(times, fileName);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
struct StreamInfo
{
    ClientContext context;
    niPerfTest::StreamLatencyClient clientData;
    std::unique_ptr< ::grpc::ClientReader< ::niPerfTest::StreamLatencyServer>> rstream;

    ClientContext wcontext;
    std::unique_ptr< ::grpc::ClientWriter< ::niPerfTest::StreamLatencyClient>> wstream;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformLatencyStreamTest2(NIPerfTestClient& client, NIPerfTestClient& client2, int streamCount, std::string fileName)
{    
    cout << "Start RPC Stream latency test, streams:" << streamCount << ", iterations=" << LatencyTestIterations << endl;

    timeVector times;
    times.reserve(LatencyTestIterations);

    StreamInfo* streamInfos = new StreamInfo[streamCount];
	niPerfTest::StreamLatencyServer serverData;
	niPerfTest::StreamLatencyServer serverResponseData;

    for (int x=0; x<streamCount; ++x)
    {
        streamInfos[x].clientData.set_message(x);       
        streamInfos[x].rstream = client.m_Stub->StreamLatencyTestServer(&streamInfos[x].context, streamInfos[x].clientData);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        streamInfos[x].wstream = client2.m_Stub->StreamLatencyTestClient(&streamInfos[x].wcontext, &serverResponseData);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    for (int x=0; x<100; ++x)
    {
        for (int i=0; i<streamCount; ++i)
        {
            streamInfos[i].wstream->Write(streamInfos[i].clientData);
        }
        for (int i=0; i<streamCount; ++i)
        {
            streamInfos[i].rstream->Read(&serverData);
        }
    }
    for (int x=0; x<LatencyTestIterations; ++x)
    {
        auto start = chrono::high_resolution_clock::now();
        for (int i=0; i<streamCount; ++i)
        {
            streamInfos[i].wstream->Write(streamInfos[i].clientData);
        }
        for (int i=0; i<streamCount; ++i)
        {
            streamInfos[i].rstream->Read(&serverData);
        }
        auto end = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
        times.emplace_back(elapsed);
    }

    for (int x=0; x<streamCount; ++x)
    {
        streamInfos[x].wstream->WritesDone();
    }

    delete [] streamInfos;
    WriteLatencyData(times, fileName);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformMessageLatencyTest(NIPerfTestClient& client, std::string fileName)
{
    cout << "Start RPC latency test, iterations=" << LatencyTestIterations << endl;

    timeVector times;
    times.reserve(LatencyTestIterations);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    InitParameters request;
    request.set_id(123);

    InitResult reply;

    for (int x=0; x<100; ++x)
    {
        ClientContext context;
        client.m_Stub->Init(&context, request, &reply);
    }
    for (int x=0; x<LatencyTestIterations; ++x)
    {
        auto start = chrono::high_resolution_clock::now();
        ClientContext context;
        client.m_Stub->Init(&context, request, &reply);
        auto end = chrono::high_resolution_clock::now();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
        times.emplace_back(elapsed);
    }
    WriteLatencyData(times, fileName);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformReadTest(NIPerfTestClient& client, int numSamples, int numIterations)
{    
    cout << "Start " << numSamples << " Read Test" << endl;

    int index = 0;
    double* samples = new double[numSamples];

    auto start = chrono::high_resolution_clock::now();
    for (int x=0; x<numIterations; ++x)
    {
        client.Read(1000, numSamples, samples);
    }
    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
    double msgsPerSecond = (numIterations * 1000.0 * 1000.0) / (double)elapsed.count();
    double timePerMessage = elapsed.count() / numIterations;

    delete [] samples;
    cout << "Result: " << msgsPerSecond << " reads per second, Microseconds per read: " << timePerMessage << endl << endl;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformWriteTest(NIPerfTestClient& client, int numSamples)
{   
    cout << "Start " << numSamples << " Write Test" << endl;

    int index = 0;
    double* samples = new double[numSamples];

    auto start = chrono::high_resolution_clock::now();
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
    auto end = chrono::high_resolution_clock::now();
    auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
    double msgsPerSecond = (1000.0 * 1000.0 * 1000.0) / (double)elapsed.count();

    delete [] samples;
    cout << "Result: " << msgsPerSecond << " reads per second" << endl << endl;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformStreamingTest(NIPerfTestClient& client, int numSamples)
{
    auto start = chrono::high_resolution_clock::now();
    ReadSamples(&client, numSamples, DefaultTestIterations);
    auto end = chrono::high_resolution_clock::now();
    ReportMBPerSecond(start, end, numSamples, DefaultTestIterations);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformTwoStreamTest(NIPerfTestClient& client, NIPerfTestClient& client2, int numSamples)
{
    auto start = chrono::high_resolution_clock::now();

    auto thread1 = new thread(ReadSamples, &client, numSamples, DefaultTestIterations);
    auto thread2 = new thread(ReadSamples, &client2, numSamples, DefaultTestIterations);

    thread1->join();
    thread2->join();

    auto end = chrono::high_resolution_clock::now();
    ReportMBPerSecond(start, end, numSamples * 2, DefaultTestIterations);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformFourStreamTest(NIPerfTestClient& client, NIPerfTestClient& client2, NIPerfTestClient& client3, NIPerfTestClient& client4, int numSamples)
{
    auto start = chrono::high_resolution_clock::now();

    auto thread1 = new thread(ReadSamples, &client, numSamples, DefaultTestIterations);
    auto thread2 = new thread(ReadSamples, &client2, numSamples, DefaultTestIterations);
    auto thread3 = new thread(ReadSamples, &client3, numSamples, DefaultTestIterations);
    auto thread4 = new thread(ReadSamples, &client4, numSamples, DefaultTestIterations);

    thread1->join();
    thread2->join();
    thread3->join();
    thread4->join();

    auto end = chrono::high_resolution_clock::now();
    ReportMBPerSecond(start, end, numSamples * 4, DefaultTestIterations);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformNStreamTest(std::vector<NIPerfTestClient*>& clients, int numSamples)
{
    auto start = chrono::high_resolution_clock::now();

    std::vector<thread*> threads;
    threads.reserve(clients.size());
    for (auto client: clients)
    {
        auto t = new thread(ReadSamples, client, numSamples, DefaultTestIterations);
        threads.emplace_back(t);
    }
    for (auto thread: threads)
    {
        thread->join();
    }
    auto end = chrono::high_resolution_clock::now();
    ReportMBPerSecond(start, end, numSamples * clients.size(), DefaultTestIterations);
}
