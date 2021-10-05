#include <Windows.h>
#include <iostream>
#include <cctype>
#include <unordered_set>
#include <sstream>

#define MBR_SIZE 512

std::string getLastErrorStr()
{
	DWORD errorMessageID = ::GetLastError();
	if (errorMessageID == 0)
	{
		return std::string();
	}

	LPSTR messageBuffer = nullptr;

	size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

	std::string message(messageBuffer, size);
	LocalFree(messageBuffer);

	return message;
}

void signalError(const char *msg)
{
	std::cout << msg << "\nErr message:\n";
	std::cout << getLastErrorStr() << "\n";
}

void errorOut(const char* msg)
{
	signalError(msg);
	system("pause");
	exit(0);
};

int main()
{

	#pragma region read logical drives
	char logicalDrives['Z' - 'A' + 1]{};
	{
		char buf[1024]{};
		GetLogicalDriveStrings(sizeof(buf), buf);

		bool doubleZero = 0;
		for (int i = 0; i < sizeof(buf); i++)
		{
			if (buf[i] == 0)
			{
				//std::cout << "\n";

				if (doubleZero) { break; }
				doubleZero = true;
			}
			else
			{
				if(isalpha(buf[i]))
				{
					char letter = toupper(buf[i]);
					logicalDrives[letter - 'A'] = true;
				}

				//std::cout << buf[i];
				doubleZero = false;
			}
		}
	}
	#pragma endregion

	#pragma region reading all drives
	std::unordered_set<int> drives;
	
	for (int d = 0; d < sizeof(logicalDrives); d++)
	{
		if (logicalDrives[d] == false)
		{
			continue;
		}

		std::string driveName = "\\\\.\\";
		driveName += (char)('A' + d);
		driveName += ':';


		HANDLE logicalDrive = CreateFile(driveName.c_str(), GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, NULL, NULL);
		if (logicalDrive == INVALID_HANDLE_VALUE)
		{
			signalError(("Not able to open " + driveName).c_str());
		}
		else
		{
			char diskExtents[4028]{};
			DWORD dwSize;
			if (!DeviceIoControl(
				logicalDrive,
				IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS,
				NULL,
				0,
				(LPVOID)&diskExtents,
				(DWORD)sizeof(diskExtents),
				(LPDWORD)&dwSize,
				NULL)
				)
			{
				signalError("Not able to retrive logical drive info");
			}
			else
			{

				for (int i = 0; i < ((VOLUME_DISK_EXTENTS *)(&diskExtents))->NumberOfDiskExtents; i++)
				{
					drives.insert(((VOLUME_DISK_EXTENTS *)(&diskExtents))->Extents[i].DiskNumber);
				}

			}
			
			CloseHandle(logicalDrive);

		}
		


	}

	
	#pragma endregion
	for (auto d : drives)
	{
		char mbrData[MBR_SIZE] = {};

		std::string driveName = "\\\\.\\PhysicalDrive";
		driveName += std::to_string(d);


		HANDLE file = CreateFile(driveName.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

		if (file == INVALID_HANDLE_VALUE)
		{
			signalError("Not able to open file");
		}
		else
		{
			DWORD sizeRead = 0;
			if (!ReadFile(file, mbrData, MBR_SIZE, &sizeRead, NULL))
			{
				signalError("Not able to read file");
			}
			else
			{
				//check if it is a boot sector
				if ( *((unsigned short*)(&mbrData[MBR_SIZE - 2])) == 0xAA55 )
				{
					std::cout << "Found mbr: " << driveName << "\n";
					//std::cout << "Readed " << sizeRead << "\n\n";

					int v = 0;
					while (v < MBR_SIZE)
					{

						for (int i = 0; i < 16; i++)
						{
							std::cout << std::hex << (int)(*((unsigned char *)&mbrData[v])) << " ";
							v++;

							if (v >= MBR_SIZE) { break; }
						}
						std::cout << "\n";
					}

					std::cout << "\n";
				}
				
			}

			CloseHandle(file);
		}

		
	}

	

	system("pause");

	return 0;
}