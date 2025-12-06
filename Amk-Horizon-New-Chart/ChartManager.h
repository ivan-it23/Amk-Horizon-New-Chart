#pragma once

#include "framework.h"
#include "resource.h"
#include "DataModule.h"
#include "ChartModule.h"
#include "UIModule.h"
#include "LayoutModule.h"
#include <commdlg.h>
#include <memory>

// Предварительные объявления новых модулей
class SchematicModule;
class CurveSelectionModule;

// Forward declarations для структур
struct CurveDefinition;
struct ChartConfiguration;
enum class PresetType;

// Главный класс приложения с модульной архитектурой
class ChartManager {
private:
    // Модули приложения
    std::unique_ptr<DataModule> dataModule;
    std::unique_ptr<ChartModule> chartModule;
    std::unique_ptr<UIModule> uiModule;
    std::unique_ptr<LayoutModule> layoutModule;
    std::unique_ptr<SchematicModule> schematicModule;
    std::unique_ptr<CurveSelectionModule> curveSelectionModule;
    
    // Структура для хранения диапазона масштабирования
    struct ZoomRange {
        double minX, maxX, minY, maxY;
        bool isZoomed;
        
        ZoomRange() : minX(0.0), maxX(0.0), minY(0.0), maxY(0.0), isZoomed(false) {}
    };
    
    // Переменные для масштабирования
    bool isSelecting;
    POINT selectionStart;
    POINT selectionEnd;
    int activeChart; // 1 или 2, указывает на активный график для зума
    
    // Независимые диапазоны масштабирования для каждого графика
    ZoomRange chart1Zoom;
    ZoomRange chart2Zoom;
    
    // Структура для хранения позиции перекрестия (crosshair)
    struct CrosshairPosition {
        int screenX, screenY;   // Экранные координаты
        double dataX, dataY;    // Координаты данных
        bool isVisible;         // Видимо ли перекрестие
        int chartNumber;        // На каком графике (1 или 2)
        
        CrosshairPosition() : screenX(0), screenY(0), dataX(0.0), dataY(0.0), isVisible(false), chartNumber(0) {}
    };
    
    // Перекрестие для каждого графика
    CrosshairPosition chart1Crosshair;
    CrosshairPosition chart2Crosshair;
    
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
    void UpdateStatusBarFileInfo(); // Обновление информации о файле в статусной строке
    bool HandleCommand(WPARAM wParam, LPARAM lParam);
    bool IsDualMode() const;
    
    // Методы для обработки событий мыши
    void HandleMouseDown(HWND hWnd, int x, int y);
    void HandleMouseMove(HWND hWnd, int x, int y);
    void HandleMouseUp(HWND hWnd, int x, int y);
    void HandleRightClick(HWND hWnd, int x, int y);
    void HandleMouseMoveForCoordinates(HWND hWnd, int x, int y); // Для обновления координат в статусной строке

    // Новый метод для обработки кликов по чекбоксам легенды
    void HandleLegendCheckboxClick(HWND hWnd, int x, int y, int chartId);

    // Методы для обработки скроллинга
    void HandleScrollEvent(HWND hWnd, WPARAM wParam, LPARAM lParam);
    void UpdateScrollBars(HWND hWnd);
    
    // Методы для обработки X Range
    void HandleXRangeApply(HWND hWnd, int chartNumber);
    void HandleXRangeReset(HWND hWnd, int chartNumber);
    void UpdateXRangeUI(HWND hWnd);
    
    // Метод для сброса zoom (кнопка Reset Zoom)
    void HandleResetZoom(HWND hWnd, int chartNumber);
    
    // Методы для сохранения диапазона
    void HandleSaveRange(HWND hWnd, int chartNumber);
    
    // Метод для частичной перерисовки конкретного графика
    void InvalidateChart(HWND hWnd, int chartNumber);
    
    // Метод для отображения схемы прибора
    void ShowSchematicDialog(HWND hWnd, int chartNumber);
    
    // Методы для работы с конфигурациями кривых
    void ApplyCurveConfiguration(int chartNumber, const ChartConfiguration& config);
    ChartConfiguration GetChartConfiguration(int chartNumber) const;
    bool HasCurveConfiguration(int chartNumber) const;
    
    // Методы для копирования настроек между графиками
    void CopyChartSettings(int fromChart, int toChart);
    
    // Методы для применения пресетов
    void ApplyPreset(int chartNumber, PresetType preset, FrequencyBand frequency);
    
    // Методы для обработки событий от диалогового окна схемы
    void HandleSchematicDialogResult(HWND hWnd, int chartNumber, bool applied);
    void HandlePresetCommand(HWND hWnd, int chartNumber, int presetId, FrequencyBand frequency);
    void HandleCopySettingsCommand(HWND hWnd, int fromChart, int toChart);
    
    // Методы для обеспечения совместимости
    void ResetToLegacyMode(int chartNumber);
    bool IsUsingNewCurveSystem(int chartNumber) const;
    void SynchronizeLegacyUIWithNewSystem(int chartNumber);
    void HandleLegacyUIChange(HWND hWnd, int chartNumber);

private:
    // Методы для новой системы кривых
    void DrawChartWithCurves(HDC hdc, const RECT& chartRect, int chartNumber);
    void DrawChartLegacy(HDC hdc, const RECT& chartRect, int chartNumber);
    
    // Вспомогательные методы для интеграции
    void UpdateChartAfterConfigurationChange(HWND hWnd, int chartNumber);
    void SynchronizeUIWithConfiguration(int chartNumber);
    bool ValidateChartConfiguration(const ChartConfiguration& config) const;
    // Вспомогательные методы
    std::wstring BuildChartTitle(int chartNumber, DataType dataType, FrequencyBand frequency) const;
    int GetChartAtPoint(int x, int y) const;
    void ConvertScreenToDataCoords(int chartNum, int screenX, int screenY, double& dataX, double& dataY) const;
    
    // Методы для работы с координатами курсора
    bool ConvertScreenToChartCoords(int chartNum, int screenX, int screenY, 
                                    double& dataX, double& dataY1, double& dataY2,
                                    std::wstring& y1Unit, std::wstring& y2Unit,
                                    bool& hasDualAxis) const;
    std::wstring GetUnitForDataType(DataType dataType) const;
    
    // Метод для отрисовки перекрестия
    void DrawCrosshair(HDC hdc, const RECT& chartRect, int chartNumber) const;
};