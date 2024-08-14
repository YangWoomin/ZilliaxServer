
#ifndef __ZS_NETWORK_CONNECTION_H__
#define __ZS_NETWORK_CONNECTION_H__

#include    "network/common.h"

#include    <string>

namespace zs
{
namespace network
{
    class Connection final 
        : protected std::enable_shared_from_this<Connection>
    {
    public:
        __ZS_NETWORK_API bool Send(std::string&& buf);
        __ZS_NETWORK_API bool Send(std::string& buf);
        __ZS_NETWORK_API bool Send(const char* buf, std::size_t len);
        __ZS_NETWORK_API void Close();

        __ZS_NETWORK_API IPVer           GetIPVer()      const;
        __ZS_NETWORK_API Protocol        GetProtocol()   const;
        __ZS_NETWORK_API ConnectionID    GetID()         const       { return _id; }
        __ZS_NETWORK_API const char*     GetPeer()       const;

        ~Connection() = default;

    private:
        ConnectionID    _id;
        SocketWPtr      _sock;

        Connection(ConnectionID id, SocketWPtr sock);

        Connection(const Connection&) = delete;
        Connection(const Connection&&) = delete;
        Connection& operator=(const Connection&) = delete;

    friend class SocketTCPAceepter;
    friend class SocketTCPMessenger;
    friend class SocketTCPConnector;
    };
}
}

#endif // __ZS_NETWORK_CONNECTION_H__
