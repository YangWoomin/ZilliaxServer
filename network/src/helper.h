
#ifndef __ZS_NETWORK_HELPER_H__
#define __ZS_NETWORK_HELPER_H__

#include    "internal_common.h"

namespace zs
{
namespace network
{
    class Helper final
    {
    public:
        static bool Initialize();
#if defined(_WIN64_)
        static LPFN_ACCEPTEX                _lpfnAcceptEx;
        static LPFN_CONNECTEX               _lpfnConnectEx;
        static LPFN_GETACCEPTEXSOCKADDRS    _lpfnGetAcceptExSockAddr;
#endif // defined(_WIN64_)
        
        static Socket CreateSocket(IPVer ipVer, Protocol protocol, bool isNonBlocking = false);
#if defined(_LINUX_) 
        static bool MakeSocketNonBlocking(Socket sock);
#endif // defined(_LINUX_) 
        static int32_t GetIPVerValue(IPVer ipVer);
        static int32_t GetProtocolValue(Protocol protocol);
        static int32_t GetSocketTypeValue(Protocol protocol);
        static void GenSockAddr(IPVer ipVer, int32_t port, sockaddr_storage& addr, size_t& len, std::string& host);
        static void GenSockAddr(IPVer ipVer, const char* ip, int32_t port, sockaddr_storage* addr, size_t& len);
        static void GetSockAddr(sockaddr* addr, std::string& host, int32_t& port);
        static void GetSockLocalAddr(Socket sock, IPVer ipVer, std::string& host, int32_t& port);
        static void GetSockRemoteAddr(Socket sock, IPVer ipVer, std::string& host, int32_t& port);
        static bool IsValidIP(const char* ip, IPVer ipVer);
        static void GenAddrInfo(Protocol protocol, addrinfo& hints);
        static void ConvertAddrInfoToSockAdddr(addrinfo* in, int32_t port, sockaddr_storage* out, size_t& len);
     };
}
}

#endif // __ZS_NETWORK_HELPER_H__
