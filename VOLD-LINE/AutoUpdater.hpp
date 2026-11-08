#pragma once
#include <windows.h>
#include <wininet.h>
#include <string>
#include <iostream>
#include <fstream>
#include <thread>
#include <regex>
#include <vector>
#include <sstream>

#pragma comment(lib, "wininet.lib")

namespace AutoUpdater {
	// Current application version (Semantic versioning: e.g. "1.0.1")
	inline const std::string CURRENT_VERSION = "1.0.0";
	
	// Target GitHub repository: "AI-Dev-Forge/VOLDLINE"
	inline const std::string GITHUB_REPO = "AI-Dev-Forge/VOLDLINE";

	// HTTP GET request using WinINet with custom User-Agent
	inline std::string FetchUrl(const std::string& url) {
		std::string response;
		HINTERNET hInternet = InternetOpenA("VOLDLINE-AutoUpdater", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
		if (!hInternet) return "";

		// INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_CACHE_WRITE
		DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_CACHE_WRITE;
		HINTERNET hUrl = InternetOpenUrlA(hInternet, url.c_str(), "Accept: application/vnd.github.v3+json\r\n", -1, flags, 0);
		if (!hUrl) {
			InternetCloseHandle(hInternet);
			return "";
		}

		char buffer[4096];
		DWORD bytesRead = 0;
		while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
			response.append(buffer, bytesRead);
		}

		InternetCloseHandle(hUrl);
		InternetCloseHandle(hInternet);
		return response;
	}

	// Download binary file to disk
	inline bool DownloadFile(const std::string& url, const std::wstring& destinationPath) {
		HINTERNET hInternet = InternetOpenA("VOLDLINE-AutoUpdater", INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
		if (!hInternet) return false;

		DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_SECURE | INTERNET_FLAG_NO_CACHE_WRITE;
		HINTERNET hUrl = InternetOpenUrlA(hInternet, url.c_str(), NULL, 0, flags, 0);
		if (!hUrl) {
			InternetCloseHandle(hInternet);
			return false;
		}

		std::ofstream outFile(destinationPath, std::ios::binary);
		if (!outFile.is_open()) {
			InternetCloseHandle(hUrl);
			InternetCloseHandle(hInternet);
			return false;
		}

		char buffer[8192];
		DWORD bytesRead = 0;
		while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
			outFile.write(buffer, bytesRead);
		}

		outFile.close();
		InternetCloseHandle(hUrl);
		InternetCloseHandle(hInternet);
		return true;
	}

	// Get current executable path
	inline std::wstring GetCurrentExecutablePath() {
		wchar_t path[MAX_PATH];
		GetModuleFileNameW(NULL, path, MAX_PATH);
		return std::wstring(path);
	}

	// Check if remote version string is strictly newer than current local version (Semantic versioning: e.g. "1.0.3" > "1.0.2")
	inline bool IsVersionNewer(std::string remote, std::string current) {
		auto parseVersion = [](std::string v) -> std::vector<int> {
			if (!v.empty() && (v[0] == 'v' || v[0] == 'V')) {
				v = v.substr(1);
			}
			std::vector<int> parts;
			std::stringstream ss(v);
			std::string token;
			while (std::getline(ss, token, '.')) {
				try {
					parts.push_back(std::stoi(token));
				} catch (...) {
					parts.push_back(0);
				}
			}
			while (parts.size() < 3) parts.push_back(0);
			return parts;
		};

		std::vector<int> remoteParts = parseVersion(remote);
		std::vector<int> currentParts = parseVersion(current);

		for (size_t i = 0; i < remoteParts.size() && i < currentParts.size(); ++i) {
			if (remoteParts[i] > currentParts[i]) return true;
			if (remoteParts[i] < currentParts[i]) return false;
		}
		return remoteParts.size() > currentParts.size();
	}

	// Helper to launch updater batch and exit
	inline void ApplyUpdateAndRestart(const std::wstring& newExePath) {
		std::wstring currentExe = GetCurrentExecutablePath();
		std::wstring batPath = currentExe + L".update.bat";

		// Generate batch script to replace current exe when it exits
		std::wofstream bat(batPath);
		if (bat.is_open()) {
			bat << L"@echo off\n";
			bat << L"timeout /t 1 /nobreak > nul\n";
			bat << L":loop\n";
			bat << L"del \"" << currentExe << L"\" >nul 2>&1\n";
			bat << L"if exist \"" << currentExe << L"\" goto loop\n";
			bat << L"move \"" << newExePath << L"\" \"" << currentExe << L"\" >nul 2>&1\n";
			bat << L"start \"\" \"" << currentExe << L"\"\n";
			bat << L"del \"%~f0\" >nul 2>&1\n";
			bat << L"exit\n";
			bat.close();

			// Run batch invisibly
			STARTUPINFOW si = { sizeof(si) };
			PROCESS_INFORMATION pi;
			si.dwFlags = STARTF_USESHOWWINDOW;
			si.wShowWindow = SW_HIDE;

			std::wstring cmd = L"cmd.exe /C \"" + batPath + L"\"";
			if (CreateProcessW(NULL, &cmd[0], NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
				ExitProcess(0);
			}
		}
	}

	// Main update check function (can run synchronously or in a thread)
	inline void CheckForUpdates() {
		std::cout << "[*] Checking for updates (Current: v" << CURRENT_VERSION << ")..." << std::endl;

		std::string apiUrl = "https://api.github.com/repos/" + GITHUB_REPO + "/releases/latest";
		std::string json = FetchUrl(apiUrl);

		if (json.empty()) {
			std::cout << "[*] No response from update server or offline." << std::endl;
			return;
		}

		// Simple regex extraction for tag_name: "tag_name": "v1.0.1"
		std::regex tagRegex("\"tag_name\"\\s*:\\s*\"([^\"]+)\"");
		std::smatch match;
		std::string latestTag;
		if (std::regex_search(json, match, tagRegex) && match.size() > 1) {
			latestTag = match[1].str();
		}

		if (latestTag.empty() || !IsVersionNewer(latestTag, CURRENT_VERSION)) {
			std::cout << "[+] You are running the latest version!" << std::endl;
			return;
		}

		std::cout << "[!] New version available: " << latestTag << std::endl;

		// Extract browser_download_url for .exe
		std::regex downloadRegex("\"browser_download_url\"\\s*:\\s*\"([^\"]+\\.exe)\"");
		std::string downloadUrl;
		if (std::regex_search(json, match, downloadRegex) && match.size() > 1) {
			downloadUrl = match[1].str();
		}

		if (downloadUrl.empty()) {
			std::cout << "[!] Found release " << latestTag << " but no .exe asset was found in release." << std::endl;
			return;
		}

		// Prompt user
		std::wstring message = L"A new version of VOLD LINE (" + std::wstring(latestTag.begin(), latestTag.end()) +
			L") is available!\n\nYour current version: v" + std::wstring(CURRENT_VERSION.begin(), CURRENT_VERSION.end()) +
			L"\n\nDo you want to update now?";

		int result = MessageBoxW(NULL, message.c_str(), L"VOLD LINE - Update Available", MB_YESNO | MB_ICONINFORMATION | MB_TOPMOST);
		if (result == IDYES) {
			std::cout << "[*] Downloading update..." << std::endl;
			std::wstring currentExe = GetCurrentExecutablePath();
			std::wstring newExePath = currentExe + L".new";

			if (DownloadFile(downloadUrl, newExePath)) {
				std::cout << "[+] Download complete! Restarting to apply update..." << std::endl;
				ApplyUpdateAndRestart(newExePath);
			} else {
				MessageBoxW(NULL, L"Failed to download the update. Please check your internet connection.", L"Update Error", MB_OK | MB_ICONERROR);
			}
		} else {
			std::cout << "[*] Update postponed by user." << std::endl;
		}
	}
}

