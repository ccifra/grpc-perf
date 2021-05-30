//---------------------------------------------------------------------
// Implementation objects for the NIScope gRPC Server
//---------------------------------------------------------------------
#pragma once

#ifdef __WIN32__
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <perftest.grpc.pb.h>
#include <condition_variable>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerWriter;
using namespace std;

#ifdef _WIN32
    #define LIBRARY_EXPORT extern "C" __declspec(dllexport)
#else
    #define LIBRARY_EXPORT extern "C"
#endif

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class NIScopeServer final : public niPerfTest::niPerfTestService::Service
{
public:
    ::grpc::Status StreamLatencyTest(::grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::niPerfTest::StreamLatencyServer, ::niPerfTest::StreamLatencyClient>* stream) override;
    ::grpc::Status StreamLatencyTestClient(::grpc::ServerContext* context, ::grpc::ServerReader< ::niPerfTest::StreamLatencyClient>* reader, ::niPerfTest::StreamLatencyServer* response) override;
    ::grpc::Status StreamLatencyTestServer(::grpc::ServerContext* context, const ::niPerfTest::StreamLatencyClient* request, ::grpc::ServerWriter< ::niPerfTest::StreamLatencyServer>* writer) override;
    Status Init(ServerContext* context, const niPerfTest::InitParameters*, niPerfTest::InitResult* response) override;
    Status InitWithOptions(ServerContext* context, const niPerfTest::InitWithOptionsParameters* request, niPerfTest::InitWithOptionsResult* response) override;
    Status Read(ServerContext* context, const niPerfTest::ReadParameters* request, niPerfTest::ReadResult* response) override;
    Status ReadContinuously(ServerContext* context, const niPerfTest::ReadContinuouslyParameters* request, grpc::ServerWriter<niPerfTest::ReadContinuouslyResult>* writer) override;
    Status TestWrite(ServerContext* context, const niPerfTest::TestWriteParameters* request, niPerfTest::TestWriteResult* response) override;
    Status TestWriteContinuously(ServerContext* context, ::grpc::ServerReaderWriter<niPerfTest::TestWriteResult, niPerfTest::TestWriteParameters>* stream) override;
};

class MonikerServer final : public niPerfTest::MonikerService::Service
{
public:
    Status InitiateMonikerStream(::grpc::ServerContext* context, const ::niPerfTest::MonikerList* request, ::niPerfTest::MonikerStreamId* response) override;
    Status StreamReadWrite(::grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::niPerfTest::MonikerReadResult, ::niPerfTest::MonikerWriteRequest>* stream) override;
};
