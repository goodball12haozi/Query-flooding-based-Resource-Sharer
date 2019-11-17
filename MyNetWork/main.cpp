#include "Configuration.h"
#include "Transfer.h"
#include <thread>

void processCommand(SOCKET vSocket, Transfer vTrans, Configuration vConfig)
{

	SOCKADDR_IN AcceptAddr;
	int AddrLen = sizeof(AcceptAddr);
	while (true)
	{

		SOCKET AcceptSocket = accept(vSocket, (SOCKADDR *)&AcceptAddr, &AddrLen);
		Transfer::Comm CommandInfo = vTrans.recvCommand(AcceptSocket);
		
		if (vTrans.IsCommandRecved(CommandInfo.Number))
		{
			closesocket(AcceptSocket);
			continue;
		}

		std::cout << "<----- 检测到命令连接，开始接受命令 ----->" << std::endl << std::endl;
		vTrans.attachCommand(CommandInfo.Number);
		vTrans.showCommandInfo(CommandInfo);

		if (CommandInfo.Type == 0)
		{
			std::cout << ">>> 接收到了资源查找命中消息, 查询资源为: " << CommandInfo.ResourceName << std::endl << std::endl;
			std::cout << ">>> 对查询到的资源发送下载请求" << std::endl << std::endl;
			
			SOCKET RequestSocket = socket(AF_INET, SOCK_STREAM, 0);
			SOCKADDR_IN RequestAddr;
			inet_pton(AF_INET, CommandInfo.DstIP.c_str(), &RequestAddr.sin_addr.S_un.S_addr);
			RequestAddr.sin_port = htons(CommandInfo.DstPort);
			RequestAddr.sin_family = AF_INET;

			connect(RequestSocket, (SOCKADDR *)&RequestAddr, sizeof(RequestAddr));

			CommandInfo.Number++;
			CommandInfo.Type = 2;
			CommandInfo.DstIP = vConfig.getMyIP();
			CommandInfo.DstPort = vConfig.getMyDataPort();
			CommandInfo.VisitedPeers.clear();

			vTrans.sendCommand(RequestSocket, CommandInfo);
			closesocket(RequestSocket);
		}

		else if (CommandInfo.Type == 1)
		{
			std::cout << ">>> 接收到资源查找请求, 查找资源为: " << CommandInfo.ResourceName << std::endl << std::endl;

			CommandInfo.VisitedPeers.push_back(vConfig.getMyName());

			if (vTrans.searchResource(CommandInfo.ResourceName, vConfig.getPath(), CommandInfo))
			{
				std::cout << ">>> 资源查询成功，通知请求方查询命中..." << std::endl << std::endl;

				SOCKET NoticeSocket = socket(AF_INET, SOCK_STREAM, 0);
				SOCKADDR_IN NoticeAddr;
				inet_pton(AF_INET, CommandInfo.DstIP.c_str(), &NoticeAddr.sin_addr.S_un.S_addr);
				NoticeAddr.sin_port = htons(CommandInfo.DstPort);
				NoticeAddr.sin_family = AF_INET;

				connect(NoticeSocket, (SOCKADDR *)&NoticeAddr, sizeof(NoticeAddr));

				CommandInfo.Type = 0;
				CommandInfo.DstIP = vConfig.getMyIP();
				CommandInfo.DstPort = vConfig.getMyCommandPort();
				CommandInfo.VisitedPeers.clear();

				vTrans.sendCommand(NoticeSocket, CommandInfo);
				closesocket(NoticeSocket);
			}
			else
			{
				std::cout << ">>> 资源查询失败，正在转发查询请求..." << std::endl << std::endl;
			}
		}

		else if (CommandInfo.Type == 2)
		{
			std::cout << ">>> 接收到资源下载请求，正在核实查询命中列表..." << std::endl << std::endl;

			if (vTrans.IsHaveHit(CommandInfo.ResourceName))
			{
				std::cout << "存在命中，允许下载，开始发送资源..." << std::endl << std::endl;
				
				SOCKET ResourceSocket = socket(AF_INET, SOCK_STREAM, 0);
				SOCKADDR_IN ResourceAddr;
				inet_pton(AF_INET, CommandInfo.DstIP.c_str(), &ResourceAddr.sin_addr.S_un.S_addr);
				ResourceAddr.sin_port = htons(CommandInfo.DstPort);
				ResourceAddr.sin_family = AF_INET;

				connect(ResourceSocket, (SOCKADDR *)&ResourceAddr, sizeof(ResourceAddr));

				if (vTrans.sendResource(ResourceSocket, CommandInfo.ResourceName))
					std::cout << "<----- 发送资源成功 ----->" << std::endl << std::endl;
				else
					std::cout << "<----- 发送资源失败 ----->" << std::endl << std::endl;

				closesocket(ResourceSocket);
			}
			else
			{
				std::cout << "未存在命中，拒绝下载..." << std::endl;
			}
		}

		else
		{
			std::cerr << "接收到了错误的命令类型," << std::endl;
		}

		closesocket(AcceptSocket);
	}
}

void processData(SOCKET vSocket, Transfer vTrans, Configuration vConfig)
{
	SOCKADDR_IN AcceptAddr;
	int AddrLen = sizeof(AcceptAddr);
	while (true)
	{
		SOCKET AcceptSocket = accept(vSocket, (SOCKADDR *)&AcceptAddr, &AddrLen);
		std::cout << "<----- 检测到数据连接，开始接受资源 ----->" << std::endl << std::endl;
		if (vTrans.recvResource(AcceptSocket, vConfig.getPath()))
			std::cout << ">>>资源接受成功" << std::endl << std::endl;
		else
			std::cerr << ">>>资源接受失败" << std::endl << std::endl;

		closesocket(AcceptSocket);
	}
}

int main()
{
	WORD Version = MAKEWORD(2, 2);
	WSADATA Wsa;

	if (WSAStartup(Version, &Wsa) != 0)
	{
		std::cout << "<-----初始化Socket资源失败----->" << std::endl << std::endl;
		return 0;
	}
	std::cout << "<-----初始化Socket资源成功----->" << std::endl << std::endl;

	Configuration Config;
	Config.parserConfig("Config.ini");

	std::cout << ">>> 当前对等方基本信息如下: " << std::endl;
	Config.showConfigInfo();

	Transfer Trans;
	Trans.setConnectedPeers(Config.getConnectedPeers());

	SOCKET ComSocket = socket(AF_INET, SOCK_STREAM, 0);
	
	SOCKADDR_IN ComAddr;
	inet_pton(AF_INET, Config.getMyIP().c_str(), &ComAddr.sin_addr.S_un.S_addr);
	ComAddr.sin_port = htons(Config.getMyCommandPort());
	ComAddr.sin_family = AF_INET;

	bind(ComSocket, (SOCKADDR *)&ComAddr, sizeof(ComAddr));
	listen(ComSocket, 5);

	std::thread CommandThread([ComSocket, Trans, Config]() {
		processCommand(ComSocket, Trans, Config);
	});
	std::cout << "<-----已经开启接受命令的线程----->" << std::endl << std::endl;

	SOCKET DataSocket = socket(AF_INET, SOCK_STREAM, 0);

	SOCKADDR_IN DataAddr;
	inet_pton(AF_INET, Config.getMyIP().c_str(), &DataAddr.sin_addr.S_un.S_addr);
	DataAddr.sin_port = htons(Config.getMyDataPort());
	DataAddr.sin_family = AF_INET;

	bind(DataSocket, (SOCKADDR *)&DataAddr, sizeof(DataAddr));
	listen(DataSocket, 5);

	std::thread DataThread([DataSocket, Trans, Config]() {
		processData(DataSocket, Trans, Config);
	});
	std::cout << "<-----已经开启接受数据的线程----->" << std::endl << std::endl;

	int Number = 0;

	while (true)
	{
		Transfer::Comm CommandInfo;
		std::cout << "||请输入你需要查找的文件:> ";

		std::cin >> CommandInfo.ResourceName;
		CommandInfo.Number = Number;
		CommandInfo.DstIP = Config.getMyIP();
		CommandInfo.DstPort = Config.getMyCommandPort();
		CommandInfo.Type = 1;
		CommandInfo.VisitedPeers.push_back(Config.getMyName());

		std::cout << std::endl;
		if (Trans.searchResource(CommandInfo.ResourceName, Config.getPath(), CommandInfo))
			std::cout << ">>>您所需要的资源您已经拥有" << std::endl << std::endl;
		else
		{
			std::cout << ">>>未在本地查找到所需资源，开始进行洪泛查询" << std::endl << std::endl;
			Sleep(8000);
		}

		Number = Number + 2;
	}
	
	CommandThread.join();
	DataThread.join();

	closesocket(ComSocket);
	system("pause");
	return 0;
}