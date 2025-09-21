#pragma once

#include "resource.h"
#include <vector>
#include <string>
#include <commdlg.h>

// Структура данных прибора
struct GP_TOOL_DATA {
    uint32_t signature;      // Сигнатура для идентификации данных
    uint32_t condition;      // Состояние измерения (контроль качества)
    float Ro_smt[8];        // Симметризованное удельное сопротивление
    float PH_smt[8];        // Симметризованная разность фаз
    uint32_t frame;         // Номер кадра
    float temperature;      // Температура
    float AM_RX_1[10];      // Амплитуды приемника RX1
    float AM_RX_2[10];      // Амплитуды приемника RX2
    float PH_RX_1[10];      // Фазы приемника RX1
    float PH_RX_2[10];      // Фазы приемника RX2
};

// Типы данных для отображения
enum class DataType {
    PHASE_DIFFERENCE,        // Разность фаз RX2-RX1 (4 канала T1-T4)
    AMPLITUDE_RATIO,         // Отношение амплитуд RX2/RX1 (4 канала T1-T4)
    AMPLITUDE_RX1,           // Амплитуды RX1 активных передатчиков (4 канала)
    AMPLITUDE_RX2,           // Амплитуды RX2 активных передатчиков (4 канала)
    PHASE_RX1,              // Фазы RX1 активных передатчиков (4 канала)
    PHASE_RX2,              // Фазы RX2 активных передатчиков (4 канала)
    SILENT_AMPLITUDE_RX1,    // Амплитуды молчащих передатчиков RX1 (1 канал)
    SILENT_AMPLITUDE_RX2,    // Амплитуды молчащих передатчиков RX2 (1 канал)
    TEMPERATURE             // Температура прибора (1 значение на запись)
};

// Частотные диапазоны
enum class FrequencyBand {
    FREQ_400_KHZ,    // 400 кГц
    FREQ_2000_KHZ    // 2000 кГц
};

// Класс для управления данными и графиками
class ChartManager {
private:
    std::vector<GP_TOOL_DATA> data;
    bool dualMode;
    DataType chart1DataType;
    DataType chart2DataType;
    FrequencyBand chart1Frequency;
    FrequencyBand chart2Frequency;
    
    // Элементы управления
    HWND hDualModeCheck;
    HWND hChart1DataCombo;
    HWND hChart1FreqCombo;
    HWND hChart1RefreshBtn;
    HWND hChart2DataCombo;
    HWND hChart2FreqCombo;
    HWND hChart2RefreshBtn;
    HWND hChart2Panel;
    
    // Области графиков
    RECT chart1Rect;
    RECT chart2Rect;
    
public:
    ChartManager();
    void CreateControls(HWND hWnd, HINSTANCE hInst);
    void UpdateLayout(HWND hWnd);
    void ToggleDualMode();
    void RefreshChart1();
    void RefreshChart2();
    void DrawChart1(HDC hdc);
    void DrawChart2(HDC hdc);
    void LoadDataFromFile(const std::wstring& filename);
    void HandleCommand(WPARAM wParam, LPARAM lParam);
    bool IsDualMode() const { return dualMode; }
    
private:
    void CalculateChartData(DataType type, FrequencyBand freq, std::vector<std::vector<float>>& outData);
    void DrawChart(HDC hdc, const RECT& rect, const std::vector<std::vector<float>>& data, DataType type);
    std::wstring GetDataTypeName(DataType type);
    int GetActiveTransmitterIndex(int transmitter, FrequencyBand freq);
    int GetSilentTransmitterIndex(FrequencyBand freq);
};
