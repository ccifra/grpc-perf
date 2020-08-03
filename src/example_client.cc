//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <niScope.grpc.pb.h>
#include <sstream>
#include <fstream>
#include <iostream>

//---------------------------------------------------------------------
//---------------------------------------------------------------------
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using namespace std;
using namespace niScope;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class NIScope
{
public:
    NIScope(shared_ptr<Channel> channel);

public:
    int InitWithOptions(std::string resourceName, bool idQuery, bool resetDevice, std::string options, ViSession* session);
    int ConfigureHorizontalTiming(ViSession session, double minSampleRate, int numPoints, double refPosition, int numRecords, bool enforceRealtime);
    int AutoSetup(ViSession session);
    int Read(ViSession session, std::string channels, double timeout, int numSamples, double* samples, ScopeWaveformInfo* waveformInfo);
    std::unique_ptr<grpc::ClientReader<niScope::ReadContinuouslyResult>> ReadContinuously(grpc::ClientContext* context, ViSession session, std::string channels, double timeout, int numSamples);
private:
    unique_ptr<niScopeService::Stub> m_Stub;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
NIScope::NIScope(shared_ptr<Channel> channel)
    : m_Stub(niScopeService::NewStub(channel))
{        
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int NIScope::InitWithOptions(std::string resourceName, bool idQuery, bool resetDevice, std::string options, ViSession* session)
{
    InitWithOptionsParameters request;
    request.set_resourcename(resourceName.c_str());
    request.set_idquery(idQuery);
    request.set_resetdevice(resetDevice);
    request.set_optionstring(options.c_str());

    ClientContext context;
    InitWithOptionsResult reply;
    Status status = m_Stub->InitWithOptions(&context, request, &reply);
    if (!status.ok())
    {
        cout << status.error_code() << ": " << status.error_message() << endl;
    }
    *session = reply.newvi();
    return reply.status();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int NIScope::ConfigureHorizontalTiming(ViSession session, double minSampleRate, int numPoints, double refPosition, int numRecords, bool enforceRealtime)
{    
    ConfigureHorizontalTimingParameters request;
    auto requestSession = new ViSession;
    requestSession->set_id(session.id());
    request.set_allocated_vi(requestSession);
    request.set_minsamplerate(minSampleRate);
    request.set_minnumpts(numPoints);
    request.set_refposition(refPosition);
    request.set_numrecords(numRecords);
    request.set_enforcerealtime(enforceRealtime);

    ClientContext context;
    ConfigureHorizontalTimingResult reply;
    Status status = m_Stub->ConfigureHorizontalTiming(&context, request, &reply);
    if (!status.ok())
    {
        cout << status.error_code() << ": " << status.error_message() << endl;
    }
    return reply.status();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int NIScope::AutoSetup(ViSession session)
{
    AutoSetupParameters request;
    auto requestSession = new ViSession;
    requestSession->set_id(session.id());
    request.set_allocated_vi(requestSession);

    ClientContext context;
    AutoSetupResult reply;
    Status status = m_Stub->AutoSetup(&context, request, &reply);
    if (!status.ok())
    {
        cout << status.error_code() << ": " << status.error_message() << endl;
    }
    return reply.status();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int NIScope::Read(ViSession session, std::string channels, double timeout, int numSamples, double* samples, ScopeWaveformInfo* waveformInfo)
{
    ReadParameters request;
    auto requestSession = new ViSession;
    requestSession->set_id(session.id());
    request.set_allocated_vi(requestSession);
    request.set_channellist(channels.c_str());
    request.set_timeout(timeout);
    request.set_numsamples(numSamples);

    ClientContext context;
    ReadResult reply;
    Status status = m_Stub->Read(&context, request, &reply);    
    if (!status.ok())
    {
        cout << status.error_code() << ": " << status.error_message() << endl;
    }
    memcpy(samples, reply.wfm().data(), numSamples * sizeof(double));
    //*waveformInfo = reply.wfminfo()[0];
    return reply.status();
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
std::unique_ptr<grpc::ClientReader<niScope::ReadContinuouslyResult>> NIScope::ReadContinuously(grpc::ClientContext* context, ViSession session, std::string channels, double timeout, int numSamples)
{    
    ReadContinuouslyParameters request;
    auto requestSession = new ViSession;
    requestSession->set_id(session.id());
    request.set_allocated_vi(requestSession);
    request.set_channellist(channels.c_str());
    request.set_timeout(timeout);
    request.set_numsamples(numSamples);

    return m_Stub->ReadContinuously(context, request);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
string GetServerAddress(int argc, char** argv)
{
    string target_str;
    string arg_str("--target");
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
                cout << "The only correct argument syntax is --target=" << endl;
                return 0;
            }
        }
        else
        {
            cout << "The only acceptable argument is --target=" << endl;
            return 0;
        }
    }
    else
    {
        target_str = "localhost:50051";
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
shared_ptr<grpc::ChannelCredentials> CreateCredentials(int argc, char **argv)
{
    shared_ptr<grpc::ChannelCredentials> creds;
    auto certificatePath = GetCertPath(argc, argv);
    if (!certificatePath.empty())
    {
        std::string cacert = read_keycert(certificatePath);
        grpc::SslCredentialsOptions ssl_opts;
        ssl_opts.pem_root_certs=cacert;
        creds = grpc::SslCredentials(ssl_opts);
    }
    else
    {
        creds = grpc::InsecureChannelCredentials();
    }
    return creds;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int main(int argc, char **argv)
{
    auto resourceName = "SimulatedScope7c632f66-e7c2-4fab-85a4-cd15c8be4130";
    auto options = "Simulate=1, DriverSetup=Model:5164; BoardType:PXIe; MemorySize:1610612736";

    auto target_str = GetServerAddress(argc, argv);
    auto creds = CreateCredentials(argc, argv);

    NIScope client(grpc::CreateChannel(target_str, creds));

    ViSession session;
    auto result = client.InitWithOptions((char*)resourceName, false, false, options, &session);
    result = client.ConfigureHorizontalTiming(session, 100000000, 100000, 50, 1, true);
    result = client.AutoSetup(session);

    double* samples = new double[1000000];
    ScopeWaveformInfo info[1];
    result = client.Read(session, (char*)"0", 5.0, 400000, samples, info);
    cout << "First 10 samples: " << std::endl;


    for (int x = 0; x < 10; ++x)
    {
        cout << "    " << samples[x] << std::endl;
    }

    std::cout << "Reading 1000 waveforms from stream." << std::endl;
    {
        int index = 0;
        auto start = std::chrono::steady_clock::now();
        grpc::ClientContext context;
        auto readResult = client.ReadContinuously(&context, session, (char*)"0", 5.0, 100000);
        
        ReadContinuouslyResult cresult;
        while(readResult->Read(&cresult))
        {
            index += 1;
        }
        auto end = std::chrono::steady_clock::now();
        cout << "Read " << index << " Continous waveforms" << std::endl;
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        std::cout << "It took me " << elapsed.count() << " microseconds." << std::endl;
    }
}
