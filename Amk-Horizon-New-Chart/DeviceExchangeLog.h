#pragma once

#include <windows.h>
#include <string>
#include <vector>

struct DeviceLogLine {
    bool bold = false;
    std::wstring text;
};

// Кольцевой буфер строк лога обмена с прибором (потокобезопасно).
void DeviceExchangeLogInit();
void DeviceExchangeLogSetMainWindow(HWND hWnd);
void DeviceExchangeLogAppend(const std::wstring& line);
void DeviceExchangeLogAppendFormatted(const wchar_t* fmt, ...);
void DeviceExchangeLogClear();
std::wstring DeviceExchangeLogGetFullText();
std::vector<DeviceLogLine> DeviceExchangeLogGetLines();

// После успешного чтения метрологии: тип прибора, число передатчиков, S/N (строки помечаются как «важные» для жирного в UI).
void DeviceExchangeLogAppendDeviceIdentity(uint32_t signature, uint32_t serial);
