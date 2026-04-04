#pragma once

#include <windows.h>
#include <string>
#include <vector>

struct DeviceLogLine {
    bool bold = false;
    /// Строки с полным hex и т.п.; скрываются, если выключено «Сырые данные».
    bool isRawPayload = false;
    std::wstring text;
};

// Кольцевой буфер строк лога обмена с прибором (потокобезопасно).
void DeviceExchangeLogInit();
void DeviceExchangeLogSetMainWindow(HWND hWnd);
void DeviceExchangeLogAppend(const std::wstring& line);
/// Дополнительные строки (hex и т.д.), показываются только при включённом «Сырые данные».
void DeviceExchangeLogAppendRaw(const std::wstring& line);
void DeviceExchangeLogAppendFormatted(const wchar_t* fmt, ...);
void DeviceExchangeLogSetShowRawData(bool show);
bool DeviceExchangeLogGetShowRawData();
void DeviceExchangeLogClear();
std::wstring DeviceExchangeLogGetFullText();
std::vector<DeviceLogLine> DeviceExchangeLogGetLines();

// После успешного чтения метрологии: тип прибора, число передатчиков, S/N (строки помечаются как «важные» для жирного в UI).
void DeviceExchangeLogAppendDeviceIdentity(uint32_t signature, uint32_t serial);
