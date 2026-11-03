#pragma once
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <iomanip>
#include <Windows.h>

namespace ConfigManager
{
    inline std::string GetConfigPath()
    {
        char buffer[MAX_PATH];
        GetModuleFileNameA(NULL, buffer, MAX_PATH);
        std::string currentPath(buffer);
        size_t pos = currentPath.find_last_of("\\/");
        if (pos != std::string::npos) {
            return currentPath.substr(0, pos + 1) + "settings.ini";
        }
        return "settings.ini";
    }

    inline void Trim(std::string& s)
    {
        while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n'))
            s.erase(s.begin());
        while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n'))
            s.pop_back();
    }

    inline std::unordered_map<std::string, std::unordered_map<std::string, std::string>> ReadIni(const std::string& path)
    {
        std::unordered_map<std::string, std::unordered_map<std::string, std::string>> ini;
        std::ifstream file(path);
        if (!file.is_open()) return ini;

        std::string line;
        std::string currentSection = "";

        while (std::getline(file, line))
        {
            Trim(line);
            if (line.empty() || line[0] == ';' || line[0] == '#') continue;

            if (line.front() == '[' && line.back() == ']') {
                currentSection = line.substr(1, line.length() - 2);
                Trim(currentSection);
            }
            else {
                size_t eq = line.find('=');
                if (eq != std::string::npos) {
                    std::string key = line.substr(0, eq);
                    std::string val = line.substr(eq + 1);
                    Trim(key);
                    Trim(val);
                    if (!currentSection.empty()) {
                        ini[currentSection][key] = val;
                    }
                }
            }
        }
        return ini;
    }

    inline bool ParseBool(const std::string& val, bool def)
    {
        if (val == "1" || val == "true" || val == "True" || val == "TRUE") return true;
        if (val == "0" || val == "false" || val == "False" || val == "FALSE") return false;
        return def;
    }

    inline int ParseInt(const std::string& val, int def)
    {
        try {
            return std::stoi(val);
        }
        catch (...) {
            return def;
        }
    }

    inline float ParseFloat(const std::string& val, float def)
    {
        try {
            return std::stof(val);
        }
        catch (...) {
            return def;
        }
    }

    inline void ParseColor(const std::string& val, float col[4])
    {
        std::stringstream ss(val);
        float r, g, b, a;
        char c1, c2, c3;
        if (ss >> r >> c1 >> g >> c2 >> b >> c3 >> a) {
            col[0] = r; col[1] = g; col[2] = b; col[3] = a;
        }
    }

    inline std::string ColorToString(const float col[4])
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(4)
           << col[0] << "," << col[1] << "," << col[2] << "," << col[3];
        return ss.str();
    }

    inline void Load(AppConfig& cfg, int& menuToggleKey)
    {
        std::string path = GetConfigPath();
        auto ini = ReadIni(path);
        if (ini.empty()) return;

        // [General]
        if (ini.find("General") != ini.end()) {
            auto& s = ini["General"];
            if (s.count("Watermark")) cfg.general.watermark = ParseBool(s["Watermark"], cfg.general.watermark);
            if (s.count("ShowFPS")) cfg.general.showFPS = ParseBool(s["ShowFPS"], cfg.general.showFPS);
            if (s.count("MenuToggleKey")) menuToggleKey = ParseInt(s["MenuToggleKey"], menuToggleKey);
        }

        // [Aimbot]
        if (ini.find("Aimbot") != ini.end()) {
            auto& s = ini["Aimbot"];
            if (s.count("Enabled")) cfg.aimbot.enabled = ParseBool(s["Enabled"], cfg.aimbot.enabled);
            if (s.count("ShowFOV")) cfg.aimbot.showFov = ParseBool(s["ShowFOV"], cfg.aimbot.showFov);
            if (s.count("ShowTargetLine")) cfg.aimbot.showTargetLine = ParseBool(s["ShowTargetLine"], cfg.aimbot.showTargetLine);
            if (s.count("RequireKey")) cfg.aimbot.requireKey = ParseBool(s["RequireKey"], cfg.aimbot.requireKey);
            if (s.count("FOVRadius")) cfg.aimbot.fovRadius = ParseFloat(s["FOVRadius"], cfg.aimbot.fovRadius);
            if (s.count("Smoothing")) cfg.aimbot.smoothing = ParseFloat(s["Smoothing"], cfg.aimbot.smoothing);
        }

        // [ESP]
        if (ini.find("ESP") != ini.end()) {
            auto& s = ini["ESP"];
            if (s.count("Enabled")) cfg.esp.enabled = ParseBool(s["Enabled"], cfg.esp.enabled);
            if (s.count("EnemyOnly")) cfg.esp.enemyOnly = ParseBool(s["EnemyOnly"], cfg.esp.enemyOnly);
            if (s.count("HPDot")) cfg.esp.hpDot = ParseBool(s["HPDot"], cfg.esp.hpDot);
            if (s.count("HPDotRadius")) cfg.esp.hpDotRadius = ParseFloat(s["HPDotRadius"], cfg.esp.hpDotRadius);
            if (s.count("HPDotOutline")) cfg.esp.hpDotOutline = ParseBool(s["HPDotOutline"], cfg.esp.hpDotOutline);
            if (s.count("HPText")) cfg.esp.hpText = ParseBool(s["HPText"], cfg.esp.hpText);
            if (s.count("HPTextFormat")) cfg.esp.hpTextFormat = ParseInt(s["HPTextFormat"], cfg.esp.hpTextFormat);
            if (s.count("Box")) cfg.esp.box = ParseBool(s["Box"], cfg.esp.box);
            if (s.count("BoxStyle")) cfg.esp.boxStyle = ParseInt(s["BoxStyle"], cfg.esp.boxStyle);
            if (s.count("BoxThickness")) cfg.esp.boxThickness = ParseFloat(s["BoxThickness"], cfg.esp.boxThickness);
            if (s.count("BoxOutline")) cfg.esp.boxOutline = ParseBool(s["BoxOutline"], cfg.esp.boxOutline);
            if (s.count("BoxFill")) cfg.esp.boxFill = ParseBool(s["BoxFill"], cfg.esp.boxFill);
            if (s.count("BoxFillAlpha")) cfg.esp.boxFillAlpha = ParseFloat(s["BoxFillAlpha"], cfg.esp.boxFillAlpha);
            if (s.count("BoxHealthColor")) cfg.esp.boxHealthColor = ParseBool(s["BoxHealthColor"], cfg.esp.boxHealthColor);
            if (s.count("BoxColor")) ParseColor(s["BoxColor"], cfg.esp.boxColor);
            if (s.count("HealthBar")) cfg.esp.healthBar = ParseBool(s["HealthBar"], cfg.esp.healthBar);
            if (s.count("HealthBarWidth")) cfg.esp.healthBarWidth = ParseFloat(s["HealthBarWidth"], cfg.esp.healthBarWidth);
            if (s.count("Snaplines")) cfg.esp.snaplines = ParseBool(s["Snaplines"], cfg.esp.snaplines);
            if (s.count("SnaplineOrigin")) cfg.esp.snaplineOrigin = ParseInt(s["SnaplineOrigin"], cfg.esp.snaplineOrigin);
            if (s.count("SnaplineThickness")) cfg.esp.snaplineThickness = ParseFloat(s["SnaplineThickness"], cfg.esp.snaplineThickness);
            if (s.count("SnaplineColor")) ParseColor(s["SnaplineColor"], cfg.esp.snaplineColor);
            if (s.count("HeadCircle")) cfg.esp.headCircle = ParseBool(s["HeadCircle"], cfg.esp.headCircle);
            if (s.count("HeadCircleRadius")) cfg.esp.headCircleRadius = ParseFloat(s["HeadCircleRadius"], cfg.esp.headCircleRadius);
            if (s.count("HeadCircleColor")) ParseColor(s["HeadCircleColor"], cfg.esp.headCircleColor);
            if (s.count("Distance")) cfg.esp.distance = ParseBool(s["Distance"], cfg.esp.distance);
        }

        // [Crosshair]
        if (ini.find("Crosshair") != ini.end()) {
            auto& s = ini["Crosshair"];
            if (s.count("Enabled")) cfg.crosshair.enabled = ParseBool(s["Enabled"], cfg.crosshair.enabled);
            if (s.count("Style")) cfg.crosshair.style = ParseInt(s["Style"], cfg.crosshair.style);
            if (s.count("Size")) cfg.crosshair.size = ParseFloat(s["Size"], cfg.crosshair.size);
            if (s.count("Thickness")) cfg.crosshair.thickness = ParseFloat(s["Thickness"], cfg.crosshair.thickness);
            if (s.count("Gap")) cfg.crosshair.gap = ParseFloat(s["Gap"], cfg.crosshair.gap);
            if (s.count("CenterDot")) cfg.crosshair.centerDot = ParseBool(s["CenterDot"], cfg.crosshair.centerDot);
            if (s.count("DotRadius")) cfg.crosshair.dotRadius = ParseFloat(s["DotRadius"], cfg.crosshair.dotRadius);
            if (s.count("Outline")) cfg.crosshair.outline = ParseBool(s["Outline"], cfg.crosshair.outline);
            if (s.count("OutlineThickness")) cfg.crosshair.outlineThickness = ParseFloat(s["OutlineThickness"], cfg.crosshair.outlineThickness);
            if (s.count("Color")) ParseColor(s["Color"], cfg.crosshair.color);
            if (s.count("SeparateDotColor")) cfg.crosshair.separateDotColor = ParseBool(s["SeparateDotColor"], cfg.crosshair.separateDotColor);
            if (s.count("DotColor")) ParseColor(s["DotColor"], cfg.crosshair.dotColor);
            if (s.count("LockToCenter")) cfg.crosshair.lockToCenter = ParseBool(s["LockToCenter"], cfg.crosshair.lockToCenter);
            if (s.count("OffsetX")) cfg.crosshair.offsetX = ParseFloat(s["OffsetX"], cfg.crosshair.offsetX);
            if (s.count("OffsetY")) cfg.crosshair.offsetY = ParseFloat(s["OffsetY"], cfg.crosshair.offsetY);
        }
    }

    inline void Save(const AppConfig& cfg, int menuToggleKey)
    {
        std::string path = GetConfigPath();
        std::ofstream file(path);
        if (!file.is_open()) return;

        file << "# VOLD LINE Configuration File\n\n";

        // [General]
        file << "[General]\n";
        file << "Watermark=" << (cfg.general.watermark ? "1" : "0") << "\n";
        file << "ShowFPS=" << (cfg.general.showFPS ? "1" : "0") << "\n";
        file << "MenuToggleKey=" << menuToggleKey << "\n\n";

        // [Aimbot]
        file << "[Aimbot]\n";
        file << "Enabled=" << (cfg.aimbot.enabled ? "1" : "0") << "\n";
        file << "ShowFOV=" << (cfg.aimbot.showFov ? "1" : "0") << "\n";
        file << "ShowTargetLine=" << (cfg.aimbot.showTargetLine ? "1" : "0") << "\n";
        file << "RequireKey=" << (cfg.aimbot.requireKey ? "1" : "0") << "\n";
        file << "FOVRadius=" << cfg.aimbot.fovRadius << "\n";
        file << "Smoothing=" << cfg.aimbot.smoothing << "\n\n";

        // [ESP]
        file << "[ESP]\n";
        file << "Enabled=" << (cfg.esp.enabled ? "1" : "0") << "\n";
        file << "EnemyOnly=" << (cfg.esp.enemyOnly ? "1" : "0") << "\n";
        file << "HPDot=" << (cfg.esp.hpDot ? "1" : "0") << "\n";
        file << "HPDotRadius=" << cfg.esp.hpDotRadius << "\n";
        file << "HPDotOutline=" << (cfg.esp.hpDotOutline ? "1" : "0") << "\n";
        file << "HPText=" << (cfg.esp.hpText ? "1" : "0") << "\n";
        file << "HPTextFormat=" << cfg.esp.hpTextFormat << "\n";
        file << "Box=" << (cfg.esp.box ? "1" : "0") << "\n";
        file << "BoxStyle=" << cfg.esp.boxStyle << "\n";
        file << "BoxThickness=" << cfg.esp.boxThickness << "\n";
        file << "BoxOutline=" << (cfg.esp.boxOutline ? "1" : "0") << "\n";
        file << "BoxFill=" << (cfg.esp.boxFill ? "1" : "0") << "\n";
        file << "BoxFillAlpha=" << cfg.esp.boxFillAlpha << "\n";
        file << "BoxHealthColor=" << (cfg.esp.boxHealthColor ? "1" : "0") << "\n";
        file << "BoxColor=" << ColorToString(cfg.esp.boxColor) << "\n";
        file << "HealthBar=" << (cfg.esp.healthBar ? "1" : "0") << "\n";
        file << "HealthBarWidth=" << cfg.esp.healthBarWidth << "\n";
        file << "Snaplines=" << (cfg.esp.snaplines ? "1" : "0") << "\n";
        file << "SnaplineOrigin=" << cfg.esp.snaplineOrigin << "\n";
        file << "SnaplineThickness=" << cfg.esp.snaplineThickness << "\n";
        file << "SnaplineColor=" << ColorToString(cfg.esp.snaplineColor) << "\n";
        file << "HeadCircle=" << (cfg.esp.headCircle ? "1" : "0") << "\n";
        file << "HeadCircleRadius=" << cfg.esp.headCircleRadius << "\n";
        file << "HeadCircleColor=" << ColorToString(cfg.esp.headCircleColor) << "\n";
        file << "Distance=" << (cfg.esp.distance ? "1" : "0") << "\n\n";

        // [Crosshair]
        file << "[Crosshair]\n";
        file << "Enabled=" << (cfg.crosshair.enabled ? "1" : "0") << "\n";
        file << "Style=" << cfg.crosshair.style << "\n";
        file << "Size=" << cfg.crosshair.size << "\n";
        file << "Thickness=" << cfg.crosshair.thickness << "\n";
        file << "Gap=" << cfg.crosshair.gap << "\n";
        file << "CenterDot=" << (cfg.crosshair.centerDot ? "1" : "0") << "\n";
        file << "DotRadius=" << cfg.crosshair.dotRadius << "\n";
        file << "Outline=" << (cfg.crosshair.outline ? "1" : "0") << "\n";
        file << "OutlineThickness=" << cfg.crosshair.outlineThickness << "\n";
        file << "Color=" << ColorToString(cfg.crosshair.color) << "\n";
        file << "SeparateDotColor=" << (cfg.crosshair.separateDotColor ? "1" : "0") << "\n";
        file << "DotColor=" << ColorToString(cfg.crosshair.dotColor) << "\n";
        file << "LockToCenter=" << (cfg.crosshair.lockToCenter ? "1" : "0") << "\n";
        file << "OffsetX=" << cfg.crosshair.offsetX << "\n";
        file << "OffsetY=" << cfg.crosshair.offsetY << "\n";
    }
}
