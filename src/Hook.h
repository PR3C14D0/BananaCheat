#include <iostream>
#include <Windows.h>

namespace Hook {
	void DisableSteamOverlay(LPVOID addr);

	LPVOID CreateHook(LPVOID src, LPVOID dst, UINT nMangledBytes, LPVOID& gateway);
	void Detour32(LPVOID src, LPVOID dst, UINT nMangledBytes);
}