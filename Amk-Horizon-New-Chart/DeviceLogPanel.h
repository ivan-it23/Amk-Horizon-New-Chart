#pragma once

#include <windows.h>

class UIModule;
class LayoutModule;

void DeviceLogPanel_Init(HWND hMain, HINSTANCE hInst);
void DeviceLogPanel_Shutdown(HWND hMain);
void DeviceLogPanel_Toggle(HWND hMain);
bool DeviceLogPanel_IsDockVisible();
void DeviceLogPanel_UpdateLayout(HWND hMain, UIModule* ui, const LayoutModule* layout);
void DeviceLogPanel_OnRefresh(HWND hMain);
void DeviceLogPanel_OnExpandCommand();
/// Вызывать из WM_COMMAND для IDC_DEVICE_LOG_RAW_CHECK (lParam = HWND чекбокса).
void DeviceLogPanel_OnRawDataToggled(HWND hCheckbox);
