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
using namespace std;
using namespace niPerfTest;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
static int LatencyTestIterations = 300000;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformMessagePerformanceTest(NIPerfTestClient& client)
{
    cout << "Start Messages per second test" << endl;

    //auto s = chrono::high_resolution_clock::now();
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
        auto start = chrono::steady_clock::now();
        stream->Write(clientData);
        stream->Read(&serverData);
        auto end = chrono::steady_clock::now();
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

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformMonikerLatencyReadWriteTest(NIMonikerClient& client, int numItems, bool useAnyType, std::string fileName)
{    
    cout << "Start Moniker Read Write latency test, items: " << numItems << " use any: " << useAnyType << " iterations:" << LatencyTestIterations << endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    timeVector times;
    times.reserve(LatencyTestIterations);

	niPerfTest::StreamLatencyClient clientData;
	niPerfTest::StreamLatencyServer serverData;

    ClientContext context;
    niPerfTest::MonikerList monikerList;
    monikerList.set_useanytype(useAnyType);

    for (int x=0; x<numItems; ++x)
    {
        auto readMoniker = monikerList.add_readmonikers();
        readMoniker->set_datainstance("1");
        auto writeMoniker = monikerList.add_writemonikers();
        writeMoniker->set_datainstance("1");
    }
    niPerfTest::MonikerStreamId monikerId;
    auto streamId = client.m_Stub->InitiateMonikerStream(&context, monikerList, &monikerId);
    
    ClientContext streamContext;
    MonikerWriteRequest writeRequest;
    writeRequest.set_monikerstreamid(monikerId.streamid());
    auto stream = client.m_Stub->StreamReadWrite(&streamContext);

    google::protobuf::Arena areana;

    for (int x=0; x<100; ++x)
    {
        auto writeRequest = google::protobuf::Arena::CreateMessage<MonikerWriteRequest>(&areana);
        writeRequest->set_monikerstreamid(monikerId.streamid());
        for (int i=0; i<numItems; ++i)
        {
            auto writeValue = google::protobuf::Arena::CreateMessage<niPerfTest::StreamLatencyClient>(&areana);
            writeValue->set_message(42 + i);
            auto v = writeRequest->add_values();
            if (useAnyType)
            {
                auto any = google::protobuf::Arena::CreateMessage<google::protobuf::Any>(&areana);
                any->PackFrom(*writeValue);
                v->set_allocated_values(any);
            }
            else
            {
                auto doubleValue = google::protobuf::Arena::CreateMessage<DoubleArray>(&areana);
                doubleValue->add_values(42 + i);
                v->set_allocated_doublearray(doubleValue);
            }
        }
        stream->Write(*writeRequest);

        auto readResult = google::protobuf::Arena::CreateMessage<MonikerReadResult>(&areana);
        stream->Read(readResult);
        for (int i=0; i<numItems; ++i)
        {
            if (useAnyType)
            {
                auto readValue = google::protobuf::Arena::CreateMessage<StreamLatencyServer>(&areana);
                readResult->values()[i].values().UnpackTo(readValue);
            }
            else
            {
                auto result = readResult->values()[i].doublearray().values(0);
            }
        }
        areana.Reset();
    }

    for (int x=0; x<LatencyTestIterations; ++x)
    {
        auto start = chrono::steady_clock::now();
        auto writeRequest = google::protobuf::Arena::CreateMessage<MonikerWriteRequest>(&areana);
        writeRequest->set_monikerstreamid(monikerId.streamid());
        for (int i=0; i<numItems; ++i)
        {
            auto writeValue = google::protobuf::Arena::CreateMessage<niPerfTest::StreamLatencyClient>(&areana);
            writeValue->set_message(42 + i);
            auto v = writeRequest->add_values();
            if (useAnyType)
            {
                auto any = google::protobuf::Arena::CreateMessage<google::protobuf::Any>(&areana);
                any->PackFrom(*writeValue);
                v->set_allocated_values(any);
            }
            else
            {
                auto doubleValue = google::protobuf::Arena::CreateMessage<DoubleArray>(&areana);
                doubleValue->add_values(42 + i);
                v->set_allocated_doublearray(doubleValue);
            }
        }
        stream->Write(*writeRequest);

        auto readResult = google::protobuf::Arena::CreateMessage<MonikerReadResult>(&areana);
        stream->Read(readResult);
        for (int i=0; i<numItems; ++i)
        {
            if (useAnyType)
            {
                auto readValue = google::protobuf::Arena::CreateMessage<StreamLatencyServer>(&areana);
                readResult->values()[i].values().UnpackTo(readValue);
            }
            else
            {
                auto result = readResult->values()[i].doublearray().values(0);
            }
        }
        auto end = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
        times.emplace_back(elapsed);
        areana.Reset();
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    WriteLatencyData(times, fileName);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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
    request.mutable_wfm()->Reserve(numSamples);
    request.mutable_wfm()->Resize(numSamples, 0);
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
        auto start = chrono::steady_clock::now();
        ClientContext context;
        client.m_Stub->TestWrite(&context, request, &reply);
        auto end = chrono::steady_clock::now();
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
    request.mutable_wfm()->Reserve(numSamples);
    request.mutable_wfm()->Resize(numSamples, 0);
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
        auto start = chrono::steady_clock::now();
        stream->Write(request);
        stream->Read(&reply);
        auto end = chrono::steady_clock::now();
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
        auto start = chrono::steady_clock::now();
        for (int i=0; i<streamCount; ++i)
        {
            streamInfos[i].wstream->Write(streamInfos[i].clientData);
        }
        for (int i=0; i<streamCount; ++i)
        {
            streamInfos[i].rstream->Read(&serverData);
        }
        auto end = chrono::steady_clock::now();
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
        auto start = chrono::steady_clock::now();
        ClientContext context;
        client.m_Stub->Init(&context, request, &reply);
        auto end = chrono::steady_clock::now();
        auto elapsed = chrono::duration_cast<chrono::microseconds>(end - start);
        times.emplace_back(elapsed);
    }
    WriteLatencyData(times, fileName);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformReadTest(NIPerfTestClient& client, int numSamples)
{    
    cout << "Start " << numSamples << " Read Test" << endl;

    ViSession session;
    WaveformInfo info;
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
void PerformWriteTest(NIPerfTestClient& client, int numSamples)
{   
    cout << "Start " << numSamples << " Write Test" << endl;

    ViSession session;
    WaveformInfo info;
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
void PerformStreamingTest(NIPerfTestClient& client, int numSamples)
{
    auto start = chrono::steady_clock::now();
    ReadSamples(&client, numSamples);
    auto end = chrono::steady_clock::now();
    ReportMBPerSecond(start, end, numSamples);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void PerformTwoStreamTest(NIPerfTestClient& client, NIPerfTestClient& client2, int numSamples)
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
void PerformFourStreamTest(NIPerfTestClient& client, NIPerfTestClient& client2, NIPerfTestClient& client3, NIPerfTestClient& client4, int numSamples)
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
void PerformNStreamTest(std::vector<NIPerfTestClient*>& clients, int numSamples)
{
    auto start = chrono::steady_clock::now();

    std::vector<thread*> threads;
    threads.reserve(clients.size());
    for (auto client: clients)
    {
        auto t = new thread(ReadSamples, client, numSamples);
        threads.emplace_back(t);
    }
    for (auto thread: threads)
    {
        thread->join();
    }
    auto end = chrono::steady_clock::now();
    ReportMBPerSecond(start, end, numSamples * clients.size());
}
