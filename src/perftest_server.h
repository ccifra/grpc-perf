//---------------------------------------------------------------------
// Defimnition of the NIPerfTestServer and MonikerServer gRPC Server
//---------------------------------------------------------------------
#pragma once

//---------------------------------------------------------------------
//---------------------------------------------------------------------
#ifdef __WIN32__
    #define WIN32_LEAN_AND_MEAN
    #include <windows.h>
#endif

//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <perftest.grpc.pb.h>
#include <condition_variable>

//---------------------------------------------------------------------
//---------------------------------------------------------------------
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerWriter;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class NIPerfTestServer final : public niPerfTest::niPerfTestService::Service
{
public:
    grpc::Status StreamLatencyTest(grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::niPerfTest::StreamLatencyServer, niPerfTest::StreamLatencyClient>* stream) override;
    grpc::Status StreamLatencyTestClient(grpc::ServerContext* context, ::grpc::ServerReader< ::niPerfTest::StreamLatencyClient>* reader, niPerfTest::StreamLatencyServer* response) override;
    grpc::Status StreamLatencyTestServer(grpc::ServerContext* context, const ::niPerfTest::StreamLatencyClient* request, ::grpc::ServerWriter<niPerfTest::StreamLatencyServer>* writer) override;
    Status Init(ServerContext* context, const niPerfTest::InitParameters*, niPerfTest::InitResult* response) override;
    Status ConfigureVertical(grpc::ServerContext* context, const niPerfTest::ConfigureVerticalRequest* request, niPerfTest::ConfigureVerticalResponse* response) override;
    Status ConfigureHorizontalTiming(grpc::ServerContext* context, const niPerfTest::ConfigureHorizontalTimingRequest* request, niPerfTest::ConfigureHorizontalTimingResponse* response) override;
    Status InitiateAcquisition(grpc::ServerContext* context, const niPerfTest::InitiateAcquisitionRequest* request, niPerfTest::InitiateAcquisitionResponse* response) override;
    Status Read(ServerContext* context, const niPerfTest::ReadParameters* request, niPerfTest::ReadResult* response) override;
    Status ReadContinuously(ServerContext* context, const niPerfTest::ReadContinuouslyParameters* request, grpc::ServerWriter<niPerfTest::ReadContinuouslyResult>* writer) override;
    Status TestWrite(ServerContext* context, const niPerfTest::TestWriteParameters* request, niPerfTest::TestWriteResult* response) override;
    Status TestWriteContinuously(ServerContext* context, grpc::ServerReaderWriter<niPerfTest::TestWriteResult, niPerfTest::TestWriteParameters>* stream) override;
};
