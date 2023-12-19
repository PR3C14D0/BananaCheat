#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>

DWORD GetPid(const char* procName);

int main() {
	std::cout << "Welcome to Banana shooter cheat by Preciado" << std::endl;

	const char* dllName = "BananaDLL.dll";
	char dllPath[MAX_PATH];

	GetFullPathName(dllName, MAX_PATH, dllPath, nullptr);

	const char* procName = "Banana Shooter.exe";
	DWORD dwPid = GetPid(procName);

	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPid);

	LPVOID lpvMem = VirtualAllocEx(hProc, nullptr, MAX_PATH, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);

	WriteProcessMemory(hProc, lpvMem, dllPath, MAX_PATH, nullptr);

	HANDLE hThread = CreateRemoteThreadEx(hProc, nullptr, NULL, (LPTHREAD_START_ROUTINE)LoadLibrary, lpvMem, NULL, nullptr, nullptr);

	if (hThread)
		CloseHandle(hThread);

	if (hProc)
		CloseHandle(hProc);
	
	return 0;
}

DWORD GetPid(const char* procName) {
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	PROCESSENTRY32 pe = { };
	pe.dwSize = sizeof(PROCESSENTRY32);

	DWORD dwPid = 0;

	if (Process32First(hSnap, &pe)) {
		do {
			if (strcmp(pe.szExeFile, procName) == 0) {
				dwPid = pe.th32ProcessID;
				break;
			}
		} while (Process32Next(hSnap, &pe));
	}

	return dwPid;
}