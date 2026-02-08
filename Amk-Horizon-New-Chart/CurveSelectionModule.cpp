#include "framework.h"
#include "CurveSelectionModule.h"
#include <algorithm>
#include <sstream>

// Определение стандартных цветов для кривых T1-T5
// Цвета для левой оси (приоритетные цвета для T1-T5)
const COLORREF CurveSelectionModule::LEFT_AXIS_COLORS[5] = {
    RGB(0, 128, 0),    // T1 - Зелёный (привычный цвет)
    RGB(139, 69, 19),  // T2 - Коричневый (отличная видимость и контраст)
    RGB(255, 0, 0),    // T3 - Красный (привычный цвет)
    RGB(0, 0, 255),    // T4 - Синий (привычный цвет)
    RGB(255, 20, 147)  // T5 - Розовый
};

// Цвета для правой оси (альтернативные цвета для T1-T5)
const COLORREF CurveSelectionModule::RIGHT_AXIS_COLORS[5] = {
    RGB(0, 100, 100),  // T1 - Тёмно-бирюзовый
    RGB(139, 69, 19),  // T2 - Коричневый
    RGB(128, 0, 128),  // T3 - Фиолетовый
    RGB(255, 20, 147), // T4 - Розовый
    RGB(255, 140, 0)   // T5 - Тёмно-оранжевый
};

// Специальные цвета для других типов данных
const COLORREF CurveSelectionModule::SILENT_COLOR = RGB(128, 0, 128);  // Фиолетовый для молчащих
const COLORREF CurveSelectionModule::TEMPERATURE_COLOR = RGB(255, 165, 0); // Оранжевый для температуры

// Расширенная палитра для умного назначения цветов (12 уникальных цветов)
const COLORREF CurveSelectionModule::EXTENDED_PALETTE[12] = {
    RGB(0, 128, 0),      // 0 - Зелёный (T1 приоритет)
    RGB(139, 69, 19),    // 1 - Коричневый (T2 приоритет)
    RGB(255, 0, 0),      // 2 - Красный (T3 приоритет)
    RGB(0, 0, 255),      // 3 - Синий (T4 приоритет)
    RGB(0, 100, 100),    // 4 - Тёмно-бирюзовый
    RGB(139, 69, 19),    // 5 - Коричневый
    RGB(128, 0, 128),    // 6 - Фиолетовый
    RGB(255, 20, 147),   // 7 - Розовый
    RGB(255, 140, 0),    // 8 - Тёмно-оранжевый
    RGB(0, 128, 128),    // 9 - Бирюзовый
    RGB(128, 128, 0),    // 10 - Оливковый
    RGB(75, 0, 130)      // 11 - Индиго
};

// Старый массив для обратной совместимости (теперь не используется)
const COLORREF CurveSelectionModule::CURVE_COLORS[5] = {
    RGB(0, 128, 0),    // T1 - Зелёный
    RGB(139, 69, 19),  // T2 - Коричневый
    RGB(255, 0, 0),    // T3 - Красный
    RGB(0, 0, 255),    // T4 - Синий
    RGB(255, 20, 147)  // T5 - Розовый
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
    
    // РЕЖИМ ПРЕСЕТА: Если dataFilter != ALL, генерируем кривые на основе фильтра
    // Элементы схемы не используются (все false)
    if (selection.dataFilter != DataTypeFilter::ALL) {
        // Определяем, какой метод вызвать на основе фильтра
        if (selection.dataFilter == DataTypeFilter::RATIO_ONLY || 
            selection.dataFilter == DataTypeFilter::DIFFERENCE_ONLY) {
            // Отношения амплитуд или разности фаз -> используем логику для двух приемников
            AddTwoReceiversCurves(curves, selection);
        }
        else if (selection.dataFilter == DataTypeFilter::AMPLITUDE_ONLY || 
                 selection.dataFilter == DataTypeFilter::PHASE_ONLY) {
            // Амплитуды или фазы -> используем логику для одного приемника
            // Нужно определить, какой приемник (по умолчанию R1, индекс 0)
            AddSingleReceiverCurves(curves, selection);
        }
        else if (selection.dataFilter == DataTypeFilter::TEMPERATURE_ONLY) {
            // Температура -> создаём одну кривую температуры
            CurveDefinition tempCurve;
            tempCurve.id = "TEMPERATURE";
            // Локализованное отображаемое имя, единицы остаются ASCII (degC)
            tempCurve.displayName = "Температура";
            tempCurve.units = "degC";  // Используем ASCII-совместимое обозначение
            tempCurve.yAxisGroup = 0;  // Левая ось
            tempCurve.enabled = true;
            tempCurve.color = TEMPERATURE_COLOR;  // Оранжевый цвет
            tempCurve.lineStyle = PS_SOLID;
            tempCurve.sourceElement = ElementType::TRANSMITTER;  // Не используется для температуры
            tempCurve.elementIndex = 0;  // Не используется для температуры
            tempCurve.dataType = DataType::TEMPERATURE;
            tempCurve.frequency = selection.frequency;
            curves.push_back(tempCurve);
        }
    }
    // РЕЖИМ РУЧНОГО ВЫБОРА: Если dataFilter == ALL, используем выбранные элементы схемы
    else {
        // ИСПРАВЛЕНО: Поддержка комбинаций элементов
        // Логика определения доступных кривых на основе выбора элементов
        if (selectedReceivers == 2) {
            // Два приемника -> отношения амплитуд и разности фаз (5 кривых T1-T5)
            AddTwoReceiversCurves(curves, selection);
        }
        else if (selectedReceivers == 1) {
            // Один приемник -> амплитуды и фазы для T1-T5
            AddSingleReceiverCurves(curves, selection);
        }
        
        // ИСПРАВЛЕНО: Используем отдельные if вместо else if для поддержки комбинаций
        if (selectedTransmitters > 0) {
            // Выбранные передатчики -> амплитуды и фазы для R1 и R2
            AddTransmitterCurves(curves, selection);
        }
        
        if (selectedSilent > 0) {
            // Молчащие передатчики -> только амплитуды для соответствующих приемников
            AddSilentTransmitterCurves(curves, selection);
        }
    }
    
    // ИЗМЕНЕНО v2.6.2: Отключена сортировка, чтобы сохранить порядок добавления кривых
    // Теперь фазы добавляются первыми (более важны), затем амплитуды
    // SortCurvesByType(curves);
    
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
    // В режиме выбора двух приёмников (R1+R2) список может включать 10 кривых (T1-T5)
    // для разности фаз и отношения амплитуд.
    if (enabledCurvesCount > 12) {
        return "Too many curves for display (maximum 12)";
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
    
    // НОВОЕ v2.6.3: Нормализация осей - если нет включённых кривых на левой оси,
    // но есть на правой, перемещаем правые кривые на левую ось
    bool hasEnabledLeftAxis = false;
    bool hasEnabledRightAxis = false;
    
    for (const auto& curve : config.curves) {
        if (!curve.enabled) continue;
        if (curve.yAxisGroup == 0) hasEnabledLeftAxis = true;
        if (curve.yAxisGroup == 1) hasEnabledRightAxis = true;
    }
    
    // Если есть только кривые на правой оси (фазы без амплитуд), перемещаем их на левую
    bool normalizeAxes = (!hasEnabledLeftAxis && hasEnabledRightAxis);
    
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
            case DataType::PHASE_SMT:
                CalculatePhaseData(curve.elementIndex, 0, curve.frequency, dataModule, curveData);
                break;
            case DataType::RHO_SMT:
                {
                    const auto& data = dataModule->GetData();
                    curveData.reserve(data.size());
                    const int freqIdx = (curve.frequency == FrequencyBand::FREQ_400_KHZ) ? 0 : 1;
                    for (const auto& record : data) {
                        curveData.push_back(record.rho_smt[freqIdx][curve.elementIndex]);
                    }
                }
                break;
            case DataType::SILENT_AMPLITUDE_RX1:
                CalculateSilentAmplitudeData(0, curve.frequency, dataModule, curveData);
                break;
            case DataType::SILENT_AMPLITUDE_RX2:
                CalculateSilentAmplitudeData(1, curve.frequency, dataModule, curveData);
                break;
            case DataType::TEMPERATURE:
                // Температура - извлекаем напрямую из данных
                {
                    const auto& data = dataModule->GetData();
                    curveData.reserve(data.size());
                    for (const auto& record : data) {
                        curveData.push_back(record.temperature);
                    }
                }
                break;
            default:
                continue; // Неподдерживаемый тип данных
        }
        
        // Добавляем данные к соответствующей оси
        // НОВОЕ v2.6.3: Если нормализация включена, все кривые идут на левую ось
        int effectiveAxisGroup = normalizeAxes ? 0 : curve.yAxisGroup;
        
        if (effectiveAxisGroup == 0) {
            leftAxisData.push_back(curveData);
            // Создаём копию кривой с исправленным yAxisGroup для корректного отображения
            CurveDefinition normalizedCurve = curve;
            normalizedCurve.yAxisGroup = 0;
            normalizedCurve.lineStyle = PS_SOLID; // Сплошная линия для левой оси
            leftAxisCurves.push_back(normalizedCurve);
        } else {
            rightAxisData.push_back(curveData);
            rightAxisCurves.push_back(curve);
        }
    }
}

// Перегрузка с поддержкой zoom
void CurveSelectionModule::CalculateCurveData(const ChartConfiguration& config, 
                                             const DataModule* dataModule,
                                             std::vector<std::vector<float>>& leftAxisData,
                                             std::vector<std::vector<float>>& rightAxisData,
                                             std::vector<CurveDefinition>& leftAxisCurves,
                                             std::vector<CurveDefinition>& rightAxisCurves,
                                             double minX, double maxX, double minY, double maxY) const {
    // Очищаем выходные данные
    leftAxisData.clear();
    rightAxisData.clear();
    leftAxisCurves.clear();
    rightAxisCurves.clear();
    
    if (!dataModule || !dataModule->HasData()) {
        return;
    }
    
    // НОВОЕ v2.6.3: Нормализация осей - если нет включённых кривых на левой оси,
    // но есть на правой, перемещаем правые кривые на левую ось
    bool hasEnabledLeftAxis = false;
    bool hasEnabledRightAxis = false;
    
    for (const auto& curve : config.curves) {
        if (!curve.enabled) continue;
        if (curve.yAxisGroup == 0) hasEnabledLeftAxis = true;
        if (curve.yAxisGroup == 1) hasEnabledRightAxis = true;
    }
    
    // Если есть только кривые на правой оси (фазы без амплитуд), перемещаем их на левую
    bool normalizeAxes = (!hasEnabledLeftAxis && hasEnabledRightAxis);
    
    // Разделяем кривые по осям
    for (const auto& curve : config.curves) {
        if (!curve.enabled) continue;
        
        std::vector<float> curveData;
        
        // Вычисляем данные в зависимости от типа кривой с учетом zoom
        switch (curve.dataType) {
            case DataType::AMPLITUDE_RATIO:
                dataModule->CalculateRatioData(curve.elementIndex, curve.frequency, curveData, minX, maxX);
                break;
            case DataType::PHASE_DIFFERENCE:
                dataModule->CalculateDifferenceData(curve.elementIndex, curve.frequency, curveData, minX, maxX);
                break;
            case DataType::AMPLITUDE_RX1:
            case DataType::AMPLITUDE_RX2:
                {
                    int receiverIndex = (curve.dataType == DataType::AMPLITUDE_RX1) ? 0 : 1;
                    dataModule->CalculateAmplitudeData(curve.elementIndex, receiverIndex, curve.frequency, curveData, minX, maxX);
                }
                break;
            case DataType::PHASE_SMT:
                dataModule->CalculatePhaseData(curve.elementIndex, 0, curve.frequency, curveData, minX, maxX);
                break;
            case DataType::RHO_SMT:
                {
                    const auto& data = dataModule->GetData();
                    size_t startIndex = static_cast<size_t>(std::max(0.0, minX));
                    size_t endIndex = static_cast<size_t>(std::min(static_cast<double>(data.size()), maxX + 1));

                    curveData.reserve(endIndex - startIndex);
                    const int freqIdx = (curve.frequency == FrequencyBand::FREQ_400_KHZ) ? 0 : 1;
                    for (size_t i = startIndex; i < endIndex; ++i) {
                        curveData.push_back(data[i].rho_smt[freqIdx][curve.elementIndex]);
                    }
                }
                break;
            case DataType::SILENT_AMPLITUDE_RX1:
                dataModule->CalculateSilentAmplitudeData(0, curve.frequency, curveData, minX, maxX);
                break;
            case DataType::SILENT_AMPLITUDE_RX2:
                dataModule->CalculateSilentAmplitudeData(1, curve.frequency, curveData, minX, maxX);
                break;
            case DataType::TEMPERATURE:
                // Температура - извлекаем с учётом диапазона
                {
                    const auto& data = dataModule->GetData();
                    size_t startIndex = static_cast<size_t>(std::max(0.0, minX));
                    size_t endIndex = static_cast<size_t>(std::min(static_cast<double>(data.size()), maxX + 1));
                    
                    curveData.reserve(endIndex - startIndex);
                    for (size_t i = startIndex; i < endIndex; ++i) {
                        curveData.push_back(data[i].temperature);
                    }
                }
                break;
            default:
                continue; // Неподдерживаемый тип данных
        }
        
        // Добавляем данные к соответствующей оси
        // НОВОЕ v2.6.3: Если нормализация включена, все кривые идут на левую ось
        int effectiveAxisGroup = normalizeAxes ? 0 : curve.yAxisGroup;
        
        if (effectiveAxisGroup == 0) {
            leftAxisData.push_back(curveData);
            // Создаём копию кривой с исправленным yAxisGroup для корректного отображения
            CurveDefinition normalizedCurve = curve;
            normalizedCurve.yAxisGroup = 0;
            normalizedCurve.lineStyle = PS_SOLID; // Сплошная линия для левой оси
            leftAxisCurves.push_back(normalizedCurve);
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
    // 1. Разности фаз R2-R1 для каждого передатчика T1-T5 (СНАЧАЛА - более важны)
    // 2. Отношения амплитуд R2/R1 для каждого передатчика T1-T5 (ПОТОМ)
    
    // Проверяем фильтр типов данных
    bool addRatios = (selection.dataFilter == DataTypeFilter::ALL || 
                      selection.dataFilter == DataTypeFilter::RATIO_ONLY);
    bool addDifferences = (selection.dataFilter == DataTypeFilter::ALL || 
                           selection.dataFilter == DataTypeFilter::DIFFERENCE_ONLY);
    
    // v2.12.4: Определяем, односортные ли данные (только один тип)
    // Если данные односортные, все кривые идут на левую ось
    bool isSingleType = (addRatios && !addDifferences) || (!addRatios && addDifferences);
    
    // ИЗМЕНЕНО v2.7.0: Сначала добавляем разности фаз (более важны для анализа)
    for (int i = 0; i < 5; i++) {
        // Разность фаз (ЛЕВАЯ ось, единицы: миллиградусы)
        if (addDifferences) {
            CurveDefinition differenceCurve = CreateDifferenceCurve(i, selection.frequency);
            differenceCurve.yAxisGroup = 0; // ЛЕВАЯ ось всегда для фаз
            differenceCurve.enabled = true; // По умолчанию включена
            // Умное назначение цвета с учетом уже созданных кривых
            differenceCurve.color = AssignSmartColor(curves, i, differenceCurve.yAxisGroup);
            curves.push_back(differenceCurve);
        }
    }
    
    // ИЗМЕНЕНО v2.7.0 + v2.12.4: Потом добавляем отношения амплитуд
    for (int i = 0; i < 5; i++) {
        // Отношение амплитуд
        if (addRatios) {
            CurveDefinition ratioCurve = CreateRatioCurve(i, selection.frequency);
            // v2.12.4: Если данные односортные (только амплитуды), используем левую ось
            // Если данные разносортные (амплитуды + фазы), используем правую ось
            ratioCurve.yAxisGroup = isSingleType ? 0 : 1;
            ratioCurve.enabled = true; // По умолчанию включена
            // Умное назначение цвета с учетом уже созданных кривых
            ratioCurve.color = AssignSmartColor(curves, i, ratioCurve.yAxisGroup);
            curves.push_back(ratioCurve);
        }
    }
}

void CurveSelectionModule::AddSingleReceiverCurves(std::vector<CurveDefinition>& curves, 
                                                  const SchematicSelection& selection) const {
    // Определяем, какой приемник выбран
    int receiverIndex = -1;
    
    // Режим ручного выбора: ищем выбранный приемник
    for (int i = 0; i < 2; i++) {
        if (selection.receivers[i]) {
            receiverIndex = i;
            break;
        }
    }
    
    // Режим пресета: если приемник не выбран, используем R1 по умолчанию (индекс 0)
    // Это нужно для пресетов "All Ampl R1/R2" и "All Phase R1/R2"
    if (receiverIndex == -1 && selection.dataFilter != DataTypeFilter::ALL) {
        receiverIndex = 0; // По умолчанию R1
    }
    
    if (receiverIndex == -1) return;
    
    // Когда выбран один приемник, доступны:
    // 1. Фазы для всех передатчиков T1-T5 (ЛЕВАЯ ось, миллиградусы) - СНАЧАЛА (более важны)
    // 2. Амплитуды для всех передатчиков T1-T5 (ПРАВАЯ ось, милливольты) - ПОТОМ
    
    // Проверяем фильтр типов данных
    bool addAmplitudes = (selection.dataFilter == DataTypeFilter::ALL || 
                          selection.dataFilter == DataTypeFilter::AMPLITUDE_ONLY);
    bool addPhases = (selection.dataFilter == DataTypeFilter::ALL || 
                      selection.dataFilter == DataTypeFilter::PHASE_ONLY);
    
    // v2.12.4: Определяем, односортные ли данные (только один тип)
    // Если данные односортные, все кривые идут на левую ось
    bool isSingleType = (addAmplitudes && !addPhases) || (!addAmplitudes && addPhases);
    
    // ИЗМЕНЕНО v2.7.0: Сначала добавляем фазы (более важны для анализа)
    for (int i = 0; i < 5; i++) {
        // Разность фаз (ЛЕВАЯ ось)
        if (addPhases) {
            CurveDefinition phaseCurve = CreateDifferenceCurve(i, selection.frequency);
            phaseCurve.yAxisGroup = 0; // ЛЕВАЯ ось всегда для фаз
            phaseCurve.enabled = true; // Включена по умолчанию для пресетов
            // Умное назначение цвета с учетом уже созданных кривых
            phaseCurve.color = AssignSmartColor(curves, i, phaseCurve.yAxisGroup);
            curves.push_back(phaseCurve);
        }
    }
    
    // ИЗМЕНЕНО v2.7.0 + v2.12.4: Потом добавляем амплитуды
    for (int i = 0; i < 5; i++) {
        // Амплитуда
        if (addAmplitudes) {
            CurveDefinition amplitudeCurve = CreateAmplitudeCurve(i, receiverIndex, selection.frequency);
            // v2.12.4: Если данные односортные (только амплитуды), используем левую ось
            // Если данные разносортные (амплитуды + фазы), используем правую ось
            amplitudeCurve.yAxisGroup = isSingleType ? 0 : 1;
            amplitudeCurve.enabled = true; // По умолчанию включена
            // Умное назначение цвета с учетом уже созданных кривых
            amplitudeCurve.color = AssignSmartColor(curves, i, amplitudeCurve.yAxisGroup);
            curves.push_back(amplitudeCurve);
        }
    }
}

void CurveSelectionModule::AddTransmitterCurves(std::vector<CurveDefinition>& curves, 
                                               const SchematicSelection& selection) const {
    // Когда выбраны передатчики, доступны амплитуды и фазы для R1 и R2
    // Создаем кривые только для выбранных передатчиков
    // ИЗМЕНЕНО v2.6.5: Для каждого передатчика сначала все фазы, потом все амплитуды
    // ИЗМЕНЕНО v2.7.0: Фазы на ЛЕВОЙ оси, амплитуды на ПРАВОЙ оси
    
    for (int txIndex = 0; txIndex < 5; txIndex++) {
        if (!selection.transmitters[txIndex]) continue; // Пропускаем невыбранные передатчики

        // Разность фаз (ЛЕВАЯ ось) - разность фаз R1-R2, одна кривая на передатчик.
        CurveDefinition phaseCurve = CreateDifferenceCurve(txIndex, selection.frequency);
        phaseCurve.yAxisGroup = 0; // ЛЕВАЯ ось
        phaseCurve.enabled = true;
        phaseCurve.displayName = "T" + std::to_string(txIndex + 1) + " Разность фаз";
        phaseCurve.color = AssignSmartColor(curves, txIndex, phaseCurve.yAxisGroup);
        curves.push_back(phaseCurve);
        
        // ИЗМЕНЕНО v2.6.5: Потом добавляем все амплитуды для этого передатчика
        for (int rxIndex = 0; rxIndex < 2; rxIndex++) {
            // Амплитуда (ПРАВАЯ ось)
            CurveDefinition amplitudeCurve = CreateAmplitudeCurve(txIndex, rxIndex, selection.frequency);
            amplitudeCurve.yAxisGroup = 1; // ПРАВАЯ ось
            amplitudeCurve.enabled = true;
            
            // Обновляем отображаемое имя для ясности (локализовано)
            amplitudeCurve.displayName = "T" + std::to_string(txIndex + 1) +
                                        " Амплитуда R" + std::to_string(rxIndex + 1);
            
            // Умное назначение цвета с учетом уже созданных кривых
            amplitudeCurve.color = AssignSmartColor(curves, txIndex, amplitudeCurve.yAxisGroup);
            curves.push_back(amplitudeCurve);
        }
    }
}

void CurveSelectionModule::AddSilentTransmitterCurves(std::vector<CurveDefinition>& curves, 
                                                     const SchematicSelection& selection) const {
    // Молчащие передатчики предоставляют только амплитуды (для контроля качества)
    // Каждый молчащий передатчик соответствует определенному приемнику
    // ИЗМЕНЕНО v2.7.0: Если есть фазы на графике, амплитуды идут на правую ось
    
    // Проверяем, есть ли уже фазовые кривые
    bool hasPhases = false;
    for (const auto& curve : curves) {
        if (curve.dataType == DataType::PHASE_SMT || 
            curve.dataType == DataType::PHASE_DIFFERENCE) {
            hasPhases = true;
            break;
        }
    }
    
    for (int i = 0; i < 2; i++) {
        if (selection.silentTx[i]) {
            CurveDefinition silentCurve = CreateSilentAmplitudeCurve(i, selection.frequency);
            // ИЗМЕНЕНО v2.7.0: Правая ось если есть фазы, иначе левая
            silentCurve.yAxisGroup = hasPhases ? 1 : 0;
            silentCurve.enabled = true;
            
            // Используем умное назначение цвета вместо фиксированного фиолетового
            // Передаём индекс элемента (i) для выбора цвета из палитры
            silentCurve.color = AssignSmartColor(curves, i, silentCurve.yAxisGroup);
            
            // Обновляем отображаемое имя
            silentCurve.displayName = "Молчащий R" + std::to_string(i + 1) + " Амплитуда";
            
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
    
    CurveDefinition curve(id, name, "mV", ElementType::TRANSMITTER, transmitterIndex, 
                         DataType::AMPLITUDE_RATIO, frequency);
    curve.yAxisGroup = 1; // ИЗМЕНЕНО v2.7.0: Правая ось для амплитуд (отношений)
    curve.color = GetCurveColorForAxis(transmitterIndex, curve.yAxisGroup);
    curve.lineStyle = GetLineStyle(curve.yAxisGroup);
    
    return curve;
}

CurveDefinition CurveSelectionModule::CreateDifferenceCurve(int transmitterIndex, FrequencyBand frequency) const {
    std::string id = GenerateCurveId(ElementType::TRANSMITTER, transmitterIndex, 
                                    DataType::PHASE_DIFFERENCE, frequency);
    std::string name = GenerateDisplayName(ElementType::TRANSMITTER, transmitterIndex, 
                                          DataType::PHASE_DIFFERENCE);
    
    CurveDefinition curve(id, name, "mdeg", ElementType::TRANSMITTER, transmitterIndex, 
                         DataType::PHASE_DIFFERENCE, frequency);
    curve.yAxisGroup = 0; // ИЗМЕНЕНО v2.7.0: Левая ось для фаз
    curve.color = GetCurveColorForAxis(transmitterIndex, curve.yAxisGroup);
    curve.lineStyle = GetLineStyle(curve.yAxisGroup);
    
    return curve;
}

CurveDefinition CurveSelectionModule::CreateAmplitudeCurve(int transmitterIndex, int receiverIndex, 
                                                          FrequencyBand frequency) const {
    DataType dataType = (receiverIndex == 0) ? DataType::AMPLITUDE_RX1 : DataType::AMPLITUDE_RX2;
    std::string id = GenerateCurveId(ElementType::TRANSMITTER, transmitterIndex, dataType, frequency, receiverIndex);
    std::string name = GenerateDisplayName(ElementType::TRANSMITTER, transmitterIndex, dataType, receiverIndex);
    
    CurveDefinition curve(id, name, "mV", ElementType::TRANSMITTER, transmitterIndex, dataType, frequency);
    curve.yAxisGroup = 1; // ИЗМЕНЕНО v2.7.0: Правая ось для амплитуд
    curve.color = GetCurveColorForAxis(transmitterIndex, curve.yAxisGroup);
    curve.lineStyle = GetLineStyle(curve.yAxisGroup);
    
    return curve;
}

CurveDefinition CurveSelectionModule::CreatePhaseCurve(int transmitterIndex, int receiverIndex, 
                                                      FrequencyBand frequency) const {
    (void)receiverIndex;
    DataType dataType = DataType::PHASE_SMT;
    std::string id = GenerateCurveId(ElementType::TRANSMITTER, transmitterIndex, dataType, frequency);
    std::string name = GenerateDisplayName(ElementType::TRANSMITTER, transmitterIndex, dataType);
    
    CurveDefinition curve(id, name, "mdeg", ElementType::TRANSMITTER, transmitterIndex, dataType, frequency);
    curve.yAxisGroup = 0; // ИЗМЕНЕНО v2.7.0: Левая ось для фаз
    curve.color = GetCurveColorForAxis(transmitterIndex, curve.yAxisGroup);
    curve.lineStyle = GetLineStyle(curve.yAxisGroup);
    
    return curve;
}

CurveDefinition CurveSelectionModule::CreateSilentAmplitudeCurve(int receiverIndex, FrequencyBand frequency) const {
    DataType dataType = (receiverIndex == 0) ? DataType::SILENT_AMPLITUDE_RX1 : DataType::SILENT_AMPLITUDE_RX2;
    std::string id = GenerateCurveId(ElementType::SILENT_TX, receiverIndex, dataType, frequency);
    std::string name = GenerateDisplayName(ElementType::SILENT_TX, receiverIndex, dataType);
    
    CurveDefinition curve(id, name, "mV", ElementType::SILENT_TX, receiverIndex, dataType, frequency);
    curve.yAxisGroup = 1; // ИЗМЕНЕНО v2.7.0: Правая ось для амплитуд
    curve.color = GetSpecialDataColor(dataType); // Фиолетовый цвет для молчащих
    curve.lineStyle = GetLineStyle(curve.yAxisGroup);
    
    return curve;
}

// Вспомогательные методы для работы с цветами
COLORREF CurveSelectionModule::GetCurveColor(int transmitterIndex) const {
    // Старый метод для обратной совместимости - использует цвета левой оси
    return GetCurveColorForAxis(transmitterIndex, 0);
}

COLORREF CurveSelectionModule::GetCurveColorForAxis(int transmitterIndex, int yAxisGroup) const {
    if (transmitterIndex >= 0 && transmitterIndex < 5) {
        if (yAxisGroup == 0) {
            // Левая ось - приоритетные цвета для T1-T5
            return LEFT_AXIS_COLORS[transmitterIndex];
        } else {
            // Правая ось - альтернативные цвета для T1-T5
            return RIGHT_AXIS_COLORS[transmitterIndex];
        }
    }
    return RGB(0, 0, 0); // Черный по умолчанию
}

COLORREF CurveSelectionModule::GetSpecialDataColor(DataType dataType) const {
    switch (dataType) {
        case DataType::SILENT_AMPLITUDE_RX1:
        case DataType::SILENT_AMPLITUDE_RX2:
            return SILENT_COLOR;  // Фиолетовый для молчащих передатчиков
        case DataType::TEMPERATURE:
            return TEMPERATURE_COLOR;  // Оранжевый для температуры
        default:
            return RGB(128, 128, 128);  // Серый для неизвестных типов
    }
}

COLORREF CurveSelectionModule::AssignSmartColor(const std::vector<CurveDefinition>& existingCurves, 
                                                int transmitterIndex, int yAxisGroup) const {
    // Собираем уже использованные цвета на ОБЕИХ осях
    // ВАЖНО: Учитываем ВСЕ кривые (включенные и отключенные), чтобы избежать конфликтов
    // ВАЖНО: Учитываем цвета с обеих осей, чтобы избежать совпадений между осями
    std::vector<COLORREF> usedColors;
    for (const auto& curve : existingCurves) {
        // Собираем цвета со ВСЕХ осей, не только с текущей
        usedColors.push_back(curve.color);
    }
    
    // Проверяем, можно ли использовать приоритетный цвет для T1-T5 на левой оси
    if (yAxisGroup == 0 && transmitterIndex >= 0 && transmitterIndex < 5) {
        COLORREF priorityColor = LEFT_AXIS_COLORS[transmitterIndex];
        
        // Проверяем, не используется ли уже этот цвет
        bool colorUsed = false;
        for (COLORREF usedColor : usedColors) {
            if (usedColor == priorityColor) {
                colorUsed = true;
                break;
            }
        }
        
        // Если цвет свободен, используем приоритетный
        if (!colorUsed) {
            return priorityColor;
        }
    }
    
    // Если приоритетный цвет занят или это правая ось, ищем первый свободный из расширенной палитры
    for (int i = 0; i < 12; i++) {
        COLORREF candidateColor = EXTENDED_PALETTE[i];
        
        // Проверяем, не используется ли этот цвет НА ЛЮБОЙ ИЗ ОСЕЙ
        bool colorUsed = false;
        for (COLORREF usedColor : usedColors) {
            if (usedColor == candidateColor) {
                colorUsed = true;
                break;
            }
        }
        
        if (!colorUsed) {
            return candidateColor;
        }
    }
    
    // Если все цвета заняты (маловероятно), возвращаем цвет по модулю
    return EXTENDED_PALETTE[existingCurves.size() % 12];
}

int CurveSelectionModule::GetLineStyle(int yAxisGroup) const {
    // ИЗМЕНЕНО v2.7.0: Левая ось (фазы) - сплошная, правая ось (амплитуды) - пунктир
    return (yAxisGroup == 0) ? PS_SOLID : PS_DASH;
}

// Методы создания пресетов
ChartConfiguration CurveSelectionModule::CreateAllAmplitudesR1Preset(FrequencyBand frequency) const {
    ChartConfiguration config;
    
    // Создаем кривые амплитуд для всех передатчиков T1-T5 на приемнике R1
    for (int i = 0; i < 5; i++) {
        CurveDefinition curve = CreateAmplitudeCurve(i, 0, frequency); // 0 = R1
        curve.yAxisGroup = 0; // Левая ось
        curve.enabled = true;
        config.AddCurve(curve);
    }
    
    config.leftAxisUnits = "V";
    config.hasDualAxis = false;
    config.activePreset = PresetType::ALL_AMPLITUDES_R1;
    config.presetDisplayName = L"Амплитуда RX1";
    
    return config;
}

ChartConfiguration CurveSelectionModule::CreateAllAmplitudesR2Preset(FrequencyBand frequency) const {
    ChartConfiguration config;
    
    // Создаем кривые амплитуд для всех передатчиков T1-T5 на приемнике R2
    for (int i = 0; i < 5; i++) {
        CurveDefinition curve = CreateAmplitudeCurve(i, 1, frequency); // 1 = R2
        curve.yAxisGroup = 0; // Левая ось
        curve.enabled = true;
        config.AddCurve(curve);
    }
    
    config.leftAxisUnits = "V";
    config.hasDualAxis = false;
    config.activePreset = PresetType::ALL_AMPLITUDES_R2;
    config.presetDisplayName = L"Амплитуда RX2";
    
    return config;
}

ChartConfiguration CurveSelectionModule::CreateAmplitudeRatiosPreset(FrequencyBand frequency) const {
    ChartConfiguration config;
    
    // Создаем кривые отношений амплитуд для всех передатчиков T1-T5
    for (int i = 0; i < 5; i++) {
        CurveDefinition curve = CreateRatioCurve(i, frequency);
        curve.yAxisGroup = 0; // Левая ось
        curve.enabled = true;
        config.AddCurve(curve);
    }
    
    config.leftAxisUnits = "mV";
    config.hasDualAxis = false;
    config.activePreset = PresetType::AMPLITUDE_RATIOS;
    config.presetDisplayName = L"Отношение амплитуд";
    
    return config;
}

ChartConfiguration CurveSelectionModule::CreatePhaseDifferencesPreset(FrequencyBand frequency) const {
    ChartConfiguration config;
    
    // Создаем кривые разностей фаз для всех передатчиков T1-T5
    for (int i = 0; i < 5; i++) {
        CurveDefinition curve = CreateDifferenceCurve(i, frequency);
        curve.yAxisGroup = 0; // Левая ось
        curve.enabled = true;
        config.AddCurve(curve);
    }
    
    config.leftAxisUnits = "deg";
    config.hasDualAxis = false;
    config.activePreset = PresetType::PHASE_DIFFERENCES;
    config.presetDisplayName = L"Разность фаз";
    
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
            oss << "Молчащий R" << (elementIndex + 1) << " ";
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
            return "Амплитуда";
        case DataType::PHASE_SMT:
            return "Фаза";
        case DataType::AMPLITUDE_RATIO:
            return "Отношение амплитуд";
        case DataType::PHASE_DIFFERENCE:
            return "Разность фаз";
        case DataType::RHO_SMT:
            return "УЭС";
        case DataType::SILENT_AMPLITUDE_RX1:
        case DataType::SILENT_AMPLITUDE_RX2:
            return "Амплитуда";
        case DataType::TEMPERATURE:
            return "Температура";
        default:
            return "";
    }
}

std::string CurveSelectionModule::GetFrequencyDisplayName(FrequencyBand frequency) const {
    switch (frequency) {
        case FrequencyBand::FREQ_400_KHZ:
            return "400 кГц";
        case FrequencyBand::FREQ_2000_KHZ:
            return "2000 кГц";
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
        case DataType::PHASE_SMT:
        case DataType::PHASE_DIFFERENCE:
            return "deg";
        case DataType::AMPLITUDE_RATIO:
            return "";
        case DataType::TEMPERATURE:
            return "degC";
        case DataType::RHO_SMT:
            return "";
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
            
        case DataType::PHASE_SMT:
        case DataType::PHASE_DIFFERENCE:
            return 2; // Фазы вторыми

        case DataType::RHO_SMT:
            return 3;
            
        case DataType::TEMPERATURE:
            return 4; // Температура последней
            
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





