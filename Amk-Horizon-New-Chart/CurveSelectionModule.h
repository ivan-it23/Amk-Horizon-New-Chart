#pragma once

#include "framework.h"
#include "SchematicModule.h"
#include "DataModule.h"
#include "MetrologyModule.h"
#include <vector>
#include <string>
#include <memory>

// Класс для управления выбором и конфигурацией кривых
class CurveSelectionModule {
private:
    // Цветовые палитры для разных осей
    static const COLORREF LEFT_AXIS_COLORS[SchematicSelection::UI_TRANSMITTER_COUNT];   // Приоритетные цвета для левой оси (T1-T5)
    static const COLORREF RIGHT_AXIS_COLORS[SchematicSelection::UI_TRANSMITTER_COUNT];  // Альтернативные цвета для правой оси (T1-T5)
    static const COLORREF SILENT_COLOR;          // Цвет для молчащих передатчиков
    static const COLORREF TEMPERATURE_COLOR;     // Цвет для температуры
    
    // Расширенная палитра для умного назначения цветов (12 цветов)
    static const COLORREF EXTENDED_PALETTE[12];
    
    // Старый массив для обратной совместимости
    static const COLORREF CURVE_COLORS[SchematicSelection::UI_TRANSMITTER_COUNT];
    
    // Текущие конфигурации для каждого графика
    ChartConfiguration chart1Config;
    ChartConfiguration chart2Config;
    const MetrologyModule* metrologyModule = nullptr;
    
public:
    CurveSelectionModule();
    ~CurveSelectionModule();
    
    // Основные методы управления кривыми
    std::vector<CurveDefinition> GetAvailableCurves(const SchematicSelection& selection) const;
    void SetMetrologyModule(const MetrologyModule* module) { metrologyModule = module; }
    void ApplyCurveConfiguration(int chartNumber, const ChartConfiguration& config);
    ChartConfiguration GetChartConfiguration(int chartNumber) const;
    
    // Управление пресетами
    void ApplyPreset(PresetType preset, int chartNumber, FrequencyBand frequency);
    ChartConfiguration CreatePresetConfiguration(PresetType preset, FrequencyBand frequency) const;
    ChartConfiguration BuildFullCurveSet(FrequencyBand frequency) const;
    
    // Копирование настроек между графиками
    void CopySettings(int fromChart, int toChart);
    bool HasConfiguration(int chartNumber) const;
    
    // Валидация и проверки
    bool ValidateConfiguration(const ChartConfiguration& config) const;
    std::string GetConfigurationError(const ChartConfiguration& config) const;
    
    // Вычисление данных для кривых
    void CalculateCurveData(const ChartConfiguration& config, 
                           const DataModule* dataModule,
                           std::vector<std::vector<float>>& leftAxisData,
                           std::vector<std::vector<float>>& rightAxisData,
                           std::vector<CurveDefinition>& leftAxisCurves,
                           std::vector<CurveDefinition>& rightAxisCurves) const;
    
    // Перегрузка с поддержкой zoom
    void CalculateCurveData(const ChartConfiguration& config, 
                           const DataModule* dataModule,
                           std::vector<std::vector<float>>& leftAxisData,
                           std::vector<std::vector<float>>& rightAxisData,
                           std::vector<CurveDefinition>& leftAxisCurves,
                           std::vector<CurveDefinition>& rightAxisCurves,
                           double minX, double maxX, double minY, double maxY) const;
    
private:
    // Методы определения доступных кривых для различных выборов
    void AddTwoReceiversCurves(std::vector<CurveDefinition>& curves, 
                              const SchematicSelection& selection) const;
    void AddSingleReceiverCurves(std::vector<CurveDefinition>& curves, 
                                const SchematicSelection& selection) const;
    void AddTransmitterCurves(std::vector<CurveDefinition>& curves, 
                             const SchematicSelection& selection) const;
    void AddSilentTransmitterCurves(std::vector<CurveDefinition>& curves, 
                                   const SchematicSelection& selection) const;
    
    // Вспомогательные методы создания кривых
    CurveDefinition CreateRatioCurve(int transmitterIndex, FrequencyBand frequency) const;
    CurveDefinition CreateDifferenceCurve(int transmitterIndex, FrequencyBand frequency) const;
    CurveDefinition CreateAmplitudeCurve(int transmitterIndex, int receiverIndex, 
                                        FrequencyBand frequency) const;
    CurveDefinition CreatePhaseCurve(int transmitterIndex, int receiverIndex, 
                                    FrequencyBand frequency) const;
    CurveDefinition CreateSilentAmplitudeCurve(int receiverIndex, FrequencyBand frequency) const;
    CurveDefinition CreateSilentPhaseDifferenceCurve(FrequencyBand frequency) const;
    CurveDefinition CreateRhoSmtCurve(int transmitterIndex, FrequencyBand frequency) const;     // v2.16.0
    CurveDefinition CreatePhaseSmtCurve(int transmitterIndex, FrequencyBand frequency) const;   // v2.16.0
    
    // Методы для работы с цветами и стилями
    COLORREF GetCurveColor(int transmitterIndex) const;  // Старый метод для обратной совместимости
    COLORREF GetCurveColorForAxis(int transmitterIndex, int yAxisGroup) const;  // Новый метод с учетом оси
    COLORREF GetSpecialDataColor(DataType dataType) const;  // Цвета для специальных типов данных
    COLORREF AssignSmartColor(const std::vector<CurveDefinition>& existingCurves, 
                             int transmitterIndex, int yAxisGroup) const;  // Умное назначение цветов
    void RefreshCurveColors(std::vector<CurveDefinition>& curves) const;
    int GetLineStyle(int yAxisGroup) const;  // Всегда PS_SOLID (однородные линии)
    
    // Методы для создания пресетов
    ChartConfiguration CreateAllAmplitudesR1Preset(FrequencyBand frequency) const;
    ChartConfiguration CreateAllAmplitudesR2Preset(FrequencyBand frequency) const;
    ChartConfiguration CreateAmplitudeRatiosPreset(FrequencyBand frequency) const;
    ChartConfiguration CreatePhaseDifferencesPreset(FrequencyBand frequency) const;
    ChartConfiguration CreateRhoSmtPreset(FrequencyBand frequency) const;       // v2.16.0: УЭС
    ChartConfiguration CreatePhaseSmtPreset(FrequencyBand frequency) const;     // v2.16.0: Симм. фазы
    void AddSmtSelectionCurves(std::vector<CurveDefinition>& curves,
                               FrequencyBand frequency,
                               int transmitterCount,
                               CurveDefinition (CurveSelectionModule::*createCurve)(int, FrequencyBand) const,
                               SmtComputationMode mode) const;
    void FinalizeSilentPresetCurve(CurveDefinition& curve,
                                   const std::vector<CurveDefinition>& existingCurves,
                                   int silentIndex) const;
    
    // Вспомогательные методы для вычисления данных
    void CalculateRatioData(int transmitterIndex, FrequencyBand frequency,
                           const DataModule* dataModule, std::vector<float>& outData) const;
    void CalculateDifferenceData(int transmitterIndex, FrequencyBand frequency,
                                const DataModule* dataModule, std::vector<float>& outData) const;
    void CalculateAmplitudeData(int transmitterIndex, int receiverIndex, FrequencyBand frequency,
                               const DataModule* dataModule, std::vector<float>& outData) const;
    void CalculatePhaseData(int transmitterIndex, int receiverIndex, FrequencyBand frequency,
                           const DataModule* dataModule, std::vector<float>& outData) const;
    void CalculateSilentAmplitudeData(int receiverIndex, FrequencyBand frequency,
                                     const DataModule* dataModule, std::vector<float>& outData) const;

    void CalculatePhaseSmtData(int transmitterIndex, FrequencyBand frequency,
                               SmtComputationMode mode,
                               const DataModule* dataModule,
                               std::vector<float>& outData,
                               double minX, double maxX) const;

    void CalculateRhoSmtData(int transmitterIndex, FrequencyBand frequency,
                             SmtComputationMode mode,
                             const DataModule* dataModule,
                             std::vector<float>& outData,
                             double minX, double maxX) const;
    
    // Методы для работы с индексами данных
    int GetActiveTransmitterIndex(int transmitterIndex, FrequencyBand frequency) const;
    int GetSilentTransmitterIndex(FrequencyBand frequency) const;
    
    // Генерация уникальных идентификаторов кривых
    std::string GenerateCurveId(ElementType element, int elementIndex, 
                               DataType dataType, FrequencyBand frequency, 
                               int receiverIndex = -1) const;
    std::string GenerateDisplayName(ElementType element, int elementIndex, 
                                   DataType dataType, int receiverIndex = -1) const;
    std::string GetDataTypeDisplayName(DataType dataType) const;
    std::string GetFrequencyDisplayName(FrequencyBand frequency) const;
    std::string GetUnitsForDataType(DataType dataType) const;
    
    // Вспомогательные методы
    void SortCurvesByType(std::vector<CurveDefinition>& curves) const;
    int GetDataTypePriority(DataType dataType) const;
};

// Вспомогательные функции для работы с кривыми
namespace CurveUtils {
    // Проверка совместимости типов данных для двойных осей
    bool CanUseDualAxis(const std::vector<CurveDefinition>& curves);
    
    // Группировка кривых по осям
    void GroupCurvesByAxis(const std::vector<CurveDefinition>& curves,
                          std::vector<CurveDefinition>& leftAxisCurves,
                          std::vector<CurveDefinition>& rightAxisCurves);
    
    // Определение единиц измерения для оси
    std::string DetermineAxisUnits(const std::vector<CurveDefinition>& axisCurves);
    
    // Валидация конфигурации кривых
    bool ValidateCurveConfiguration(const std::vector<CurveDefinition>& curves, 
                                   std::string& errorMessage);
}