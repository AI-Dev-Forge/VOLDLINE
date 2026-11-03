#include "overlay.h"
#include "MemoryManager.hpp"
#include "AutoUpdater.hpp"
#include <string>
#include <chrono>
#include <thread>
#include <algorithm>
#include <vector>
#include <cmath>
#include <shobjidl.h>
#pragma comment(lib, "shell32.lib")

// =====================================================================
//  CS2 Offsets (client.dll)
// =====================================================================
namespace Offsets {
	constexpr uintptr_t dwEntityList = 0x271EBA8;
	constexpr uintptr_t dwViewMatrix = 0x256F250;
	constexpr uintptr_t dwLocalPlayerPawn = 0x2569948;
	constexpr uintptr_t dwViewAngles = 0x2580098;

	// CCSPlayerController
	constexpr uintptr_t m_hPlayerPawn = 0x92C;

	// C_BaseEntity / C_CSPlayerPawn
	constexpr uintptr_t m_iHealth = 0x34C;
	constexpr uintptr_t m_lifeState = 0x354;
	constexpr uintptr_t m_iTeamNum = 0x3E7;
	constexpr uintptr_t m_vOldOrigin = 0x14A4;
	constexpr uintptr_t m_vecViewOffset = 0xF60;
}

// =====================================================================
//  Configuration & Feature Settings
// =====================================================================
enum CrosshairStyle {
	Crosshair_Cross = 0,
	Crosshair_T_Style,
	Crosshair_Circle,
	Crosshair_Dot,
	Crosshair_CrossAndDot,
	Crosshair_CircleAndDot
};

struct CrosshairConfig {
	bool  enabled = true;
	int   style = Crosshair_Cross;
	float size = 8.0f;
	float thickness = 2.0f;
	float gap = 4.0f;
	bool  centerDot = false;
	float dotRadius = 2.0f;
	bool  outline = true;
	float outlineThickness = 1.0f;
	float color[4] = { 0.0f, 1.0f, 0.0f, 1.0f }; // Neon green
	bool  separateDotColor = false;
	float dotColor[4] = { 1.0f, 0.0f, 0.0f, 1.0f }; // Red
	bool  lockToCenter = true;
	float offsetX = 0.0f;
	float offsetY = 0.0f;
};

struct ESPConfig {
	bool  enabled = true;
	bool  enemyOnly = true;

	// HP Dot (Original Feature)
	bool  hpDot = true;
	float hpDotRadius = 4.5f;
	bool  hpDotOutline = true;

	// HP Text (Original Feature)
	bool  hpText = true;
	int   hpTextFormat = 0; // 0: "100 HP", 1: "100", 2: "[100%]"

	// Box ESP
	bool  box = true;
	int   boxStyle = 0; // 0: 2D Box, 1: Corner Box
	float boxThickness = 1.5f;
	bool  boxOutline = true;
	bool  boxFill = false;
	float boxFillAlpha = 0.20f;
	float boxColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	bool  boxHealthColor = false;

	// Health Bar
	bool  healthBar = true;
	float healthBarWidth = 3.0f;

	// Snaplines / Tracers
	bool  snaplines = false;
	int   snaplineOrigin = 0; // 0: Bottom, 1: Center, 2: Top
	float snaplineThickness = 1.0f;
	float snaplineColor[4] = { 0.2f, 0.7f, 1.0f, 0.7f };

	// Head Circle
	bool  headCircle = false;
	float headCircleRadius = 5.0f;
	float headCircleColor[4] = { 1.0f, 0.2f, 0.2f, 1.0f };

	// Distance
	bool  distance = true;
};

struct AimbotConfig {
	bool  enabled = false;
	bool  showFov = true;
	bool  showTargetLine = false;
	float fovRadius = 80.0f;
	float smoothing = 3.0f;
	bool  requireKey = true; // Requires holding left mouse button to activate
};

struct GeneralConfig {
	bool watermark = true;
	bool showFPS = true;
};

struct AppConfig {
	CrosshairConfig crosshair;
	ESPConfig       esp;
	AimbotConfig    aimbot;
	GeneralConfig   general;
} g_Config;

// =====================================================================
//  CS2 Entity & Memory Helpers
// =====================================================================
uintptr_t GetPlayer(uint32_t i) {
	uintptr_t entityList = mem.Read<uintptr_t>(module_base + Offsets::dwEntityList);
	if (!entityList) return 0;

	uintptr_t listEntry = mem.Read<uintptr_t>(entityList + (8 * ((i & 0x7FFF) >> 9)) + 16);
	if (!listEntry) return 0;

	uintptr_t controller = mem.Read<uintptr_t>(listEntry + 112 * (i & 0x1FF));
	if (controller < 0x1000000 || (controller >> 48) != 0) return 0;

	uint32_t pawnHandle = mem.Read<uint32_t>(controller + Offsets::m_hPlayerPawn);
	if (!pawnHandle) return 0;

	uintptr_t listEntry2 = mem.Read<uintptr_t>(entityList + (8 * ((pawnHandle & 0x7FFF) >> 9)) + 16);
	if (!listEntry2) return 0;

	uintptr_t pawn = mem.Read<uintptr_t>(listEntry2 + 112 * (pawnHandle & 0x1FF));
	if (pawn < 0x1000000 || (pawn >> 48) != 0) return 0;

	return pawn;
}

Vector3 GetLocationOfPlayer(uintptr_t player) {
	return mem.Read<Vector3>(player + Offsets::m_vOldOrigin);
}

int GetPlayerHealth(uintptr_t player) {
	return mem.Read<int>(player + Offsets::m_iHealth);
}

uint8_t GetPlayerLifeState(uintptr_t player) {
	return mem.Read<uint8_t>(player + Offsets::m_lifeState);
}

uint8_t GetPlayerTeam(uintptr_t player) {
	return mem.Read<uint8_t>(player + Offsets::m_iTeamNum);
}

bool IsPlayerAlive(uintptr_t player) {
	if (!player) return false;
	int health = GetPlayerHealth(player);
	uint8_t lifeState = GetPlayerLifeState(player);
	return (health > 0 && lifeState == 0);
}

bool WorldToScreen(const Vector3& pos, Vector2& screen)
{
	Matrix4x4 matrix = mem.Read<Matrix4x4>(module_base + Offsets::dwViewMatrix);
	Vector2 screenSize = GetScreenSize();

	float w = matrix.m[3][0] * pos.x + matrix.m[3][1] * pos.y + matrix.m[3][2] * pos.z + matrix.m[3][3];
	if (w < 0.01f)
		return false;

	float invW = 1.0f / w;
	float x = (matrix.m[0][0] * pos.x + matrix.m[0][1] * pos.y + matrix.m[0][2] * pos.z + matrix.m[0][3]) * invW;
	float y = (matrix.m[1][0] * pos.x + matrix.m[1][1] * pos.y + matrix.m[1][2] * pos.z + matrix.m[1][3]) * invW;

	screen.x = (screenSize.x / 2.0f) * (1.0f + x);
	screen.y = (screenSize.y / 2.0f) * (1.0f - y);

	return true;
}

// =====================================================================
//  Aimbot Hardware Input & Logic
// =====================================================================
void MoveMouseRelative(int x, int y)
{
	INPUT input = { 0 };
	input.type = INPUT_MOUSE;
	input.mi.dwFlags = MOUSEEVENTF_MOVE;
	input.mi.dx = x;
	input.mi.dy = y;
	SendInput(1, &input, sizeof(INPUT));
}

void RunAimbot(uintptr_t localPlayer, uint8_t localTeam)
{
	if (!g_Config.aimbot.enabled)
		return;

	Vector2 screenSize = GetScreenSize();
	float centerX = screenSize.x / 2.0f;
	float centerY = screenSize.y / 2.0f;

	ImDrawList* drawList = ImGui::GetBackgroundDrawList();

	// Draw FOV Circle
	if (g_Config.aimbot.showFov) {
		drawList->AddCircle(ImVec2(centerX, centerY), g_Config.aimbot.fovRadius, IM_COL32(255, 255, 255, 200), 64, 1.0f);
	}

	uintptr_t bestTargetPawn = 0;
	float closestDistance = g_Config.aimbot.fovRadius;
	Vector2 bestTargetScreenPos = { 0.0f, 0.0f };

	for (uint32_t i = 0; i < 64; i++)
	{
		uintptr_t player = GetPlayer(i);
		if (!player || player == localPlayer || !IsPlayerAlive(player))
			continue;

		if (localTeam != 0 && GetPlayerTeam(player) == localTeam)
			continue;

		Vector3 targetWorldPos = GetLocationOfPlayer(player);
		targetWorldPos.z += 50.0f; // Target upper chest/neck area
		Vector2 targetScreenPos;

		if (WorldToScreen(targetWorldPos, targetScreenPos))
		{
			float deltaX = targetScreenPos.x - centerX;
			float deltaY = targetScreenPos.y - centerY;
			float currentPixelDistance = std::sqrt(deltaX * deltaX + deltaY * deltaY);

			if (currentPixelDistance < closestDistance)
			{
				closestDistance = currentPixelDistance;
				bestTargetPawn = player;
				bestTargetScreenPos = targetScreenPos;
			}
		}
	}

	if (bestTargetPawn != 0)
	{
		if (g_Config.aimbot.showTargetLine) {
			drawList->AddLine(ImVec2(centerX, centerY), ImVec2(bestTargetScreenPos.x, bestTargetScreenPos.y), IM_COL32(255, 0, 0, 220), 2.0f);
		}

		bool shouldShootOrHold = !g_Config.aimbot.requireKey || (GetAsyncKeyState(VK_LBUTTON) & 0x8000);
		if (shouldShootOrHold)
		{
			float moveX = bestTargetScreenPos.x - centerX;
			float moveY = bestTargetScreenPos.y - centerY;

			if (g_Config.aimbot.smoothing > 0.0f)
			{
				moveX /= g_Config.aimbot.smoothing;
				moveY /= g_Config.aimbot.smoothing;
			}

			MoveMouseRelative(static_cast<int>(moveX), static_cast<int>(moveY));
		}
	}
}

// =====================================================================
//  Customizable Crosshair Drawing Helper
// =====================================================================
void DrawCrosshairAt(ImDrawList* drawList, Vector2 center, const CrosshairConfig& cfg)
{
	ImU32 color = ImGui::ColorConvertFloat4ToU32(*(ImVec4*)cfg.color);
	ImU32 dotColor = cfg.separateDotColor ? ImGui::ColorConvertFloat4ToU32(*(ImVec4*)cfg.dotColor) : color;
	ImU32 outlineColor = IM_COL32(0, 0, 0, (int)(cfg.color[3] * 255.0f));

	float cx = center.x;
	float cy = center.y;
	float halfThick = cfg.thickness / 2.0f;
	float ot = cfg.outlineThickness;

	auto DrawBar = [&](float x1, float y1, float x2, float y2) {
		if (cfg.outline) {
			drawList->AddRectFilled(ImVec2(x1 - ot, y1 - ot), ImVec2(x2 + ot, y2 + ot), outlineColor);
		}
		drawList->AddRectFilled(ImVec2(x1, y1), ImVec2(x2, y2), color);
		};

	if (cfg.style == Crosshair_Cross || cfg.style == Crosshair_T_Style || cfg.style == Crosshair_CrossAndDot) {
		DrawBar(cx - cfg.gap - cfg.size, cy - halfThick, cx - cfg.gap, cy + halfThick);
		DrawBar(cx + cfg.gap, cy - halfThick, cx + cfg.gap + cfg.size, cy + halfThick);
		DrawBar(cx - halfThick, cy + cfg.gap, cx + halfThick, cy + cfg.gap + cfg.size);
		if (cfg.style != Crosshair_T_Style) {
			DrawBar(cx - halfThick, cy - cfg.gap - cfg.size, cx + halfThick, cy - cfg.gap);
		}
	}
	else if (cfg.style == Crosshair_Circle || cfg.style == Crosshair_CircleAndDot) {
		float radius = cfg.gap + cfg.size;
		if (cfg.outline) {
			drawList->AddCircle(ImVec2(cx, cy), radius, outlineColor, 0, cfg.thickness + 2.0f * ot);
		}
		drawList->AddCircle(ImVec2(cx, cy), radius, color, 0, cfg.thickness);
	}

	bool needDot = cfg.centerDot || cfg.style == Crosshair_Dot || cfg.style == Crosshair_CrossAndDot || cfg.style == Crosshair_CircleAndDot;
	if (needDot) {
		if (cfg.outline) {
			drawList->AddCircleFilled(ImVec2(cx, cy), cfg.dotRadius + ot, outlineColor);
		}
		drawList->AddCircleFilled(ImVec2(cx, cy), cfg.dotRadius, dotColor);
	}
}

void RenderCrosshairPreview(const CrosshairConfig& cfg)
{
	ImGui::Text("Crosshair Live Preview:");
	ImDrawList* drawList = ImGui::GetWindowDrawList();
	ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
	ImVec2 canvas_size = ImVec2(ImGui::GetContentRegionAvail().x, 110.0f);
	ImVec2 canvas_end = ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y);

	drawList->AddRectFilled(canvas_pos, canvas_end, IM_COL32(18, 20, 26, 255), 6.0f);
	drawList->AddRect(canvas_pos, canvas_end, IM_COL32(50, 54, 68, 255), 6.0f);

	Vector2 previewCenter(canvas_pos.x + canvas_size.x / 2.0f, canvas_pos.y + canvas_size.y / 2.0f);
	DrawCrosshairAt(drawList, previewCenter, cfg);

	ImGui::Dummy(canvas_size);
}

// =====================================================================
//  Key Name Helper for Toggle Key Binding
// =====================================================================
std::string GetKeyName(int vk) {
	switch (vk) {
	case VK_INSERT:   return "INSERT";
	case VK_DELETE:   return "DELETE";
	case VK_HOME:     return "HOME";
	case VK_END:      return "END";
	case VK_PRIOR:    return "PAGE UP";
	case VK_NEXT:     return "PAGE DOWN";
	case VK_RSHIFT:   return "RIGHT SHIFT";
	case VK_LSHIFT:   return "LEFT SHIFT";
	case VK_RCONTROL: return "RIGHT CTRL";
	case VK_LCONTROL: return "LEFT CTRL";
	case VK_RMENU:    return "RIGHT ALT";
	case VK_LMENU:    return "LEFT ALT";
	case VK_TAB:      return "TAB";
	case VK_CAPITAL:  return "CAPS LOCK";
	case VK_ESCAPE:   return "ESCAPE";
	case VK_SPACE:    return "SPACE";
	case VK_OEM_3:    return "TILDE (~)";
	case VK_F1:       return "F1";
	case VK_F2:       return "F2";
	case VK_F3:       return "F3";
	case VK_F4:       return "F4";
	case VK_F5:       return "F5";
	case VK_F6:       return "F6";
	case VK_F7:       return "F7";
	case VK_F8:       return "F8";
	case VK_F9:       return "F9";
	case VK_F10:      return "F10";
	case VK_F11:      return "F11";
	case VK_F12:      return "F12";
	default: {
		UINT scanCode = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
		char name[64] = { 0 };
		int len = GetKeyNameTextA(scanCode << 16, name, sizeof(name));
		if (len > 0) return std::string(name);
		return "Key 0x" + std::to_string(vk);
	}
	}
}

// =====================================================================
//  VOLD LINE Palette & Modern Menu Theme
// =====================================================================
namespace VoldLineColors {
	constexpr ImVec4 LimeAccent = ImVec4(0.831f, 0.996f, 0.251f, 1.000f);
	constexpr ImVec4 LimeHover = ImVec4(0.890f, 1.000f, 0.400f, 1.000f);
	constexpr ImVec4 LimeActive = ImVec4(0.750f, 0.920f, 0.180f, 1.000f);
	constexpr ImVec4 LimeMuted = ImVec4(0.831f, 0.996f, 0.251f, 0.250f);

	constexpr ImVec4 OffWhiteText = ImVec4(0.937f, 0.937f, 0.910f, 1.000f);
	constexpr ImVec4 TextMuted = ImVec4(0.680f, 0.690f, 0.650f, 1.000f);

	constexpr ImVec4 DarkBase = ImVec4(0.075f, 0.078f, 0.067f, 0.960f); // #131411
	constexpr ImVec4 DarkSurface = ImVec4(0.110f, 0.114f, 0.098f, 1.000f);
	constexpr ImVec4 DarkSurfaceElev = ImVec4(0.145f, 0.150f, 0.130f, 1.000f);
	constexpr ImVec4 DarkBorder = ImVec4(0.220f, 0.230f, 0.190f, 0.900f);
}

void ApplyMenuTheme()
{
	static bool themeApplied = false;
	if (themeApplied) return;
	themeApplied = true;

	ImGuiStyle& style = ImGui::GetStyle();
	ImVec4* colors = style.Colors;

	style.WindowRounding = 8.0f;
	style.ChildRounding = 6.0f;
	style.FrameRounding = 5.0f;
	style.PopupRounding = 6.0f;
	style.ScrollbarRounding = 6.0f;
	style.GrabRounding = 5.0f;
	style.TabRounding = 6.0f;

	style.WindowPadding = ImVec2(14, 14);
	style.FramePadding = ImVec2(10, 6);
	style.ItemSpacing = ImVec2(10, 8);
	style.ItemInnerSpacing = ImVec2(8, 5);

	colors[ImGuiCol_Text] = VoldLineColors::OffWhiteText;
	colors[ImGuiCol_TextDisabled] = VoldLineColors::TextMuted;
	colors[ImGuiCol_WindowBg] = VoldLineColors::DarkBase;
	colors[ImGuiCol_ChildBg] = VoldLineColors::DarkSurface;
	colors[ImGuiCol_PopupBg] = VoldLineColors::DarkSurface;
	colors[ImGuiCol_Border] = VoldLineColors::DarkBorder;
	colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

	colors[ImGuiCol_FrameBg] = VoldLineColors::DarkSurface;
	colors[ImGuiCol_FrameBgHovered] = VoldLineColors::DarkSurfaceElev;
	colors[ImGuiCol_FrameBgActive] = VoldLineColors::LimeMuted;

	colors[ImGuiCol_TitleBg] = VoldLineColors::DarkSurface;
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.110f, 0.114f, 0.098f, 1.0f);
	colors[ImGuiCol_TitleBgCollapsed] = VoldLineColors::DarkBase;
	colors[ImGuiCol_MenuBarBg] = VoldLineColors::DarkSurface;

	colors[ImGuiCol_ScrollbarBg] = VoldLineColors::DarkBase;
	colors[ImGuiCol_ScrollbarGrab] = VoldLineColors::DarkSurfaceElev;
	colors[ImGuiCol_ScrollbarGrabHovered] = VoldLineColors::LimeMuted;
	colors[ImGuiCol_ScrollbarGrabActive] = VoldLineColors::LimeAccent;

	colors[ImGuiCol_CheckMark] = VoldLineColors::LimeAccent;
	colors[ImGuiCol_SliderGrab] = VoldLineColors::LimeAccent;
	colors[ImGuiCol_SliderGrabActive] = VoldLineColors::LimeHover;

	colors[ImGuiCol_Button] = VoldLineColors::DarkSurfaceElev;
	colors[ImGuiCol_ButtonHovered] = VoldLineColors::LimeAccent;
	colors[ImGuiCol_ButtonActive] = VoldLineColors::LimeActive;

	colors[ImGuiCol_Header] = VoldLineColors::DarkSurfaceElev;
	colors[ImGuiCol_HeaderHovered] = VoldLineColors::LimeMuted;
	colors[ImGuiCol_HeaderActive] = VoldLineColors::LimeAccent;

	colors[ImGuiCol_Separator] = VoldLineColors::DarkBorder;
	colors[ImGuiCol_SeparatorHovered] = VoldLineColors::LimeAccent;
	colors[ImGuiCol_SeparatorActive] = VoldLineColors::LimeAccent;

	colors[ImGuiCol_ResizeGrip] = VoldLineColors::DarkBorder;
	colors[ImGuiCol_ResizeGripHovered] = VoldLineColors::LimeHover;
	colors[ImGuiCol_ResizeGripActive] = VoldLineColors::LimeAccent;

	colors[ImGuiCol_Tab] = VoldLineColors::DarkSurface;
	colors[ImGuiCol_TabHovered] = VoldLineColors::LimeMuted;
	colors[ImGuiCol_TabActive] = ImVec4(0.200f, 0.235f, 0.080f, 1.0f);
	colors[ImGuiCol_TabUnfocused] = VoldLineColors::DarkBase;
	colors[ImGuiCol_TabUnfocusedActive] = VoldLineColors::DarkSurface;
}

// =====================================================================
//  Interactive Settings Menu
// =====================================================================
void RenderMenu()
{
	ApplyMenuTheme();

	ImGui::SetNextWindowSize(ImVec2(580, 540), ImGuiCond_FirstUseEver);
	bool menuOpen = OverlayInternal::g_ShowMenu;

	std::string title = "VOLD LINE | CS2  -  Toggle: [" + GetKeyName(OverlayInternal::g_MenuToggleKey) + "]###MainWindow";

	if (ImGui::Begin(title.c_str(), &menuOpen, ImGuiWindowFlags_NoCollapse))
	{
		bool isAttached = mem.IsAttached() && module_base != 0;
		if (isAttached) {
			ImGui::TextColored(VoldLineColors::LimeAccent, "[Connected] Attached to cs2.exe (client.dll: 0x%llX)", (unsigned long long)module_base);
		}
		else {
			ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.1f, 1.0f), "[Standby] Waiting for cs2.exe... (Overlay is active)");
		}
		ImGui::Separator();

		if (ImGui::BeginTabBar("OverlayTabs"))
		{
			// TAB 1: AIMBOT
			if (ImGui::BeginTabItem("Aimbot"))
			{
				ImGui::Spacing();
				ImGui::Checkbox("Enable Aimbot", &g_Config.aimbot.enabled);
				ImGui::Separator();

				if (g_Config.aimbot.enabled) {
					ImGui::Checkbox("Show Field Of View (FOV Circle)", &g_Config.aimbot.showFov);
					ImGui::Checkbox("Show Target Snap Line", &g_Config.aimbot.showTargetLine);
					ImGui::Checkbox("Require Left Mouse Button (Hold to Aim)", &g_Config.aimbot.requireKey);

					ImGui::Spacing();
					ImGui::SliderFloat("Field Of View Radius", &g_Config.aimbot.fovRadius, 20.0f, 300.0f, "%.0f px");
					ImGui::SliderFloat("Aim Smoothing", &g_Config.aimbot.smoothing, 1.0f, 20.0f, "%.1f");
					ImGui::TextDisabled("Note: Higher smoothing values yield slower, more human-like aiming curves.");
				}

				ImGui::EndTabItem();
			}

			// TAB 2: VISUALS (ESP)
			if (ImGui::BeginTabItem("Visuals (ESP)"))
			{
				ImGui::Spacing();
				ImGui::Checkbox("Master ESP Switch", &g_Config.esp.enabled);
				ImGui::SameLine(220);
				ImGui::Checkbox("Enemies Only (Team Check)", &g_Config.esp.enemyOnly);

				ImGui::Separator();
				ImGui::TextDisabled("--- Player Boxes & Bars ---");
				ImGui::Checkbox("Draw Player Box", &g_Config.esp.box);
				if (g_Config.esp.box) {
					ImGui::Indent();
					const char* boxStyles[] = { "2D Full Box", "Corner Box" };
					ImGui::Combo("Box Style", &g_Config.esp.boxStyle, boxStyles, IM_ARRAYSIZE(boxStyles));
					ImGui::SliderFloat("Box Thickness", &g_Config.esp.boxThickness, 1.0f, 4.0f, "%.1f px");
					ImGui::Checkbox("Box Outline", &g_Config.esp.boxOutline);
					ImGui::SameLine();
					ImGui::Checkbox("Health-based Color##Box", &g_Config.esp.boxHealthColor);
					if (!g_Config.esp.boxHealthColor) {
						ImGui::ColorEdit4("Box Color", g_Config.esp.boxColor, ImGuiColorEditFlags_NoInputs);
					}
					ImGui::Checkbox("Fill Box", &g_Config.esp.boxFill);
					if (g_Config.esp.boxFill) {
						ImGui::SliderFloat("Fill Transparency", &g_Config.esp.boxFillAlpha, 0.05f, 0.6f, "%.2f");
					}
					ImGui::Unindent();
				}

				ImGui::Checkbox("Health Bar", &g_Config.esp.healthBar);
				if (g_Config.esp.healthBar) {
					ImGui::Indent();
					ImGui::SliderFloat("Bar Width", &g_Config.esp.healthBarWidth, 1.0f, 6.0f, "%.1f px");
					ImGui::Unindent();
				}

				ImGui::Separator();
				ImGui::TextDisabled("--- Head & Health Indicators ---");
				ImGui::Checkbox("HP Dot Above Head", &g_Config.esp.hpDot);
				if (g_Config.esp.hpDot) {
					ImGui::Indent();
					ImGui::SliderFloat("Dot Radius", &g_Config.esp.hpDotRadius, 2.0f, 8.0f, "%.1f px");
					ImGui::Checkbox("Dot Outline", &g_Config.esp.hpDotOutline);
					ImGui::Unindent();
				}

				ImGui::Checkbox("HP Text", &g_Config.esp.hpText);
				if (g_Config.esp.hpText) {
					ImGui::Indent();
					const char* hpFormats[] = { "\"100 HP\"", "\"100\"", "\"[100%]\"" };
					ImGui::Combo("Text Format", &g_Config.esp.hpTextFormat, hpFormats, IM_ARRAYSIZE(hpFormats));
					ImGui::Unindent();
				}

				ImGui::Checkbox("Head Circle", &g_Config.esp.headCircle);
				if (g_Config.esp.headCircle) {
					ImGui::Indent();
					ImGui::SliderFloat("Circle Radius", &g_Config.esp.headCircleRadius, 2.0f, 10.0f, "%.1f px");
					ImGui::ColorEdit4("Circle Color", g_Config.esp.headCircleColor, ImGuiColorEditFlags_NoInputs);
					ImGui::Unindent();
				}

				ImGui::Separator();
				ImGui::TextDisabled("--- Tracers & Distance ---");
				ImGui::Checkbox("Snaplines (Tracers)", &g_Config.esp.snaplines);
				if (g_Config.esp.snaplines) {
					ImGui::Indent();
					const char* origins[] = { "Bottom of Screen", "Center of Screen", "Top of Screen" };
					ImGui::Combo("Line Origin", &g_Config.esp.snaplineOrigin, origins, IM_ARRAYSIZE(origins));
					ImGui::SliderFloat("Line Thickness", &g_Config.esp.snaplineThickness, 1.0f, 4.0f, "%.1f px");
					ImGui::ColorEdit4("Line Color", g_Config.esp.snaplineColor, ImGuiColorEditFlags_NoInputs);
					ImGui::Unindent();
				}

				ImGui::Checkbox("Show Distance", &g_Config.esp.distance);

				ImGui::EndTabItem();
			}

			// TAB 3: CUSTOMIZABLE CROSSHAIR
			if (ImGui::BeginTabItem("Crosshair"))
			{
				ImGui::Spacing();
				ImGui::Checkbox("Enable Crosshair", &g_Config.crosshair.enabled);

				if (g_Config.crosshair.enabled) {
					RenderCrosshairPreview(g_Config.crosshair);
					ImGui::Spacing();

					const char* styles[] = { "Classic Cross (+)", "T-Style", "Circle", "Dot Only", "Cross + Dot", "Circle + Dot" };
					ImGui::Combo("Style", &g_Config.crosshair.style, styles, IM_ARRAYSIZE(styles));

					ImGui::ColorEdit4("Crosshair Color", g_Config.crosshair.color, ImGuiColorEditFlags_AlphaBar);

					if (g_Config.crosshair.style != Crosshair_Dot) {
						ImGui::SliderFloat("Arm Length / Size", &g_Config.crosshair.size, 2.0f, 30.0f, "%.1f px");
						ImGui::SliderFloat("Line Thickness", &g_Config.crosshair.thickness, 1.0f, 8.0f, "%.1f px");
						ImGui::SliderFloat("Center Gap", &g_Config.crosshair.gap, 0.0f, 25.0f, "%.1f px");
					}

					ImGui::Separator();
					ImGui::Checkbox("Black Outline", &g_Config.crosshair.outline);
					if (g_Config.crosshair.outline) {
						ImGui::Indent();
						ImGui::SliderFloat("Outline Thickness", &g_Config.crosshair.outlineThickness, 1.0f, 3.0f, "%.1f px");
						ImGui::Unindent();
					}

					ImGui::Checkbox("Center Dot", &g_Config.crosshair.centerDot);
					if (g_Config.crosshair.centerDot || g_Config.crosshair.style == Crosshair_Dot || g_Config.crosshair.style == Crosshair_CrossAndDot || g_Config.crosshair.style == Crosshair_CircleAndDot) {
						ImGui::Indent();
						ImGui::SliderFloat("Dot Radius", &g_Config.crosshair.dotRadius, 1.0f, 6.0f, "%.1f px");
						ImGui::Checkbox("Custom Dot Color", &g_Config.crosshair.separateDotColor);
						if (g_Config.crosshair.separateDotColor) {
							ImGui::ColorEdit4("Dot Color", g_Config.crosshair.dotColor, ImGuiColorEditFlags_AlphaBar);
						}
						ImGui::Unindent();
					}

					ImGui::Separator();
					ImGui::Checkbox("Lock to Screen Center", &g_Config.crosshair.lockToCenter);
					if (!g_Config.crosshair.lockToCenter) {
						ImGui::Indent();
						ImGui::SliderFloat("Offset X", &g_Config.crosshair.offsetX, -100.0f, 100.0f, "%.0f px");
						ImGui::SliderFloat("Offset Y", &g_Config.crosshair.offsetY, -100.0f, 100.0f, "%.0f px");
						if (ImGui::Button("Reset Offsets")) {
							g_Config.crosshair.offsetX = 0.0f;
							g_Config.crosshair.offsetY = 0.0f;
						}
						ImGui::Unindent();
					}
				}

				ImGui::EndTabItem();
			}

			// TAB 4: SETTINGS & MENU HOTKEY
			if (ImGui::BeginTabItem("Settings / Menu"))
			{
				ImGui::Spacing();
				ImGui::Text("Menu & Overlay Hotkey Configuration");
				ImGui::Separator();

				ImGui::Text("Current Menu Toggle Key: ");
				ImGui::SameLine();
				ImGui::TextColored(VoldLineColors::LimeAccent, "[ %s ]", GetKeyName(OverlayInternal::g_MenuToggleKey).c_str());

				static bool isBindingKey = false;
				if (isBindingKey) {
					OverlayInternal::g_IsBindingKey = true;
					ImGui::Button("[ Press any key on keyboard... ]", ImVec2(230, 30));

					for (int k = 1; k < 255; k++) {
						if (k == VK_LBUTTON || k == VK_RBUTTON || k == VK_MBUTTON) continue;
						if ((GetAsyncKeyState(k) & 0x8000) != 0) {
							if (k != VK_ESCAPE) {
								OverlayInternal::g_MenuToggleKey = k;
							}
							isBindingKey = false;
							OverlayInternal::g_IsBindingKey = false;
							std::this_thread::sleep_for(std::chrono::milliseconds(150));
							break;
						}
					}
				}
				else {
					OverlayInternal::g_IsBindingKey = false;
					if (ImGui::Button("Click to Bind New Key", ImVec2(230, 30))) {
						isBindingKey = true;
					}
				}

				ImGui::SameLine();
				const char* keyPresetNames[] = { "INSERT", "DELETE", "HOME", "PAGE UP", "PAGE DOWN", "RIGHT SHIFT", "F1", "F2", "F3", "F5", "F11", "TILDE (~)" };
				const int keyPresetValues[] = { VK_INSERT, VK_DELETE, VK_HOME, VK_PRIOR, VK_NEXT, VK_RSHIFT, VK_F1, VK_F2, VK_F3, VK_F5, VK_F11, VK_OEM_3 };
				static int selectedPreset = 0;
				ImGui::SetNextItemWidth(130);
				if (ImGui::Combo("Preset Keys", &selectedPreset, keyPresetNames, IM_ARRAYSIZE(keyPresetNames))) {
					OverlayInternal::g_MenuToggleKey = keyPresetValues[selectedPreset];
				}

				ImGui::Separator();
				ImGui::Checkbox("Show Watermark (Top-Left)", &g_Config.general.watermark);
				ImGui::Checkbox("Show Overlay FPS", &g_Config.general.showFPS);

				ImGui::Spacing();
				ImGui::Separator();
				if (ImGui::Button("Close Menu", ImVec2(120, 28))) {
					OverlayInternal::SetMenuState(false);
				}
				ImGui::SameLine();
				if (ImGui::Button("Exit Overlay", ImVec2(120, 28))) {
					OverlayInternal::g_ShouldExit = true;
				}

				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}

		ImGui::End();
	}

	if (!menuOpen) {
		OverlayInternal::SetMenuState(false);
	}
}

// =====================================================================
//  Watermark Drawing Helper
// =====================================================================
void DrawWatermark()
{
	std::string text = "VOLD LINE | [" + GetKeyName(OverlayInternal::g_MenuToggleKey) + "] Menu";
	if (g_Config.general.showFPS) {
		float fps = ImGui::GetIO().Framerate;
		text += " | " + std::to_string((int)fps) + " FPS";
	}
	ImU32 limeCol = ImGui::ColorConvertFloat4ToU32(VoldLineColors::LimeAccent);
	DrawOutlinedString(Vector2(14, 14), text.c_str(), limeCol, Colors::Black);
}

// =====================================================================
//  Main Render Loop
// =====================================================================
void MyDrawings()
{
	static auto lastAttachAttempt = std::chrono::steady_clock::now();
	if (!mem.IsAttached() || module_base == 0) {
		auto now = std::chrono::steady_clock::now();
		if (std::chrono::duration_cast<std::chrono::seconds>(now - lastAttachAttempt).count() >= 1) {
			lastAttachAttempt = now;
			if (mem.Attach(L"cs2.exe")) {
				module_base = mem.GetModuleBaseAddress(L"client.dll");
			}
		}
	}

	if (g_Config.general.watermark)
		DrawWatermark();

	if (g_Config.crosshair.enabled) {
		Vector2 screen = GetScreenSize();
		Vector2 center(screen.x / 2.0f, screen.y / 2.0f);
		if (!g_Config.crosshair.lockToCenter) {
			center.x += g_Config.crosshair.offsetX;
			center.y += g_Config.crosshair.offsetY;
		}
		DrawCrosshairAt(ImGui::GetForegroundDrawList(), center, g_Config.crosshair);
	}

	if (!mem.IsAttached() || module_base == 0)
		return;

	uintptr_t localPlayer = mem.Read<uintptr_t>(module_base + Offsets::dwLocalPlayerPawn);
	uint8_t localTeam = localPlayer ? GetPlayerTeam(localPlayer) : 0;

	// Execute Aimbot Logic
	RunAimbot(localPlayer, localTeam);

	if (!g_Config.esp.enabled)
		return;

	Vector2 screenSize = GetScreenSize();

	for (uint32_t i = 0; i < 64; i++)
	{
		uintptr_t player = GetPlayer(i);

		if (!player || player == localPlayer || !IsPlayerAlive(player))
			continue;

		if (g_Config.esp.enemyOnly && localTeam != 0 && GetPlayerTeam(player) == localTeam)
			continue;

		Vector3 feetPos = GetLocationOfPlayer(player);
		Vector3 viewOffset = mem.Read<Vector3>(player + Offsets::m_vecViewOffset);
		float playerHeight = (viewOffset.z > 10.0f && viewOffset.z < 100.0f) ? (viewOffset.z + 8.0f) : 72.0f;

		Vector3 headPos = feetPos;
		headPos.z += playerHeight;

		Vector2 screenFeet{};
		Vector2 screenHead{};
		if (!WorldToScreen(feetPos, screenFeet) || !WorldToScreen(headPos, screenHead))
			continue;

		float boxHeight = screenFeet.y - screenHead.y;
		if (boxHeight < 2.0f) continue;
		float boxWidth = boxHeight / 2.2f;
		Vector2 topLeft(screenHead.x - boxWidth / 2.0f, screenHead.y);

		int health = GetPlayerHealth(player);
		ImU32 hpColor = (health > 50) ? Colors::Green : (health > 25 ? Colors::Yellow : Colors::Red);

		if (g_Config.esp.snaplines) {
			Vector2 snapOrigin;
			if (g_Config.esp.snaplineOrigin == 0)
				snapOrigin = Vector2(screenSize.x / 2.0f, screenSize.y);
			else if (g_Config.esp.snaplineOrigin == 1)
				snapOrigin = Vector2(screenSize.x / 2.0f, screenSize.y / 2.0f);
			else
				snapOrigin = Vector2(screenSize.x / 2.0f, 0.0f);

			ImU32 snapColor = ImGui::ColorConvertFloat4ToU32(*(ImVec4*)g_Config.esp.snaplineColor);
			DrawLine(snapOrigin, screenFeet, snapColor, g_Config.esp.snaplineThickness);
		}

		if (g_Config.esp.box) {
			ImU32 boxColor = g_Config.esp.boxHealthColor ? hpColor : ImGui::ColorConvertFloat4ToU32(*(ImVec4*)g_Config.esp.boxColor);
			if (g_Config.esp.boxFill) {
				DrawFilledBox(topLeft, boxWidth, boxHeight, IM_COL32(0, 0, 0, (int)(g_Config.esp.boxFillAlpha * 255.0f)));
			}
			if (g_Config.esp.boxStyle == 0) {
				if (g_Config.esp.boxOutline)
					DrawOutlinedBox(topLeft, boxWidth, boxHeight, boxColor, g_Config.esp.boxThickness);
				else
					DrawBox(topLeft, boxWidth, boxHeight, boxColor, g_Config.esp.boxThickness);
			}
			else {
				DrawCornerBox(topLeft, boxWidth, boxHeight, boxColor, g_Config.esp.boxThickness);
			}
		}

		if (g_Config.esp.healthBar) {
			float barWidth = g_Config.esp.healthBarWidth;
			float barMargin = 4.0f;
			float barX = topLeft.x - barMargin - barWidth;
			float barY = topLeft.y;
			float barH = boxHeight;

			DrawFilledBox(Vector2(barX - 1, barY - 1), barWidth + 2, barH + 2, IM_COL32(0, 0, 0, 180));

			float clampedHp = (float)std::clamp(health, 0, 100);
			float hpHeight = barH * (clampedHp / 100.0f);
			float hpY = (barY + barH) - hpHeight;
			DrawFilledBox(Vector2(barX, hpY), barWidth, hpHeight, hpColor);
		}

		if (g_Config.esp.headCircle) {
			ImU32 headCol = ImGui::ColorConvertFloat4ToU32(*(ImVec4*)g_Config.esp.headCircleColor);
			DrawCircle(Vector2(screenHead.x, screenHead.y + g_Config.esp.headCircleRadius), g_Config.esp.headCircleRadius, headCol, 1.5f);
		}

		if (g_Config.esp.hpDot) {
			Vector2 dotPos(screenHead.x, screenHead.y - 8.0f);
			DrawFilledCircle(dotPos, g_Config.esp.hpDotRadius, hpColor);
			if (g_Config.esp.hpDotOutline)
				ImGui::GetBackgroundDrawList()->AddCircle(ImVec2(dotPos.x, dotPos.y), g_Config.esp.hpDotRadius + 0.5f, Colors::Black, 0, 1.0f);
		}

		if (g_Config.esp.hpText) {
			std::string hpText;
			if (g_Config.esp.hpTextFormat == 0) hpText = std::to_string(health) + " HP";
			else if (g_Config.esp.hpTextFormat == 1) hpText = std::to_string(health);
			else hpText = "[" + std::to_string(health) + "%]";

			Vector2 textPos;
			if (g_Config.esp.hpDot)
				textPos = Vector2(screenHead.x + g_Config.esp.hpDotRadius + 4.0f, screenHead.y - 12.0f);
			else
				textPos = Vector2(topLeft.x + boxWidth + 4.0f, topLeft.y);

			DrawOutlinedString(textPos, hpText.c_str(), hpColor);
		}

		if (g_Config.esp.distance && localPlayer) {
			Vector3 localPos = mem.Read<Vector3>(localPlayer + Offsets::m_vOldOrigin);
			float distUnits = feetPos.Distance(localPos);
			int distMeters = (int)(distUnits * 0.0254f);
			std::string distText = std::to_string(distMeters) + "m";
			Vector2 distPos(screenFeet.x - (distText.length() * 3.5f), screenFeet.y + 4.0f);
			DrawOutlinedString(distPos, distText.c_str(), Colors::White);
		}
	}
}

// =====================================================================
//  Entry Point
// =====================================================================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	SetCurrentProcessExplicitAppUserModelID(L"AIDevForge.VoldLine.CS2");


	std::cout << "===========================================" << std::endl;
	std::cout << "                 VOLD LINE                 " << std::endl;
	std::cout << "        Counter-Strike 2 ESP & Aimbot      " << std::endl;
	std::cout << "===========================================" << std::endl;
	std::cout << "Controls:" << std::endl;
	std::cout << "  INSERT = Toggle Feature Menu" << std::endl;
	std::cout << "  END    = Exit" << std::endl;
	std::cout << "===========================================" << std::endl;

	AutoUpdater::CheckForUpdates();

	if (!mem.Attach(L"cs2.exe")) {
		std::wcout << L"[!] CS2 is not currently running." << std::endl;
		std::wcout << L"[*] Starting VOLD LINE overlay. Auto-connecting when CS2 launches." << std::endl;
	}
	else {
		std::wcout << L"[+] Successfully connected to CS2!" << std::endl;
	}

	RunOverlay(MyDrawings, RenderMenu);
	return 0;
}