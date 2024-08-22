
#include    "network_test/chat_massive_test_client.h"

#include    <iostream>
#include    <atomic>
#include    <thread>
#include    <fstream>
#include    <filesystem>

#if defined(_WIN64_)
#include    <windows.h>
#elif defined(_POSIX_)
#include    <csignal>
#include    <unistd.h>
#endif // defined(_WIN64_)

namespace fs = std::filesystem;

// from chatgpt
void readTextFilesInDirectory(const std::string& directoryPath, std::vector<std::string>& text, std::size_t& totalSize) {
    for (const auto& entry : fs::directory_iterator(directoryPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            std::ifstream file(entry.path());
            if (!file.is_open()) {
                std::cerr << "Could not open file: " << entry.path() << std::endl;
                continue;
            }

            std::string line;
            while (std::getline(file, line)) {
                if (!line.empty()) {
                    text.push_back(line);
                    totalSize += line.size();
                }
            }

            file.close();
        }
    }
}

static std::atomic<bool> run { true };

#if defined(_WIN64_)
static BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT) 
    {
        ZS_LOG_WARN(network_test, "Ctrl+C detected in ChatMassiveTestClient");
        
        Network::Finalize();

        run = false;

        return TRUE;
    }
    return FALSE;
}
#elif defined(_POSIX_)
static void signalHandler(int signum)
{
    if (signum == SIGINT)
    {
        ZS_LOG_WARN(network_test, "Interrupt signal (SIGINT) received in ChatMassiveTestClient. Exiting gracefully...");
    }
    else
    {
        ZS_LOG_ERROR(network_test, "Unhandled interrupt signal %d received in ChatMassiveTestClient", signum);
    }
    
    run = false;
}
#endif // defined(_WIN64_)

void ChatMassiveTestClient(Logger::Messenger msgr, IPVer ipVer, Protocol protocol, std::string serverHost, int32_t serverPort, std::size_t clientCount, std::string sampleFileDir)
{

#if defined(_WIN64_)
    if (SetConsoleCtrlHandler(ConsoleHandler, TRUE))
    {
        ZS_LOG_INFO(network_test, "Ctrl+C handler installed in ChatMassiveTestClient");
    } 
    else 
    {
        ZS_LOG_ERROR(network_test, "Failed to install Ctrl+C handler in ChatMassiveTestClient");
        return;
    }
#elif defined(_POSIX_)
    signal(SIGINT, signalHandler);
    ZS_LOG_INFO(network_test, "Ctrl+C handler installed.");
#endif // defined(_WIN64_)

    if (false == Network::Initialize(msgr))
    {
        ZS_LOG_ERROR(network_test, "Network::Initialize failed in ChatMassiveTestClient");
        return;
    }

    if (false == Network::Start())
    {
        ZS_LOG_ERROR(network_test, "Network::Start failed in ChatMassiveTestClient");
        return;
    }

    // create testers
    std::vector<std::shared_ptr<ChatMassiveTester>> testers;
    for (std::size_t i = 0; i < clientCount; ++i)
    {
        auto tester = std::make_shared<ChatMassiveTester>();
        if (false == tester->Connect(ipVer, protocol, serverHost, serverPort))
        {
            ZS_LOG_ERROR(network_test, "conneting to server in tester failed in ChatMassiveTestClient, tester num : %llu",
                i);
            return;
        }
        testers.push_back(tester);
    }

    // load sample files
    std::vector<std::string> text;
    std::size_t totalSize = 0;
    readTextFilesInDirectory(sampleFileDir, text, totalSize);

    ZS_LOG_INFO(network_test, "total text line number : %llu, size : %llu", 
        text.size(), totalSize);

    // send sample text
    std::vector<std::thread> threads;
    for (std::size_t i = 0; i < clientCount; ++i)
    {
        std::thread thread(&ChatMassiveTester::Send, testers[i].get(), text);
        threads.push_back(std::move(thread));
    }

    for (std::size_t i = 0; i < clientCount; ++i)
    {
        threads[i].join();
    }

    ZS_LOG_INFO(network_test, "sending all text finished.. waiting..");

    while (run.load())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    Network::Finalize();

    std::this_thread::sleep_for(std::chrono::seconds(5));
}

bool ChatMassiveTester::Connect(IPVer ipVer, Protocol protocol, const std::string& serverHost, int32_t serverPort)
{
    OnConnected onConnected = [this](ConnectionSPtr conn) {
        _server = conn;
        if (nullptr != conn)
        {
            ZS_LOG_INFO(network_test, "connected to server in ChatMassiveTestClient, conn id : %llu, peer : %s",
                conn->GetID(), conn->GetPeer());
        }
        else
        {
            ZS_LOG_ERROR(network_test, "connetion is nullptr in ChatMassiveTestClient");
        }
    };

    // OnReceived
    OnReceived onReceived = [this](ConnectionSPtr conn, const char* buf, std::size_t len) {
        ++_recvMsgCount;

        if (0 == _recvMsgCount.load() % 1000)
        {
            ZS_LOG_INFO(network_test, "data received, conn id : %llu, peer : %s, data : %s",
                conn ? conn->GetID() : 0, conn ? conn->GetPeer() : "unknown", buf);
        }
    };

    // OnClosed
    OnClosed onClosed = [](ConnectionSPtr conn) {
        ZS_LOG_WARN(network_test, "the connection closed, conn id : %llu, peer : %s", 
            conn ? conn->GetID() : 0, conn ? conn->GetPeer() : "unknown");
    };

    if (Protocol::TCP == protocol)
    {
        return Network::ConnectTCP(ipVer, serverHost, serverPort, onConnected, onReceived, onClosed);
    }
    else if (Protocol::UDP == protocol)
    {
        _server = Network::ConnectUDP(ipVer, serverHost, serverPort, onReceived);
        return nullptr != _server;
    }
    else
    {
        ZS_LOG_ERROR(network_test, "invalid protocol in ChatMassiveTestClient, protocol : %d",
            protocol);
        return false;
    }

    return false;
}

void ChatMassiveTester::Send(const std::vector<std::string>& text)
{
    if (nullptr == _server)
    {
        ZS_LOG_WARN(network_test, "server is not connected yet");
        return;
    }

    for (const auto& i : text)
    {
        _server->Send(i);
    }
}
