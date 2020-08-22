//---------------------------------------------------------------------
// Implementation objects for the LabVIEW implementation of the
// gRPC QueryServer
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
    Status Init(ServerContext* context, const niScope::InitParameters*, niScope::InitResult* response);
    Status InitWithOptions(ServerContext* context, const niScope::InitWithOptionsParameters* request, niScope::InitWithOptionsResult* response);
    Status AutoSetup(ServerContext* context, const niScope::AutoSetupParameters* request, niScope::AutoSetupResult* response);
    Status ConfigureHorizontalTiming(ServerContext* context, const niScope::ConfigureHorizontalTimingParameters* request, niScope::ConfigureHorizontalTimingResult* response);
    Status Read(ServerContext* context, const niScope::ReadParameters* request, niScope::ReadResult* response);
    Status ReadContinuously(ServerContext* context, const niScope::ReadContinuouslyParameters* request, grpc::ServerWriter<niScope::ReadContinuouslyResult>* writer);
    Status TestWrite(ServerContext* context, const niScope::TestWriteParameters* request, niScope::TestWriteResult* response);
};
