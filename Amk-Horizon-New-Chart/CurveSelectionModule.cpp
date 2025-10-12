#include "framework.h"
#include "CurveSelectionModule.h"
#include <algorithm>
#include <sstream>

// Определение стандартных цветов для кривых T1-T4
const COLORREF CurveSelectionModule::CURVE_COLORS[4] = {
    RGB(255, 0, 0),    // T1 - Красный
    RGB(0, 0, 255),    // T2 - Синий  
    RGB(0, 128, 0),    // T3 - Зеленый
    RGB(255, 165, 0)   // T4 - Оранжевый
};

// Реализация CurveSelectionModule
CurveSelectionModule::CurveSelectionModule() {
}

CurveSelectionModule::~CurveSelectionModule() {
}

std::vector<CurveDefinition> CurveSelectionModule::GetAvailableCurves(const SchematicSelection& selection) const {
    std::vector<CurveDefinition> curves;
    
    // Проверяем валидность выбора
    if (!selection.IsValid()) {
        return curves; // Возвращаем пустой список для некорректного выбора
    }
    
    int selectedReceivers = selection.GetSelectedReceiversCount();
    int selectedTransmitters = selection.GetSelectedTransmittersCount();
    int selectedSilent = selection.GetSelectedSilentCount();
    
    // Логика определения доступных кривых на основе выбора
    if (selectedReceivers == 2) {
        // Два приемника -> отношения амплитуд и разности фаз (4 кривые T1-T4)
        AddTwoReceiversCurves(curves, selection);
    }
    else if (selectedReceivers == 1) {
        // Один приемник -> амплитуды и фазы для T1-T4 (8 кривых)
        AddSingleReceiverCurves(curves, selection);
    }
    else if (selectedTransmitters > 0) {
        // Выбранные передатчики -> амплитуды и фазы для R1 и R2
        AddTransmitterCurves(curves, selection);
    }
    else if (selectedSilent > 0) {
        // Молчащие передатчики -> только амплитуды для соответствующих приемников
        AddSilentTransmitterCurves(curves, selection);
    }
    
    // Сортируем кривые по типу данных и передатчикам для удобства
    SortCurvesByType(curves);
    
    return curves;
}

void CurveSelectionModule::ApplyCurveConfiguration(int chartNumber, const ChartConfiguration& config) {
    if (chartNumber == 1) {
        chart1Config = config;
    } else if (chartNumber == 2) {
        chart2Config = config;
    }
}

ChartConfiguration CurveSelectionModule::GetChartConfiguration(int chartNumber) const {
    if (chartNumber == 1) {
        return chart1Config;
    } else if (chartNumber == 2) {
        return chart2Config;
    }
    return ChartConfiguration();
}

void CurveSelectionModule::ApplyPreset(PresetType preset, int chartNumber, FrequencyBand frequency) {
    ChartConfiguration config = CreatePresetConfiguration(preset, frequency);
    ApplyCurveConfiguration(chartNumber, config);
}

ChartConfiguration CurveSelectionModule::CreatePresetConfiguration(PresetType preset, FrequencyBand frequency) const {
    switch (preset) {
        case PresetType::ALL_AMPLITUDES_R1:
            return CreateAllAmplitudesR1Preset(frequency);
        case PresetType::ALL_AMPLITUDES_R2:
            return CreateAllAmplitudesR2Preset(frequency);
        case PresetType::ALL_PHASES_R1:
            return CreateAllPhasesR1Preset(frequency);
        case PresetType::ALL_PHASES_R2:
            return CreateAllPhasesR2Preset(frequency);
        case PresetType::AMPLITUDE_RATIOS:
            return CreateAmplitudeRatiosPreset(frequency);
        case PresetType::PHASE_DIFFERENCES:
            return CreatePhaseDifferencesPreset(frequency);
        default:
            return ChartConfiguration();
    }
}

void CurveSelectionModule::CopySettings(int fromChart, int toChart) {
    if (fromChart == 1 && toChart == 2) {
        chart2Config = chart1Config.Copy();
    } else if (fromChart == 2 && toChart == 1) {
        chart1Config = chart2Config.Copy();
    }
}

bool CurveSelectionModule::HasConfiguration(int chartNumber) const {
    const ChartConfiguration& config = (chartNumber == 1) ? chart1Config : chart2Config;
    return !config.IsEmpty();
}

bool CurveSelectionModule::ValidateConfiguration(const ChartConfiguration& config) const {
    return GetConfigurationError(config).empty();
}

std::string CurveSelectionModule::GetConfigurationError(const ChartConfiguration& config) const {
    if (config.curves.empty()) {
        return "No curves selected for display";
    }
    
    // Проверяем, что хотя бы одна кривая включена
    bool hasEnabledCurve = false;
    int enabledCurvesCount = 0;
    
    for (const auto& curve : config.curves) {
        if (curve.enabled) {
            hasEnabledCurve = true;
            enabledCurvesCount++;
        }
    }
    
    if (!hasEnabledCurve) {
        return "All curves are disabled";
    }
    
    // Проверяем совместимость типов данных для двойных осей
    if (config.hasDualAxis) {
        bool hasLeftAxisCurves = false;
        bool hasRightAxisCurves = false;
        
        for (const auto& curve : config.curves) {
            if (!curve.enabled) continue;
            
            if (curve.yAxisGroup == 0) hasLeftAxisCurves = true;
            if (curve.yAxisGroup == 1) hasRightAxisCurves = true;
        }
        
        if (config.hasDualAxis && (!hasLeftAxisCurves || !hasRightAxisCurves)) {
            return "Dual axis requires curves on both axes";
        }
    }
    
    // Проверяем максимальное количество кривых
    if (enabledCurvesCount > 8) {
        return "Too many curves for display (maximum 8)";
    }
    
    return ""; // Конфигурация корректна
}

void CurveSelectionModule::CalculateCurveData(const ChartConfiguration& config, 
                                             const DataModule* dataModule,
                                             std::vector<std::vector<float>>& leftAxisData,
                                             std::vector<std::vector<float>>& rightAxisData,
                                             std::vector<CurveDefinition>& leftAxisCurves,
                                             std::vector<CurveDefinition>& rightAxisCurves) const {
    // Очищаем выходные данные
    leftAxisData.clear();
    rightAxisData.clear();
    leftAxisCurves.clear();
    rightAxisCurves.clear();
    
    if (!dataModule || !dataModule->HasData()) {
        return;
    }
    
    // Разделяем кривые по осям
    for (const auto& curve : config.curves) {
        if (!curve.enabled) continue;
        
        std::vector<float> curveData;
        
        // Вычисляем данные в зависимости от типа кривой
        switch (curve.dataType) {
            case DataType::AMPLITUDE_RATIO:
                CalculateRatioData(curve.elementIndex, curve.frequency, dataModule, curveData);
                break;
            case DataType::PHASE_DIFFERENCE:
                CalculateDifferenceData(curve.elementIndex, curve.frequency, dataModule, curveData);
                break;
            case DataType::AMPLITUDE_RX1:
            case DataType::AMPLITUDE_RX2:
                {
                    int receiverIndex = (curve.dataType == DataType::AMPLITUDE_RX1) ? 0 : 1;
                    CalculateAmplitudeData(curve.elementIndex, receiverIndex, curve.frequency, dataModule, curveData);
                }
                break;
            case DataType::PHASE_RX1:
            case DataType::PHASE_RX2:
                {
                    int receiverIndex = (curve.dataType == DataType::PHASE_RX1) ? 0 : 1;
                    CalculatePhaseData(curve.elementIndex, receiverIndex, curve.frequency, dataModule, curveData);
                }
                break;
            case DataType::SILENT_AMPLITUDE_RX1:
                CalculateSilentAmplitudeData(0, curve.frequency, dataModule, curveData);
                break;
            case DataType::SILENT_AMPLITUDE_RX2:
                CalculateSilentAmplitudeData(1, curve.frequency, dataModule, curveData);
                break;
            default:
                continue; // Неподдерживаемый тип данных
        }
        
        // Добавляем данные к соответствующей оси
        if (curve.yAxisGroup == 0) {
            leftAxisData.push_back(curveData);
            leftAxisCurves.push_back(curve);
        } else {
            rightAxisData.push_back(curveData);
            rightAxisCurves.push_back(curve);
        }
    }
}

// Методы для добавления кривых различных типов
void CurveSelectionModule::AddTwoReceiversCurves(std::vector<CurveDefinition>& curves, 
                                                const SchematicSelection& selection) const {
    // Когда выбраны оба приемника (R1 и R2), доступны:
    // 1. Отношения амплитуд R2/R1 для каждого передатчика T1-T4
    // 2. Разности фаз R2-R1 для каждого передатчика T1-T4
    
    for (int i = 0; i < 4; i++) {
        // Отношение амплитуд (левая ось, единицы: В/В)
        CurveDefinition ratioCurve = CreateRatioCurve(i, selection.frequency);
        ratioCurve.yAxisGroup = 0; // Левая ось
        ratioCurve.enabled = true; // По умолчанию включена
        curves.push_back(ratioCurve);
        
        // Разность фаз (правая ось, единицы: градусы)
        CurveDefinition differenceCurve = CreateDifferenceCurve(i, selection.frequency);
        differenceCurve.yAxisGroup = 1; // Правая ось
        differenceCurve.enabled = true; // По умолчанию включена
        curves.push_back(differenceCurve);
    }
}

void CurveSelectionModule::AddSingleReceiverCurves(std::vector<CurveDefinition>& curves, 
                                                  const SchematicSelection& selection) const {
    // Определяем, какой приемник выбран
    int receiverIndex = -1;
    for (int i = 0; i < 2; i++) {
        if (selection.receivers[i]) {
            receiverIndex = i;
            break;
        }
    }
    
    if (receiverIndex == -1) return;
    
    // Когда выбран один приемник, доступны:
    // 1. Амплитуды для всех передатчиков T1-T4 (левая ось, Вольты)
    // 2. Фазы для всех передатчиков T1-T4 (правая ось, градусы)
    
    for (int i = 0; i < 4; i++) {
        // Амплитуда (левая ось)
        CurveDefinition amplitudeCurve = CreateAmplitudeCurve(i, receiverIndex, selection.frequency);
        amplitudeCurve.yAxisGroup = 0; // Левая ось
        amplitudeCurve.enabled = true; // По умолчанию включена
        curves.push_back(amplitudeCurve);
        
        // Фаза (правая ось)
        CurveDefinition phaseCurve = CreatePhaseCurve(i, receiverIndex, selection.frequency);
        phaseCurve.yAxisGroup = 1; // Правая ось
        phaseCurve.enabled = false; // По умолчанию отключена для избежания перегрузки
        curves.push_back(phaseCurve);
    }
}

void CurveSelectionModule::AddTransmitterCurves(std::vector<CurveDefinition>& curves, 
                                               const SchematicSelection& selection) const {
    // Когда выбраны передатчики, доступны амплитуды и фазы для R1 и R2
    // Создаем кривые только для выбранных передатчиков
    
    for (int txIndex = 0; txIndex < 4; txIndex++) {
        if (!selection.transmitters[txIndex]) continue; // Пропускаем невыбранные передатчики
        
        // Для каждого выбранного передатчика создаем кривые для обоих приемников
        for (int rxIndex = 0; rxIndex < 2; rxIndex++) {
            // Амплитуда (левая ось)
            CurveDefinition amplitudeCurve = CreateAmplitudeCurve(txIndex, rxIndex, selection.frequency);
            amplitudeCurve.yAxisGroup = 0; // Левая ось
            amplitudeCurve.enabled = true;
            
            // Обновляем отображаемое имя для ясности
            amplitudeCurve.displayName = "T" + std::to_string(txIndex + 1) + 
                                        " Amplitude R" + std::to_string(rxIndex + 1);
            curves.push_back(amplitudeCurve);
            
            // Фаза (правая ось)
            CurveDefinition phaseCurve = CreatePhaseCurve(txIndex, rxIndex, selection.frequency);
            phaseCurve.yAxisGroup = 1; // Правая ось
            phaseCurve.enabled = false; // По умолчанию отключена
            
            // Обновляем отображаемое имя для ясности
            phaseCurve.displayName = "T" + std::to_string(txIndex + 1) + 
                                    " Phase R" + std::to_string(rxIndex + 1);
            curves.push_back(phaseCurve);
        }
    }
}

void CurveSelectionModule::AddSilentTransmitterCurves(std::vector<CurveDefinition>& curves, 
                                                     const SchematicSelection& selection) const {
    // Молчащие передатчики предоставляют только амплитуды (для контроля качества)
    // Каждый молчащий передатчик соответствует определенному приемнику
    
    for (int i = 0; i < 2; i++) {
        if (selection.silentTx[i]) {
            CurveDefinition silentCurve = CreateSilentAmplitudeCurve(i, selection.frequency);
            silentCurve.yAxisGroup = 0; // Левая ось (амплитуды)
            silentCurve.enabled = true;
            silentCurve.color = RGB(128, 128, 128); // Серый цвет для молчащих
            
            // Обновляем отображаемое имя
            silentCurve.displayName = "Silent R" + std::to_string(i + 1) + " Amplitude";
            
            curves.push_back(silentCurve);
        }
    }
}

// Методы создания конкретных кривых
CurveDefinition CurveSelectionModule::CreateRatioCurve(int transmitterIndex, FrequencyBand frequency) const {
    std::string id = GenerateCurveId(ElementType::TRANSMITTER, transmitterIndex, 
                                    DataType::AMPLITUDE_RATIO, frequency);
    std::string name = GenerateDisplayName(ElementType::TRANSMITTER, transmitterIndex, 
                                          DataType::AMPLITUDE_RATIO);
    
    CurveDefinition curve(id, name, "V/V", ElementType::TRANSMITTER, transmitterIndex, 
                         DataType::AMPLITUDE_RATIO, frequency);
    curve.color = GetCurveColor(transmitterIndex);
    curve.yAxisGroup = 0; // Левая ось по умолчанию
    curve.lineStyle = GetLineStyle(curve.yAxisGroup);
    
    return curve;
}

CurveDefinition CurveSelectionModule::CreateDifferenceCurve(int transmitterIndex, FrequencyBand frequency) const {
    std::string id = GenerateCurveId(ElementType::TRANSMITTER, transmitterIndex, 
                                    DataType::PHASE_DIFFERENCE, frequency);
    std::string name = GenerateDisplayName(ElementType::TRANSMITTER, transmitterIndex, 
                                          DataType::PHASE_DIFFERENCE);
    
    CurveDefinition curve(id, name, "deg", ElementType::TRANSMITTER, transmitterIndex, 
                         DataType::PHASE_DIFFERENCE, frequency);
    curve.color = GetCurveColor(transmitterIndex);
    curve.yAxisGroup = 1; // Правая ось для фаз
    curve.lineStyle = GetLineStyle(curve.yAxisGroup);
    
    return curve;
}

CurveDefinition CurveSelectionModule::CreateAmplitudeCurve(int transmitterIndex, int receiverIndex, 
                                                          FrequencyBand frequency) const {
    DataType dataType = (receiverIndex == 0) ? DataType::AMPLITUDE_RX1 : DataType::AMPLITUDE_RX2;
    std::string id = GenerateCurveId(ElementType::TRANSMITTER, transmitterIndex, dataType, frequency, receiverIndex);
    std::string name = GenerateDisplayName(ElementType::TRANSMITTER, transmitterIndex, dataType, receiverIndex);
    
    CurveDefinition curve(id, name, "V", ElementType::TRANSMITTER, transmitterIndex, dataType, frequency);
    curve.color = GetCurveColor(transmitterIndex);
    curve.yAxisGroup = 0; // Левая ось для амплитуд
    curve.lineStyle = GetLineStyle(curve.yAxisGroup);
    
    return curve;
}

CurveDefinition CurveSelectionModule::CreatePhaseCurve(int transmitterIndex, int receiverIndex, 
                                                      FrequencyBand frequency) const {
    DataType dataType = (receiverIndex == 0) ? DataType::PHASE_RX1 : DataType::PHASE_RX2;
    std::string id = GenerateCurveId(ElementType::TRANSMITTER, transmitterIndex, dataType, frequency, receiverIndex);
    std::string name = GenerateDisplayName(ElementType::TRANSMITTER, transmitterIndex, dataType, receiverIndex);
    
    CurveDefinition curve(id, name, "deg", ElementType::TRANSMITTER, transmitterIndex, dataType, frequency);
    curve.color = GetCurveColor(transmitterIndex);
    curve.yAxisGroup = 1; // Правая ось для фаз
    curve.lineStyle = GetLineStyle(curve.yAxisGroup);
    
    return curve;
}

CurveDefinition CurveSelectionModule::CreateSilentAmplitudeCurve(int receiverIndex, FrequencyBand frequency) const {
    DataType dataType = (receiverIndex == 0) ? DataType::SILENT_AMPLITUDE_RX1 : DataType::SILENT_AMPLITUDE_RX2;
    std::string id = GenerateCurveId(ElementType::SILENT_TX, receiverIndex, dataType, frequency);
    std::string name = GenerateDisplayName(ElementType::SILENT_TX, receiverIndex, dataType);
    
    CurveDefinition curve(id, name, "V", ElementType::SILENT_TX, receiverIndex, dataType, frequency);
    curve.color = RGB(128, 128, 128); // Серый цвет для молчащих
    curve.yAxisGroup = 0; // Левая ось для амплитуд
    curve.lineStyle = GetLineStyle(curve.yAxisGroup);
    
    return curve;
}

// Вспомогательные методы
COLORREF CurveSelectionModule::GetCurveColor(int transmitterIndex) const {
    if (transmitterIndex >= 0 && transmitterIndex < 4) {
        return CURVE_COLORS[transmitterIndex];
    }
    return RGB(0, 0, 0); // Черный по умолчанию
}

int CurveSelectionModule::GetLineStyle(int yAxisGroup) const {
    return (yAxisGroup == 0) ? PS_SOLID : PS_DASH;
}

// Методы создания пресетов
ChartConfiguration CurveSelectionModule::CreateAllAmplitudesR1Preset(FrequencyBand frequency) const {
    ChartConfiguration config;
    
    // Создаем кривые амплитуд для всех передатчиков T1-T4 на приемнике R1
    for (int i = 0; i < 4; i++) {
        CurveDefinition curve = CreateAmplitudeCurve(i, 0, frequency); // 0 = R1
        curve.yAxisGroup = 0; // Левая ось
        curve.enabled = true;
        config.AddCurve(curve);
    }
    
    config.leftAxisUnits = "V";
    config.hasDualAxis = false;
    
    return config;
}

ChartConfiguration CurveSelectionModule::CreateAllAmplitudesR2Preset(FrequencyBand frequency) const {
    ChartConfiguration config;
    
    // Создаем кривые амплитуд для всех передатчиков T1-T4 на приемнике R2
    for (int i = 0; i < 4; i++) {
        CurveDefinition curve = CreateAmplitudeCurve(i, 1, frequency); // 1 = R2
        curve.yAxisGroup = 0; // Левая ось
        curve.enabled = true;
        config.AddCurve(curve);
    }
    
    config.leftAxisUnits = "V";
    config.hasDualAxis = false;
    
    return config;
}

ChartConfiguration CurveSelectionModule::CreateAllPhasesR1Preset(FrequencyBand frequency) const {
    ChartConfiguration config;
    
    // Создаем кривые фаз для всех передатчиков T1-T4 на приемнике R1
    for (int i = 0; i < 4; i++) {
        CurveDefinition curve = CreatePhaseCurve(i, 0, frequency); // 0 = R1
        curve.yAxisGroup = 0; // Левая ось (для фаз тоже можно использовать левую)
        curve.enabled = true;
        config.AddCurve(curve);
    }
    
    config.leftAxisUnits = "deg";
    config.hasDualAxis = false;
    
    return config;
}

ChartConfiguration CurveSelectionModule::CreateAllPhasesR2Preset(FrequencyBand frequency) const {
    ChartConfiguration config;
    
    // Создаем кривые фаз для всех передатчиков T1-T4 на приемнике R2
    for (int i = 0; i < 4; i++) {
        CurveDefinition curve = CreatePhaseCurve(i, 1, frequency); // 1 = R2
        curve.yAxisGroup = 0; // Левая ось
        curve.enabled = true;
        config.AddCurve(curve);
    }
    
    config.leftAxisUnits = "deg";
    config.hasDualAxis = false;
    
    return config;
}

ChartConfiguration CurveSelectionModule::CreateAmplitudeRatiosPreset(FrequencyBand frequency) const {
    ChartConfiguration config;
    
    // Создаем кривые отношений амплитуд для всех передатчиков T1-T4
    for (int i = 0; i < 4; i++) {
        CurveDefinition curve = CreateRatioCurve(i, frequency);
        curve.yAxisGroup = 0; // Левая ось
        curve.enabled = true;
        config.AddCurve(curve);
    }
    
    config.leftAxisUnits = "V/V";
    config.hasDualAxis = false;
    
    return config;
}

ChartConfiguration CurveSelectionModule::CreatePhaseDifferencesPreset(FrequencyBand frequency) const {
    ChartConfiguration config;
    
    // Создаем кривые разностей фаз для всех передатчиков T1-T4
    for (int i = 0; i < 4; i++) {
        CurveDefinition curve = CreateDifferenceCurve(i, frequency);
        curve.yAxisGroup = 0; // Левая ось
        curve.enabled = true;
        config.AddCurve(curve);
    }
    
    config.leftAxisUnits = "deg";
    config.hasDualAxis = false;
    
    return config;
}

// Методы вычисления данных (заглушки)
void CurveSelectionModule::CalculateRatioData(int transmitterIndex, FrequencyBand frequency,
                                             const DataModule* dataModule, std::vector<float>& outData) const {
    if (!dataModule || !dataModule->HasData()) {
        outData.clear();
        return;
    }
    
    // Используем полный диапазон данных
    double minX = 0.0;
    double maxX = static_cast<double>(dataModule->GetDataSize() - 1);
    
    // Вызываем метод DataModule для вычисления отношений амплитуд
    dataModule->CalculateRatioData(transmitterIndex, frequency, outData, minX, maxX);
}

void CurveSelectionModule::CalculateDifferenceData(int transmitterIndex, FrequencyBand frequency,
                                                  const DataModule* dataModule, std::vector<float>& outData) const {
    if (!dataModule || !dataModule->HasData()) {
        outData.clear();
        return;
    }
    
    // Используем полный диапазон данных
    double minX = 0.0;
    double maxX = static_cast<double>(dataModule->GetDataSize() - 1);
    
    // Вызываем метод DataModule для вычисления разностей фаз
    dataModule->CalculateDifferenceData(transmitterIndex, frequency, outData, minX, maxX);
}

void CurveSelectionModule::CalculateAmplitudeData(int transmitterIndex, int receiverIndex, FrequencyBand frequency,
                                                 const DataModule* dataModule, std::vector<float>& outData) const {
    if (!dataModule || !dataModule->HasData()) {
        outData.clear();
        return;
    }
    
    // Используем полный диапазон данных
    double minX = 0.0;
    double maxX = static_cast<double>(dataModule->GetDataSize() - 1);
    
    // Вызываем метод DataModule для вычисления амплитуд
    dataModule->CalculateAmplitudeData(transmitterIndex, receiverIndex, frequency, outData, minX, maxX);
}

void CurveSelectionModule::CalculatePhaseData(int transmitterIndex, int receiverIndex, FrequencyBand frequency,
                                             const DataModule* dataModule, std::vector<float>& outData) const {
    if (!dataModule || !dataModule->HasData()) {
        outData.clear();
        return;
    }
    
    // Используем полный диапазон данных
    double minX = 0.0;
    double maxX = static_cast<double>(dataModule->GetDataSize() - 1);
    
    // Вызываем метод DataModule для вычисления фаз
    dataModule->CalculatePhaseData(transmitterIndex, receiverIndex, frequency, outData, minX, maxX);
}

void CurveSelectionModule::CalculateSilentAmplitudeData(int receiverIndex, FrequencyBand frequency,
                                                       const DataModule* dataModule, std::vector<float>& outData) const {
    if (!dataModule || !dataModule->HasData()) {
        outData.clear();
        return;
    }
    
    // Используем полный диапазон данных
    double minX = 0.0;
    double maxX = static_cast<double>(dataModule->GetDataSize() - 1);
    
    // Вызываем метод DataModule для вычисления амплитуд молчащих передатчиков
    dataModule->CalculateSilentAmplitudeData(receiverIndex, frequency, outData, minX, maxX);
}

// Генерация идентификаторов и имен
std::string CurveSelectionModule::GenerateCurveId(ElementType element, int elementIndex, 
                                                 DataType dataType, FrequencyBand frequency, 
                                                 int receiverIndex) const {
    std::ostringstream oss;
    
    switch (element) {
        case ElementType::TRANSMITTER:
            oss << "T" << (elementIndex + 1);
            break;
        case ElementType::RECEIVER:
            oss << "R" << (elementIndex + 1);
            break;
        case ElementType::SILENT_TX:
            oss << "SILENT_R" << (elementIndex + 1);
            break;
    }
    
    oss << "_" << GetDataTypeDisplayName(dataType);
    oss << "_" << GetFrequencyDisplayName(frequency);
    
    if (receiverIndex >= 0) {
        oss << "_R" << (receiverIndex + 1);
    }
    
    return oss.str();
}

std::string CurveSelectionModule::GenerateDisplayName(ElementType element, int elementIndex, 
                                                     DataType dataType, int receiverIndex) const {
    std::ostringstream oss;
    
    switch (element) {
        case ElementType::TRANSMITTER:
            oss << "T" << (elementIndex + 1) << " ";
            break;
        case ElementType::RECEIVER:
            oss << "R" << (elementIndex + 1) << " ";
            break;
        case ElementType::SILENT_TX:
            oss << "Silent R" << (elementIndex + 1) << " ";
            break;
    }
    
    oss << GetDataTypeDisplayName(dataType);
    
    if (receiverIndex >= 0) {
        oss << " R" << (receiverIndex + 1);
    }
    
    return oss.str();
}

std::string CurveSelectionModule::GetDataTypeDisplayName(DataType dataType) const {
    switch (dataType) {
        case DataType::AMPLITUDE_RX1:
        case DataType::AMPLITUDE_RX2:
        case DataType::SILENT_AMPLITUDE_RX1:
        case DataType::SILENT_AMPLITUDE_RX2:
            return "Amplitude";
        case DataType::PHASE_RX1:
        case DataType::PHASE_RX2:
            return "Phase";
        case DataType::AMPLITUDE_RATIO:
            return "Amplitude Ratio";
        case DataType::PHASE_DIFFERENCE:
            return "Phase Difference";
        default:
            return "Unknown";
    }
}

std::string CurveSelectionModule::GetFrequencyDisplayName(FrequencyBand frequency) const {
    switch (frequency) {
        case FrequencyBand::FREQ_400_KHZ:
            return "400 kHz";
        case FrequencyBand::FREQ_2000_KHZ:
            return "2000 kHz";
        default:
            return "Unknown";
    }
}

std::string CurveSelectionModule::GetUnitsForDataType(DataType dataType) const {
    switch (dataType) {
        case DataType::AMPLITUDE_RX1:
        case DataType::AMPLITUDE_RX2:
        case DataType::SILENT_AMPLITUDE_RX1:
        case DataType::SILENT_AMPLITUDE_RX2:
            return "V";
        case DataType::PHASE_RX1:
        case DataType::PHASE_RX2:
        case DataType::PHASE_DIFFERENCE:
            return "deg";
        case DataType::AMPLITUDE_RATIO:
            return "V/V";
        default:
            return "";
    }
}

// Реализация вспомогательных функций CurveUtils
namespace CurveUtils {
    bool CanUseDualAxis(const std::vector<CurveDefinition>& curves) {
        // TODO: Реализовать проверку совместимости для двойных осей
        return false;
    }
    
    void GroupCurvesByAxis(const std::vector<CurveDefinition>& curves,
                          std::vector<CurveDefinition>& leftAxisCurves,
                          std::vector<CurveDefinition>& rightAxisCurves) {
        leftAxisCurves.clear();
        rightAxisCurves.clear();
        
        for (const auto& curve : curves) {
            if (curve.yAxisGroup == 0) {
                leftAxisCurves.push_back(curve);
            } else {
                rightAxisCurves.push_back(curve);
            }
        }
    }
    
    std::string DetermineAxisUnits(const std::vector<CurveDefinition>& axisCurves) {
        if (!axisCurves.empty()) {
            return axisCurves[0].units;
        }
        return "";
    }
    
    bool ValidateCurveConfiguration(const std::vector<CurveDefinition>& curves, 
                                   std::string& errorMessage) {
        if (curves.empty()) {
            errorMessage = "No curves to validate";
            return false;
        }
        
        // Проверяем совместимость единиц измерения на одной оси
        std::string leftAxisUnits, rightAxisUnits;
        bool hasLeftAxis = false, hasRightAxis = false;
        
        for (const auto& curve : curves) {
            if (!curve.enabled) continue;
            
            if (curve.yAxisGroup == 0) {
                if (!hasLeftAxis) {
                    leftAxisUnits = curve.units;
                    hasLeftAxis = true;
                } else if (leftAxisUnits != curve.units) {
                    errorMessage = "Incompatible units on left axis";
                    return false;
                }
            } else if (curve.yAxisGroup == 1) {
                if (!hasRightAxis) {
                    rightAxisUnits = curve.units;
                    hasRightAxis = true;
                } else if (rightAxisUnits != curve.units) {
                    errorMessage = "Incompatible units on right axis";
                    return false;
                }
            }
        }
        
        errorMessage.clear();
        return true;
    }
}

// Вспомогательные методы CurveSelectionModule
void CurveSelectionModule::SortCurvesByType(std::vector<CurveDefinition>& curves) const {
    // Сортируем кривые для удобного отображения:
    // 1. Сначала по типу данных (амплитуды, затем фазы)
    // 2. Затем по индексу передатчика (T1, T2, T3, T4)
    // 3. Затем по приемнику (R1, R2)
    
    std::sort(curves.begin(), curves.end(), [this](const CurveDefinition& a, const CurveDefinition& b) {
        // Приоритет типов данных
        int aPriority = GetDataTypePriority(a.dataType);
        int bPriority = GetDataTypePriority(b.dataType);
        
        if (aPriority != bPriority) {
            return aPriority < bPriority;
        }
        
        // Если типы одинаковые, сортируем по индексу элемента
        if (a.elementIndex != b.elementIndex) {
            return a.elementIndex < b.elementIndex;
        }
        
        // Если индексы одинаковые, сортируем по частоте
        return static_cast<int>(a.frequency) < static_cast<int>(b.frequency);
    });
}

int CurveSelectionModule::GetDataTypePriority(DataType dataType) const {
    // Определяем приоритет для сортировки
    switch (dataType) {
        case DataType::AMPLITUDE_RX1:
        case DataType::AMPLITUDE_RX2:
        case DataType::SILENT_AMPLITUDE_RX1:
        case DataType::SILENT_AMPLITUDE_RX2:
        case DataType::AMPLITUDE_RATIO:
            return 1; // Амплитуды первыми
            
        case DataType::PHASE_RX1:
        case DataType::PHASE_RX2:
        case DataType::PHASE_DIFFERENCE:
            return 2; // Фазы вторыми
            
        case DataType::TEMPERATURE:
            return 3; // Температура последней
            
        default:
            return 999; // Неизвестные типы в конце
    }
}

// Вспомогательные функции для работы с кодировкой
namespace {
    // Конвертация из UTF-8 в широкую строку для отображения
    std::wstring Utf8ToWide(const std::string& utf8str) {
        if (utf8str.empty()) return std::wstring();
        
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &utf8str[0], (int)utf8str.size(), NULL, 0);
        std::wstring wstrTo(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &utf8str[0], (int)utf8str.size(), &wstrTo[0], size_needed);
        return wstrTo;
    }
    
    // Конвертация из широкой строки в UTF-8
    std::string WideToUtf8(const std::wstring& wstr) {
        if (wstr.empty()) return std::string();
        
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
        return strTo;
    }
}

// Вспомогательные функции для конвертации кодировки (если понадобятся)
std::wstring Utf8ToWide(const std::string& utf8str) {
    if (utf8str.empty()) return std::wstring();
    
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &utf8str[0], (int)utf8str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &utf8str[0], (int)utf8str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}



