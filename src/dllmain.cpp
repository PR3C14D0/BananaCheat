#include <iostream>
#include <Windows.h>
#include <dxgi.h>
#include <d3d11.h>
#include <wrl.h>
#include "Hook.h"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_dx11.h>
#include "il2cpp.h"
#include "Offsets.h"

using namespace Microsoft::WRL;

typedef HRESULT(__stdcall* DXGIPresent_t)(IDXGISwapChain*, UINT, UINT);
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

typedef void(__fastcall* GunUpdate_t)(Weapon_Firearms_o* __this, const MethodInfo* method);

void Main();
void DebugConsole();

void HookD3D();
void HookGameFunctions();

LONG_PTR OldWndProc;

HWND g_hwnd = NULL;
int g_width = 0;
int g_height = 0;

ComPtr<ID3D11Device> g_dev;
ComPtr<ID3D11DeviceContext> g_con;
ComPtr<IDXGISwapChain> g_sc;

ComPtr<ID3D11Texture2D> g_pBackBuffer;
ComPtr<ID3D11RenderTargetView> g_pRtv;

bool g_bImGui = false;
bool g_bShowHUD = true;
bool g_bInfiniteAmmo = false;

DXGIPresent_t ogPresent = nullptr;
GunUpdate_t ogGunUpdate = nullptr;

HRESULT __stdcall hkPresent(IDXGISwapChain* This, UINT SyncInterval, UINT Flags);
LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void __fastcall hkGunUpdate(Weapon_Firearms_o* __this, const MethodInfo* method);

void SetStyle();

static void ThrowIfFailed(HRESULT hr) {
	if (FAILED(hr))
		throw std::exception();
	return;
}

BOOL WINAPI DllMain(
	HINSTANCE hInstance,
	DWORD dwReason,
	LPVOID lpvReserved) 
{
	switch (dwReason) {
	case DLL_PROCESS_ATTACH:
		CreateThread(nullptr, NULL, (LPTHREAD_START_ROUTINE)Main, hInstance, NULL, nullptr);
		break;
	}

	return TRUE;
}

void Main() {
#ifndef NDEBUG
	DebugConsole();
#endif // NDEBUG

	std::cout << "Banana shooter cheat DLL By Preciado" << std::endl;

	g_hwnd = FindWindow(nullptr, "Banana Shooter");

	RECT rect;
	GetWindowRect(g_hwnd, &rect);
	g_width = rect.right - rect.left;
	g_height = rect.bottom - rect.top;

	HookD3D();
	HookGameFunctions();
}

void DebugConsole() {
	AllocConsole();
	FILE* f;
	freopen_s(&f, "CONOUT$", "w", stdout);
	
	return;
}

void HookD3D() {
	DXGI_SWAP_CHAIN_DESC scDesc = { };
	scDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	scDesc.BufferCount = 1;
	scDesc.SampleDesc.Count = 1;
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	scDesc.Windowed = TRUE;
	scDesc.OutputWindow = g_hwnd;
	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	
	ThrowIfFailed(D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		NULL,
		nullptr,
		NULL,
		D3D11_SDK_VERSION,
		&scDesc,
		g_sc.GetAddressOf(),
		g_dev.GetAddressOf(),
		nullptr,
		g_con.GetAddressOf()
	));

	void** pVmt = *(reinterpret_cast<void***>(g_sc.Get()));

	std::cout << "IDXGISwapChain VMT at: 0x" << std::hex << pVmt << std::endl;

	LPVOID lpPresent = pVmt[8];
	ogPresent = reinterpret_cast<DXGIPresent_t>(lpPresent);

	std::cout << "IDXGISwapChain::Present address at: 0x" << std::hex << lpPresent << std::endl;

	g_sc->Release();
	g_dev->Release();
	g_con->Release();

	Hook::DisableSteamOverlay(lpPresent);

	LPVOID lpGateway;
	LPVOID lpRelay;

	lpRelay = Hook::CreateHook(lpPresent, &hkPresent, 5, lpGateway);
	ogPresent = reinterpret_cast<DXGIPresent_t>(lpGateway);
	Hook::Detour32(lpPresent, lpRelay, 5);

	OldWndProc = SetWindowLongPtr(g_hwnd, GWLP_WNDPROC, (LONG_PTR)WndProc);
}

void HookGameFunctions() {
	HMODULE GameAssembly = GetModuleHandle("GameAssembly.dll");
	LPVOID lpGameAssembly = reinterpret_cast<LPVOID>(GameAssembly);

	LPVOID lpGunUpdate = (char*)lpGameAssembly + GUN_UPDATE;
	ogGunUpdate = reinterpret_cast<GunUpdate_t>(lpGunUpdate);

	LPVOID lpGateway;
	LPVOID lpRelay = Hook::CreateHook(lpGunUpdate, &hkGunUpdate, 7, lpGateway);
	ogGunUpdate = reinterpret_cast<GunUpdate_t>(lpGateway);

	Hook::Detour32(lpGunUpdate, lpRelay, 7);
}

void SetStyle() {
	/* Taken from: https://www.unknowncheats.me/forum/c-and-c-/189635-imgui-style-settings.htmls */
	ImGuiStyle* style = &ImGui::GetStyle();

	style->WindowPadding = ImVec2(15, 15);
	style->WindowRounding = 5.0f;
	style->FramePadding = ImVec2(5, 5);
	style->FrameRounding = 4.0f;
	style->ItemSpacing = ImVec2(12, 8);
	style->ItemInnerSpacing = ImVec2(8, 6);
	style->IndentSpacing = 25.0f;
	style->ScrollbarSize = 15.0f;
	style->ScrollbarRounding = 9.0f;
	style->GrabMinSize = 5.0f;
	style->GrabRounding = 3.0f;

	style->Colors[ImGuiCol_Text] = ImVec4(0.80f, 0.80f, 0.83f, 1.00f);
	style->Colors[ImGuiCol_TextDisabled] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	style->Colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_PopupBg] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
	style->Colors[ImGuiCol_Border] = ImVec4(0.80f, 0.80f, 0.83f, 0.88f);
	style->Colors[ImGuiCol_BorderShadow] = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
	style->Colors[ImGuiCol_FrameBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	style->Colors[ImGuiCol_FrameBgActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
	style->Colors[ImGuiCol_TitleBgActive] = ImVec4(0.07f, 0.07f, 0.09f, 1.00f);
	style->Colors[ImGuiCol_MenuBarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
	style->Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_CheckMark] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
	style->Colors[ImGuiCol_SliderGrab] = ImVec4(0.80f, 0.80f, 0.83f, 0.31f);
	style->Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_Button] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_ButtonHovered] = ImVec4(0.24f, 0.23f, 0.29f, 1.00f);
	style->Colors[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_Header] = ImVec4(0.10f, 0.09f, 0.12f, 1.00f);
	style->Colors[ImGuiCol_HeaderHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_HeaderActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_ResizeGrip] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style->Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.56f, 0.56f, 0.58f, 1.00f);
	style->Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.06f, 0.05f, 0.07f, 1.00f);
	style->Colors[ImGuiCol_PlotLines] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
	style->Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	style->Colors[ImGuiCol_PlotHistogram] = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
	style->Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
	style->Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
}

HRESULT __stdcall hkPresent(IDXGISwapChain* This, UINT SyncInterval, UINT Flags) {
	if (!g_bImGui) {
		This->GetDevice(IID_PPV_ARGS(g_dev.GetAddressOf()));
		g_dev->GetImmediateContext(g_con.GetAddressOf());

		ThrowIfFailed(This->GetBuffer(0, IID_PPV_ARGS(g_pBackBuffer.GetAddressOf())));
		ThrowIfFailed(g_dev->CreateRenderTargetView(g_pBackBuffer.Get(), nullptr, g_pRtv.GetAddressOf()));

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGui_ImplWin32_Init(g_hwnd);
		ImGui_ImplDX11_Init(g_dev.Get(), g_con.Get());

		ImGuiIO& io = ImGui::GetIO(); (void)io;

		SetStyle();

		g_bImGui = true;
	}

	g_con->OMSetRenderTargets(1, g_pRtv.GetAddressOf(), nullptr);

	ImGui_ImplWin32_NewFrame();
	ImGui_ImplDX11_NewFrame();
	ImGui::NewFrame();

	ImGui::SetNextWindowSize(ImVec2{ 300, 300 });
	ImGui::Begin("BananaCheat");

	if (ImGui::Button("Infinite Ammo (X)"))
		g_bInfiniteAmmo = !g_bInfiniteAmmo;

	if(ImGui::Button("Show / Hide (Z)"))
		g_bShowHUD = !g_bShowHUD;

	if (ImGui::IsKeyPressed(ImGuiKey_Z, false))
		g_bShowHUD = !g_bShowHUD;

	if(ImGui::IsKeyPressed(ImGuiKey_X, false))
		g_bInfiniteAmmo = !g_bInfiniteAmmo;

	ImGui::End();

	ImGui::Render();
	if(g_bShowHUD)
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	return ogPresent(This, SyncInterval, Flags);
}

void __fastcall hkGunUpdate(Weapon_Firearms_o* __this, const MethodInfo* method) {
	if (g_bInfiniteAmmo) {
		__this->fields.reloadTime = 0.1f;
		__this->fields.maxAmmo = 500;
		__this->fields.reloadMultiplier = 0.f;
		__this->fields.damage = 1000;
	}
	
	ogGunUpdate(__this, method);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
		return 0;

	return CallWindowProc((WNDPROC)OldWndProc, hwnd, uMsg, wParam, lParam);
}