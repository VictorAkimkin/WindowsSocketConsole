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
#include <map>
#include <thread>
#include <string>

#pragma comment(lib, "Ws2_32.lib")

void addrinf_out(ADDRINFOA& inf, PADDRINFOA res);// выводит в консоль данные из addrinfo* res
bool ChangeName(const char* str, int msgSize);// проверка наличия команды смены имени клиента
void MassSending(const std::map <SOCKET, std::string>* clConnections, const char* clMsg2,
	int Size2, bool except, SOCKET sock);// рассылка сообщений клиентам

int main()
{
	// флаг работы сервера
	volatile bool work_flag{ true };
	// данные для инициализации сервера
	WSAData wsaData;
	DWORD DllVers = MAKEWORD(2, 2);
	const int ServPort = 5053;
	SOCKADDR_IN addr;
	int sizeOfAddr = sizeof(addr);
	SOCKET sockListen;
	// клиентские сокеты и потоки
	std::map <SOCKET, std::string> clConnections;
	//std::vector <std::thread> ths;
	std::map <SOCKET, std::thread> ths;

	// переменная для сообщения клиенту при подключении
	std::string welcomeMsg = "";

	// переменные для получения информации о сервере
	ADDRINFOA inf = { 0 };
	PADDRINFOA res = NULL;

	setlocale(LC_ALL, "RUS");
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);

	// поток для принятия команды выхода
	std::thread ex([&]() {
		while (work_flag)
		{
			std::string str;  std::getline(std::cin, str);
			if (str == "exit") work_flag = false;
		}
		//    goto server_off; 

		});


	if (WSAStartup(DllVers, &wsaData)) {
		std::cout << "Error WSAData!\n"; system("pause"); exit(EXIT_FAILURE);
	}

	addr.sin_addr.S_un.S_addr = INADDR_ANY;//inet_addr("127.0.0.1");
	addr.sin_port = htons(ServPort);
	addr.sin_family = AF_INET;

	sockListen = socket(AF_INET, SOCK_STREAM, NULL);
	bind(sockListen, (SOCKADDR*)&addr, sizeOfAddr);
	if (listen(sockListen, SOMAXCONN) == SOCKET_ERROR) { std::cout << "Error Listen\n"; system("pause"); exit(EXIT_FAILURE); };



	// получение информация о сервере
	inf.ai_family = AF_UNSPEC;
	inf.ai_socktype = SOCK_STREAM;
	inf.ai_protocol = IPPROTO_TCP;

	addrinf_out(inf, res);
	std::cout << " \n\n";

	std::cout << " Type \"exit\" to quit\n";
	// инициализация структур для команды select
	fd_set fd{};
	FD_ZERO(&fd);
	FD_SET(sockListen, &fd);
	timeval tv{};
	tv.tv_sec = 1;

	// подключение клиентов и запуск потока прослушивания для каждого клиента
	while (work_flag)
	{
		int msgSize{ 0 };
		SOCKET Stmp;
		fd_set tmp_fd = fd;
		// select проверяет готовность сокета к чтению в течение tv (1 сек.) если не сокет не готов то возвращаемся к началу цыкла чтобы проверить флаг
		if (select(0, &tmp_fd, 0, 0, &tv) <= 0) continue;
		if (!(Stmp = accept(sockListen, 0, 0)))
		{
			std::cout << "Error newConnect!\n";  system("pause"); exit(EXIT_FAILURE);
		}
		else
		{
			clConnections[Stmp] = ("noname" + std::to_string((int)Stmp + 135));
			std::cout << "Succesfull Connect with a client! Socket: " << Stmp << "\n";
			std::cout << "Client name: " << clConnections[Stmp] << ".\n";

			welcomeMsg = "Welcom " + clConnections[Stmp] + " ! Use command : \"-newname <your name>\" to change name\n";
			msgSize = welcomeMsg.size();
			send(Stmp, (char*)&msgSize, sizeof(int), NULL);
			send(Stmp, welcomeMsg.c_str(), msgSize, NULL);
			ths[Stmp] = std::thread([&](SOCKET index)     // ретрансляция сообщений от клиента всем остальным клиентам
				{
					char* clMsg, * clMsg2;
					int msgSize{ 0 }, Size2{ 0 };
					fd_set r_fd{};
					FD_ZERO(&r_fd);
					FD_SET(index, &r_fd);
					//timeval rtv{};
					//rtv.tv_sec = 1;
					while (work_flag)
					{
						fd_set r_tmp = r_fd;
						//int r_ready = select(0, &r_tmp, 0, 0, &tv);
						if (select(0, &r_tmp, 0, 0, &tv) <= 0) continue;
						if (SOCKET_ERROR == recv(index, (char*)&msgSize, sizeof(int), NULL))
						{
							std::cout << "Error SOCKET_ERROR!\n" << "  SOCKET " << index << "\n";

							// создаем строку типа :  "\"noname325\" leaves the chat"
							int namesize = (clConnections[index].length());
							Size2 = namesize + 19;
							clMsg2 = new char[Size2];
							memset(clMsg2, 0, Size2);
							strcat(clMsg2, "\"");
							strcat(clMsg2, clConnections[index].c_str()); // имя
							strcat(clMsg2, "\" leaves the chat");
							MassSending(&clConnections, clMsg2, Size2, true, index);
							std::cout << clMsg2 << "\n";
							delete[] clMsg2;
							clConnections.erase(index);
							ths[index].detach();
							ths.erase(index);
							break;
						}

						clMsg = new char[msgSize + 1];
						recv(index, clMsg, msgSize, NULL);
						clMsg[msgSize] = '\0';
						if (ChangeName(clMsg, msgSize))
						{
							// создаем строку типа :  "\"noname325\" changed name to \""
							int namesize = (clConnections[index].length());
							Size2 = namesize + msgSize + 13;   // "-newname 123" "\"noname325\" changed name to \"123\""
							clMsg2 = new char[Size2];
							memset(clMsg2, 0, Size2);
							strcat(clMsg2, "\"");
							strcat(clMsg2, clConnections[index].c_str()); // старое имя
							strcat(clMsg2, "\" changed name to \"");

							// меняем имя
							int pos{ 9 };
							clConnections[index] = "";
							while (clMsg[pos])
							{
								clConnections[index] += clMsg[pos++];
							}

							// дописываем новое имя в конец строки и получаем строку типа :"\"noname325\" changed name to \"123\""
							strcat(clMsg2, clConnections[index].c_str()); // новое имя
							strcat(clMsg2, "\"");
							MassSending(&clConnections, clMsg2, Size2, 0, 0);
						}
						else
						{
							// создаем строку типа :  "\"noname325\" : <сообщение>"
							int namesize = (clConnections[index].size());
							Size2 = namesize + msgSize + 4;
							clMsg2 = new char[Size2];
							memset(clMsg2, 0, Size2);
							strcat(clMsg2, clConnections[index].c_str());
							strcat(clMsg2, " : ");
							strcat(clMsg2, clMsg);
							MassSending(&clConnections, clMsg2, Size2, true, index);
						}
						delete[] clMsg2;
						delete[] clMsg;
					}
				}, Stmp);// map
		}
	}
	// безопасное завершение потоков и WSA
	ex.join();
	for (auto& t : ths)
		// t.second.join();
		t.second.join();
	WSACleanup();
	system("pause");
}
// вывод в консоль информации о сервере
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
// проверка строку на содержащуюся в ней команду смены имени
bool ChangeName(const char* str, int msgSize)
{
	const char tampl[] = "-newname ";
	int i{ 0 };
	while (str[i] && i < 9)
	{
		if (str[i] != tampl[i++]) break;
	}
	return (i == 9);
}
// рассылка сообщения clMsg по всем сокетам map-контейнера сокетов либо(except)
// исключая определенный сокет(sock) из рассылки либо не исключая
void MassSending(const std::map <SOCKET, std::string>* clConnections, const char* clMsg, int Size, bool except, SOCKET sock)
{
	for (auto& p : *clConnections)
	{
		if (except && p.first == sock) continue;
		send(p.first, (char*)&Size, sizeof(int), NULL);
		send(p.first, clMsg, Size, NULL);
	}
}