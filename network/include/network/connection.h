
#ifndef __ZS_NETWORK_CONNECTION_H__
#define __ZS_NETWORK_CONNECTION_H__

#include    "internal_common.h"

#include    <string>
#include    <mutex>

namespace zs
{
namespace network
{
    class Connection final : protected std::enable_shared_from_this<Connection>
    {
    public:
        bool Send(const char* buf, std::size_t len);
        bool Send(const std::string& buf);

        IPVer           GetIPVer()      const;
        Protocol        GetProtocol()   const;
        ConnectionID    GetID()         const       { return _id; }

        ~Connection() = default;

    private:
        ConnectionID    _id;
        SocketWPtr      _sock;

        std::mutex      _lock;

        // socket receiving buffer
        // received messages


        void handleConnected();
        void handleReceived();

        Connection(ConnectionID id, SocketWPtr sock);

        Connection(const Connection&) = delete;
        Connection(const Connection&&) = delete;
        Connection& operator=(const Connection&) = delete;

    friend class Manager;
    };
}
}

#endif // __ZS_NETWORK_CONNECTION_H__
