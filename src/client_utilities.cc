//---------------------------------------------------------------------
//---------------------------------------------------------------------
#include "client_utilities.h"
#include <fstream>
#include <WinSock2.h>
#include "detours.h"
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

//---------------------------------------------------------------------
// 
// User Mode Networking
// 
//---------------------------------------------------------------------

//---------------------------------------------------------------------
//---------------------------------------------------------------------
typedef int (WSAAPI* pWSASend)(
    SOCKET                             s,
    LPWSABUF                           lpBuffers,
    DWORD                              dwBufferCount,
    LPDWORD                            lpNumberOfBytesSent,
    DWORD                              dwFlags,
    LPWSAOVERLAPPED                    lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

//---------------------------------------------------------------------
//---------------------------------------------------------------------
typedef int (WSAAPI* pWSARecv)(
    SOCKET                             s,
    LPWSABUF                           lpBuffers,
    DWORD                              dwBufferCount,
    LPDWORD                            lpNumberOfBytesRecvd,
    LPDWORD                            lpFlags,
    LPWSAOVERLAPPED                    lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

//---------------------------------------------------------------------
//---------------------------------------------------------------------
typedef int (WSAAPI* pConnect)(
    SOCKET         s,
    const sockaddr* name,
    int            namelen);

//---------------------------------------------------------------------
//---------------------------------------------------------------------
typedef SOCKET (WSAAPI* pWSAAccept)(
    SOCKET          s,
    sockaddr* addr,
    LPINT           addrlen,
    LPCONDITIONPROC lpfnCondition,
    DWORD_PTR       dwCallbackData
);

//---------------------------------------------------------------------
//---------------------------------------------------------------------
typedef BOOL(*pConnectEx)(
    SOCKET s,
    const sockaddr* name,
    int namelen,
    PVOID lpSendBuffer,
    DWORD dwSendDataLength,
    LPDWORD lpdwBytesSent,
    LPOVERLAPPED lpOverlapped
    );

//---------------------------------------------------------------------
//---------------------------------------------------------------------
typedef BOOL (WSAAPI* pWSAGetOverlappedResult)(
    SOCKET          s,
    LPWSAOVERLAPPED lpOverlapped,
    LPDWORD         lpcbTransfer,
    BOOL            fWait,
    LPDWORD         lpdwFlags
);

//---------------------------------------------------------------------
//---------------------------------------------------------------------
typedef int (*pClosesocket)(
    SOCKET s
    );

//---------------------------------------------------------------------
//---------------------------------------------------------------------
typedef int (WSAAPI* pWSAIoctl)(
    SOCKET                             s,
    DWORD                              dwIoControlCode,
    LPVOID                             lpvInBuffer,
    DWORD                              cbInBuffer,
    LPVOID                             lpvOutBuffer,
    DWORD                              cbOutBuffer,
    LPDWORD                            lpcbBytesReturned,
    LPWSAOVERLAPPED                    lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

//---------------------------------------------------------------------
//---------------------------------------------------------------------
typedef BOOL (* pAcceptEx)(
    SOCKET       sListenSocket,
    SOCKET       sAcceptSocket,
    PVOID        lpOutputBuffer,
    DWORD        dwReceiveDataLength,
    DWORD        dwLocalAddressLength,
    DWORD        dwRemoteAddressLength,
    LPDWORD      lpdwBytesReceived,
    LPOVERLAPPED lpOverlapped
);

//---------------------------------------------------------------------
//---------------------------------------------------------------------
typedef BOOL(*pGetQueuedCompletionStatus)(
    HANDLE       CompletionPort,
    LPDWORD      lpNumberOfBytesTransferred,
    PULONG_PTR   lpCompletionKey,
    LPOVERLAPPED* lpOverlapped,
    DWORD        dwMilliseconds
    );

//---------------------------------------------------------------------
//---------------------------------------------------------------------
typedef HANDLE(WINAPI* pCreateIoCompletionPort)(
    _In_     HANDLE    FileHandle,
    _In_opt_ HANDLE    ExistingCompletionPort,
    _In_     ULONG_PTR CompletionKey,
    _In_     DWORD     NumberOfConcurrentThreads);

class SocketInfo;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
static pWSASend _wsaSend = WSASend;
static pWSARecv _wsaRecv = WSARecv;
static pConnect _connect = connect;
static pWSAAccept _wsaAccept = WSAAccept;
static pWSAGetOverlappedResult _wsaGetOverlappedResult = WSAGetOverlappedResult;
static pClosesocket _closesocket = closesocket;
static pWSAIoctl _wsaIoctl = WSAIoctl;
static pAcceptEx _acceptEx;
static pConnectEx _connectEx;
static pGetQueuedCompletionStatus _getQueuedCompletionStatus = GetQueuedCompletionStatus;
static pCreateIoCompletionPort _createIoCompletionPort = CreateIoCompletionPort;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class CompletionPortInfo
{
public:
    HANDLE _handle;
    ULONG_PTR _key;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class SocketManager
{
public:
    SocketInfo* AddSocket(SOCKET s, bool acceptSocket, int port);
    void RemoveSocket(SOCKET s);
    int SocketWrite(SOCKET s, LPWSABUF buffers, DWORD _numBuffers, LPWSAOVERLAPPED overlapped);
    int SocketRead(SOCKET s, LPWSABUF buffers, DWORD _numBuffers, LPWSAOVERLAPPED overlapped);

    BOOL GetOverlappedByteCount(SOCKET s, LPWSAOVERLAPPED lpOverlapped, LPDWORD lpcbTransfer, BOOL fWait, LPDWORD lpdwFlags);
    void SetOverlappedByteCount(int, LPWSAOVERLAPPED overlapped);

    SocketInfo* GetPairedSocket(SOCKET s);
    SocketInfo* GetSocket(SOCKET s);

    void SetCompletionPortInfo(SOCKET socket, HANDLE handle, ULONG_PTR key);
    CompletionPortInfo* GetCompletionPortInfo(SOCKET s);

private:
    std::map<SOCKET, CompletionPortInfo*> _completionPortInfo;
    std::map<SOCKET, SocketInfo*> _sockets;
    std::map< LPWSAOVERLAPPED, int> _overlappedByteCounts;
    std::map< LPWSAOVERLAPPED, int> _registeredByteCounts;
};


//---------------------------------------------------------------------
//---------------------------------------------------------------------
class SocketInfo
{
public:
    SocketInfo();

    void AddWriteBuffer(LPWSABUF buffer);
    int CompleteRead();

    SOCKET _s;
    bool _acceptSocket;
    int _port;
    std::vector<WSABUF> _readBuffers;
    LPWSAOVERLAPPED _readOverlapped;

    CHAR* _currentPos;
    ULONG _remainingCount;

    std::vector<LPWSABUF> _pendingReads;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SocketInfo::SocketInfo() :
    _s(0),
    _readOverlapped(nullptr),
    _currentPos(nullptr),
    _remainingCount(0)
{
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SocketManager _socketManager;
std::mutex _lock;
bool _detourEnable = true;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void SocketInfo::AddWriteBuffer(LPWSABUF buffer)
{
    _pendingReads.emplace_back(buffer);
    if (_readOverlapped)
    {
        CompleteRead();
    }
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int SocketInfo::CompleteRead()
{
    cout << "Completing Read: " << _s << endl;

    if (_currentPos == nullptr)
    {
        _currentPos = _pendingReads.front()->buf;
        _remainingCount = _pendingReads.front()->len;
    }
    auto totalWritten = 0;
    for (auto dest: _readBuffers)
    {
        auto toWrite = min(_remainingCount, dest.len);
        totalWritten += toWrite;
        memcpy(dest.buf, _currentPos, toWrite);

        _remainingCount -= toWrite;
        _currentPos += toWrite;

        if (_remainingCount == 0)
        {
            break;
        }
    }
    if (_remainingCount == 0)
    {
        _currentPos = nullptr;
        _pendingReads.erase(_pendingReads.begin());
    }
    if (_readOverlapped != nullptr)
    {
        _socketManager.SetOverlappedByteCount(totalWritten, _readOverlapped);
        if (_readOverlapped->hEvent != NULL)
        {
            SetEvent(_readOverlapped->hEvent);
        }
        auto completionInfo = _socketManager.GetCompletionPortInfo(_s);
        if (completionInfo != nullptr)
        {
            PostQueuedCompletionStatus(completionInfo->_handle, totalWritten, completionInfo->_key, _readOverlapped);
        }

    }
    _readOverlapped = nullptr;
    _readBuffers.clear();
    cout << "Complete Read, total written: " << totalWritten << endl;
    return totalWritten;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void SocketManager::SetCompletionPortInfo(SOCKET socket, HANDLE handle, ULONG_PTR key)
{
    CompletionPortInfo* info = new CompletionPortInfo();
    info->_handle = handle;
    info->_key = key;
    _completionPortInfo.emplace(socket, info);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
CompletionPortInfo* SocketManager::GetCompletionPortInfo(SOCKET s)
{
    auto found = _completionPortInfo.find(s);
    if (found != _completionPortInfo.end())
    {
        return (*found).second;
    }
    return nullptr;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
BOOL SocketManager::GetOverlappedByteCount(SOCKET s, LPWSAOVERLAPPED lpOverlapped, LPDWORD lpcbTransfer, BOOL fWait, LPDWORD lpdwFlags)
{
    auto info = GetSocket(s);
    auto found = _overlappedByteCounts.find(lpOverlapped);
    if (found != _overlappedByteCounts.end())
    {
        *lpcbTransfer = (*found).second;
        cout << "Transfer complete: " << *lpcbTransfer << endl;
        return TRUE;
    }
    if (_registeredByteCounts.find(lpOverlapped) == _registeredByteCounts.end())
    {
        auto result = _wsaGetOverlappedResult(s, lpOverlapped, lpcbTransfer, fWait, lpdwFlags);
        cout << "OS get overlapped result: " << result << endl;
        return result;
    }
    cout << "Setting overlapped is incomplete!!!" << endl;
    WSASetLastError(WSA_IO_INCOMPLETE);
    *lpcbTransfer = 0;
    return FALSE;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void SocketManager::SetOverlappedByteCount(int count, LPWSAOVERLAPPED overlapped)
{
    _overlappedByteCounts.emplace(overlapped, count);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SocketInfo* SocketManager::AddSocket(SOCKET s, bool acceptSocket, int port)
{
    if (port == 0)
    {
        BYTE storage[0x80];
        sockaddr_in6* address = (sockaddr_in6*)storage;
        sockaddr_in* address4 = (sockaddr_in*)storage;
        memset(storage, 0, 0x80);
        int nameLen = 0x80;
        auto result = getsockname(s, (sockaddr*)address, &nameLen);
        auto error = WSAGetLastError();
        if (error == 0)
        {
            auto kind = address->sin6_family;
            port = ntohs(address4->sin_port);
            if (kind == AF_INET6)
            {
                port = ntohs(address->sin6_port);
            }
        }
    }
    auto found = _sockets.find(s);

    if (found == _sockets.end())
    {
        if (port != 0 && port != 50051)
        {
            cout << "Oddness when adding, socket: " << s << ", port: " << port << endl;
        }

        auto info = new SocketInfo();
        info->_s = s;
        info->_acceptSocket = acceptSocket;
        info->_port = port;
        _sockets.emplace(s, info);
        return info;
    }
    if ((*found).second->_port == 0)
    {
        if (port != 0 && port != 50051)
        {
            cout << "Oddness when updating, socket: " << s << ", port: " << port << endl;
        }

        (*found).second->_port = port;
    }
    return (*found).second;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void SocketManager::RemoveSocket(SOCKET s)
{
    auto found = _sockets.find(s);
    if (found != _sockets.end())
    {
        _sockets.erase(found);
    }
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SocketInfo* SocketManager::GetPairedSocket(SOCKET s)
{
    auto socket = GetSocket(s);
    if (socket == nullptr)
    {
        return nullptr;
    }
    bool found0 = false;
    for (auto it : _sockets)
    {
        AddSocket(it.first, false, 0);
        if ((it.second->_acceptSocket == false) && (it.first != s) && (it.second->_port == socket->_port))
        {
            return it.second;
        }
    }
    return nullptr;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SocketInfo* SocketManager::GetSocket(SOCKET s)
{
    for (auto it : _sockets)
    {
        if (it.first == s)
        {
            return it.second;
        }
    }
    return nullptr;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int SocketManager::SocketRead(SOCKET s, LPWSABUF buffers, DWORD numBuffers, LPWSAOVERLAPPED overlapped)
{
    auto info = GetSocket(s);
    if (info->_port != 50051)
    {
        DWORD w = 0;
        DWORD flags = 0;
        _wsaRecv(s, buffers, numBuffers, &w, &flags, overlapped, nullptr);
        return w;
    }

    if (overlapped == nullptr)
    {
        if (info->_pendingReads.size() != 0)
        {
            for (int x = 0; x < numBuffers; ++x)
            {
                info->_readBuffers.emplace_back(buffers[x]);
            }
            info->_readOverlapped = overlapped;

            return info->CompleteRead();
        }
        return 0;
    }
    for (int x = 0; x < numBuffers; ++x)
    {
        info->_readBuffers.emplace_back(buffers[x]);
    }
    info->_readOverlapped = overlapped;
    _socketManager._registeredByteCounts.emplace(overlapped, 0);

    if (info->_pendingReads.size() != 0)
    {
        info->CompleteRead();
    }
    return 0;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int SocketManager::SocketWrite(SOCKET s, LPWSABUF buffers, DWORD numBuffers, LPWSAOVERLAPPED overlapped)
{
    auto other = GetPairedSocket(s);
    cout << "Writing from socket: " << s << " to socket: " << other->_s << " with " << _socketManager._sockets.size() << " total sockets" << endl;
    if (other == nullptr || other->_port != 50051)
    {
        DWORD w = 0;
        _wsaSend(s, buffers, numBuffers, &w, 0, overlapped, nullptr);
        return w;
    }

    int byteCount = 0;
    for (int x = 0; x < numBuffers; ++x)
    {
        byteCount += buffers[x].len;
    }

    if (byteCount == 0)
    {
        return 0;
    }

    auto buffer = new CHAR[byteCount];
    auto start = buffer;
    auto totalCount = byteCount;

    for (int x = 0; x < numBuffers; ++x)
    {
        memcpy(start, buffers[x].buf, buffers[x].len);
        start += buffers[x].len;
    }
    auto pending = new WSABUF();
    pending->buf = buffer;
    pending->len = totalCount;
    other->AddWriteBuffer(pending);
    return totalCount;
}


//---------------------------------------------------------------------
//---------------------------------------------------------------------
int WSAAPI DetouredConnect(
    SOCKET         s,
    const sockaddr* name,
    int            namelen
)
{
    cout << "Connect: " << s << endl;

    sockaddr_in6* address = (sockaddr_in6*)name;
    sockaddr_in* address4 = (sockaddr_in*)name;
    auto kind = address->sin6_family;
    auto port = ntohs(address4->sin_port);
    if (kind == AF_INET6)
    {
        port = ntohs(address->sin6_port);
    }

    {
        std::lock_guard<std::mutex> lock(_lock);
        auto info = _socketManager.AddSocket(s, false, port);
    }

    auto result = _connect(s, name, namelen);
    return result;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
BOOL DetouredConnectEx(
    SOCKET s,
    const sockaddr* name,
    int namelen,
    PVOID lpSendBuffer,
    DWORD dwSendDataLength,
    LPDWORD lpdwBytesSent,
    LPOVERLAPPED lpOverlapped
)
{
    cout << "ConnectEx: " << s << endl;

    sockaddr_in6* address = (sockaddr_in6*)name;
    sockaddr_in* address4 = (sockaddr_in*)name;
    auto kind = address->sin6_family;
    auto port = ntohs(address4->sin_port);
    if (kind == AF_INET6)
    {
        port = ntohs(address->sin6_port);
    }

    {
        std::lock_guard<std::mutex> lock(_lock);
        auto info = _socketManager.AddSocket(s, false, port);
    }

    return _connectEx(s, name, namelen, lpSendBuffer, dwSendDataLength, lpdwBytesSent, lpOverlapped);
}


//---------------------------------------------------------------------
//---------------------------------------------------------------------
SOCKET WSAAPI DetouredWSAAccept(
    SOCKET          s,
    sockaddr* addr,
    LPINT           addrlen,
    LPCONDITIONPROC lpfnCondition,
    DWORD_PTR       dwCallbackData
)
{
    {
        std::lock_guard<std::mutex> lock(_lock);
        _socketManager.AddSocket(s, false, 0);
    }
    return _wsaAccept(s, addr, addrlen, lpfnCondition, dwCallbackData);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int WSAAPI DetouredWSASend(
    SOCKET                             s,
    LPWSABUF                           lpBuffers,
    DWORD                              dwBufferCount,
    LPDWORD                            lpNumberOfBytesSent,
    DWORD                              dwFlags,
    LPWSAOVERLAPPED                    lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
)
{
    cout << "DetouredWSASend: " << s << ", buffers:" << dwBufferCount << endl;

    WSASetLastError(0);
    if (_detourEnable)
    {
        std::lock_guard<std::mutex> lock(_lock);
        _socketManager.AddSocket(s, false, 0);

        auto written = _socketManager.SocketWrite(s, lpBuffers, dwBufferCount, lpOverlapped);
        if (lpNumberOfBytesSent != nullptr)
        {
            *lpNumberOfBytesSent = written;
        }
        return 0;
    }
    auto result = _wsaSend(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine);
    cout << "Result: " << result << ", bytes sent: " << *lpNumberOfBytesSent << endl;
    return result;
}

bool _didAccept = false;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
BOOL DetouredAcceptEx(
    SOCKET       sListenSocket,
    SOCKET       sAcceptSocket,
    PVOID        lpOutputBuffer,
    DWORD        dwReceiveDataLength,
    DWORD        dwLocalAddressLength,
    DWORD        dwRemoteAddressLength,
    LPDWORD      lpdwBytesReceived,
    LPOVERLAPPED lpOverlapped
)
{
    cout << "AcceptEx: " << sListenSocket << " : " << sAcceptSocket << endl;
    {
        std::lock_guard<std::mutex> lock(_lock);
        if (!_didAccept)
        {
            _didAccept = true;
            auto info = _socketManager.AddSocket(sListenSocket, true, 0);
            info = _socketManager.AddSocket(sAcceptSocket, false, info->_port);
        }
    }
    auto result = _acceptEx(sListenSocket, sAcceptSocket, lpOutputBuffer, dwReceiveDataLength, dwLocalAddressLength, dwRemoteAddressLength, lpdwBytesReceived, lpOverlapped);
    return result;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int WSAAPI DetouredWSARecv(
    SOCKET                             s,
    LPWSABUF                           lpBuffers,
    DWORD                              dwBufferCount,
    LPDWORD                            lpNumberOfBytesRecvd,
    LPDWORD                            lpFlags,
    LPWSAOVERLAPPED                    lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
)
{
    cout << "DetouredWSARecv: " << s << " ov: " << lpOverlapped << endl;
    WSASetLastError(0);

    if (_detourEnable)
    {
        std::lock_guard<std::mutex> lock(_lock);
        _socketManager.AddSocket(s, false, 0);

        int count = _socketManager.SocketRead(s, lpBuffers, dwBufferCount, lpOverlapped);
        if (lpOverlapped == nullptr)
        {
            *lpNumberOfBytesRecvd = count;
        }
        if (count == 0)
        {
            if (lpOverlapped != nullptr)
            {
                WSASetLastError(ERROR_IO_PENDING);
            }
            else
            {
                WSASetLastError(WSAEWOULDBLOCK);
            }
            auto error = WSAGetLastError();
            cout << "wsaRecv would block: -1, error: " << error << endl;
            return -1;
        }
        cout << "WSARecv did not block" << endl;
        return 0;
    }
    {
        std::lock_guard<std::mutex> lock(_lock);
        auto info = _socketManager.AddSocket(s, false, 0);
        auto port = info->_port;
    }
    auto result = _wsaRecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpOverlapped, lpCompletionRoutine);
    auto error = WSAGetLastError();
    cout << "wsaRecv result: " << result << ", error: " << error << endl;
    return result;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
BOOL WSAAPI DetouredWSAGetOverlappedResult(
    SOCKET          s,
    LPWSAOVERLAPPED lpOverlapped,
    LPDWORD         lpcbTransfer,
    BOOL            fWait,
    LPDWORD         lpdwFlags
)
{
    cout << "DetouredWSAGetOverlappedResult: " << s << ", ov: " << lpOverlapped << endl;
    WSASetLastError(0);
    if (_detourEnable)
    {
        std::lock_guard<std::mutex> lock(_lock);

        WSASetLastError(0);
        return _socketManager.GetOverlappedByteCount(s, lpOverlapped, lpcbTransfer, fWait, lpdwFlags);
    }

    auto result = _wsaGetOverlappedResult(s, lpOverlapped, lpcbTransfer, fWait, lpdwFlags);
    auto error = WSAGetLastError();
    cout << "Get overlapped result: " << result << ", error: " << error << " count: " << *lpcbTransfer << endl;
    return result;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int DetouredClosesocket(
    SOCKET s
)
{
    cout << "Close Socket: " << s << endl;
    {
        std::lock_guard<std::mutex> lock(_lock);
        _socketManager.RemoveSocket(s);
    }
    return _closesocket(s);
}

int WSAAPI DetouredWSAIoctl(
    SOCKET                             s,
    DWORD                              dwIoControlCode,
    LPVOID                             lpvInBuffer,
    DWORD                              cbInBuffer,
    LPVOID                             lpvOutBuffer,
    DWORD                              cbOutBuffer,
    LPDWORD                            lpcbBytesReturned,
    LPWSAOVERLAPPED                    lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
)
{
    auto result = _wsaIoctl(s, dwIoControlCode, lpvInBuffer, cbInBuffer, lpvOutBuffer, cbOutBuffer, lpcbBytesReturned, lpOverlapped, lpCompletionRoutine);
    if (dwIoControlCode == SIO_GET_EXTENSION_FUNCTION_POINTER)
    {
        auto acceptId = GUID(WSAID_ACCEPTEX);
        auto connectId = GUID(WSAID_CONNECTEX);
        auto in = *(GUID*)lpvInBuffer;
        if (in == acceptId)
        {
            _acceptEx = *(pAcceptEx*)lpvOutBuffer;
            *((pAcceptEx*)lpvOutBuffer) = DetouredAcceptEx;
            cout << "Got acceptex" << endl;
        }
        if (in == connectId)
        {
            _connectEx = *(pConnectEx*)lpvOutBuffer;
            *((pConnectEx*)lpvOutBuffer) = DetouredConnectEx;
            cout << "Got connect" << endl;
        }
    }
    return result;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
BOOL DetouredGetQueuedCompletionStatus(
    HANDLE       CompletionPort,
    LPDWORD      lpNumberOfBytesTransferred,
    PULONG_PTR   lpCompletionKey,
    LPOVERLAPPED* lpOverlapped,
    DWORD        dwMilliseconds
)
{
    return _getQueuedCompletionStatus(CompletionPort, lpNumberOfBytesTransferred, lpCompletionKey, lpOverlapped, dwMilliseconds);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
HANDLE WINAPI DetouredCreateIoCompletionPort(
    _In_     HANDLE    FileHandle,
    _In_opt_ HANDLE    ExistingCompletionPort,
    _In_     ULONG_PTR CompletionKey,
    _In_     DWORD     NumberOfConcurrentThreads
)
{
    auto result = _createIoCompletionPort(FileHandle, ExistingCompletionPort, CompletionKey, NumberOfConcurrentThreads);
    if (FileHandle != NULL && FileHandle != (HANDLE)-1)
    {
        std::lock_guard<std::mutex> lock(_lock);
        _socketManager.SetCompletionPortInfo((SOCKET)FileHandle, result, CompletionKey);
    }
    return result;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void InitDetours()
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)_connect, DetouredConnect);
    DetourAttach(&(PVOID&)_wsaIoctl, DetouredWSAIoctl);
    DetourAttach(&(PVOID&)_closesocket, DetouredClosesocket);
    DetourAttach(&(PVOID&)_wsaAccept, DetouredWSAAccept);
    DetourAttach(&(PVOID&)_wsaSend, DetouredWSASend);
    DetourAttach(&(PVOID&)_wsaRecv, DetouredWSARecv);
    DetourAttach(&(PVOID&)_wsaGetOverlappedResult, DetouredWSAGetOverlappedResult);
    DetourAttach(&(PVOID&)_getQueuedCompletionStatus, DetouredGetQueuedCompletionStatus);
    DetourAttach(&(PVOID&)_createIoCompletionPort, DetouredCreateIoCompletionPort);
    DetourTransactionCommit();
}
