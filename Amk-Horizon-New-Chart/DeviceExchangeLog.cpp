#include "framework.h"
#include "DeviceExchangeLog.h"
#include "ToolDataAdapters.h"
#include "resource.h"
#include <deque>
#include <mutex>
#include <cstdarg>
#include <fstream>
#include <atomic>
#include <shlobj.h>

namespace {

constexpr size_t kMaxLines = 500;

struct LogEntry {
    bool bold = false;
    bool isRawPayload = false;
    std::wstring text;
};

std::mutex g_mutex;
std::deque<LogEntry> g_lines;
HWND g_notifyWnd = nullptr;
bool g_showRawData = false;
std::atomic<bool> g_panelVisible{false};
std::atomic<bool> g_loggingEnabled{true};

// === Файловый лог ===
std::mutex g_fileMutex;
std::ofstream g_logFile;
std::wstring g_logFilePath;

static std::wstring BuildLogFilePath() {
    wchar_t localAppData[MAX_PATH] = {0};
    if (SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppData) != S_OK) {
        DWORD len = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
        if (len == 0 || len >= MAX_PATH) return L"";
    }
    std::wstring dir = std::wstring(localAppData) + L"\\AMK-Horizon-New-Chart";
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir + L"\\device_log.txt";
}

static void FileWriteLine(const std::wstring& text) {
    std::lock_guard<std::mutex> flock(g_fileMutex);
    if (!g_logFile.is_open()) return;
    // UTF-8 конвертация
    int needed = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (needed > 0) {
        std::string utf8(static_cast<size_t>(needed), '\0');
        WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), &utf8[0], needed, nullptr, nullptr);
        g_logFile << utf8 << "\n";
    }
}

void PostRefresh() {
    if (!g_panelVisible.load(std::memory_order_relaxed)) return;
    if (g_notifyWnd && IsWindow(g_notifyWnd)) {
        SetTimer(g_notifyWnd, IDT_DEVICE_LOG_REFRESH, 400, nullptr);
    }
}

static std::wstring ToolKindToDisplayName(ToolDataKind kind) {
    switch (kind) {
    case ToolDataKind::AUTONOM_5TX:
        return L"Автоном 5TX";
    case ToolDataKind::AUTONOM_5TX_SDR:
        return L"Автоном 5TX SDR";
    case ToolDataKind::LWD_3TX:
        return L"LWD 3TX";
    case ToolDataKind::LWD_4TX:
        return L"LWD 4TX";
    default:
        return L"Неизвестно";
    }
}

} // namespace

void DeviceExchangeLogInit() {
    std::lock_guard<std::mutex> lock(g_mutex);
    g_lines.clear();
}

void DeviceExchangeLogSetMainWindow(HWND hWnd) {
    g_notifyWnd = hWnd;
}

static bool LineVisible(const LogEntry& e) {
    if (e.isRawPayload && !g_showRawData) {
        return false;
    }
    return true;
}

void DeviceExchangeLogSetShowRawData(bool show) {
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_showRawData = show;
    }
}

bool DeviceExchangeLogGetShowRawData() {
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_showRawData;
}

void DeviceExchangeLogAppend(const std::wstring& line) {
    if (!g_loggingEnabled.load(std::memory_order_relaxed)) return;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_lines.push_back(LogEntry{false, false, line});
        while (g_lines.size() > kMaxLines) {
            g_lines.pop_front();
        }
    }
    FileWriteLine(line);
    PostRefresh();
}

void DeviceExchangeLogAppendRaw(const std::wstring& line) {
    if (!g_loggingEnabled.load(std::memory_order_relaxed)) return;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_lines.push_back(LogEntry{false, true, line});
        while (g_lines.size() > kMaxLines) {
            g_lines.pop_front();
        }
    }
    FileWriteLine(line);
    PostRefresh();
}

void DeviceExchangeLogAppendRawBold(const std::wstring& line) {
    if (!g_loggingEnabled.load(std::memory_order_relaxed)) return;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_lines.push_back(LogEntry{true, true, line});
        while (g_lines.size() > kMaxLines) {
            g_lines.pop_front();
        }
    }
    FileWriteLine(line);
    PostRefresh();
}

void DeviceExchangeLogAppendFormatted(const wchar_t* fmt, ...) {
    if (!g_loggingEnabled.load(std::memory_order_relaxed)) return;
    wchar_t buf[2048];
    va_list args;
    va_start(args, fmt);
    _vsnwprintf_s(buf, _countof(buf), _TRUNCATE, fmt, args);
    va_end(args);
    DeviceExchangeLogAppend(std::wstring(buf));
}

void DeviceExchangeLogClear() {
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_lines.clear();
    }
    PostRefresh();
}

std::wstring DeviceExchangeLogGetFullText() {
    std::lock_guard<std::mutex> lock(g_mutex);
    std::wstring out;
    out.reserve(g_lines.size() * 64);
    bool first = true;
    for (size_t i = 0; i < g_lines.size(); ++i) {
        if (!LineVisible(g_lines[i])) {
            continue;
        }
        if (!first) {
            out += L"\r\n";
        }
        first = false;
        out += g_lines[i].text;
    }
    return out;
}

std::vector<DeviceLogLine> DeviceExchangeLogGetLines() {
    std::lock_guard<std::mutex> lock(g_mutex);
    std::vector<DeviceLogLine> out;
    out.reserve(g_lines.size());
    for (const auto& e : g_lines) {
        if (!LineVisible(e)) {
            continue;
        }
        out.push_back(DeviceLogLine{e.bold, e.isRawPayload, e.text});
    }
    return out;
}

void DeviceExchangeLogAppendDeviceIdentity(uint32_t signature, uint32_t serial,
                                          int mod, int number, uint32_t rxPosition,
                                          uint32_t dSondeMm) {
    if (!g_loggingEnabled.load(std::memory_order_relaxed)) return;
    const IToolDataAdapter* adapter = ToolDataAdapterRegistry::Get().FindAdapterBySignature(signature);
    ToolDataKind kind = ToolDataKind::UNKNOWN;
    int txCount = 0;
    std::wstring typeLabel = L"Неизвестно";
    if (adapter) {
        kind = adapter->GetKind();
        ToolCapabilities cap = adapter->GetCapabilities();
        txCount = cap.transmitterCount;
        typeLabel = ToolKindToDisplayName(kind);
    }

    // Декодирование полей из signature (low20 бит)
    const uint32_t low20 = (signature << 12) >> 12;
    const int typeClass = static_cast<int>(low20 / 100000u);
    const int decodedNtx = static_cast<int>((low20 % 100000u) / 10000u);
    const int ntx = (decodedNtx > 0) ? decodedNtx : txCount;
    
    // Если mod/number не переданы (-1), берём из signature
    const int finalMod = (mod >= 0) ? mod : static_cast<int>((low20 % 10000u) / 1000u);
    const int finalNumber = (number >= 0) ? number : static_cast<int>(low20 % 1000u);
    
    // Позиция R1
    std::wstring rxPosLabel = L"R1→T1";
    if (rxPosition == 1) {
        rxPosLabel = L"R1→T2";
    }

    std::lock_guard<std::mutex> lock(g_mutex);
    auto pushBold = [](const std::wstring& s) {
        g_lines.push_back(LogEntry{true, false, s});
        while (g_lines.size() > kMaxLines) {
            g_lines.pop_front();
        }
        FileWriteLine(s);
    };
    pushBold(L"========== Идентификация зонда ==========");
    pushBold(L"Тип: " + typeLabel);
    pushBold(L"N Tx: " + std::to_wstring(ntx));
    pushBold(L"Мод: " + std::to_wstring(finalMod));
    pushBold(L"Номер: " + std::to_wstring(finalNumber));
    pushBold(L"Серийный №: " + std::to_wstring(static_cast<unsigned long>(serial)));
    pushBold(L"Позиция R1: " + rxPosLabel);
    if (dSondeMm > 0) {
        pushBold(L"Диаметр прибора: " + std::to_wstring(dSondeMm) + L" мм");
    }
    pushBold(L"========================================");
    PostRefresh();
}

// Упрощённая версия для COM-режима (с полной метрологией)
void DeviceExchangeLogAppendDeviceIdentity(uint32_t signature, uint32_t serial) {
    DeviceExchangeLogAppendDeviceIdentity(signature, serial, -1, -1, 0, 0);
}

// === Ленивое обновление UI-панели ===

void DeviceExchangeLogSetPanelVisible(bool visible) {
    g_panelVisible.store(visible, std::memory_order_relaxed);
}

bool DeviceExchangeLogIsPanelVisible() {
    return g_panelVisible.load(std::memory_order_relaxed);
}

void DeviceExchangeLogSetEnabled(bool enabled) {
    g_loggingEnabled.store(enabled, std::memory_order_relaxed);
}

bool DeviceExchangeLogIsEnabled() {
    return g_loggingEnabled.load(std::memory_order_relaxed);
}

// === Файловый лог ===

void DeviceExchangeLogFileOpen() {
    std::lock_guard<std::mutex> flock(g_fileMutex);
    if (g_logFile.is_open()) g_logFile.close();
    g_logFilePath = BuildLogFilePath();
    if (g_logFilePath.empty()) return;
    g_logFile.open(g_logFilePath, std::ios::out | std::ios::trunc);
}

void DeviceExchangeLogFileClose() {
    std::lock_guard<std::mutex> flock(g_fileMutex);
    if (g_logFile.is_open()) g_logFile.close();
}

std::wstring DeviceExchangeLogGetFilePath() {
    std::lock_guard<std::mutex> flock(g_fileMutex);
    return g_logFilePath;
}
