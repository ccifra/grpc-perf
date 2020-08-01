//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include <thread>
#include <sstream>
#include <fstream>
#include <iostream>
#include <niscope.h>
#include <niscope_server.h>

//---------------------------------------------------------------------
//---------------------------------------------------------------------
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using grpc::ServerWriter;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
using namespace std;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIScopeServer::InitWithOptions(ServerContext* context, const niScope::InitWithOptionsParameters* request, niScope::InitWithOptionsResult* response)
{	
	ViSession vi;
	auto status = niScope_InitWithOptions((char*)request->resourcename().c_str(), request->idquery(), request->resetdevice(), request->optionstring().c_str(), &vi);
	response->set_status(status);
	niScope::ViSession* session = new niScope::ViSession();
	session->set_id(vi);
	response->set_allocated_newvi(session);
	response->set_status(status);
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIScopeServer::AutoSetup(ServerContext* context, const niScope::AutoSetupParameters* request, niScope::AutoSetupResult* response)
{
	auto status = niScope_AutoSetup(request->vi().id());
	response->set_status(status);
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIScopeServer::ConfigureHorizontalTiming(ServerContext* context, const niScope::ConfigureHorizontalTimingParameters* request, niScope::ConfigureHorizontalTimingResult* response)
{	
	auto status = niScope_ConfigureHorizontalTiming(request->vi().id(), request->minsamplerate(), request->minnumpts(), request->refposition(), request->numrecords(), request->enforcerealtime());
	response->set_status(status);
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIScopeServer::Read(ServerContext* context, const niScope::ReadParameters* request, niScope::ReadResult* response)
{	
	response->mutable_wfm()->Reserve(request->numsamples());
	response->mutable_wfm()->Resize(request->numsamples(), 0.0);
	niScope_wfmInfo* info = new niScope_wfmInfo[1];
	auto status = niScope_Read(request->vi().id(), request->channellist().c_str(), request->timeout(), request->numsamples(), response->mutable_wfm()->mutable_data(), info);
	response->set_status(status);
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
Status NIScopeServer::ReadContinuously(ServerContext* context, const niScope::ReadContinuouslyParameters* request, grpc::ServerWriter<niScope::ReadContinuouslyResult>* writer)
{			
	niScope::ReadContinuouslyResult response;
	response.mutable_wfm()->Reserve(request->numsamples());
	response.mutable_wfm()->Resize(request->numsamples(), 0.0);
	niScope_wfmInfo* info = new niScope_wfmInfo[1];
	for (int x=0; x<1000; ++x)
	{
		auto status = niScope_Read(request->vi().id(), request->channellist().c_str(), request->timeout(), request->numsamples(), response.mutable_wfm()->mutable_data(), info);
		writer->Write(response);
	}
	return Status::OK;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
string GetServerAddress(int argc, char** argv)
{
    string target_str = "0.0.0.0:50051";
    string arg_str("--address");
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
                cout << "The only correct argument syntax is --address=" << endl;
            }
        }
        else
        {
            cout << "The only acceptable argument is --address=" << endl;
        }
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
std::string read_keycert( const std::string& filename)
{	
	std::string data;
	std::ifstream file(filename.c_str(), std::ios::in);
	if (file.is_open())
	{
		std::stringstream ss;
		ss << file.rdbuf();
		file.close();
		data = ss.str();
	}
	return data;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
std::shared_ptr<grpc::ServerCredentials> CreateCredentials(int argc, char **argv)
{
	auto certPath = GetCertPath(argc, argv);

	std::shared_ptr<grpc::ServerCredentials> creds;
	if (!certPath.empty())
	{
		std::string servercert = read_keycert(certPath + ".crt");
		std::string serverkey = read_keycert(certPath + ".key");

		grpc::SslServerCredentialsOptions::PemKeyCertPair pkcp;
		pkcp.private_key = serverkey;
		pkcp.cert_chain = servercert;

		grpc::SslServerCredentialsOptions ssl_opts;
		ssl_opts.pem_root_certs="";
		ssl_opts.pem_key_cert_pairs.push_back(pkcp);

		creds = grpc::SslServerCredentials(ssl_opts);
	}
	else
	{
		creds = grpc::InsecureServerCredentials();
	}
	return creds;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void RunServer(int argc, char **argv)
{
	auto server_address = GetServerAddress(argc, argv);
	auto creds = CreateCredentials(argc, argv);

	NIScopeServer service;
	grpc::EnableDefaultHealthCheckService(true);
	grpc::reflection::InitProtoReflectionServerBuilderPlugin();

	ServerBuilder builder;
	// Listen on the given address without any authentication mechanism.
	builder.AddListeningPort(server_address, creds);
	// Register "service" as the instance through which we'll communicate with
	// clients. In this case it corresponds to an *synchronous* service.
	builder.RegisterService(&service);
	// Finally assemble the server.
	auto server = builder.BuildAndStart();
	cout << "Server listening on " << server_address << endl;
	server->Wait();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int main(int argc, char **argv)
{
	RunServer(argc, argv);
	return 0;
}
