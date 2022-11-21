#include "MemReader.h"

MemReader::MemReader()
{
	processNames.push_back(L"eldenring.exe");
	processNames.push_back(L"start_protected_game.exe");

	pHandle = NULL;
	pID = 0;
	baseAddress = 0;
	moduleSize = 0;
	scriptMemory = 0;
	scriptSize = 0;
}

MemReader::~MemReader()
{
	free();
}

bool MemReader::initialize()
{
	if (pHandle != NULL && !getProcessStatus())
		closeProc();

	if (pHandle == NULL)
	{
		getProcess();
		getModule();
		if (openProc())
			hookMemory();
	}

	return getProcessStatus();
}

bool MemReader::validateProcessName(const wchar_t* processName)
{
	for (wstring validName : processNames)
	{
		if (validName == processName)
			return true;
	}

	return false;
}

DWORD MemReader::getProcess()
{
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	PROCESSENTRY32 pInfo;
	pInfo.dwSize = sizeof(PROCESSENTRY32);

	if (Process32First(snapshot, &pInfo)) {
		do {
			if (validateProcessName(pInfo.szExeFile)) {
				chosenProcessName = pInfo.szExeFile;
				pID = pInfo.th32ProcessID;
				break;
			}
		} while (Process32Next(snapshot, &pInfo));
	}

	CloseHandle(snapshot);
	return pID;
}

DWORD64 MemReader::getModule()
{
	HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pID);
	MODULEENTRY32 mInfo;
	mInfo.dwSize = sizeof(MODULEENTRY32);

	if (Module32First(snapshot, &mInfo)) {
		do {
			if (wcsstr(mInfo.szExePath, chosenProcessName.c_str())) {
				baseAddress = (DWORD64)mInfo.modBaseAddr;
				moduleSize = mInfo.modBaseSize;
				break;
			}
		} while (Module32Next(snapshot, &mInfo));
	}

	CloseHandle(snapshot);
	return baseAddress;
}

HANDLE MemReader::openProc()
{
	return pHandle = OpenProcess(PROCESS_ALL_ACCESS, 0, pID);
}

bool MemReader::getProcessStatus()
{
	DWORD exitCode = 0;
	GetExitCodeProcess(pHandle, &exitCode);

	return exitCode == STILL_ACTIVE ? true : false;
}

void MemReader::closeProc()
{
	CloseHandle(pHandle);
	pHandle = NULL;
	pID = NULL;
}

void MemReader::spawnBullet(int bulletId, float x, float y, float z, float vectorX, float vectorZ)
{

	float coords[3]{ x, y, z };
	float vectors[3]{ vectorX, 0, vectorZ };

	writeMemory((LPVOID)(dataMemory + BULLET_ID), (LPVOID)&bulletId, 4);
	writeMemory((LPVOID)(dataMemory + BULLET_COORDS), (LPVOID)&coords, 12);
	writeMemory((LPVOID)(dataMemory + BULLET_VECTORS), (LPVOID)&vectors, 12);

	CreateRemoteThread(pHandle, 0, 0, (LPTHREAD_START_ROUTINE)scriptMemory, nullptr, 0, 0);

	//wstringstream str;
	//str << std::fixed << x << " " << y << " " << z;
	//str << endl;
	//str << std::fixed << vectorX << " "  << " " << vectorZ;
	//MessageBox(0, str.str().c_str(), L"test", MB_OK);
}

POINT3D MemReader::getCharPos()
{
	POINT3D p;
	DWORD64 addr;
	readMemory((LPVOID)worldChrManAddr, (LPVOID)&addr, 8);
	readMemory((LPVOID)(addr + 0x18468), (LPVOID)&addr, 8);
	readMemory((LPVOID)(addr + 0x190), (LPVOID)&addr, 8);
	readMemory((LPVOID)(addr + 0x68), (LPVOID)&addr, 8);
	readMemory((LPVOID)(addr + 0x70), (LPVOID)&p, 12);

	return p;
}

float MemReader::getCharAngle()
{
	float angle;
	DWORD64 addr;

	readMemory((LPVOID)worldChrManAddr, (LPVOID)&addr, 8);
	readMemory((LPVOID)(addr + 0x18468), (LPVOID)&addr, 8);
	readMemory((LPVOID)(addr + 0x6D0), (LPVOID)&angle, 4);
	return angle;
}

bool MemReader::readMemory(LPVOID source, LPVOID destination, int size)
{
	return ReadProcessMemory(pHandle, source, destination, size, NULL);
}

bool MemReader::writeMemory(LPVOID destination, LPVOID source, int size)
{
	return WriteProcessMemory(pHandle, destination, source, size, NULL);
}

DWORD64	MemReader::alloc(int size)
{
	MEMORY_BASIC_INFORMATION mbi;
	DWORD64 regionAddress = 0;
	DWORD64 offset = 0;
	DWORD64 allocatedMemory = 0;
	while ((regionAddress = baseAddress - offset) <= baseAddress)
	{
		VirtualQueryEx(pHandle, (LPCVOID)regionAddress, &mbi, sizeof(MEMORY_BASIC_INFORMATION));

		if (mbi.State == MEM_FREE)
		{
			if (allocatedMemory = (DWORD64)VirtualAllocEx(pHandle, (LPVOID)regionAddress, size, MEM_RESERVE, PAGE_EXECUTE_READWRITE))
			{
				allocatedMemory = (DWORD64)VirtualAllocEx(pHandle, (LPVOID)allocatedMemory, size, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
				break;
			}
		}

		offset += mbi.RegionSize;
	}

	return allocatedMemory;
}

void MemReader::hookMemory()
{
	//Allocate memory in game process for our code and get its size
	try
	{
		scriptMemory = alloc(4096);
		dataMemory = alloc(8192);
		if (!scriptMemory || !dataMemory)
			throw runtime_error("Couldn't allocate memory");
	}
	catch (const runtime_error& e)
	{
		free();
		throw;
	}

	scriptSize = getScriptSize(spawnBulletProc);

	//Write script code to the allocated memory
	writeMemory((LPVOID)scriptMemory, (LPVOID)spawnBulletProc, scriptSize);

	//Write BULLET_MAN
	DWORD64 BULLET_MAN_ADDR = findSignature(baseAddress, moduleSize, L"48 8B 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 8D 44 24 ?? 48 89 44 24 ?? 48 89 7C 24 ?? C7 44 24 ?? ?? ?? ?? ?? 48");
	DWORD32 BULLET_MAN_OFFSET;
	readMemory((LPVOID)(BULLET_MAN_ADDR + 3), (LPVOID)&BULLET_MAN_OFFSET, 4);
	BULLET_MAN_ADDR = BULLET_MAN_ADDR + BULLET_MAN_OFFSET + 7;
	writeMemory((LPVOID)(scriptMemory + scriptSize + SCRIPT_OFFSET + BULLET_MAN), (LPVOID)&BULLET_MAN_ADDR, 8);
	
	//Write Bullet call address
	DWORD64 BULLET_CALL_ADDR = findSignature(baseAddress, moduleSize, L"40 53 55 56 57 48 81 EC 98 07 00 00 48 C7 44 24 ?? ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 33 C4");
	writeMemory((LPVOID)(scriptMemory + scriptSize + SCRIPT_OFFSET), (LPVOID)&BULLET_CALL_ADDR, 8);

	//Write data buffer address
	writeMemory((LPVOID)(scriptMemory + scriptSize + SCRIPT_OFFSET + DATA_ADDR), (LPVOID)&dataMemory, 8);

	//Write bullet data
	vector<wstring> bulletDataParsed = parseHexString(L"00 00 90 16 FF FF FF FF FF FF FF FF AA 0F 00 00 00 00 00 00 E8 B4 9E 00 FF FF FF FF 74 27 00 00 FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF FF 0A 9C 98 3F 00 00 80 3F 6B 79 83 3F 00 00 80 3F 00 00 00 00 0B 00 00 00 01 00 00 00 00 00 00 00 F2 34 76 BF 00 00 00 00 73 AB 62 3E 00 00 00 00 5B 54 12 3D FF 54 79 3F 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00");
	BYTE* bulletData = new BYTE[bulletDataParsed.size()];
	getSignBytes(bulletData, bulletDataParsed);
	writeMemory((LPVOID)(dataMemory + BULLET_MAN_AOB), (LPVOID)bulletData, bulletDataParsed.size());

	//Get and save world chr man
	worldChrManAddr = findSignature(baseAddress, moduleSize, L"48 8B 05 ?? ?? ?? ?? 48 85 C0 74 0F 48 39 88 ?? ?? ?? ?? 75 06 89 B1 5C 03 00 00 0F 28 05 ?? ?? ?? ?? 4C 8D 45 E7");
	DWORD32 WORLD_CHR_MAN_OFFSET;
	readMemory((LPVOID)(worldChrManAddr + 3), (LPVOID)&WORLD_CHR_MAN_OFFSET, 4);
	worldChrManAddr = worldChrManAddr + WORLD_CHR_MAN_OFFSET + 7;

	//7FF6D2B30828
	//7FF6D2B30048
	//7FF6D2B30000
	//wstringstream str;
	//str << std::hex << scriptMemory + scriptSize + SCRIPT_OFFSET << " " << dataMemory;
	//MessageBox(0, str.str().c_str(), L"test", MB_OK);
}

void MemReader::free()
{
	if (scriptMemory)
		VirtualFreeEx(pHandle, (LPVOID)scriptMemory, 0, MEM_RELEASE);

	if (dataMemory)
		VirtualFreeEx(pHandle, (LPVOID)dataMemory, 0, MEM_RELEASE);
}

DWORD MemReader::getScriptSize(LPVOID script)
{
	int size = 0;
	BYTE* ptr = (BYTE*)script;

	while (*((DWORD*)ptr++) != 0x90909090)
	{
		size++;
	}

	return size;
}

DWORD64 MemReader::findSignature(DWORD64 base, DWORD64 searchSize, wstring sign)
{
	vector<wstring> parsedSign = parseHexString(sign);
	BYTE* aob = new BYTE[parsedSign.size()];
	char* mask = new char[parsedSign.size()];
	getSignBytes(aob, parsedSign);
	getMask(mask, parsedSign);

	MEMORY_BASIC_INFORMATION mbi = { 0 };
	DWORD64 offset = 0;
	DWORD64 res = 0;
	while (offset < searchSize) {
		VirtualQueryEx(pHandle, (LPCVOID)(base + offset), &mbi, sizeof(MEMORY_BASIC_INFORMATION));
		if (mbi.State != MEM_FREE) {
			BYTE* buffer = new BYTE[mbi.RegionSize];
			readMemory(mbi.BaseAddress, buffer, mbi.RegionSize);
			for (int i = 0; i <= mbi.RegionSize - parsedSign.size(); i++) {
				if (dataCompare(buffer + i, aob, mask, parsedSign.size())) {
					delete[] buffer;
					res = (DWORD64)mbi.BaseAddress + i;
					goto ret;
				}
			}
			delete[] buffer;
		}
		offset += mbi.RegionSize;
	}

ret:
	delete[] aob;
	delete[] mask;
	return res;
}

bool MemReader::dataCompare(const BYTE* data, const BYTE* sign, const char* mask, int size)
{
	for (int i = 0; i < size; i++) {
		if (mask[i] == 'x' && data[i] != sign[i]) return false;
	}
	return true;
}

vector<wstring> MemReader::parseHexString(wstring aob)
{
	int offset = 0;
	int end;

	vector<wstring> res;

	while (offset < aob.length())
	{
		end = aob.find(L" ", offset);
		if (end == wstring::npos)
			end = aob.length();

		res.push_back(aob.substr(offset, 2));
		offset = end + 1;
	}

	return res;
}

void MemReader::getSignBytes(BYTE* aob, vector<wstring>& parsedSign)
{
	for (int i = 0; i < parsedSign.size(); i++)
		aob[i] = parsedSign[i] == L"??" ? 0x0 : stoul(parsedSign[i], nullptr, 16);
}

void MemReader::getMask(char* mask, vector<wstring>& parsedSign)
{
	for (int i = 0; i < parsedSign.size(); i++)
		mask[i] = parsedSign[i] == L"??" ? '?' : 'x';
}