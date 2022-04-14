#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

//#include <winsock2.h> включен в ws2tcpip.h
#include <ws2tcpip.h> 
#include <iostream>
#include <vector>
#include <thread>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

// выводит в консоль данные из addrinfo* res
void addrinf_out(ADDRINFOA& inf, PADDRINFOA res);

int main()
{
    // данные для инициализации сервера
    WSAData wsaData;
    DWORD DllVers = MAKEWORD(2, 2);
    const int ServPort = 5053;
    SOCKADDR_IN addr;
    int sizeOfAddr = sizeof(addr);
    SOCKET sockListen;
    // клиентские сокеты и потоки
    std::vector <SOCKET> clConnections;
    std::vector <std::thread> ths;

    // переменная для сообщения клиенту при подключении
    std::string welcomeMsg = "";

    // переменные для получения информации о сервере
    ADDRINFOA inf = { 0 };
    PADDRINFOA res = NULL;



    if (WSAStartup(DllVers, &wsaData)) {
        std::cout << "Error WSAData!\n"; system("pause"); exit(EXIT_FAILURE);
    }

    addr.sin_addr.S_un.S_addr = INADDR_ANY;//inet_addr("127.0.0.1");
    addr.sin_port = htons(ServPort);
    addr.sin_family = AF_INET;

    sockListen = socket(AF_INET, SOCK_STREAM, NULL);
    bind(sockListen, (SOCKADDR*)&addr, sizeOfAddr);
    listen(sockListen, SOMAXCONN);

    // получение информация о сервере
    inf.ai_family = AF_UNSPEC;
    inf.ai_socktype = SOCK_STREAM;
    inf.ai_protocol = IPPROTO_TCP;
    addrinf_out(inf, res);
    std::cout << " \n\n";

    // ретрансляция сообщений от клиента всем остальным клиентам
    auto ClientHandler = [&](int index)
    { char* clMsg;
    int msgSize{ 0 };
    while (1)
    {
        if (SOCKET_ERROR == recv(clConnections[index], (char*)&msgSize, sizeof(int), NULL))
        {
            std::cout << "Error SOCKET_ERROR!\n" << "  SOCKET " << index << "\n";
            ths[index].detach();
            break;
        }
        clMsg = new char[msgSize + 1];
        recv(clConnections[index], clMsg, msgSize, NULL);
        clMsg[msgSize] = '\0';
        for (int i = 0; i <= clConnections.size() - 1; ++i)
            if (i != index) {
                send(clConnections[i], (char*)&msgSize, sizeof(int), NULL);
                send(clConnections[i], clMsg, msgSize, NULL);
            }
        delete[] clMsg;
    }
    };

    // подключение клиентов и запуск потока прослушивания для каждого клиента
    while (1)
    {
        int msgSize{ 0 };
        clConnections.emplace_back();
        if (!(clConnections[clConnections.size() - 1] = accept(sockListen, (SOCKADDR*)&addr, &sizeOfAddr)))
        {
            std::cout << "Error newConnect!\n";  system("pause"); exit(EXIT_FAILURE);
        }
        else std::cout << "Succesfull Connect with a client " << clConnections.size() - 1 << " !\n";
        welcomeMsg = "Test message " + std::to_string(clConnections.size());
        msgSize = welcomeMsg.size();
        send(clConnections[clConnections.size() - 1], (char*)&msgSize, sizeof(int), NULL);
        send(clConnections[clConnections.size() - 1], welcomeMsg.c_str(), msgSize, NULL);
        ths.emplace_back(ClientHandler, (clConnections.size() - 1));
    }

    // безопасное завершение потоков и WSA
    for (auto& t : ths)
        t.join();

    WSACleanup();
    system("pause");
}

void addrinf_out(ADDRINFOA& inf, PADDRINFOA res)
{
    PADDRINFOA ptr = NULL;
    char ipstringbuffer[50]{ 0 };
    DWORD ipbufferlength = sizeof(ipstringbuffer);
    int i = 0;
    char comp_name[256];
    int size = sizeof(comp_name); //unsigned long совместим с LPDWORD необходимымы в GetComputerNameA

    GetComputerNameA(comp_name, (LPDWORD)&size);
    if (getaddrinfo(comp_name, "0", &inf, &res))
    {
        std::cout << "Getaddrinfo  Error  \n\n"; system("pause");
        freeaddrinfo(res);
        return;
    }

    for (ptr = res; ptr != NULL; ptr = ptr->ai_next)
    {
        std::cout << "getaddrinfo response " << i++ << "\n";
        std::cout << "\tFlags: " << ptr->ai_flags << "\n";
        std::cout << "\tFamily: ";
        switch (ptr->ai_family)
        {
        case AF_UNSPEC:
            std::cout << "Unspecified\n";
            break;
        case AF_INET:
            std::cout << "AF_INET (IPv4)\n";
            std::cout << "\tIPv4 address " << inet_ntoa(((PSOCKADDR_IN)ptr->ai_addr)->sin_addr) << "\n";
            break;
        case AF_INET6:
            std::cout << "AF_INET6 (IPv6)\n";
            if (WSAAddressToStringA((LPSOCKADDR)ptr->ai_addr, (DWORD)ptr->ai_addrlen, NULL,
                ipstringbuffer, &ipbufferlength))
                std::cout << "WSAAddressToString failed with " << WSAGetLastError() << "\n";
            else
                std::cout << "\tIPv6 address " << ipstringbuffer << "\n";
            break;
        case AF_NETBIOS:
            std::cout << "AF_NETBIOS (NetBIOS)\n";
            break;
        default:
            std::cout << "Other " << ptr->ai_family << "\n";
            break;
        }

    }
    freeaddrinfo(res);

}