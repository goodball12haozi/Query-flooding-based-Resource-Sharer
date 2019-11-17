#pragma once
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

class Configuration
{
public:
	Configuration();
	~Configuration();

	typedef struct ConnectedPeer
	{
		std::string Name;
		int CommandPort;
		int DataPort;
		std::string IP;
	}Peer;

	bool parserConfig(std::string vFilename);
	std::string& getMyName();
	std::string& getMyIP();
	int& getMyDataPort();
	int& getMyCommandPort();
	std::string& getPath();
	std::vector<Peer>& getConnectedPeers();
	void showConfigInfo();

private:
	bool __readConfig(std::string vFilename);
	bool __parserConfigInfo(std::string vConfigInfo);
	bool __parserValueString(std::string vValueString, std::string vName);

private:
	std::string m_MyName;
	std::string m_MyIP;
	int m_MyDataPort;
	int m_MyComandPort;
	std::string m_Path;
	std::vector<std::string> m_Names;
	std::vector<int> m_CommandPorts;
	std::vector<int> m_DataPorts;
	std::vector<std::string> m_IPs;
	std::vector<Peer> m_Peers;
};

