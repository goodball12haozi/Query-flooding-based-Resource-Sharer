#include "Transfer.h"

Transfer::Transfer()
{

}

bool Transfer::searchResource(std::string vResourceName, std::string vPath, Transfer::Comm vCommandInfo)
{
	bool IsHaveFind = false;
	if (vResourceName.find(".") != std::string::npos)
		IsHaveFind = __searchFile(vResourceName, vPath);
	else
		IsHaveFind = __searchDirectory(vResourceName, vPath);

	if (!IsHaveFind)
		__sendInquire(vCommandInfo);

	return IsHaveFind;
}

bool Transfer::setConnectedPeers(std::vector<Configuration::Peer> vPeers)
{
	m_ConnectedPeers = vPeers;
	return true;
}

bool Transfer::sendResource(SOCKET& vSocket, std::string vResourceName)
{
	std::string Path = __getPath(vResourceName);
	std::cout << ">>> 获取到请求资源的路径: " << Path << std::endl;

	bool Send = false;

	if (vResourceName.find(".") != std::string::npos)
	{
		Send = __sendFile(vSocket, vResourceName, Path);
	}
	else
	{
		Send = __sendDirectory(vSocket, vResourceName, Path);
	}

	Pack PackageInfo;
	PackageInfo.Number = 0;
	PackageInfo.Type = 2;
	PackageInfo.ResourceName = vResourceName;
	PackageInfo.ParentDir = "";
	PackageInfo.DataLength = 0;
	PackageInfo.Data = "";
	std::string Package = __combinePackage(PackageInfo);
	send(vSocket, Package.c_str(), Package.length(), 0);
	
	return Send;
}

bool Transfer::recvResource(SOCKET vSocket, std::string vPath)
{	
	do
	{
		const int MaxSize = 1124;
		char Package[MaxSize] = { 0 };
		int ReadLength = 0;
		ReadLength = recv(vSocket, Package, MaxSize, 0);

		Pack PackageInfo = __parserPackeage(Package);

		std::cout << ">接受数据包为: " << std::endl;
		this->showPackageInfo(PackageInfo);

		if (PackageInfo.Type == 0)
		{
			std::ofstream OutputFile;
		
			std::string FilePath = vPath + PackageInfo.ParentDir + "\\" + PackageInfo.ResourceName;
			OutputFile.open(FilePath, std::ios::binary);
			if (!OutputFile)
			{
				std::cerr << ">>> 文件创建失败, 停止资源接收" << std::endl << std::endl;
				return false;
			}
			OutputFile.write(PackageInfo.Data.c_str(), PackageInfo.DataLength);

			do
			{
				memset(Package, 0, MaxSize);
				ReadLength = recv(vSocket, Package, MaxSize, 0);
				PackageInfo = __parserPackeage(Package);

				std::cout << ">接受数据包为: " << std::endl;
				this->showPackageInfo(PackageInfo);

				if (PackageInfo.Type == 3)
				{
					std::cout << ">>> 接收到文件结束命令，该文件接收完成" << std::endl << std::endl << std::endl;
					break;
				}

				OutputFile.write(PackageInfo.Data.c_str(), PackageInfo.DataLength);
			} while (true);

			OutputFile.close();
		}
		else if (PackageInfo.Type == 1)
		{
			std::string DirPath = vPath + PackageInfo.ParentDir + "\\" + PackageInfo.ResourceName;
			if (!CreateDirectory(DirPath.c_str(), NULL))
			{
				std::cerr << ">>> 文件夹创建失败, 停止资源接收" << std::endl << std::endl;
				return false;
			}
		}
		else if (PackageInfo.Type == 2)
		{
			std::cout << ">>> 接收到资源结束命令，本此资源接收结束" << std::endl << std::endl << std::endl;
			break;
		}
		else
		{
			std::cerr << ">>> 检测到无法识别的文件类型, 停止资源接收" << std::endl << std::endl;
			return false;
		}
	} while (true);

	return true;
}

void Transfer::sendCommand(SOCKET& vSocket, Transfer::Comm vCommandInfo)
{
	std::string Command = __combineCommand(vCommandInfo);
	send(vSocket, Command.c_str(), Command.length(), 0);
	closesocket(vSocket);
}

Transfer::Comm Transfer::recvCommand(SOCKET vSocket)
{
	const int MaxSize = 1024;
	char Command[MaxSize] = { 0 };
	recv(vSocket, Command, MaxSize, 0);

	return __parserCommand(Command);
}

std::vector<std::pair<std::string, std::string>>& Transfer::getHitInquireSet()
{
	return m_HitInquireSet;
}

bool Transfer::IsCommandRecved(int vNumber)
{
	if (std::find(m_RecvedCommand.begin(), m_RecvedCommand.end(), vNumber) != m_RecvedCommand.end())
	{
		return true;
	}
	return false;
}

bool Transfer::IsHaveHit(std::string vResourceName)
{
	for (auto HitInquire : m_HitInquireSet)
	{
		if (HitInquire.first == vResourceName)
			return true;
	}
	return false;
}

void Transfer::attachCommand(int vNumber)
{
	m_RecvedCommand.push_back(vNumber);
}

void Transfer::showCommandInfo(Transfer::Comm vCommandInfo)
{
	std::cout << "----------------------------------" << std::endl;
	std::cout << "Number: " << vCommandInfo.Number << std::endl;
	std::cout << "Type: " << vCommandInfo.Type << std::endl;
	std::cout << "ResourceName: " << vCommandInfo.ResourceName << std::endl;
	std::cout << "DstIP: " << vCommandInfo.DstIP << std::endl;
	std::cout << "DstPort: " << vCommandInfo.DstPort << std::endl;
	std::cout << "VisitedPeers: ";
	for (auto Peer : vCommandInfo.VisitedPeers)
	{
		std::cout << Peer << ":";
	}
	std::cout << std::endl;
	std::cout << "----------------------------------" << std::endl << std::endl << std::endl;
}

void Transfer::showPackageInfo(Transfer::Pack vPackageInfo)
{
	std::cout << "----------------------------------" << std::endl;
	std::cout << "Number: " << vPackageInfo.Number << std::endl;
	std::cout << "Type: " << vPackageInfo.Type << std::endl;
	std::cout << "ResourceName: " << vPackageInfo.ResourceName << std::endl;
	std::cout << "ParentDir: " << vPackageInfo.ParentDir << std::endl;
	std::cout << "DataLength: " << vPackageInfo.DataLength << std::endl;
	std::cout << "Data: " << vPackageInfo.Data << std::endl;
	std::cout << "----------------------------------" << std::endl << std::endl << std::endl;
}

bool Transfer::__searchFile(std::string vFileName, std::string vPath)
{
	WIN32_FIND_DATA FindData;
	std::string NewPath = vPath;
	NewPath.append("\\*.*");
	HANDLE HandleFind = FindFirstFile(NewPath.c_str(), &FindData);

	if (HandleFind == INVALID_HANDLE_VALUE)
	{
		std::cerr << "无法打开指定目录" << std::endl;
		return false;
	}

	do
	{
		if (strcmp(FindData.cFileName, ".") == 0 || strcmp(FindData.cFileName, "..") == 0)
			continue;

		else if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
		{
			NewPath = vPath;
			NewPath.append("\\");
			NewPath.append(FindData.cFileName);
			if (__searchFile(vFileName, NewPath))
				return true;
		}

		else
		{
			if (vFileName == FindData.cFileName)
			{
				m_HitInquireSet.push_back(std::make_pair(vFileName, vPath));
				return true;
			}
		}

	} while (FindNextFile(HandleFind, &FindData));

	FindClose(HandleFind);

	return false;
}

bool Transfer::__searchDirectory(std::string vDirectoryName, std::string vPath)
{
	WIN32_FIND_DATA FindData;
	std::string NewPath = vPath;
	NewPath.append("\\*.*");
	HANDLE HandleFind = FindFirstFile(NewPath.c_str(), &FindData);

	if (HandleFind == INVALID_HANDLE_VALUE)
	{
		std::cerr << "无法打开指定目录" << std::endl;
		return false;
	}

	do
	{
		if (strcmp(FindData.cFileName, ".") == 0 || strcmp(FindData.cFileName, "..") == 0 || (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			continue;
		else
		{
			if (FindData.cFileName == vDirectoryName)
			{
				m_HitInquireSet.push_back(std::make_pair(vDirectoryName, vPath));
				return true;
			}
			else
			{
				NewPath = vPath;
				NewPath.append("\\");
				NewPath.append(FindData.cFileName);
				if (__searchDirectory(vDirectoryName, NewPath))
					return true;
			}
		}
	} while (FindNextFile(HandleFind, &FindData));

	FindClose(HandleFind);

	return false;
}

void Transfer::__sendInquire(Transfer::Comm vCommandInfo)
{
	for (auto Peer : m_ConnectedPeers)
	{
		if (std::find(vCommandInfo.VisitedPeers.begin(), vCommandInfo.VisitedPeers.end(), Peer.Name) != vCommandInfo.VisitedPeers.end())
			continue;
		SOCKET Socket = socket(AF_INET, SOCK_STREAM, 0);
		SOCKADDR_IN AddrSrc;
		inet_pton(AF_INET, Peer.IP.c_str(), &AddrSrc.sin_addr.S_un.S_addr);
		AddrSrc.sin_port = htons(Peer.CommandPort);
		AddrSrc.sin_family = AF_INET;
		connect(Socket, (SOCKADDR*)&AddrSrc, sizeof(AddrSrc));

		std::string Command = __combineCommand(vCommandInfo);
		send(Socket, Command.c_str(), Command.length(), 0);

		closesocket(Socket);
	}
}

bool Transfer::__sendFile(SOCKET vSocket, std::string vFileName, std::string vPath, std::string vParentDir)
{
	const int BufSize = 1024;
	char Buffer[BufSize] = { 0 };
	Pack PackageInfo;
	PackageInfo.Number = 0;
	PackageInfo.Type = 0;
	PackageInfo.ResourceName = vFileName;
	PackageInfo.ParentDir = vParentDir;
	PackageInfo.DataLength = 0;

	std::ifstream SendFile;
	vFileName = vPath + "\\" + vFileName;
	SendFile.open(vFileName, std::iostream::binary);

	if (!SendFile)
	{
		std::cerr << "文件打开错误，或者文件不存在" << std::endl << std::endl;
		return false;
	}

	while (!SendFile.eof())
	{
		SendFile.read(Buffer, BufSize);
		PackageInfo.Data = Buffer;
		PackageInfo.DataLength = SendFile.gcount();
		PackageInfo.Number++;
		std::string Package = __combinePackage(PackageInfo);

		std::cout << "发送数据包为: " << std::endl;
		this->showPackageInfo(PackageInfo);

		send(vSocket, Package.c_str(), Package.length(), 0);
	}
	
	SendFile.close();

	PackageInfo.Number = 0;
	PackageInfo.Type = 3;
	PackageInfo.ParentDir = "";
	PackageInfo.DataLength = 0;
	PackageInfo.Data = "";
	std::string EmptyPack = __combinePackage(PackageInfo);

	Sleep(100);
	send(vSocket, EmptyPack.c_str(), EmptyPack.length(), 0);

	return true;
}

bool Transfer::__sendDirectory(SOCKET vSocket, std::string vDirectoryName, std::string vPath, std::string vParentDir)
{
	Pack PackageInfo;
	PackageInfo.Number = 0;
	PackageInfo.Type = 1;
	PackageInfo.ResourceName = vDirectoryName;
	PackageInfo.ParentDir = vParentDir;
	PackageInfo.DataLength = 0;
	PackageInfo.Data = "";
	
	std::string Package = __combinePackage(PackageInfo);

	std::cout << "发送需创建的文件夹信息: " << std::endl;
	this->showPackageInfo(PackageInfo);

	std::cout << "发送数据包字符串为: " << Package << std::endl << std::endl;

	send(vSocket, Package.c_str(), Package.length(), 0);

	WIN32_FIND_DATA FindData;
	vPath = vPath + "\\" + vDirectoryName;
	std::string NewPath = vPath + "\\*.*";
	std::string NewParentDir = vParentDir + "\\" + vDirectoryName;
	HANDLE HandleFind = FindFirstFile(NewPath.c_str(), &FindData);

	NewPath = vPath;

	if (HandleFind == INVALID_HANDLE_VALUE)
	{
		std::cerr << "无法打开指定目录" << std::endl;
		return false;
	}

	do
	{
		std::cout << "当前文件或文件夹的名字: " << FindData.cFileName << std::endl << std::endl;
		if (strcmp(FindData.cFileName, ".") == 0 || strcmp(FindData.cFileName, "..") == 0)
			continue;

		if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
		{
			if (!__sendDirectory(vSocket, FindData.cFileName, NewPath, NewParentDir))
			{
				std::cerr << "目录 " << FindData.cFileName << " 传输错误" << std::endl << std::endl;
				return false;
			}
		}

		else
		{
			if (!__sendFile(vSocket, FindData.cFileName, NewPath, NewParentDir))
			{
				std::cerr << "文件 " << FindData.cFileName << " 传输错误" << std::endl << std::endl;
				return false;
			}
		}
		
	} while (FindNextFile(HandleFind, &FindData));

	FindClose(HandleFind);

	return true;
}

Transfer::Comm Transfer::__parserCommand(std::string vCommand)
{
	Comm CommandInfo;
	int StartPos = 0;
	int FindPos;

	while ((FindPos = vCommand.find(";", StartPos)) != std::string::npos)
	{
		std::string SubCommand = vCommand.substr(StartPos, FindPos - StartPos);
		int Pos = SubCommand.find("=");
		std::string Key = SubCommand.substr(0, Pos);
		std::string Values = SubCommand.substr(Pos + 1, SubCommand.length() - 1 - Pos);

		if (Key == "Number")
		{
			std::stringstream ss;
			ss << Values;
			ss >> CommandInfo.Number;
		}
		else if (Key == "Type")
		{
			std::stringstream ss;
			ss << Values;
			ss >> CommandInfo.Type;
		}
		else if (Key == "ResourceName")
		{
			CommandInfo.ResourceName = Values;
		}
		else if (Key == "DstIP")
		{
			CommandInfo.DstIP = Values;
		}
		else if (Key=="DstPort")
		{
			std::stringstream ss;
			ss << Values;
			ss >> CommandInfo.DstPort;
		}
		else if (Key == "VisitedPeers")
		{
			int SegStartPos = 0;
			int SegPos;
			while ((SegPos = Values.find(":", SegStartPos)) != std::string::npos)
			{
				CommandInfo.VisitedPeers.push_back(Values.substr(SegStartPos, SegPos - SegStartPos));
				SegStartPos = SegPos + 1;
			}
		}
		else
		{
			std::cerr << "检测到接受的命令中存在无法解析的Key" << std::endl;
		}

		StartPos = FindPos + 1;
	}

	return CommandInfo;
}

std::string Transfer::__combineCommand(Transfer::Comm vCommandInfo)
{
	std::string Command;
	Command = Command + "Number=" + std::to_string(vCommandInfo.Number) + ";";
	Command = Command + "Type=" + std::to_string(vCommandInfo.Type) + ";";
	Command = Command + "ResourceName=" + vCommandInfo.ResourceName + ";";
	Command = Command + "DstIP=" + vCommandInfo.DstIP + ";";
	Command = Command + "DstPort=" + std::to_string(vCommandInfo.DstPort) + ";";
	Command = Command + "VisitedPeers=";
	for (auto Peer : vCommandInfo.VisitedPeers)
	{
		Command = Command + Peer + ":";
	}
	Command = Command + ";";

	return Command;
}

Transfer::Pack Transfer::__parserPackeage(std::string vPackage)
{
	Pack PackageInfo;
	int LeftPos = 0;
	int RightPos = 0;
	int StartPos = 0;

	while ((LeftPos = vPackage.find("{", StartPos)) != std::string::npos && (RightPos = vPackage.find("}", StartPos)) != std::string::npos)
	{
		std::string SubPackage = vPackage.substr(LeftPos + 1, RightPos - LeftPos - 1);
		int Pos = SubPackage.find(":");
		std::string Key = SubPackage.substr(0, Pos);
		std::string Values = SubPackage.substr(Pos + 1, SubPackage.length() - 1 - Pos);

		if (Key == "Number")
		{
			std::stringstream ss;
			ss << Values;
			ss >> PackageInfo.Number;
		}
		else if (Key == "Type")
		{
			std::stringstream ss;
			ss << Values;
			ss >> PackageInfo.Type;
		}
		else if (Key == "ResourceName")
		{
			PackageInfo.ResourceName = Values;
		}
		else if (Key == "ParentDir")
		{
			PackageInfo.ParentDir = Values;
		}
		else if (Key == "DataLength")
		{
			std::stringstream ss;
			ss << Values;
			ss >> PackageInfo.DataLength;
		}
		else if (Key == "Data")
		{
			PackageInfo.Data = Values;
		}
		else
		{
			std::cerr << "检测到接受的命令中存在无法解析的Key" << std::endl;
		}

		StartPos = RightPos + 1;
	}

	return PackageInfo;
}

std::string Transfer::__combinePackage(Transfer::Pack vPackageInfo)
{
	std::string Package;
	Package = Package + "{Number:" + std::to_string(vPackageInfo.Number) + "}";
	Package = Package + "{Type:" + std::to_string(vPackageInfo.Type) + "}";
	Package = Package + "{ResourceName:" + vPackageInfo.ResourceName + "}";
	Package = Package + "{ParentDir:" + vPackageInfo.ParentDir + "}";
	Package = Package + "{DataLength:" + std::to_string(vPackageInfo.DataLength) + "}";
	Package = Package + "{Data:" + vPackageInfo.Data + "}";

	return Package;
}

std::string Transfer::__getPath(std::string vResourceName)
{
	for (auto HitInquire : m_HitInquireSet)
	{
		if (HitInquire.first == vResourceName)
			return HitInquire.second;
	}
	return nullptr;
}

Transfer::~Transfer()
{

}
