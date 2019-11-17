#include "Configuration.h"

Configuration::Configuration()
{
}

std::string& Configuration::getMyName()
{
	return m_MyName;
}

std::string& Configuration::getMyIP()
{
	return m_MyIP;
}

int& Configuration::getMyDataPort()
{
	return m_MyDataPort;
}

int& Configuration::getMyCommandPort()
{
	return m_MyComandPort;
}

std::string& Configuration::getPath()
{
	return m_Path;
}

std::vector<Configuration::Peer>& Configuration::getConnectedPeers()
{
	return m_Peers;
}

bool Configuration::parserConfig(std::string vFilename)
{
	if (!__readConfig(vFilename))
	{
		std::cerr << "读取配置文件错误" << std::endl;
		return false;
	}

	else
	{
		for (int i = 0; i < m_IPs.size() && i < m_CommandPorts.size() && i<m_DataPorts.size() && m_Names.size(); i++)
		{
			Peer SPeer;
			SPeer.IP = m_IPs[i];
			SPeer.CommandPort = m_CommandPorts[i];
			SPeer.DataPort = m_DataPorts[i];
			SPeer.Name = m_Names[i];
			m_Peers.push_back(SPeer);
		}
	}

	return true;
}

void Configuration::showConfigInfo()
{
	std::cout << "----------------------------------" << std::endl;
	std::cout << "MyName: " << this->getMyName() << std::endl;
	std::cout << "MyIP: " << this->getMyIP() << std::endl;
	std::cout << "MyDataPort: " << this->getMyDataPort() << std::endl;
	std::cout << "MyCommandPort: " << this->getMyCommandPort() << std::endl;
	std::cout << "DirectoryPath: " << this->getPath() << std::endl;
	std::cout << "----------------------------------" << std::endl << std::endl << std::endl;
}

bool Configuration::__readConfig(std::string vFilename)
{
	std::ifstream ConfigFile;
	ConfigFile.open(vFilename, std::ios::in);

	if (!ConfigFile)
	{
		std::cerr << "打开配置文件错误，或没有该配置文件" << std::endl;
		return false;
	}

	while (!ConfigFile.eof())
	{
		std::string ConfigInfo;
		getline(ConfigFile, ConfigInfo);
		
		if (ConfigInfo.empty())
		{
			std::cerr << "读取到空行！" << std::endl;
			continue;
		}		
		
		__parserConfigInfo(ConfigInfo);

		ConfigInfo.clear();
	}

	return true;
}

bool Configuration::__parserConfigInfo(std::string vConfigInfo)
{
	int Len = vConfigInfo.length();
	int StartPos = 0;
	int EndPos = Len - 1;
	int Pos;
	std::string Section;
	if ((vConfigInfo.find('[') == StartPos) && (vConfigInfo.find(']') == EndPos))
	{
		Section = vConfigInfo.substr(StartPos + 1, EndPos - 1);
		std::cout << ">正在读取配置信息 " << Section << " 中的内容..." << std::endl;
	}
	else if (vConfigInfo.find('='))
	{
		std::string Name;
		std::string ValueString;
		Pos = vConfigInfo.find('=');
		Name = vConfigInfo.substr(StartPos, Pos);
		ValueString = vConfigInfo.substr(Pos + 1, EndPos - Pos);
		__parserValueString(ValueString, Name);
	}

	return true;
}

bool Configuration::__parserValueString(std::string vValueString, std::string vName)
{
	std::vector<std::string> Values;
	int StartPos = 0;
	int Pos = 0;
	for (int i = StartPos; i < vValueString.length(); i++)
	{
		if (vValueString[i] == ';')
		{
			Values.push_back(vValueString.substr(Pos, i - Pos));
			Pos = i + 1;
		}
	}
	
	if (strcmp(vName.c_str(), "MyName") == 0)
	{
		m_MyName = Values[0];
		return true;
	}

	else if (strcmp(vName.c_str(), "MyIP") == 0)
	{
		m_MyIP = Values[0];
		return true;
	}

	else if (strcmp(vName.c_str(), "MyDataPort") == 0)
	{
		std::stringstream ss;
		ss << Values[0];
		ss >> m_MyDataPort;
		return true;
	}

	else if (strcmp(vName.c_str(), "MyCommandPort") == 0)
	{
		std::stringstream ss;
		ss << Values[0];
		ss >> m_MyComandPort;
		return true;
	}

	else if (strcmp(vName.c_str(), "Name") == 0)
	{
		for (int i = 0; i < Values.size(); i++)
		{
			m_Names.push_back(Values[i]);
		}
	}

	else if (strcmp(vName.c_str(), "IP") == 0)
	{
		for (int i = 0; i < Values.size(); i++)
		{
			m_IPs.push_back(Values[i]);
		}
		return true;
	}

	else if (strcmp(vName.c_str(), "CommandPort") == 0)
	{
		for (int i = 0; i < Values.size(); i++)
		{
			int Value;
			std::stringstream ss;
			ss << Values[i];
			ss >> Value;
			m_CommandPorts.push_back(Value);
		}
		return true;
	}

	else if (strcmp(vName.c_str(), "DataPort") == 0)
	{
		for (int i = 0; i < Values.size(); i++)
		{
			int Value;
			std::stringstream ss;
			ss << Values[i];
			ss >> Value;
			m_DataPorts.push_back(Value);
		}
		return true;
	}

	else if(strcmp(vName.c_str(), "Path") == 0)
	{
		m_Path = Values[0];
		return true;
	}

	else
	{
		std::cerr << "未检测到键值" << std::endl;
		return false;
	}

	return false;
}

Configuration::~Configuration()
{
}
