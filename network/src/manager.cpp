
#include    "manager.h"

#include    "common/log.h"

#include    "network/network.h"
#include    "helper.h"
#include    "dispatcher.h"
#include    "dispatcher_worker.h"
#include    "socket.h"
#include    "network/connection.h"

#include    <cstring>

using namespace zs::common;
using namespace zs::network;

bool Manager::Start(std::size_t assitantWorkerCount, std::size_t& dispatcherWorkerCount)
{
    if (nullptr != _dispatcher)
    {
        ZS_LOG_ERROR(network, "network manager already started");
        return false;
    }

    if (false == Helper::Initialize())
    {
        ZS_LOG_ERROR(network, "helper init failed");
        return false;
    }

    if (0 == dispatcherWorkerCount)
    {
        dispatcherWorkerCount = std::thread::hardware_concurrency();
    }
    else if (Network::MAX_WORKER_COUNT < dispatcherWorkerCount)
    {
        ZS_LOG_ERROR(network, "invalid worker count, count : %lu, max : %lu", 
            dispatcherWorkerCount, Network::MAX_WORKER_COUNT);
        return false;
    }

    // dispatcher
    _dispatcher = std::make_shared<Dispatcher>();
    if (false == _dispatcher->Initialize(dispatcherWorkerCount))
    {
        ZS_LOG_ERROR(network, "dispatcher init failed");
        return false;
    }

    // dispatcher workers
    for (std::size_t i = 0; i < dispatcherWorkerCount; ++i)
    {
        DispatcherWorkerSPtr worker = std::make_shared<DispatcherWorker>(*this, _dispatcher, i);
        if (false == worker->Start())
        {
            ZS_LOG_ERROR(network, "worker start failed, worker id : %lu", i);
            return false;
        }

        _dispatcherWorkers.push_back(worker);
    }

    // assistant workers

    return true;
}

void Manager::Stop()
{
    if (nullptr != _dispatcher)
    {
        for (std::size_t i = 0; i < _dispatcherWorkers.size(); ++i)
        {
            _dispatcherWorkers[i]->Stop();
        }

        _dispatcher->Stop(_dispatcherWorkers.size());

        for (std::size_t i = 0; i < _dispatcherWorkers.size(); ++i)
        {
            _dispatcherWorkers[i]->Join();
        }

        _dispatcher->Finalize();
    }
}

bool Manager::Bind(IPVer ipVer, Protocol protocol, int32_t port, SocketID& sockID)
{
    if (nullptr == _dispatcher)
    {
        ZS_LOG_ERROR(network, "network manager not started");
        return false;
    }

    SocketSPtr sock = SocketGenerator::CreateSocket(*this, ++_sockIDGen, SocketType::ACCEPTER, ipVer, protocol);
    if (nullptr == sock)
    {
        ZS_LOG_ERROR(network, "creating listen socket failed, sock id : %llu, ip ver : %d, protocol : %d, port : %d",
            sock->GetID(), ipVer, protocol, port);
        return false;
    }

    if (false == sock->Bind(port))
    {
        ZS_LOG_ERROR(network, "binding listen socket failed, sock id : %llu, ip ver : %d, protocol : %d, port : %d",
            sock->GetID(), ipVer, protocol, port);
        return false;
    }

    if (false == InsertSocket(sock))
    {
        ZS_LOG_ERROR(network, "inserting listen socket failed, sock id : %llu, socket name : %s, ip ver : %d, protocol : %d",
            sock->GetID(), sock->GetName(), ipVer, protocol);
        return false;
    }

    // bind the socket on dispatcher
#if defined(_LINUX_) 
    std::size_t workerID = 0;
    workerID = _workerAllocator++ % _dispatcherWorkers.size();
    if (false == _dispatcher->Bind(workerID, sock.get(), BindType::BIND, EventType::INBOUND))
#elif defined(_WIN64_)
    if (false == _dispatcher->Bind(sock.get()))
#endif // defined(_LINUX_) 
    {
        ZS_LOG_ERROR(network, "binding listen socket on dispatcher failed, sock id : %llu, socket name : %s, ip ver : %d, protocol : %d",
            sock->GetID(), sock->GetName(), ipVer, protocol);
        RemoveSocket(sock->GetID()); // not bound yet
        return false;
    }

    sockID = sock->GetID();

    ZS_LOG_INFO(network, "binding listen socket succeeded, sock id : %llu, socket name : %s, ip ver : %d, protocol : %d",
        sock->GetID(), sock->GetName(), ipVer, protocol);

    return true;
}

bool Manager::Listen(SocketID sockID, int32_t backlog, OnConnectedSPtr onConnected, OnReceivedSPtr onReceived, OnClosedSPtr onClosed)
{
    if (nullptr == _dispatcher)
    {
        ZS_LOG_ERROR(network, "network manager not started");
        return false;
    }

    SocketSPtr sock = GetSocket(sockID);
    if (nullptr == sock)
    {
        ZS_LOG_ERROR(network, "invalid socket id for listen, socket id : %llu", 
            sockID);
        return false;
    }

    if (false == sock->Listen(backlog))
    {
        ZS_LOG_ERROR(network, "listen failed, socket id : %llu, socket name : %s", 
            sockID, sock->GetName());
        sock->Close();
        return false;
    }

    sock->SetCallback(onConnected, onReceived, onClosed);

    ZS_LOG_INFO(network, "listen succeeded, sock id : %llu, socket name : %s",
        sock->GetID(), sock->GetName());

    return true;
}

bool Manager::Close(SocketID sockID)
{
    SocketSPtr sock = GetSocket(sockID);
    if (nullptr == sock)
    {
        ZS_LOG_WARN(network, "invalid socket id for close, socket id : %llu", 
            sockID);
        return false;
    }

    sock->Close();

    return true;
}

bool Manager::Connect(IPVer ipVer, Protocol protocol, const std::string& host, int32_t port, OnConnectedSPtr onConnected, OnReceivedSPtr onReceived, OnClosedSPtr onClosed)
{
    if (nullptr == _dispatcher)
    {
        ZS_LOG_ERROR(network, "network manager not started");
        return false;
    }

    SocketSPtr sock = SocketGenerator::CreateSocket(*this, ++_sockIDGen, SocketType::CONNECTOR, ipVer, protocol, true/*nonBlocking*/);
    if (nullptr == sock)
    {
        ZS_LOG_ERROR(network, "creating listen socket failed, sock id : %llu, ip ver : %d, protocol : %d, port : %d",
            sock->GetID(), ipVer, protocol, port);
        return false;
    }

    if (false == InsertSocket(sock))
    {
        ZS_LOG_ERROR(network, "inserting connect socket failed, sock id : %llu, ip ver : %d, protocol : %d, host : %s, port : %d",
            sock->GetID(), sock->GetName(), ipVer, protocol, host.c_str(), port);
        return false;
    }

    if (false == sock->Bind(0))
    {
        ZS_LOG_ERROR(network, "binding connect socket failed, sock id : %llu, ip ver : %d, protocol : %d, host : %s, port : %d",
            sock->GetID(), sock->GetName(), ipVer, protocol, host.c_str(), port);
        RemoveSocket(sock->GetID()); // not bound yet
        return false;
    }

    // bind the socket on dispatcher
#if defined(_LINUX_) 
    std::size_t workerID = 0;
    workerID = _workerAllocator++ % _dispatcherWorkers.size();
    if (false == _dispatcher->Bind(workerID, sock.get(), BindType::BIND, EventType::OUTBOUND))
#elif defined(_WIN64_)
    if (false == _dispatcher->Bind(sock.get()))
#endif // defined(_LINUX_) 
    {
        ZS_LOG_ERROR(network, "binding connect socket on dispatcher failed, sock id : %llu, socket name : %s, ip ver : %d, protocol : %d, host : %s, port : %d",
            sock->GetID(), sock->GetName(), ipVer, protocol, host.c_str(), port);
        RemoveSocket(sock->GetID()); // not bound yet
        return false;
    }

    if (false == sock->Connect(host, port))
    {
        ZS_LOG_ERROR(network, "inserting connect socket failed, sock id : %llu, ip ver : %d, protocol : %d, host : %s, port : %d",
            sock->GetID(), sock->GetName(), ipVer, protocol);
        sock->Close();
        return false;
    }

    sock->SetCallback(onConnected, onReceived, onClosed);

    ZS_LOG_INFO(network, "connecting succeeded, sock id : %llu, ip ver : %d, protocol : %d, host : %s, port : %d",
        sock->GetID(), sock->GetName(), ipVer, protocol, host.c_str(), port);

    return true;
}

void Manager::HandleAccepted(SocketSPtr sock)
{
    // check whether a socket is accepted well
    std::string name, peer;
    if (false == sock->PostAccept(name, peer))
    {
        ZS_LOG_ERROR(network, "post accepting failed, sock id : %llu, socket name : %s",
            sock->GetID(), sock->GetName());
        sock->Close();
        return;
    }

    // create a new socket(object) for the accepted real socket
    SocketSPtr newSock = SocketGenerator::CreateSocket(
        *this, ++_sockIDGen, sock->GetAcceptContext()->_sock,
        name, peer, SocketType::MESSENGER, 
        sock->GetIPVer(), sock->GetProtocol());
    if (nullptr == newSock)
    {
        ZS_LOG_ERROR(network, "creating messenger socket failed, sock id : %llu, socket name : %s, peer : %s, ip ver : %d, protocol : %d",
            sock->GetID(), name.c_str(), peer.c_str(), sock->GetIPVer(), sock->GetProtocol());
        sock->Close();
        return;
    }

    if (false == InsertSocket(newSock))
    {
        ZS_LOG_ERROR(network, "inserting messenger socket failed, sock id : %llu, socket name : %s, peer : %s",
            newSock->GetID(), newSock->GetName(), newSock->GetPeer());
        sock->Close();
        return;
    }

    // create a connection for the accepted socket
    ConnectionSPtr conn = ConnectionSPtr(new Connection(++_connIDGen, newSock));
    if (false == InsertConnection(conn))
    {
        ZS_LOG_ERROR(network, "inserting conn failed in handling accepted, sock id : %llu, socket name : %s, peer : %s",
            newSock->GetID(), newSock->GetName(), newSock->GetPeer());
        RemoveSocket(newSock->GetID());
        return;
    }

    // bind the accepted socket to dispatcher for data received event
#if defined(_LINUX_) 
    std::size_t workerID = 0;
    workerID = _workerAllocator++ % _dispatcherWorkers.size();
    if (false == _dispatcher->Bind(workerID, newSock.get(), BindType::BIND, EventType::INBOUND))
#elif defined(_WIN64_)
    if (false == _dispatcher->Bind(newSock.get()))
#endif // defined(_LINUX_) 
    {
        ZS_LOG_ERROR(network, "binding messenger socket on dispatcher failed, sock id : %llu, socket name : %s, peer : %s",
            newSock->GetID(), newSock->GetName(), newSock->GetPeer());
        RemoveSocket(newSock->GetID()); // not bound yet
        return;
    }
    
    // invoke onConnected with the connection as a parameter
    conn->handleConnected();

    // if in windows reinitiate async accept for the next socket 
    if (false == sock->PreAccept())
    {
        ZS_LOG_ERROR(network, "pre accepting failed, sock id : %llu, socket name : %s",
            sock->GetID(), sock->GetName());
        sock->Close(); // close the listen socket
        //return; // continue for the new socket
    }

    // if in windows initiate async receive on the accepted socket 
    bool isReceived = false;
    if (false == newSock->PreRecv(isReceived))
    {
        ZS_LOG_ERROR(network, "pre receiving failed in handling accepted, sock id : %llu, socket name : %s, peer : %s",
            newSock->GetID(), newSock->GetName(), newSock->GetPeer());
        newSock->Close();
        return;
    }

    if (true == isReceived)
    {
        conn->handleReceived();
    }

    return;
}

void Manager::HandleConnected(SocketSPtr sock)
{
    // check whether the connection to remote is established well
    // if not, retry to connect with the other ip (in context)
    // if all of tries failed then invoke onConnected with null pointer of connection
    bool retry = false;
    if (false == sock->PostConnect(retry))
    {
        ZS_LOG_ERROR(network, "post connect failed, sock id : %llu, socket name : %s",
            sock->GetID(), sock->GetName());
        sock->Close();
        return;
    }
    if (true == retry)
    {
        return;
    }

    // create a connection for the connected socket
    ConnectionSPtr conn = ConnectionSPtr(new Connection(++_connIDGen, sock));
    if (false == InsertConnection(conn))
    {
        ZS_LOG_ERROR(network, "inserting conn failed in handling connected, sock id : %llu, socket name : %s, peer : %s",
            sock->GetID(), sock->GetName(), sock->GetPeer());
        sock->Close();
        return;
    }

    // only in linux
    // unbind the connected socket from dispatcher for connected event
    // bind the connected socket to dispatcher for data received event
#if defined(_LINUX_) 
    if (false == _dispatcher->Bind(sock->GetWorkerID(), sock.get(), BindType::MODIFY, EventType::INBOUND))
    {
        ZS_LOG_ERROR(network, "binding connector socket on dispatcher failed, sock id : %llu, socket name : %s, peer : %s",
            sock->GetID(), sock->GetName(), sock->GetPeer());
        sock->Close();
        return;
    }
#endif // defined(_LINUX_) 

    // change the socket type from connector to messenger
    sock->ChangeType(SocketType::MESSENGER);

    // invoke onConnected with the connection as a parameter
    conn->handleConnected();

    // if windows, initiate async receive on the connected socket
    bool isReceived = false;
    if (false == sock->PreRecv(isReceived))
    {
        ZS_LOG_ERROR(network, "pre receiving failed in handling connected, sock id : %llu, socket name : %s, peer : %s",
            sock->GetID(), sock->GetName(), sock->GetPeer());
        sock->Close();
        return;
    }

    if (true == isReceived)
    {
        conn->handleReceived();
    }

    // ** UDP ** ===>> no udp connector
    // nothing to implement

    return;
}

void Manager::HandleReceived(SocketSPtr sock)
{
    // ** in TCP socket **
    // append received data to receiving buffer in connection
    // if the buffer length is enough to make a message then make it
    // pass the message to message buffer in connection
    // invoke onReceived
    // if in windows reinitiate async receive on the socket

    // ** UDP **
    // check whether data is received well
    // check whether the socket has a connection
    // if not, create a connection and invoke onConnected
    // invoke onReceived
    // if in windows reinitiate async receive on the socket
}

void Manager::HandleSent(SocketSPtr sock)
{
    // notify to a assistant worker that the sending buffer of the socket is available
}

bool Manager::InsertSocket(SocketSPtr sock)
{
    std::lock_guard<std::mutex> locker(_lock);

    auto res = _sockets.insert(std::make_pair(sock->GetID(), sock));
    return res.second;
}

void Manager::RemoveSocket(SocketID sockID)
{
    std::lock_guard<std::mutex> locker(_lock);

    _sockets.erase(sockID);
}

SocketSPtr Manager::GetSocket(SocketID sockID)
{
    std::lock_guard<std::mutex> locker(_lock);

    auto finder = _sockets.find(sockID);
    if (_sockets.end() != finder)
    {
        return finder->second;
    }
    return nullptr;
}

bool Manager::InsertConnection(ConnectionSPtr conn)
{
    std::lock_guard<std::mutex> locker(_lock);

    auto res = _connections.insert(std::make_pair(conn->GetID(), conn));
    return res.second;
}

ConnectionSPtr Manager::RemoveConnection(ConnectionID connID)
{
    ConnectionSPtr ret { nullptr };
    std::lock_guard<std::mutex> locker(_lock);

    auto finder = _connections.find(connID);
    if (_connections.end() != finder)
    {
        ret = finder->second;
        _connections.erase(connID);
        return ret;
    }
    
    return nullptr;
}

ConnectionSPtr Manager::GetConnection(ConnectionID connID)
{
    std::lock_guard<std::mutex> locker(_lock);

    auto finder = _connections.find(connID);
    if (_connections.end() != finder)
    {
        return finder->second;
    }
    return nullptr;
}
