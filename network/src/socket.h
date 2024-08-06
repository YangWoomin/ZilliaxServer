
#ifndef __ZS_NETWORK_SOCKET_H__
#define __ZS_NETWORK_SOCKET_H__

#include    "internal_common.h"

namespace zs
{
namespace network
{
    // ISocket
    class ISocket
    {
    public:
        ISocket(SocketID sockID, SocketType type, IPVer ipVer, Protocol protocol, bool nonBlocking);
        virtual ~ISocket() = default;

        virtual bool Bind(int32_t port) = 0;
        virtual bool Listen(int32_t backlog, OnConnectedSPtr onConnected, OnReceivedSPtr onReceived) = 0;
        virtual void Close();
        virtual bool Connect(const std::string& host, int32_t port, OnConnectedSPtr onConnected, OnReceivedSPtr onReceived) = 0;

        SocketID    GetID()         const       { return _sockID; }
        SocketType  GetType()       const       { return _type; }
        Socket      GetSocket()     const       { return _sock; }
        IPVer       GetIPVer()      const       { return _ipVer; }
        Protocol    GetProtocol()   const       { return _protocol; }
        const char* GetName()       const       { return _name.c_str(); }
        IOContext*  GetContext()                { return _ctx;}

    protected:
        bool        bind(int32_t port);

        SocketID                        _sockID;
        SocketType                      _type;
        Socket                          _sock = INVALID_SOCKET;
        IPVer                           _ipVer;
        Protocol                        _protocol;
        int32_t                         _port = 0;
        std::string                     _name;
        IOContext*                      _ctx = nullptr;
    };

    // SocketGenerator
    class SocketGenerator final
    {
    public:
        static SocketSPtr CreateSocket(SocketID sockID, SocketType type, IPVer ipVer, Protocol protocol, bool nonBlocking = false);
    };

    // SocketTCPListener
    class SocketTCPListener : public ISocket
    {
    public:
        virtual bool Bind(int32_t port) override;
        virtual bool Listen(int32_t backlog, OnConnectedSPtr onConnected, OnReceivedSPtr onReceived) override;
        virtual bool Connect(const std::string& host, int32_t port, OnConnectedSPtr onConnected, OnReceivedSPtr onReceived) override;

        virtual ~SocketTCPListener();

    private:
        SocketTCPListener(SocketID sockID, IPVer ipVer, bool nonBlocking);

        OnConnectedSPtr _onConnected = nullptr;
        OnReceivedSPtr _onReceived = nullptr;

        friend class SocketGenerator;
    };

    // SocketTCPConnector
    class SocketTCPConnector : public ISocket
    {
    public:
        virtual bool Bind(int32_t) override;
        virtual bool Listen(int32_t backlog, OnConnectedSPtr onConnected, OnReceivedSPtr onReceived) override;
        virtual bool Connect(const std::string& host, int32_t port, OnConnectedSPtr onConnected, OnReceivedSPtr onReceived) override;

        virtual ~SocketTCPConnector();

    private:
        SocketTCPConnector(SocketID sockID, IPVer ipVer, bool nonBlocking);

        ConnectContext* prepare(const std::string& host, int32_t port);

        OnConnectedSPtr _onConnected = nullptr;
        OnReceivedSPtr _onReceived = nullptr;

        friend class SocketGenerator;
    };

    class SocketTCP : public ISocket
    {
    public:

    private:
        SocketTCP(SocketID sockID, IPVer ipVer);

        friend class SocketGenerator;
    };

    class SocketUDPListener : public ISocket
    {
    

    private:
        friend class SocketGenerator;
    };
}
}

#endif // __ZS_NETWORK_SOCKET_H__

