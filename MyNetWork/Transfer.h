#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>

#include "Configuration.h"

#pragma comment(lib, "WS2_32")

class Transfer
{
public:
	Transfer();

	typedef struct Command {
		int Number;
		int Type;
		std::string ResourceName;
		std::string DstIP;
		int DstPort;
		std::vector<std::string> VisitedPeers;
	}Comm;

	typedef struct Package {
		int Number;
		int Type;
		std::string ResourceName;
		std::string ParentDir;
		int DataLength;
		std::string Data;
	}Pack;

	bool searchResource(std::string vResourceName, std::string vPath, Comm vCommandInfo);
	bool setConnectedPeers(std::vector<Configuration::Peer> vPeers);
	bool sendResource(SOCKET& vSocket, std::string vResourceName);
	bool recvResource(SOCKET vSocket, std::string vPath);
	void sendCommand(SOCKET& vSocket, Comm vCommandInfo);
	Comm recvCommand(SOCKET vSocket);
	std::vector<std::pair<std::string, std::string>>& getHitInquireSet();
	bool IsCommandRecved(int vNumber);
	bool IsHaveHit(std::string ResourceName);
	void attachCommand(int vNumber);
	void showCommandInfo(Comm vCommandInfo);
	void showPackageInfo(Pack vPackageInfo);
	~Transfer();
private:
	bool __searchFile(std::string vFileName, std::string vPath);
	bool __searchDirectory(std::string vDirectoryName, std::string vPath);
	void __sendInquire(Comm vCommmandInfo);
	bool __sendFile(SOCKET vSocket, std::string vFileName, std::string vPath, std::string vParentDir = "");
	bool __sendDirectory(SOCKET vSocket, std::string vFileName, std::string vPath, std::string vParentDir = "");
	Comm __parserCommand(std::string vCommand);
	std::string __combineCommand(Comm vCommandInfo);
	Pack __parserPackeage(std::string vPackage);
	std::string __combinePackage(Pack vPackageInfo);
	std::string __getPath(std::string vResourceName);
private:
	std::vector<std::pair<std::string, std::string>> m_HitInquireSet;
	std::vector<Configuration::Peer> m_ConnectedPeers;
	std::vector<int> m_RecvedCommand;
};

