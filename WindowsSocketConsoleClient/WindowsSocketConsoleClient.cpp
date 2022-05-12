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
#include <string>
#include <thread>

#pragma comment(lib, "Ws2_32.lib")
const char* GetIP();
int main()
{
    // данные для инициализации клиента
    WSAData wsaData;
    DWORD DllVers = MAKEWORD(2, 2);
    SOCKADDR_IN addr;
    int sizeOfAddr = sizeof(addr);
    char srvIP[20]{ 0 }; strcpy(srvIP, GetIP());
    const int ServPort = 5053;
    // сокет для связи с сервером
    SOCKET Connect;
    // переменная для отправки сообщений в чат через сервер
    std::string msg = "";
    // переменная для контроля состояния соединения с сервером
    volatile bool work = true;
    setlocale(LC_ALL, "RUS");
    SetConsoleCP(1251);
    SetConsoleOutputCP(1251);

    if (WSAStartup(DllVers, &wsaData)) {
        std::cout << "Error WSAData!\n"; exit(EXIT_FAILURE);
    }

    addr.sin_addr.S_un.S_addr = inet_addr(srvIP);
    addr.sin_port = htons(ServPort);
    addr.sin_family = AF_INET;

    Connect = socket(AF_INET, SOCK_STREAM, NULL);
    if (connect(Connect, (SOCKADDR*)&addr, sizeOfAddr)) {
        std::cout << "Connection Error!\n";  system("pause"); exit(EXIT_FAILURE);
    }
    else std::cout << "You have successfully connected to the server!\nServer IP: "<< srvIP<< "\n";

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
        std::getline (std::cin, msg);
        if (!work) { std::cout << "The message could not be sent because ServerDisConnected!\n"; break; }
        int msgSize{ 0 };
        msgSize = msg.length();
        send(Connect, (char*)&msgSize, sizeof(int), NULL);
        send(Connect, msg.c_str(), msgSize, NULL);
        Sleep(50);

    } while (work);

    th.join();// безопасное завершение потока
    system("pause");
}

const char* GetIP()
{
    FILE* f;
    if (!(f = fopen("ipconfig.cfg", "r"))) {
        std::cout << "Ошибка открытия файла ipconfig.cfg\n";
        if (!(f = fopen("ipconfig.cfg", "w"))) {
            std::cout << "Попытка создания нового файла ipconfig.cfg не удалась\n";
            exit;
        }  else {
                   char str[] = "127.0.0.1";
                   fprintf(f, str);
                   std::cout << "создан файл с ip 127.0.0.1";
                   fclose(f);
                   return str;
                }
    }
    char str[20]{0};
    if(!fread(str,sizeof(char), sizeof(str), f))
        std::cout << "Ошибка чтения файла ipconfig.cfg\n";
    fclose(f);
    return str;
}
