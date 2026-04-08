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
/// Как AppendRaw, но строка отображается жирным (Rich Edit); при обычном EDIT — как обычный текст.
void DeviceExchangeLogAppendRawBold(const std::wstring& line);
void DeviceExchangeLogAppendFormatted(const wchar_t* fmt, ...);
void DeviceExchangeLogSetShowRawData(bool show);
bool DeviceExchangeLogGetShowRawData();

// Глобальный выключатель логирования. Когда false — все Append-вызовы = no-op
// (не пишут ни в буфер, ни в файл, ни триггерят UI). По умолчанию true.
void DeviceExchangeLogSetEnabled(bool enabled);
bool DeviceExchangeLogIsEnabled();
void DeviceExchangeLogClear();
std::wstring DeviceExchangeLogGetFullText();
std::vector<DeviceLogLine> DeviceExchangeLogGetLines();

// После успешного чтения метрологии: тип прибора, число передатчиков, S/N (строки помечаются как «важные» для жирного в UI).
void DeviceExchangeLogAppendDeviceIdentity(uint32_t signature, uint32_t serial);

// Расширенная версия с полной идентификацией зонда (тип, N Tx, мод, номер, позиция R1, диаметр)
void DeviceExchangeLogAppendDeviceIdentity(uint32_t signature, uint32_t serial,
                                          int mod, int number, uint32_t rxPosition,
                                          uint32_t dSondeMm);

// === Ленивое обновление UI-панели ===
// Сообщить буферу, видна ли сейчас UI-панель лога.
// Если панель не видна, PostRefresh() не планирует таймер → нулевая нагрузка на main thread.
void DeviceExchangeLogSetPanelVisible(bool visible);
bool DeviceExchangeLogIsPanelVisible();

// === Файловый лог ===
// Открыть/закрыть файл лога сессии (%LOCALAPPDATA%\AMK-Horizon-New-Chart\device_log.txt).
// При открытии файл перезаписывается (новая сессия). Все Append-вызовы дописывают строку в файл.
void DeviceExchangeLogFileOpen();
void DeviceExchangeLogFileClose();
std::wstring DeviceExchangeLogGetFilePath();
