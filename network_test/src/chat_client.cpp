
#include    "network_test/chat_client.h"

#include    <iostream>
#include    <atomic>
#include    <thread>

void ChatClient(Logger::Messenger msgr, IPVer ipVer, Protocol protocol, std::string serverHost, int32_t serverPort)
{
    if (false == Network::Initialize(msgr))
    {
        ZS_LOG_ERROR(network_test, "Network::Initialize failed in ChatClient");
        return;
    }

    if (false == Network::Start())
    {
        ZS_LOG_ERROR(network_test, "Network::Start failed in ChatClient");
        return;
    }

    std::atomic<ConnectionSPtr> server { nullptr };

    // OnConnected
    OnConnectedSPtr onConnected = std::make_shared<OnConnected>([&server](ConnectionSPtr conn) {
        server = conn;
        if (nullptr != conn)
        {
            ZS_LOG_INFO(network_test, "connected to server in ChatClient, conn id : %llu, peer : %s",
                conn->GetID(), conn->GetPeer());
        }
        else
        {
            ZS_LOG_ERROR(network_test, "connetion is nullptr in ChatClient");
        }
    });

    // OnReceived
    OnReceivedSPtr onReceived = std::make_shared<OnReceived>([](ConnectionSPtr conn, const char* buf, std::size_t len) {
        ZS_LOG_INFO(network_test, "data received, conn id : %llu, peer : %s, data : %s",
            conn ? conn->GetID() : 0, conn ? conn->GetPeer() : "unknown", buf);
    });

    // OnClosed
    std::atomic<bool> run { true };
    OnClosedSPtr onClosed = std::make_shared<OnClosed>([&run](ConnectionSPtr conn) {
        ZS_LOG_WARN(network_test, "the connection closed, conn id : %llu, peer : %s", 
            conn ? conn->GetID() : 0, conn ? conn->GetPeer() : "unknown");
        
        run = false;
    });

    if (Protocol::TCP == protocol)
    {
        Network::ConnectTCP(ipVer, serverHost, serverPort, onConnected, onReceived, onClosed);
    }
    else if (Protocol::UDP == protocol)
    {
        server = Network::ConnectUDP(ipVer, serverHost, serverPort, onReceived);
    }
    else
    {
        ZS_LOG_ERROR(network_test, "invalid protocol in ChatClient, protocol : %d",
            protocol);
        return;
    }

    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::string input;
    while (true == run)
    {
        std::cout << "Enter a line of text (type 'exit' to quit): ";
        std::getline(std::cin, input);

        if ("exit" == input)
        {
            
            break;
        }

        ConnectionSPtr conn = server.load();
        if (nullptr != conn)
        {
            conn->Send(input);
        }
    }

    Network::Finalize();
}

