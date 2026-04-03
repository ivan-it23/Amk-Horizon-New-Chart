#include "framework.h"
#include "DeviceExchangeLog.h"
#include "ToolDataAdapters.h"
#include "resource.h"
#include <deque>
#include <mutex>
#include <cstdarg>

namespace {

constexpr size_t kMaxLines = 2000;

struct LogEntry {
    bool bold = false;
    std::wstring text;
};

std::mutex g_mutex;
std::deque<LogEntry> g_lines;
HWND g_notifyWnd = nullptr;

void PostRefresh() {
    if (g_notifyWnd && IsWindow(g_notifyWnd)) {
        PostMessageW(g_notifyWnd, WM_APP_DEVICE_LOG_REFRESH, 0, 0);
    }
}

static std::wstring ToolKindToDisplayName(ToolDataKind kind) {
    switch (kind) {
    case ToolDataKind::AUTONOM_5TX:
        return L"Автоном 5TX";
    case ToolDataKind::AUTONOM_5TX_SDR:
        return L"Автоном 5TX SDR";
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

void DeviceExchangeLogAppend(const std::wstring& line) {
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        g_lines.push_back(LogEntry{false, line});
        while (g_lines.size() > kMaxLines) {
            g_lines.pop_front();
        }
    }
    PostRefresh();
}

void DeviceExchangeLogAppendFormatted(const wchar_t* fmt, ...) {
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
    for (size_t i = 0; i < g_lines.size(); ++i) {
        if (i > 0) {
            out += L"\r\n";
        }
        out += g_lines[i].text;
    }
    return out;
}

std::vector<DeviceLogLine> DeviceExchangeLogGetLines() {
    std::lock_guard<std::mutex> lock(g_mutex);
    std::vector<DeviceLogLine> out;
    out.reserve(g_lines.size());
    for (const auto& e : g_lines) {
        out.push_back(DeviceLogLine{e.bold, e.text});
    }
    return out;
}

void DeviceExchangeLogAppendDeviceIdentity(uint32_t signature, uint32_t serial) {
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

    std::lock_guard<std::mutex> lock(g_mutex);
    auto pushBold = [](const std::wstring& s) {
        g_lines.push_back(LogEntry{true, s});
        while (g_lines.size() > kMaxLines) {
            g_lines.pop_front();
        }
    };
    pushBold(L"Тип прибора: " + typeLabel);
    pushBold(L"Передатчиков: " + std::to_wstring(txCount > 0 ? txCount : 0));
    pushBold(L"Серийный номер: " + std::to_wstring(static_cast<unsigned long>(serial)));
    PostRefresh();
}
