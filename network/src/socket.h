
#ifndef __ZS_NETWORK_SOCKET_H__
#define __ZS_NETWORK_SOCKET_H__

#include    "internal_common.h"

namespace zs
{
namespace network
{
    class Manager;

    class ISocket : public std::enable_shared_from_this<ISocket>
    {
    public:
        ISocket(Manager& manager, SocketID sockID, SocketType type, IPVer ipVer, Protocol protocol, bool nonBlocking);
        ISocket(Manager& manager, SocketID sockID, Socket sock, const std::string& name, const std::string& peer, SocketType type, IPVer ipVer, Protocol protocol);
        virtual ~ISocket();

        virtual bool        Bind(int32_t port);
        virtual bool        Listen(int32_t backlog);
        virtual bool        InitAccept();
                void        Close();
        virtual bool        InitConnect(const std::string& host, int32_t port);
        virtual bool        InitSend(std::string&& buf);
        virtual bool        ContinueSend();
        virtual bool        InitReceive();

#if defined(_POSIX_) 
        virtual bool        OnAccepted();
        virtual bool        OnReceived(bool& later);
#endif // defined(_POSIX_) 

        virtual SocketSPtr  PostAccept();   
        virtual bool        PostConnect(bool& retry);
        virtual bool        PostSend();
        virtual bool        PostReceive();

        SocketSPtr          GetSPtr()                           { return shared_from_this(); }
        SocketID            GetID()             const           { return _sockID; }
        SocketType          GetType()           const           { return _type; }
        void                ChangeType(SocketType type)         { _type = type; }
        Socket              GetSocket()         const           { return _sock; }
        IPVer               GetIPVer()          const           { return _ipVer; }
        Protocol            GetProtocol()       const           { return _protocol; }
        int32_t             GetPort()           const           { return _port; }
        std::size_t         GetWorkerID()       const           { return _workerID; }
        void                SetWorkerID(std::size_t id)         { _workerID = id; }
        const char*         GetName()           const           { return _name.c_str(); }
        const char*         GetPeer()           const           { return _peer.c_str(); }

        AcceptContext*      GetAcceptContext()                  { return _aCtx; }
        ConnectContext*     GetConnectContext()                 { return _cCtx; }
        SendRecvContext*    GetSendContext()                    { return _sCtx; }
        SendRecvContext*    GetRecvContext()                    { return _rCtx; }

        void SetCallback(OnConnectedSPtr onConnected, OnReceivedSPtr onReceived, OnClosedSPtr onClosed);
        OnConnectedSPtr     GetOnConnected()                    { return _onConnected; }
        OnReceivedSPtr      GetOnReceived()                     { return _onReceived; }
        OnClosedSPtr        GetOnClosed()                       { return _onClosed; }
        void                SetConnection(ConnectionSPtr conn)  { _conn = conn; }

    protected:
        bool                bind(int32_t port);

        Manager&            _manager;
        SocketID            _sockID;
        SocketType          _type;
        Socket              _sock = INVALID_SOCKET;
        IPVer               _ipVer;
        Protocol            _protocol;
        int32_t             _port = 0;
        std::size_t         _workerID = 0;
        std::string         _name;
        std::string         _peer;
        std::mutex          _lock;
        
        AcceptContext*      _aCtx = nullptr;
        ConnectContext*     _cCtx = nullptr;
        SendRecvContext*    _sCtx = nullptr;
        SendRecvContext*    _rCtx = nullptr;

        OnConnectedSPtr     _onConnected { nullptr };
        OnReceivedSPtr      _onReceived { nullptr };
        OnClosedSPtr        _onClosed { nullptr };

        ConnectionSPtr      _conn { nullptr };

        ISocket(const ISocket&) = delete;
        ISocket(const ISocket&&) = delete;
        ISocket& operator=(const ISocket&) = delete;

        static std::atomic<ConnectionID>    _connIDGen;
    };

    ////////////////////////////////////////////////////////////////////////
    // SocketGenerator
    class SocketGenerator final
    {
    public:
        static SocketSPtr CreateSocket(Manager& manager, SocketID sockID, SocketType type, IPVer ipVer, Protocol protocol, bool nonBlocking = false);
        static SocketSPtr CreateSocket(Manager& manager, SocketID sockID, Socket sock, const std::string& name, const std::string& peer, SocketType type, IPVer ipVer, Protocol protocol);
    };

    ////////////////////////////////////////////////////////////////////////
    // SocketUDP
    class SocketUDP : public ISocket
    {
    public:
        virtual bool Bind(int32_t) override;

        // bind a socket by calling "bind" function
        // bind the socket to dispatcher for data received event
        // if the socket receives data which no connection is bound with
        // create a connection and invoke onConnected callback with the connection as a parameter
        // after invoking onConnected process the received data and invoke onReceived
        // if the socket already has a connection just invoke onReceived
        virtual bool Listen(int32_t backlog) override;

        // invoke the "connect" to make the socket "connected" which has a remote address
        // immediately invoke onConnected callback with a new connection which has the connected socket
        // bind the socket to dispatcher for data received event
        // if the socket receives data invoke onReceived
        virtual bool InitConnect(const std::string& host, int32_t port) override;

        // send is available after the socket connected
        virtual bool InitSend(std::string&& buf) override;

        // recv is available after the socket connected
        virtual bool InitReceive() override;

    private:
        SocketUDP(SocketID sockID, IPVer ipVer, bool nonBlocking);

        SocketUDP(const SocketUDP&) = delete;
        SocketUDP(const SocketUDP&&) = delete;
        SocketUDP& operator=(const SocketUDP&) = delete;

        friend class SocketGenerator;
    };
}
}

#endif // __ZS_NETWORK_SOCKET_H__
