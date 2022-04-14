#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <vector>
#include <stdio.h>
#include <iostream>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")

int main()
{
    // данные для инициализации клиента
    WSAData wsaData;
    DWORD DllVers = MAKEWORD(2, 2);
    SOCKADDR_IN addr;
    int sizeOfAddr = sizeof(addr);
    const int ServPort = 5053;
    // сокет для связи с сервером
    SOCKET Connect;

    // переменная для отправки сообщений в чат через сервер
    std::string msg = "";

    // переменная для контроля состояния соединения с сервером
    bool work = true;


    if (WSAStartup(DllVers, &wsaData)) {
        std::cout << "Error WSAData!\n"; exit(EXIT_FAILURE);
    }

    addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(ServPort);
    addr.sin_family = AF_INET;

    Connect = socket(AF_INET, SOCK_STREAM, NULL);
    if (connect(Connect, (SOCKADDR*)&addr, sizeOfAddr)) {
        std::cout << "Error clConnect!\n";  system("pause"); exit(EXIT_FAILURE);
    }
    else std::cout << "Succesfull clConnect to the server!\n";

    // Лямбда функция для приема сообщений от сервера
    auto srvHandler = [&]()
    { char* inMsg;
    int msgSize{ 0 };

    while (1)
    {
        if (SOCKET_ERROR == recv(Connect, (char*)&msgSize, sizeof(int), NULL))
        {
            std::cout << "Error ServerDisConnected!\n";
            work = false;
            break;
        }
        inMsg = new char[msgSize + 1];
        recv(Connect, inMsg, msgSize, NULL);
        inMsg[msgSize] = '\0';
        std::cout << inMsg; std::cout << "\n";
        delete[] inMsg;
    }
    };

    std::thread th(srvHandler); // вызов лямбда функции в отдельном потоке

    // цикл для ввода с консоли сообщений и отправки их на сервер
    do 
    {
        std::cin >> msg;
        if (!work) { std::cout << "The message could be sent because ServerDisConnected!\n"; break; }
        int msgSize{ 0 };
        msgSize = msg.size();
        send(Connect, (char*)&msgSize, sizeof(int), NULL);
        send(Connect, msg.c_str(), msgSize, NULL);
        Sleep(50);

    } while (work);

    th.join();// безопасное завершение потока
    system("pause");
}