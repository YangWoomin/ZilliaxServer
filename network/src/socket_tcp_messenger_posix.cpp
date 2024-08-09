
#include    "socket_tcp_messenger.h"

#include    "common/log.h"

#include    "helper.h"
#include    "manager.h"

using namespace zs::common;
using namespace zs::network;

#if defined(__GNUC__) || defined(__clang__)

bool SocketTCPMessenger::PreRecv(bool& isReceived)
{
    // no pre receive on linux
    
    return true;
}

#endif // defined(__GNUC__) || defined(__clang__)
