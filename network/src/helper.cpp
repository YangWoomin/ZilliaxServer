
#include    "helper.h"

#include    "common/log.h"

using namespace zs::common;
using namespace zs::network;

#if defined(_WIN64_)
LPFN_ACCEPTEX               Helper::_lpfnAcceptEx = nullptr;
LPFN_CONNECTEX              Helper::_lpfnConnectEx = nullptr;
LPFN_GETACCEPTEXSOCKADDRS   Helper::_lpfnGetAcceptExSockAddr = nullptr;
#endif // defined(_WIN64_)

bool Helper::Initialize()
{
#if defined(_WIN64_)
    // prepare accept
    Socket sock = Helper::CreateSocket(IPVer::IP_V4, Protocol::TCP);
    if (INVALID_SOCKET == sock)
    {
        ZS_LOG_ERROR(network, "creating socket for acceptex failed");
        return false;
    }

    GUID acceptExGuid = WSAID_ACCEPTEX;
    DWORD bytesReturned = 0;
    if (SOCKET_ERROR == WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &acceptExGuid, sizeof(acceptExGuid),
        &_lpfnAcceptEx, sizeof(_lpfnAcceptEx), &bytesReturned, NULL, NULL))
    {
        ZS_LOG_ERROR(network, "preparing acceptex failed, err : %d", 
            errno);
        return false;
    }

    GUID getAcceptExSockAddrsGuid = WSAID_GETACCEPTEXSOCKADDRS;
    bytesReturned = 0;
    if (SOCKET_ERROR == WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &getAcceptExSockAddrsGuid, sizeof(getAcceptExSockAddrsGuid),
        &_lpfnGetAcceptExSockAddr, sizeof(_lpfnGetAcceptExSockAddr), &bytesReturned, NULL, NULL))
    {
        ZS_LOG_ERROR(network, "preparing getacceptexsockaddr failed, err : %d", 
            errno);
        return false;
    }

    // prepare connect
    GUID connectExGuid = WSAID_CONNECTEX;
    bytesReturned = 0;
    if (SOCKET_ERROR == WSAIoctl(sock, SIO_GET_EXTENSION_FUNCTION_POINTER, &connectExGuid, sizeof(connectExGuid),
        &_lpfnConnectEx, sizeof(_lpfnConnectEx), &bytesReturned, NULL, NULL))
    {
        ZS_LOG_ERROR(network, "preparing connectex failed, err : %d", 
            errno);
        return false;
    }
#endif // _WIN64_

    return true;
}

Socket Helper::CreateSocket(IPVer ipVer, Protocol protocol, bool isNonBlocking)
{
#if defined(_WIN64_)
    Socket sock = WSASocket(
        Helper::GetIPVerValue(ipVer), 
        Helper::GetSocketTypeValue(protocol), 
        Helper::GetProtocolValue(protocol), 
        NULL, 0, WSA_FLAG_OVERLAPPED);
    if (INVALID_SOCKET == sock)
    {
        ZS_LOG_ERROR(network, "WSASocket failed, err  : %d",
            errno);
        return INVALID_SOCKET;
    }
#elif defined(_LINUX_) 
    Socket sock = socket(
        Helper::GetIPVerValue(ipVer), 
        Helper::GetSocketTypeValue(protocol), 
        Helper::GetProtocolValue(protocol));
    if (INVALID_SOCKET == sock)
    {
        ZS_LOG_ERROR(network, "socket failed, err  : %d",
            errno);
        return INVALID_SOCKET;
    }
    if (true == isNonBlocking)
    {
        if (false == Helper::MakeSocketNonBlocking(sock))
        {
            ZS_LOG_ERROR(network, "making socket non-blocking failed, socket : %d, ip ver : %d, protocol : %d",
                sock, ipVer, protocol);
            CloseSocket(sock);
            return INVALID_SOCKET;
        }
    }
#endif // _WIN64_
    return sock;
}

#if defined(_LINUX_) 
bool Helper::MakeSocketNonBlocking(Socket sock)
{
    int flags, s;
    flags = fcntl(sock, F_GETFL, 0);
    if (SOCKET_ERROR == flags) 
    {
        ZS_LOG_ERROR(network, "fcntl for F_GETFL failed, socket : %d, err : %d",
            sock, errno);
        return false;
    }
    flags |= O_NONBLOCK;
    s = fcntl(sock, F_SETFL, flags);
    if (s == -1) 
    {
        ZS_LOG_ERROR(network, "fcntl for F_SETFL failed, socket : %d, err : %d",
            sock, errno);
        return false;
    }

    return true;
}
#endif // defined(_LINUX_) 

int32_t Helper::GetIPVerValue(IPVer ipVer)
{
    switch (ipVer)
    {
    case IPVer::IP_V4:
    {
        return AF_INET;
    }
    case IPVer::IP_V6:
    {
        return AF_INET6;
    }
    default:
    {
        ZS_LOG_FATAL(network, "invalid ip version, ip version : %d", ipVer);
        return AF_INET;
    }
    }
}

int32_t Helper::GetProtocolValue(Protocol protocol)
{
    switch (protocol)
    {
    case Protocol::TCP:
    {
        return IPPROTO_TCP;
    }
    case Protocol::UDP:
    {
        return IPPROTO_UDP;
    }
    default:
    {
        ZS_LOG_FATAL(network, "invalid protocol, protocol : %d", protocol);
        return IPPROTO_TCP;
    }
    }
}

int32_t Helper::GetSocketTypeValue(Protocol protocol)
{
    switch (protocol)
    {
    case Protocol::TCP:
    {
        return SOCK_STREAM;
    }
    case Protocol::UDP:
    {
        return SOCK_DGRAM;
    }
    default:
    {
        ZS_LOG_FATAL(network, "invalid protocol, protocol : %d", protocol);
        return SOCK_STREAM;
    }
    }
}

void Helper::GenSockAddr(IPVer ipVer, int32_t port, sockaddr_storage& addr, size_t& len, std::string& host)
{
    std::memset(&addr, 0, sizeof(addr));

    if (IPVer::IP_V4 == ipVer)
    {
        sockaddr_in* addr4 = (sockaddr_in*)&addr;
        addr4->sin_family = AF_INET;
        addr4->sin_addr.s_addr = INADDR_ANY;
        addr4->sin_port = htons((uint16_t)port);
        len = sizeof(sockaddr_in);
        host.resize(INET_ADDRSTRLEN);
        inet_ntop(AF_INET, addr4, host.data(), INET_ADDRSTRLEN);
    }
    else if (IPVer::IP_V6 == ipVer)
    {
        sockaddr_in6* addr6 = (sockaddr_in6*)&addr;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_addr = in6addr_any;
        addr6->sin6_port = htons((uint16_t)port);
        len = sizeof(sockaddr_in6);
        host.resize(INET6_ADDRSTRLEN);
        inet_ntop(AF_INET6, addr6, host.data(), INET6_ADDRSTRLEN);
    }
}

void Helper::GenSockAddr(IPVer ipVer, const char* ip, int32_t port, sockaddr_storage* addr, size_t& len)
{
    std::memset(addr, 0, sizeof(sockaddr_storage));

    if (IPVer::IP_V4 == ipVer)
    {
        sockaddr_in* addr4 = (sockaddr_in*)addr;
        addr4->sin_family = AF_INET;
        inet_pton(AF_INET, ip, &addr4->sin_addr);
        addr4->sin_port = htons((uint16_t)port);
        len = sizeof(addr4);
    }
    else if (IPVer::IP_V6 == ipVer)
    {
        sockaddr_in6* addr6 = (sockaddr_in6*)addr;
        addr6->sin6_family = AF_INET6;
        addr6->sin6_addr = in6addr_any;
        inet_pton(AF_INET6, ip, &addr6->sin6_addr);
        addr6->sin6_port = htons((uint16_t)port);
        len = sizeof(addr6);
    }
}

void Helper::GetSockAddr(sockaddr* addr, std::string& host, int32_t& port)
{
    if (AF_INET == addr->sa_family)
    {
        sockaddr_in* addr4 = (sockaddr_in*)addr;
        host.resize(INET_ADDRSTRLEN);
        inet_ntop(AF_INET, addr4, host.data(), INET_ADDRSTRLEN);
        port = ntohs(addr4->sin_port);
    }
    else if (AF_INET6 == addr->sa_family)
    {
        sockaddr_in6* addr6 = (sockaddr_in6*)addr;
        host.resize(INET6_ADDRSTRLEN);
        inet_ntop(AF_INET6, addr6, host.data(), INET6_ADDRSTRLEN);
        port = ntohs(addr6->sin6_port);
    }
}

void Helper::GetSockLocalAddr(Socket sock, IPVer ipVer, std::string& host, int32_t& port)
{
    if (IPVer::IP_V4 == ipVer)
    {
        sockaddr_in addr4;
        socklen_t len = sizeof(sockaddr_in);
        if (SOCKET_ERROR == getsockname(sock, (sockaddr*)&addr4, &len))
        {
            return;
        }
        host.resize(INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &(addr4.sin_addr), host.data(), host.size());
        port = ntohs(addr4.sin_port);
    }
    else if (IPVer::IP_V6 == ipVer)
    {
        sockaddr_in6 addr6;
        socklen_t len = sizeof(sockaddr_in6);
        if (SOCKET_ERROR == getsockname(sock, (sockaddr*)&addr6, &len))
        {
            return;
        }
        host.resize(INET6_ADDRSTRLEN);
        inet_ntop(AF_INET6, &(addr6.sin6_addr), host.data(), host.size());
    }
}

void Helper::GetSockRemoteAddr(Socket sock, IPVer ipVer, std::string& host, int32_t& port)
{
    if (IPVer::IP_V4 == ipVer)
    {
        sockaddr_in addr4;
        socklen_t len = sizeof(sockaddr_in);
        if (SOCKET_ERROR == getpeername(sock, (sockaddr*)&addr4, &len))
        {
            return;
        }
        host.resize(INET_ADDRSTRLEN);
        inet_ntop(AF_INET, &(addr4.sin_addr), host.data(), host.size());
        port = ntohs(addr4.sin_port);
    }
    else if (IPVer::IP_V6 == ipVer)
    {
        sockaddr_in6 addr6;
        socklen_t len = sizeof(sockaddr_in6);
        if (SOCKET_ERROR == getpeername(sock, (sockaddr*)&addr6, &len))
        {
            return;
        }
        host.resize(INET6_ADDRSTRLEN);
        inet_ntop(AF_INET6, &(addr6.sin6_addr), host.data(), host.size());
    }
}

bool Helper::IsValidIP(const char* ip, IPVer ipVer)
{
    if (IPVer::IP_V4 == ipVer)
    {
        struct in_addr ipv4addr;
        if (1 == inet_pton(AF_INET, ip, &ipv4addr))
        {
            return true;
        }
    }
    else if (IPVer::IP_V6 == ipVer)
    {
        struct in6_addr ipv6addr;
        if (1 == inet_pton(AF_INET6, ip, &ipv6addr))
        {
            return true;
        }
    }

    return false;
}

void Helper::GenAddrInfo(Protocol protocol, addrinfo& hints)
{
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    if (Protocol::TCP == protocol)
    {
        hints.ai_socktype = SOCK_STREAM;
    }
    else if (Protocol::UDP == protocol)
    {
        hints.ai_socktype = SOCK_DGRAM;
    }
}

void Helper::ConvertAddrInfoToSockAdddr(addrinfo* in, int32_t port, sockaddr_storage* out, size_t& len)
{
    std::memset(out, 0, sizeof(sockaddr_storage));

    if (AF_INET == in->ai_family)
    {
        struct sockaddr_in* ipv4 = (struct sockaddr_in*)in->ai_addr;
        ipv4->sin_port = htons((uint16_t)port);
        len = in->ai_addrlen;
        std::memcpy(out, ipv4, len);
    }
    else if (AF_INET6 == in->ai_family)
    {
        struct sockaddr_in6* ipv6 = (struct sockaddr_in6*)in->ai_addr;
        ipv6->sin6_port = htons((uint16_t)port);
        len = in->ai_addrlen;
        std::memcpy(out, ipv6, len);
    }
}
