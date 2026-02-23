#pragma once

#include "framework.h"
#include "resource.h"
#include "DataModule.h"
#include "ChartModule.h"
#include "UIModule.h"
#include "LayoutModule.h"
#include "SerialModule.h"
#include "MetrologyModule.h"
#include "NotesModule.h"
#include <commdlg.h>
#include <memory>
#include <vector>

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
    std::unique_ptr<SerialModule> serialModule;  // v2.13.0: Модуль работы с COM-портом
    std::unique_ptr<MetrologyModule> metrologyModule; // Метрология (файл/COM)
    std::unique_ptr<NotesModule> notesModule;    // v2.16.x: Заметки на графике

    // Текущий источник данных для привязки заметок
    std::wstring currentDataFilePath;            // полный путь к открытому файлу (если режим файла)
    std::wstring currentNotesPath;               // текущий путь sidecar заметок (чтобы не перечитывать без нужды)
    
    // Структура для хранения диапазона масштабирования
    struct ZoomRange {
        double minX, maxX, minY, maxY;
        bool isZoomed;
        bool isYCustom; // true если пользователь вручную задал диапазон Y (важно для Surf)
        
        ZoomRange() : minX(0.0), maxX(0.0), minY(0.0), maxY(0.0), isZoomed(false), isYCustom(false) {}
    };
    
    // Переменные для масштабирования
    bool isSelecting;
    POINT selectionStart;
    POINT selectionEnd;
    int activeChart; // 1 или 2, указывает на активный график для зума

    // Быстрая отрисовка выделения (без полного WM_PAINT на каждом mouse move):
    // при начале drag сохраняем "снимок" области графика, затем на каждом move
    // восстанавливаем подложку и рисуем оверлей выделения поверх.
    HDC selectionBaseDC = nullptr;
    HBITMAP selectionBaseBmp = nullptr;
    RECT selectionBaseRect = { 0, 0, 0, 0 };   // rect графика (в координатах окна), для которого сделан снимок
    bool hasSelectionBase = false;
    RECT lastSelectionDrawRect = { 0, 0, 0, 0 }; // последний нарисованный rect выделения (клипнутый)
    bool hasLastSelectionDrawRect = false;
    
    // Независимые диапазоны масштабирования для каждого графика
    ZoomRange chart1Zoom;
    ZoomRange chart2Zoom;
    
    // v2.12.5: История zoom для пошагового отката (стек состояний)
    std::vector<ZoomRange> chart1ZoomHistory;
    std::vector<ZoomRange> chart2ZoomHistory;
    
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

    // v2.16.x: кеш отрисованных маркеров заметок (для hit-test)
    std::vector<NotesModule::RenderedMarker> noteFrameMarkers;
    
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

    // === Заметки на графике (v2.16.x) ===
    bool AddNoteAtCrosshair(HWND hWnd, int chartNumber, const std::wstring& text);
    bool GetNotesForCurrentSource(std::vector<ChartNote>& outNotes, std::wstring& outNotesPath);
    bool ApplyNoteConfiguration(HWND hWnd, const ChartNote& note, int chartNumber);
    int GetActiveChartNumber() const;
    
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
    
    // === Методы для работы с COM-портом (v2.13.0) ===
    
    // Подключение/отключение от прибора
    void HandleSerialConnect(HWND hWnd);
    void HandleSerialDisconnect(HWND hWnd);
    
    // Управление режимом реального времени
    void HandleSerialStart(HWND hWnd);
    void HandleSerialStop(HWND hWnd);
    void HandleSerialClear(HWND hWnd);
    
    // Обработчик таймера опроса прибора
    void HandleSerialTimer(HWND hWnd);
    
    // Обновление UI элементов COM-порта
    void UpdateSerialUI(HWND hWnd);
    void RefreshPortList(HWND hWnd);
    
    // Проверка состояния подключения
    bool IsSerialConnected() const;
    bool IsSerialPolling() const;
    
    // Получение модулей для внешнего доступа (например, из WndProc)
    SerialModule* GetSerialModule() { return serialModule.get(); }
    MetrologyModule* GetMetrologyModule() { return metrologyModule.get(); }
    UIModule* GetUIModule() { return uiModule.get(); }
    
    // === Методы для режима Surf (v2.14.0) ===
    void HandleSurfToggle(HWND hWnd, int chartNumber);
    void UpdateSurfRange(int chartNumber);
    void UpdateSurfStatusBar();
    
    // v2.14.2: Метод для сброса перекрестия (вызывается при клике на иконку 📍)
    void ClearCrosshair(HWND hWnd, int chartNumber);

private:
    // Методы для новой системы кривых
    void DrawChartWithCurves(HDC hdc, const RECT& chartRect, int chartNumber);
    void DrawChartLegacy(HDC hdc, const RECT& chartRect, int chartNumber);
    
    // Вспомогательные методы для интеграции
    void UpdateChartAfterConfigurationChange(HWND hWnd, int chartNumber);
    void SynchronizeUIWithConfiguration(int chartNumber);
    bool ValidateChartConfiguration(const ChartConfiguration& config) const;
    // Вспомогательные методы
    std::wstring GetChartTitle(int chartNumber, FrequencyBand frequency, const std::wstring& customTitle) const;
    std::wstring BuildChartTitle(int chartNumber, DataType dataType, FrequencyBand frequency) const;
    
    int GetChartAtPoint(int x, int y) const;
    void ConvertScreenToDataCoords(int chartNum, int screenX, int screenY, double& dataX, double& dataY) const;
    void EnsureNotesLoadedForPath(const std::wstring& notesPath);
    std::wstring ResolveCurrentNotesPath() const;
    
    // Методы для работы с координатами курсора
    bool ConvertScreenToChartCoords(int chartNum, int screenX, int screenY, 
                                    double& dataX, double& dataY1, double& dataY2,
                                    std::wstring& y1Unit, std::wstring& y2Unit,
                                    bool& hasDualAxis) const;
    std::wstring GetUnitForDataType(DataType dataType) const;
    
    // Метод для отрисовки перекрестия
    void DrawCrosshair(HDC hdc, const RECT& chartRect, int chartNumber) const;

    // v2.16.x: Отрисовка точек-заметок
    void DrawNoteMarkers(HDC hdc, const RECT& chartRect, int chartNumber);

    // Вспомогательные методы для быстрого выделения области zoom
    void BeginSelectionOverlay(HWND hWnd, const RECT& chartRect);
    void EndSelectionOverlay();
    void RestoreSelectionBackground(HDC hdc, const RECT& screenRect);
    void DrawSelectionOverlay(HDC hdc, const RECT& screenRect);
};