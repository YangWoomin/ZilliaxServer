
#include    "socket_tcp_messenger.h"

#include    "common/log.h"

#include    "helper.h"
#include    "manager.h"

using namespace zs::common;
using namespace zs::network;

#if defined(_LINUX_) 

bool SocketTCPMessenger::PreRecv(bool& isReceived)
{
    // no pre receive on linux
    
    return true;
}

#endif // defined(_LINUX_) 
