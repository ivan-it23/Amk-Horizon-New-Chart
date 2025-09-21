#pragma once

#include "framework.h"
#include "resource.h"
#include "DataModule.h"
#include "ChartModule.h"
#include "UIModule.h"
#include "LayoutModule.h"
#include <commdlg.h>
#include <memory>

// Главный класс приложения с модульной архитектурой
class ChartManager {
private:
    // Модули приложения
    std::unique_ptr<DataModule> dataModule;
    std::unique_ptr<ChartModule> chartModule;
    std::unique_ptr<UIModule> uiModule;
    std::unique_ptr<LayoutModule> layoutModule;
    
public:
    ChartManager();
    ~ChartManager();

    // Основные методы интерфейса (сохраняем совместимость с существующим кодом)
    void CreateControls(HWND hWnd, HINSTANCE hInst);
    void UpdateLayout(HWND hWnd);
    void ToggleDualMode();
    void RefreshChart1();
    void RefreshChart2();
    void DrawChart1(HDC hdc);
    void DrawChart2(HDC hdc);
    void LoadDataFromFile(const std::wstring& filename);
    bool HandleCommand(WPARAM wParam, LPARAM lParam);
    bool IsDualMode() const;

private:
    // Вспомогательные методы
    std::wstring BuildChartTitle(int chartNumber, DataType dataType, FrequencyBand frequency) const;
};
