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
#include <niScope.grpc.pb.h>
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
class NIScopeServer final : public niScope::niScopeService::Service
{
public:
    ::grpc::Status StreamLatencyTest(::grpc::ServerContext* context, ::grpc::ServerReaderWriter< ::niScope::StreamLatencyServer, ::niScope::StreamLatencyClient>* stream) override;
    ::grpc::Status StreamLatencyTestClient(::grpc::ServerContext* context, ::grpc::ServerReader< ::niScope::StreamLatencyClient>* reader, ::niScope::StreamLatencyServer* response) override;
    ::grpc::Status StreamLatencyTestServer(::grpc::ServerContext* context, const ::niScope::StreamLatencyClient* request, ::grpc::ServerWriter< ::niScope::StreamLatencyServer>* writer) override;
    Status Init(ServerContext* context, const niScope::InitParameters*, niScope::InitResult* response) override;
    Status InitWithOptions(ServerContext* context, const niScope::InitWithOptionsParameters* request, niScope::InitWithOptionsResult* response) override;
    Status Read(ServerContext* context, const niScope::ReadParameters* request, niScope::ReadResult* response) override;
    Status ReadContinuously(ServerContext* context, const niScope::ReadContinuouslyParameters* request, grpc::ServerWriter<niScope::ReadContinuouslyResult>* writer) override;
    Status TestWrite(ServerContext* context, const niScope::TestWriteParameters* request, niScope::TestWriteResult* response) override;
    Status TestWriteContinuously(ServerContext* context, ::grpc::ServerReaderWriter<niScope::TestWriteResult, niScope::TestWriteParameters>* stream) override;
};
