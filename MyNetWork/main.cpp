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

		std::cout << "<----- ��⵽�������ӣ���ʼ�������� ----->" << std::endl << std::endl;
		vTrans.attachCommand(CommandInfo.Number);
		vTrans.showCommandInfo(CommandInfo);

		if (CommandInfo.Type == 0)
		{
			std::cout << ">>> ���յ�����Դ����������Ϣ, ��ѯ��ԴΪ: " << CommandInfo.ResourceName << std::endl << std::endl;
			std::cout << ">>> �Բ�ѯ������Դ������������" << std::endl << std::endl;
			
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
			std::cout << ">>> ���յ���Դ��������, ������ԴΪ: " << CommandInfo.ResourceName << std::endl << std::endl;

			CommandInfo.VisitedPeers.push_back(vConfig.getMyName());

			if (vTrans.searchResource(CommandInfo.ResourceName, vConfig.getPath(), CommandInfo))
			{
				std::cout << ">>> ��Դ��ѯ�ɹ���֪ͨ���󷽲�ѯ����..." << std::endl << std::endl;

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
				std::cout << ">>> ��Դ��ѯʧ�ܣ�����ת����ѯ����..." << std::endl << std::endl;
			}
		}

		else if (CommandInfo.Type == 2)
		{
			std::cout << ">>> ���յ���Դ�����������ں�ʵ��ѯ�����б�..." << std::endl << std::endl;

			if (vTrans.IsHaveHit(CommandInfo.ResourceName))
			{
				std::cout << "�������У��������أ���ʼ������Դ..." << std::endl << std::endl;
				
				SOCKET ResourceSocket = socket(AF_INET, SOCK_STREAM, 0);
				SOCKADDR_IN ResourceAddr;
				inet_pton(AF_INET, CommandInfo.DstIP.c_str(), &ResourceAddr.sin_addr.S_un.S_addr);
				ResourceAddr.sin_port = htons(CommandInfo.DstPort);
				ResourceAddr.sin_family = AF_INET;

				connect(ResourceSocket, (SOCKADDR *)&ResourceAddr, sizeof(ResourceAddr));

				if (vTrans.sendResource(ResourceSocket, CommandInfo.ResourceName))
					std::cout << "<----- ������Դ�ɹ� ----->" << std::endl << std::endl;
				else
					std::cout << "<----- ������Դʧ�� ----->" << std::endl << std::endl;

				closesocket(ResourceSocket);
			}
			else
			{
				std::cout << "δ�������У��ܾ�����..." << std::endl;
			}
		}

		else
		{
			std::cerr << "���յ��˴������������," << std::endl;
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
		std::cout << "<----- ��⵽�������ӣ���ʼ������Դ ----->" << std::endl << std::endl;
		if (vTrans.recvResource(AcceptSocket, vConfig.getPath()))
			std::cout << ">>>��Դ���ܳɹ�" << std::endl << std::endl;
		else
			std::cerr << ">>>��Դ����ʧ��" << std::endl << std::endl;

		closesocket(AcceptSocket);
	}
}

int main()
{
	WORD Version = MAKEWORD(2, 2);
	WSADATA Wsa;

	if (WSAStartup(Version, &Wsa) != 0)
	{
		std::cout << "<-----��ʼ��Socket��Դʧ��----->" << std::endl << std::endl;
		return 0;
	}
	std::cout << "<-----��ʼ��Socket��Դ�ɹ�----->" << std::endl << std::endl;

	Configuration Config;
	Config.parserConfig("Config.ini");

	std::cout << ">>> ��ǰ�Եȷ�������Ϣ����: " << std::endl;
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
	std::cout << "<-----�Ѿ���������������߳�----->" << std::endl << std::endl;

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
	std::cout << "<-----�Ѿ������������ݵ��߳�----->" << std::endl << std::endl;

	int Number = 0;

	while (true)
	{
		Transfer::Comm CommandInfo;
		std::cout << "||����������Ҫ���ҵ��ļ�:> ";

		std::cin >> CommandInfo.ResourceName;
		CommandInfo.Number = Number;
		CommandInfo.DstIP = Config.getMyIP();
		CommandInfo.DstPort = Config.getMyCommandPort();
		CommandInfo.Type = 1;
		CommandInfo.VisitedPeers.push_back(Config.getMyName());

		std::cout << std::endl;
		if (Trans.searchResource(CommandInfo.ResourceName, Config.getPath(), CommandInfo))
			std::cout << ">>>������Ҫ����Դ���Ѿ�ӵ��" << std::endl << std::endl;
		else
		{
			std::cout << ">>>δ�ڱ��ز��ҵ�������Դ����ʼ���к鷺��ѯ" << std::endl << std::endl;
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