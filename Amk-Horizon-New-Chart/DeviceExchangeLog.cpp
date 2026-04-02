#include "framework.h"
#include "DeviceExchangeLog.h"
#include "resource.h"
#include <deque>
#include <mutex>
#include <cstdarg>

namespace {

constexpr size_t kMaxLines = 2000;

std::mutex g_mutex;
std::deque<std::wstring> g_lines;
HWND g_notifyWnd = nullptr;

void PostRefresh() {
    if (g_notifyWnd && IsWindow(g_notifyWnd)) {
        PostMessageW(g_notifyWnd, WM_APP_DEVICE_LOG_REFRESH, 0, 0);
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
        g_lines.push_back(line);
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
        out += g_lines[i];
    }
    return out;
}
