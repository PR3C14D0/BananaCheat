#include "Hook.h"

void Hook::DisableSteamOverlay(LPVOID addr) {
	DWORD dwOldProt;
	VirtualProtect(addr, 5, PAGE_EXECUTE_READWRITE, &dwOldProt);
	for (int i = 0; i < 5; i++) {
		*((char*)addr + i) = 0x90;
	}
	
	VirtualProtect(addr, 5, dwOldProt, nullptr);

	return;
}

LPVOID Hook::CreateHook(LPVOID src, LPVOID dst, UINT nMangledBytes, LPVOID& gateway) {
	LPVOID lpLoc = (char*)src - 0x2000;
	LPVOID lpRelay = nullptr;

	while (lpRelay == nullptr) {
		lpRelay = VirtualAlloc(lpLoc, 1, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
		lpLoc = (char*)lpLoc + 0x200;
	}

	std::cout << "Relay located at: 0x" << std::hex << lpRelay << std::endl;

	LPVOID lpAddr = lpRelay;

	LPVOID lpMangledBytes = VirtualAlloc(nullptr, nMangledBytes, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	memcpy(lpMangledBytes, src, nMangledBytes);
	VirtualProtect(lpMangledBytes, nMangledBytes, PAGE_READONLY, nullptr);

	char ABS_JMP[] = {
		0xFF, 0x25, 0x00, 0x00, 0x00, 0x00
	};

	memcpy(lpAddr, ABS_JMP, sizeof(ABS_JMP));
	*(DWORD_PTR*)((char*)lpAddr + sizeof(ABS_JMP)) = (DWORD_PTR)dst;

	lpAddr = (char*)lpAddr + 14;

	memcpy(lpAddr, lpMangledBytes, nMangledBytes);

	/* JMP Rel32: Destination - Source - Size of JMP instruction */
	DWORD relAddr = (DWORD_PTR)src - (DWORD_PTR)lpAddr - 5;
	*((char*)lpAddr + nMangledBytes) = 0xE9;
	*(DWORD*)((char*)lpAddr + nMangledBytes + 1) = relAddr;

	gateway = lpAddr;

	return lpRelay;
}

void Hook::Detour32(LPVOID src, LPVOID dst, UINT nMangledBytes) {
	DWORD dwOldProt;
	VirtualProtect(src, nMangledBytes, PAGE_EXECUTE_READWRITE, &dwOldProt);
	for (int i = 0; i < nMangledBytes; i++) {
		*((char*)src + i) = 0x90;
	}

	DWORD dwRelAddr = (DWORD_PTR)dst - (DWORD_PTR)src - 5;
	*(char*)src = 0xE9;
	*(DWORD*)((char*)src + 1) = dwRelAddr;
	VirtualProtect(src, nMangledBytes, dwOldProt, nullptr);

	return;
}