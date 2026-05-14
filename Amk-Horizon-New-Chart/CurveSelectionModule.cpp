#include "framework.h"
#include "CurveSelectionModule.h"
#include "ToolPresentation.h"
#include "ToolDataAccess.h"
#include <algorithm>
#include <array>
#include <sstream>
#include <cmath>
#include <complex>
#include <limits>

namespace {
constexpr float kMdegPerRad = 57295.7795f;
constexpr int kSmtMaxTransmitterCount = SchematicSelection::UI_TRANSMITTER_COUNT;
// Делитель для нормализации Air_zz (int16) к фазе в радианах (legacy: deg*100 -> rad)
constexpr float kAirZzPhaseDivisor = 57297.0f;

using SmtPhaseOutputs = std::array<float, kSmtMaxTransmitterCount>;
using SmtMatrix = std::array<std::array<float, kSmtMaxTransmitterCount>, kSmtMaxTransmitterCount>;

float InvertPhaseToRho(const METROLOGY_DATA& metrology, float targetPhase, int txIndex, int freqIdx);

int GetUiTransmitterCount(const std::vector<bool>* transmitters = nullptr) {
    const int transmittersCount = transmitters ? static_cast<int>(transmitters->size()) : 0;
    return (transmittersCount > 0) ? transmittersCount : SchematicSelection::UI_TRANSMITTER_COUNT;
}

bool IsUiTransmitterIndex(int transmitterIndex) {
    return transmitterIndex >= 0 && transmitterIndex < GetUiTransmitterCount();
}

bool IsBatchablePresetType(PresetType preset) {
    switch (preset) {
        case PresetType::ALL_AMPLITUDES_R1:
        case PresetType::ALL_AMPLITUDES_R2:
        case PresetType::AMPLITUDE_RATIOS:
        case PresetType::PHASE_DIFFERENCES:
        case PresetType::RHO_SMT:
        case PresetType::PHASE_SMT:
            return true;
        default:
            return false;
    }
}

bool TryResolveBatchedDataType(const ChartConfiguration& config, DataType& outDataType) {
    outDataType = DataType::TEMPERATURE;
    if (!config.IsPresetActive() || !IsBatchablePresetType(config.activePreset)) {
        return false;
    }

    switch (config.activePreset) {
        case PresetType::ALL_AMPLITUDES_R1:
            outDataType = DataType::AMPLITUDE_RX1;
            return true;
        case PresetType::ALL_AMPLITUDES_R2:
            outDataType = DataType::AMPLITUDE_RX2;
            return true;
        case PresetType::AMPLITUDE_RATIOS:
            outDataType = DataType::AMPLITUDE_RATIO;
            return true;
        case PresetType::PHASE_DIFFERENCES:
            outDataType = DataType::PHASE_DIFFERENCE;
            return true;
        case PresetType::RHO_SMT:
            outDataType = DataType::RHO_SMT;
            return true;
        case PresetType::PHASE_SMT:
            outDataType = DataType::PHASE_SMT;
            return true;
        default:
            return false;
    }
}

bool TryBuildBatchedPresetCurveData(const ChartConfiguration& config,
                                    const DataModule* dataModule,
                                    double minX,
                                    double maxX,
                                    std::vector<std::vector<float>>& leftAxisData,
                                    std::vector<std::vector<float>>& rightAxisData,
                                    std::vector<CurveDefinition>& leftAxisCurves,
                                    std::vector<CurveDefinition>& rightAxisCurves) {
    if (!dataModule || !dataModule->HasData()) {
        return false;
    }

    DataType batchedDataType = DataType::TEMPERATURE;
    if (!TryResolveBatchedDataType(config, batchedDataType)) {
        return false;
    }

    if (config.curves.empty()) {
        return false;
    }

    const FrequencyBand frequency = config.curves.front().frequency;
    std::vector<bool> enabledTransmitters(static_cast<size_t>(GetUiTransmitterCount()), false);
    for (const auto& curve : config.curves) {
        if (!curve.enabled || !IsUiTransmitterIndex(curve.elementIndex)) {
            continue;
        }

        if (curve.frequency != frequency || curve.dataType != batchedDataType) {
            return false;
        }

        if ((batchedDataType == DataType::PHASE_SMT || batchedDataType == DataType::RHO_SMT) &&
            curve.smtMode != SmtComputationMode::FROM_SONDE) {
            return false;
        }

        enabledTransmitters[static_cast<size_t>(curve.elementIndex)] = true;
    }

    std::vector<std::vector<float>> batchedData;
    dataModule->CalculateChartData(batchedDataType, frequency, batchedData, enabledTransmitters, minX, maxX, -1e10, 1e10);
    if (batchedData.empty()) {
        return true;
    }

    bool hasEnabledLeftAxis = false;
    bool hasEnabledRightAxis = false;
    for (const auto& curve : config.curves) {
        if (!curve.enabled) continue;
        if (curve.yAxisGroup == 0) hasEnabledLeftAxis = true;
        if (curve.yAxisGroup == 1) hasEnabledRightAxis = true;
    }
    const bool normalizeAxes = (!hasEnabledLeftAxis && hasEnabledRightAxis);

    for (const auto& curve : config.curves) {
        if (!curve.enabled || !IsUiTransmitterIndex(curve.elementIndex)) {
            continue;
        }

        const size_t channelIndex = static_cast<size_t>(curve.elementIndex);
        if (channelIndex >= batchedData.size()) {
            continue;
        }

        const int effectiveAxisGroup = normalizeAxes ? 0 : curve.yAxisGroup;
        if (effectiveAxisGroup == 0) {
            leftAxisData.push_back(batchedData[channelIndex]);
            CurveDefinition normalizedCurve = curve;
            normalizedCurve.yAxisGroup = 0;
            normalizedCurve.lineStyle = PS_SOLID;
            leftAxisCurves.push_back(normalizedCurve);
        } else {
            rightAxisData.push_back(batchedData[channelIndex]);
            rightAxisCurves.push_back(curve);
        }
    }

    return true;
}

const wchar_t* GetPresetDisplayName(PresetType preset) {
    switch (preset) {
        case PresetType::ALL_AMPLITUDES_R1:
            return L"Амплитуда RX1";
        case PresetType::ALL_AMPLITUDES_R2:
            return L"Амплитуда RX2";
        case PresetType::AMPLITUDE_RATIOS:
            return L"Отношение амплитуд";
        case PresetType::PHASE_DIFFERENCES:
            return L"Разность фаз";
        case PresetType::SILENT_AMPLITUDES:
            return L"Ампл. молч.";
        case PresetType::SILENT_PHASE_DIFFERENCE:
            return L"Разность фаз молч.";
        case PresetType::RHO_SMT:
            return L"УЭС";
        case PresetType::PHASE_SMT:
            return L"Симм. фазы";
        default:
            return L"";
    }
}

float NormalizePhaseLocal(float phaseRadians) {
    const float pi = 3.14159265358979323846f;
    while (phaseRadians > pi) {
        phaseRadians -= 2.0f * pi;
    }
    while (phaseRadians <= -pi) {
        phaseRadians += 2.0f * pi;
    }
    return phaseRadians;
}

// ============================================================
// Симметризация фаз на компьютере (УЭС / Симм. фазы — режим FROM_COMPUTER)
// ============================================================
//
// Воспроизводит логику прибора (formula_simmetry/simmetry):
//   1) извлекает 8-битное condition (маска работающих передатчиков) из
//      32-битного GP_TOOL_DATA::condition в зависимости от частоты;
//   2) читает DELTA_PH[freq][Tx] для всех физически существующих Tx,
//      нормализует к воздуху по Air_zz[freq][Tx] из метрологии с чередованием
//      знаков (+,-,+,-,+);
//   3) выбирает матрицу симметризации K(N_Tx, condition) и применяет её.
//
// Поддерживаются N_Tx = 3, 4, 5. Для приборов с N_Tx < 5 выходы для
// несуществующих передатчиков всегда нулевые.

// Извлекает 8-битный condition для указанной частоты из 32-битного поля.
// Layout (LSB-first): byte0 = 2000 кГц (freq_idx=1), byte1 = 400 кГц (freq_idx=0).
// Внутри байта: bit0=T1, bit1=T2, ..., bit4=T5.
uint8_t ExtractConditionByte(uint32_t rawCondition, int freqIdx) {
    if (freqIdx == 0) {
        return static_cast<uint8_t>((rawCondition >> 8) & 0xFFu);
    }
    return static_cast<uint8_t>(rawCondition & 0xFFu);
}

// Зануляет матрицу полностью.
void ZeroSmtMatrix(SmtMatrix& K) {
    for (auto& row : K) {
        row.fill(0.0f);
    }
}

// Identity по маске работающих передатчиков (T_t -> T_t для k_t=1, иначе 0).
// Используется как fallback для редких комбинаций condition.
void FillIdentitySymmetryMatrix(SmtMatrix& K, uint8_t condition, int validTxCount) {
    ZeroSmtMatrix(K);
    for (int t = 0; t < validTxCount && t < kSmtMaxTransmitterCount; ++t) {
        const bool working = ((condition >> t) & 0x1u) != 0;
        K[t][t] = working ? 1.0f : 0.0f;
    }
}

// Матрицы симметризации для N_Tx = 5.
bool BuildSymmetryMatrix5Tx(SmtMatrix& K, uint8_t condition) {
    switch (condition) {
        case 0b00011111: // все 5 работают
        case 0b11111111: // все 5 работают (альтернативный код)
            K = {{ {{ +0.75f, +0.50f, -0.25f, +0.00f, +0.00f }},
                   {{ +0.25f, +0.50f, +0.25f, +0.00f, +0.00f }},
                   {{ +0.00f, +0.25f, +0.50f, +0.25f, +0.00f }},
                   {{ +0.00f, +0.00f, +0.25f, +0.50f, +0.25f }},
                   {{ +0.00f, +0.00f, -0.25f, +0.50f, +0.75f }} }};
            return true;
        case 0b00011110: // не работает T1
            K = {{ {{ +0.00f, +1.25f, +0.50f, -0.75f, +0.00f }},
                   {{ +0.00f, +0.75f, +0.50f, -0.25f, +0.00f }},
                   {{ +0.00f, +0.25f, +0.50f, +0.25f, +0.00f }},
                   {{ +0.00f, +0.00f, +0.25f, +0.50f, +0.25f }},
                   {{ +0.00f, +0.00f, -0.25f, +0.50f, +0.75f }} }};
            return true;
        case 0b00011101: // не работает T2
            K = {{ {{ +1.25f, +0.00f, -0.75f, +0.50f, +0.00f }},
                   {{ +0.75f, +0.00f, -0.25f, +0.50f, +0.00f }},
                   {{ +0.00f, +0.00f, +0.75f, +0.50f, -0.25f }},
                   {{ +0.00f, +0.00f, +0.25f, +0.50f, +0.25f }},
                   {{ +0.00f, +0.00f, -0.25f, +0.50f, +0.75f }} }};
            return true;
        case 0b00011011: // не работает T3 (NB: K[T4][T5]/K[T5][T5]=+0.05 — как в исходнике коллег)
            K = {{ {{ +0.50f, +0.75f, +0.00f, -0.25f, +0.00f }},
                   {{ +0.00f, +1.25f, +0.00f, -0.75f, +0.50f }},
                   {{ +0.00f, +0.75f, +0.00f, -0.25f, +0.50f }},
                   {{ +0.00f, +0.25f, +0.00f, +0.25f, +0.05f }},
                   {{ +0.00f, -0.25f, +0.00f, +0.75f, +0.05f }} }};
            return true;
        case 0b00010111: // не работает T4
            K = {{ {{ +0.75f, +0.50f, -0.25f, +0.00f, +0.00f }},
                   {{ +0.25f, +0.50f, +0.25f, +0.00f, +0.00f }},
                   {{ -0.25f, +0.50f, +0.75f, +0.00f, +0.00f }},
                   {{ +0.00f, +0.50f, -0.25f, +0.00f, +0.75f }},
                   {{ +0.00f, +0.50f, -0.75f, +0.00f, +1.25f }} }};
            return true;
        case 0b00001111: // не работает T5 (NB: K[T3][T1]=+0.05 — как в исходнике коллег)
            K = {{ {{ +0.75f, +0.50f, -0.25f, +0.00f, +0.00f }},
                   {{ +0.25f, +0.50f, +0.25f, +0.00f, +0.00f }},
                   {{ +0.05f, +0.25f, +0.50f, +0.25f, +0.00f }},
                   {{ +0.00f, -0.25f, +0.50f, +0.75f, +0.00f }},
                   {{ +0.00f, -0.75f, +0.50f, +1.25f, +0.00f }} }};
            return true;
        case 0b00000000: // несимметризованные значения для всех 5 передатчиков
            ZeroSmtMatrix(K);
            for (int t = 0; t < 5; ++t) K[t][t] = 1.0f;
            return true;
        default:
            return false;
    }
}

// Матрицы симметризации для N_Tx = 4 (T5 не существует).
bool BuildSymmetryMatrix4Tx(SmtMatrix& K, uint8_t condition) {
    switch (condition) {
        case 0b00001111: // все 4 работают
        case 0b11111111: // все работают (альтернативный код)
            K = {{ {{ +0.75f, +0.50f, -0.25f, +0.00f, +0.00f }},
                   {{ +0.25f, +0.50f, +0.25f, +0.00f, +0.00f }},
                   {{ +0.00f, +0.25f, +0.50f, +0.25f, +0.00f }},
                   {{ +0.00f, -0.25f, +0.50f, +0.75f, +0.00f }},
                   {{ +0.00f, +0.00f, +0.00f, +0.00f, +0.00f }} }};
            return true;
        case 0b00001110: // не работает T1
            K = {{ {{ +0.00f, +1.25f, +0.50f, -0.75f, +0.00f }},
                   {{ +0.00f, +0.75f, +0.50f, -0.25f, +0.00f }},
                   {{ +0.00f, +0.25f, +0.50f, +0.25f, +0.00f }},
                   {{ +0.00f, -0.25f, +0.50f, +0.75f, +0.00f }},
                   {{ +0.00f, +0.00f, +0.00f, +0.00f, +0.00f }} }};
            return true;
        case 0b00001101: // не работает T2
            K = {{ {{ +1.25f, +0.00f, -0.75f, +0.50f, +0.00f }},
                   {{ +0.75f, +0.00f, -0.25f, +0.50f, +0.00f }},
                   {{ +0.25f, +0.00f, +0.25f, +0.50f, +0.00f }},
                   {{ -0.25f, +0.00f, +0.75f, +0.50f, +0.00f }},
                   {{ +0.00f, +0.00f, +0.00f, +0.00f, +0.00f }} }};
            return true;
        case 0b00001011: // не работает T3
            K = {{ {{ +0.50f, +0.75f, +0.00f, -0.25f, +0.00f }},
                   {{ +0.50f, +0.25f, +0.00f, +0.25f, +0.00f }},
                   {{ +0.50f, -0.25f, +0.00f, +0.75f, +0.00f }},
                   {{ -0.50f, -0.75f, +0.00f, +1.25f, +0.00f }},
                   {{ +0.00f, +0.00f, +0.00f, +0.00f, +0.00f }} }};
            return true;
        case 0b00000111: // не работает T4
            K = {{ {{ +0.75f, +0.50f, -0.25f, +0.00f, +0.00f }},
                   {{ +0.25f, +0.50f, +0.25f, +0.00f, +0.00f }},
                   {{ -0.25f, +0.50f, +0.75f, +0.00f, +0.00f }},
                   {{ -0.75f, +0.50f, +1.25f, +0.00f, +0.00f }},
                   {{ +0.00f, +0.00f, +0.00f, +0.00f, +0.00f }} }};
            return true;
        case 0b00000000: // несимметризованные значения (только T1..T4)
            ZeroSmtMatrix(K);
            for (int t = 0; t < 4; ++t) K[t][t] = 1.0f;
            return true;
        default:
            return false;
    }
}

// Матрицы симметризации для N_Tx = 3 (T4, T5 не существуют).
bool BuildSymmetryMatrix3Tx(SmtMatrix& K, uint8_t condition) {
    switch (condition) {
        case 0b00000111: // все 3 работают
            K = {{ {{ +0.75f, +0.50f, -0.25f, +0.00f, +0.00f }},
                   {{ +0.25f, +0.50f, +0.25f, +0.00f, +0.00f }},
                   {{ -0.25f, +0.50f, +0.75f, +0.00f, +0.00f }},
                   {{ -0.75f, +0.50f, +1.25f, +0.00f, +0.00f }}, // dead row для N_Tx=3 — копия из исходника коллег
                   {{ +0.00f, +0.00f, +0.00f, +0.00f, +0.00f }} }};
            return true;
        case 0b00000000: // несимметризованные значения (только T1..T3)
            ZeroSmtMatrix(K);
            for (int t = 0; t < 3; ++t) K[t][t] = 1.0f;
            return true;
        default:
            return false;
    }
}

// Диспетчер: выбирает матрицу по (N_Tx, condition). Для нераспознанных
// комбинаций возвращает identity по маске condition.
void BuildSymmetryMatrix(SmtMatrix& K, int transmitterCount, uint8_t condition) {
    bool resolved = false;
    if (transmitterCount == 5) {
        resolved = BuildSymmetryMatrix5Tx(K, condition);
    } else if (transmitterCount == 4) {
        resolved = BuildSymmetryMatrix4Tx(K, condition);
    } else if (transmitterCount == 3) {
        resolved = BuildSymmetryMatrix3Tx(K, condition);
    }
    if (!resolved) {
        FillIdentitySymmetryMatrix(K, condition, transmitterCount);
    }
}

// Основной расчёт: возвращает массив phase_smt[0..4] для одной частоты.
// Выходы для индексов >= transmitterCount всегда нулевые.
SmtPhaseOutputs ComputePhaseSmtFromComputer(const MeasurementView& measurement,
                                            const METROLOGY_DATA& metrology,
                                            int freqIdx) {
    SmtPhaseOutputs outPhase = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    if (!measurement.IsValid() || freqIdx < 0 || freqIdx > 1) {
        return outPhase;
    }

    const GP_TOOL_DATA* record = measurement.GetRecord();
    if (!record) {
        return outPhase;
    }

    int transmitterCount = measurement.GetCapabilities().transmitterCount;
    if (transmitterCount <= 0) {
        transmitterCount = kFallbackTransmitterCount;
    }
    if (transmitterCount > kSmtMaxTransmitterCount) {
        transmitterCount = kSmtMaxTransmitterCount;
    }

    const uint8_t conditionByte = ExtractConditionByte(record->condition, freqIdx);

    // Читаем DELTA_PH и нормализуем к воздуху с чередованием знаков.
    // signs[T1..T5] = +,-,+,-,+ — соответствует physical wiring разности фаз.
    const ToolDataAccessView view = measurement.GetAccessView();
    std::array<float, kSmtMaxTransmitterCount> deltas = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    constexpr float kAlternatingSigns[kSmtMaxTransmitterCount] = { +1.0f, -1.0f, +1.0f, -1.0f, +1.0f };

    for (int t = 0; t < transmitterCount; ++t) {
        float rawDelta = 0.0f;
        if (!view.TryGetDeltaPhase(freqIdx, t, rawDelta)) {
            rawDelta = 0.0f;
        }
        const float airOffset = static_cast<float>(metrology.Air_zz[freqIdx][t]) / kAirZzPhaseDivisor;
        deltas[t] = kAlternatingSigns[t] * (rawDelta - airOffset);
    }

    // Матрица симметризации.
    SmtMatrix K = {};
    BuildSymmetryMatrix(K, transmitterCount, conditionByte);

    // outPhase[t] = sum_j K[t][j] * deltas[j] для t < transmitterCount.
    for (int t = 0; t < transmitterCount; ++t) {
        float sum = 0.0f;
        for (int j = 0; j < kSmtMaxTransmitterCount; ++j) {
            sum += K[t][j] * deltas[j];
        }
        outPhase[t] = sum;
    }

    return outPhase;
}

bool TryGetLegacyComputedPhaseSmt(const MeasurementView& measurement,
                                  const METROLOGY_DATA& metrology,
                                  int freqIdx,
                                  int transmitterIndex,
                                  float& outPhase) {
    outPhase = 0.0f;
    if (!measurement.IsValid()) {
        return false;
    }
    if (transmitterIndex < 0 || transmitterIndex >= kSmtMaxTransmitterCount) {
        return false;
    }

    int transmitterCount = measurement.GetCapabilities().transmitterCount;
    if (transmitterCount <= 0) {
        transmitterCount = kFallbackTransmitterCount;
    }
    // Несуществующие передатчики не имеют фазы.
    if (transmitterIndex >= transmitterCount) {
        return false;
    }

    const SmtPhaseOutputs phaseComputed = ComputePhaseSmtFromComputer(measurement, metrology, freqIdx);
    outPhase = phaseComputed[transmitterIndex];
    return true;
}

const METROLOGY_DATA* GetComputerSmtMetrology(SmtComputationMode mode, const MetrologyModule* metrologyModule) {
    const bool canCompute =
        mode == SmtComputationMode::FROM_COMPUTER &&
        metrologyModule && metrologyModule->HasData();
    return canCompute ? &metrologyModule->Get() : nullptr;
}

bool ShouldUseLegacyComputedSmt(const MeasurementView& measurement, const METROLOGY_DATA* metro) {
    return metro && measurement.IsValid() && measurement.GetSignature() == metro->signature;
}

bool TryGetSelectedPhaseSmt(const MeasurementView& measurement,
                           int freqIdx,
                           int transmitterIndex,
                           const METROLOGY_DATA* metro,
                           float& outPhase) {
    outPhase = 0.0f;
    if (!measurement.IsValid()) {
        return false;
    }

    if (ShouldUseLegacyComputedSmt(measurement, metro)) {
        return TryGetLegacyComputedPhaseSmt(measurement, *metro, freqIdx, transmitterIndex, outPhase);
    }

    return measurement.GetAccessView().TryGetPhaseSmt(freqIdx, transmitterIndex, outPhase);
}

bool TryGetSelectedRhoSmt(const MeasurementView& measurement,
                         int freqIdx,
                         int transmitterIndex,
                         const METROLOGY_DATA* metro,
                         float& outRho) {
    outRho = 0.0f;
    if (!measurement.IsValid()) {
        return false;
    }

    if (ShouldUseLegacyComputedSmt(measurement, metro)) {
        float phase = 0.0f;
        if (!TryGetLegacyComputedPhaseSmt(measurement, *metro, freqIdx, transmitterIndex, phase)) {
            return false;
        }

        outRho = InvertPhaseToRho(*metro, phase, transmitterIndex, freqIdx);
        return true;
    }

    return measurement.GetAccessView().TryGetRhoSmt(freqIdx, transmitterIndex, outRho);
}

bool TryPrepareFullDataRange(const DataModule* dataModule, double& outMinX, double& outMaxX) {
    outMinX = 0.0;
    outMaxX = 0.0;
    if (!dataModule || !dataModule->HasData()) {
        return false;
    }

    outMinX = 0.0;
    outMaxX = static_cast<double>(dataModule->GetDataSize() - 1);
    return true;
}

bool ContainsColor(const std::vector<COLORREF>& colors, COLORREF color) {
    return std::find(colors.begin(), colors.end(), color) != colors.end();
}

double CalculateColorDistance(COLORREF first, COLORREF second) {
    const double r1 = static_cast<double>(GetRValue(first));
    const double g1 = static_cast<double>(GetGValue(first));
    const double b1 = static_cast<double>(GetBValue(first));
    const double r2 = static_cast<double>(GetRValue(second));
    const double g2 = static_cast<double>(GetGValue(second));
    const double b2 = static_cast<double>(GetBValue(second));
    const double rMean = (r1 + r2) * 0.5;
    const double dr = r1 - r2;
    const double dg = g1 - g2;
    const double db = b1 - b2;

    return std::sqrt((2.0 + rMean / 256.0) * dr * dr +
                     4.0 * dg * dg +
                     (2.0 + (255.0 - rMean) / 256.0) * db * db);
}

double GetMinColorDistance(COLORREF candidate, const std::vector<COLORREF>& usedColors) {
    if (usedColors.empty()) {
        return std::numeric_limits<double>::max();
    }

    double minDistance = std::numeric_limits<double>::max();
    for (COLORREF usedColor : usedColors) {
        minDistance = std::min(minDistance, CalculateColorDistance(candidate, usedColor));
    }
    return minDistance;
}

double HueToRgb(double p, double q, double t) {
    if (t < 0.0) t += 1.0;
    if (t > 1.0) t -= 1.0;
    if (t < 1.0 / 6.0) return p + (q - p) * 6.0 * t;
    if (t < 1.0 / 2.0) return q;
    if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
    return p;
}

COLORREF CreateHslColor(double hueDegrees, double saturation, double lightness) {
    const double hue = std::fmod(hueDegrees, 360.0) / 360.0;

    double r = lightness;
    double g = lightness;
    double b = lightness;

    if (saturation > 0.0) {
        const double q = lightness < 0.5
            ? lightness * (1.0 + saturation)
            : lightness + saturation - lightness * saturation;
        const double p = 2.0 * lightness - q;
        r = HueToRgb(p, q, hue + 1.0 / 3.0);
        g = HueToRgb(p, q, hue);
        b = HueToRgb(p, q, hue - 1.0 / 3.0);
    }

    return RGB(static_cast<int>(std::round(r * 255.0)),
               static_cast<int>(std::round(g * 255.0)),
               static_cast<int>(std::round(b * 255.0)));
}

COLORREF GenerateAdaptivePaletteColor(int index) {
    static const double saturations[] = { 0.36, 0.42, 0.48 };
    static const double lightnessValues[] = { 0.36, 0.42, 0.47, 0.40 };

    const double hue = std::fmod(29.0 + 137.50776405 * static_cast<double>(index), 360.0);
    const double saturation = saturations[index % (sizeof(saturations) / sizeof(saturations[0]))];
    const double lightness = lightnessValues[(index / 3) % (sizeof(lightnessValues) / sizeof(lightnessValues[0]))];
    return CreateHslColor(hue, saturation, lightness);
}

ChartConfiguration BuildPresetConfigurationBase(PresetType presetType) {
    ChartConfiguration config;
    config.Clear();
    config.activePreset = presetType;
    config.presetDisplayName = GetPresetDisplayName(config.activePreset);
    return config;
}

ChartConfiguration BuildSmtPresetConfiguration(const CurveSelectionModule& module,
                                               FrequencyBand frequency,
                                               CurveDefinition (CurveSelectionModule::*createCurve)(int, FrequencyBand) const,
                                               const char* leftAxisUnits,
                                               PresetType presetType) {
    ChartConfiguration config = BuildPresetConfigurationBase(presetType);

    for (int i = 0; i < GetUiTransmitterCount(); ++i) {
        CurveDefinition curve = (module.*createCurve)(i, frequency);
        curve.smtMode = SmtComputationMode::FROM_SONDE;
        curve.yAxisGroup = 0;
        curve.enabled = true;
        config.AddCurve(curve);
    }

    config.leftAxisUnits = leftAxisUnits;
    config.hasDualAxis = false;
    return config;
}

float SignalPhase(const METROLOGY_DATA& metrology, int txIndex, int freqIdx, float rhoValue) {
    if (rhoValue <= 0.0f) {
        rhoValue = 0.01f;
    }

    const float L1 = static_cast<float>(metrology.L1[txIndex]) / 1000.0f;
    const float L2 = static_cast<float>(metrology.L2[txIndex]) / 1000.0f;
    const float omegaMu0Sigma = (0.0078957f * static_cast<float>(metrology.F[freqIdx])) / rhoValue;

    const std::complex<float> iUnit(0.0f, 1.0f);
    const std::complex<float> ik = iUnit * std::sqrt(iUnit * omegaMu0Sigma);
    const std::complex<float> sgn = std::exp(ik * (L2 - L1)) * ((1.0f - ik * L2) / (1.0f - ik * L1));
    return std::arg(sgn);
}

float InvertPhaseToRho(const METROLOGY_DATA& metrology, float targetPhase, int txIndex, int freqIdx) {
    constexpr float epsilonArg = 0.0000005f;
    constexpr float epsilonInf = 0.000000005f;
    float left = 0.01f;
    float right = 7000.0f;
    bool stopArg = false;
    bool stopInf = false;

    for (int i = 0; i < 256; ++i) {
        const float x1 = left + 0.382f * (right - left);
        const float x2 = right - 0.382f * (right - left);
        const float a = targetPhase - SignalPhase(metrology, txIndex, freqIdx, x1);
        const float b = targetPhase - SignalPhase(metrology, txIndex, freqIdx, x2);

        if (std::fabs(a) > std::fabs(b)) {
            left = x1;
        } else {
            right = x2;
        }

        if (std::fabs(a) < epsilonArg || std::fabs(b) < epsilonArg) {
            stopArg = true;
            break;
        }
        if (std::fabs(a - b) < epsilonInf) {
            stopInf = true;
            break;
        }
    }

    if (stopArg) {
        return 0.5f * (left + right);
    }
    if (stopInf) {
        return 7200.0f;
    }
    return 0.5f * (left + right);
}
}

// Определение стандартных цветов для кривых T1-T5
// Цвета для левой оси (приоритетные цвета для T1-T5)
const COLORREF CurveSelectionModule::LEFT_AXIS_COLORS[SchematicSelection::UI_TRANSMITTER_COUNT] = {
    RGB(64, 132, 92),
    RGB(184, 160, 74),
    RGB(168, 82, 82),
    RGB(60, 108, 160),
    RGB(122, 94, 146)
};

// Цвета для правой оси (альтернативные цвета для T1-T5)
const COLORREF CurveSelectionModule::RIGHT_AXIS_COLORS[SchematicSelection::UI_TRANSMITTER_COUNT] = {
    RGB(76, 140, 108),
    RGB(168, 148, 88),
    RGB(150, 92, 92),
    RGB(76, 118, 168),
    RGB(142, 90, 118)
};

// Специальные цвета для других типов данных
const COLORREF CurveSelectionModule::SILENT_COLOR = RGB(123, 123, 123);      // Нейтральный серый для "Silent"
const COLORREF CurveSelectionModule::TEMPERATURE_COLOR = RGB(176, 124, 68);  // Спокойный "оранжевый" для температуры

// Расширенная палитра для умного назначения цветов (12 уникальных цветов)
const COLORREF CurveSelectionModule::EXTENDED_PALETTE[12] = {
    RGB(64, 132, 92),
    RGB(184, 160, 74),
    RGB(168, 82, 82),
    RGB(60, 108, 160),
    RGB(122, 94, 146),
    RGB(70, 128, 138),
    RGB(126, 94, 152),
    RGB(140, 94, 68),
    RGB(96, 142, 82),
    RGB(146, 86, 118),
    RGB(84, 116, 134),
    RGB(130, 108, 86)
};

// Старый массив для обратной совместимости (теперь не используется)
const COLORREF CurveSelectionModule::CURVE_COLORS[SchematicSelection::UI_TRANSMITTER_COUNT] = {
    RGB(0, 128, 0),      // T1
    RGB(139, 69, 19),    // T2
    RGB(200, 50, 50),    // T3
    RGB(128, 0, 128),    // T4
    RGB(170, 80, 140)    // T5
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
        // НОВОЕ: поддержка пресетов для молчащих передатчиков
        // (selection.silentTx[] используется даже в режиме пресета)
        if (selectedSilent > 0 && selectedReceivers == 0 && selectedTransmitters == 0) {
            AddSilentTransmitterCurves(curves, selection);
            RefreshCurveColors(curves);
            return curves;
        }

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
        else if (selection.dataFilter == DataTypeFilter::RHO_SMT_ONLY) {
            const int transmitterCount = GetUiTransmitterCount(&selection.transmitters);
            AddSmtSelectionCurves(curves,
                                  selection.frequency,
                                  transmitterCount,
                                  &CurveSelectionModule::CreateRhoSmtCurve,
                                  selection.rhoSmtMode);
        }
        else if (selection.dataFilter == DataTypeFilter::PHASE_SMT_ONLY) {
            const int transmitterCount = GetUiTransmitterCount(&selection.transmitters);
            AddSmtSelectionCurves(curves,
                                  selection.frequency,
                                  transmitterCount,
                                  &CurveSelectionModule::CreatePhaseSmtCurve,
                                  selection.phaseSmtMode);
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
    RefreshCurveColors(curves);
    return curves;
}

void CurveSelectionModule::ApplyCurveConfiguration(int chartNumber, const ChartConfiguration& config) {
    ChartConfiguration normalizedConfig = config;
    RefreshCurveColors(normalizedConfig.curves);
    normalizedConfig.UpdateAxisConfiguration();

    if (chartNumber == 1) {
        chart1Config = normalizedConfig;
    } else if (chartNumber == 2) {
        chart2Config = normalizedConfig;
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
        case PresetType::SILENT_AMPLITUDES:
            {
                ChartConfiguration config = BuildPresetConfigurationBase(PresetType::SILENT_AMPLITUDES);

                // Две кривые: молчащий R1 и молчащий R2
                CurveDefinition r1 = CreateSilentAmplitudeCurve(0, frequency);
                FinalizeSilentPresetCurve(r1, config.curves, 0);
                config.curves.push_back(r1);

                CurveDefinition r2 = CreateSilentAmplitudeCurve(1, frequency);
                FinalizeSilentPresetCurve(r2, config.curves, 1);
                config.curves.push_back(r2);

                config.UpdateAxisConfiguration();
                return config;
            }
        case PresetType::SILENT_PHASE_DIFFERENCE:
            {
                ChartConfiguration config = BuildPresetConfigurationBase(PresetType::SILENT_PHASE_DIFFERENCE);

                CurveDefinition dph = CreateSilentPhaseDifferenceCurve(frequency);
                FinalizeSilentPresetCurve(dph, config.curves, 0);
                config.curves.push_back(dph);

                config.UpdateAxisConfiguration();
                return config;
            }
        case PresetType::RHO_SMT:
            return CreateRhoSmtPreset(frequency);
        case PresetType::PHASE_SMT:
            return CreatePhaseSmtPreset(frequency);
        default:
            return ChartConfiguration();
    }
}

ChartConfiguration CurveSelectionModule::BuildFullCurveSet(FrequencyBand frequency) const {
    ChartConfiguration config;
    const int txCount = SchematicSelection::UI_TRANSMITTER_COUNT; // 5

    // Phase differences (left axis)
    for (int tx = 0; tx < txCount; ++tx) {
        CurveDefinition c = CreateDifferenceCurve(tx, frequency);
        c.yAxisGroup = 0;
        c.enabled = false;
        c.color = AssignSmartColor(config.curves, tx, 0);
        config.curves.push_back(c);
    }

    // Amplitudes for both receivers (right axis)
    for (int tx = 0; tx < txCount; ++tx) {
        for (int rx = 0; rx < 2; ++rx) {
            CurveDefinition c = CreateAmplitudeCurve(tx, rx, frequency);
            c.yAxisGroup = 1;
            c.enabled = false;
            c.color = AssignSmartColor(config.curves, tx, 1);
            config.curves.push_back(c);
        }
    }

    // Amplitude ratios (left axis)
    for (int tx = 0; tx < txCount; ++tx) {
        CurveDefinition c = CreateRatioCurve(tx, frequency);
        c.yAxisGroup = 0;
        c.enabled = false;
        c.color = AssignSmartColor(config.curves, tx, 0);
        config.curves.push_back(c);
    }

    // Phase SMT (left axis)
    for (int tx = 0; tx < txCount; ++tx) {
        CurveDefinition c = CreatePhaseSmtCurve(tx, frequency);
        c.yAxisGroup = 0;
        c.enabled = false;
        c.color = AssignSmartColor(config.curves, tx, 0);
        config.curves.push_back(c);
    }

    // Rho SMT (left axis)
    for (int tx = 0; tx < txCount; ++tx) {
        CurveDefinition c = CreateRhoSmtCurve(tx, frequency);
        c.yAxisGroup = 0;
        c.enabled = false;
        c.color = AssignSmartColor(config.curves, tx, 0);
        config.curves.push_back(c);
    }

    // Silent amplitudes
    for (int rx = 0; rx < 2; ++rx) {
        CurveDefinition c = CreateSilentAmplitudeCurve(rx, frequency);
        c.yAxisGroup = 1;
        c.enabled = false;
        c.color = AssignSmartColor(config.curves, rx, 1);
        config.curves.push_back(c);
    }

    // Silent phase difference
    {
        CurveDefinition c = CreateSilentPhaseDifferenceCurve(frequency);
        c.yAxisGroup = 0;
        c.enabled = false;
        c.color = AssignSmartColor(config.curves, 0, 0);
        config.curves.push_back(c);
    }

    // Temperature
    {
        CurveDefinition c;
        c.id = "TEMPERATURE";
        c.displayName = "Температура";
        c.units = "degC";
        c.yAxisGroup = 0;
        c.enabled = false;
        c.color = TEMPERATURE_COLOR;
        c.lineStyle = PS_SOLID;
        c.sourceElement = ElementType::TRANSMITTER;
        c.elementIndex = 0;
        c.dataType = DataType::TEMPERATURE;
        c.frequency = frequency;
        config.curves.push_back(c);
    }

    return config;
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
    

    if (enabledCurvesCount > 64) {
        return "Too many curves for display (maximum 64)";
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

    double minX = 0.0;
    double maxX = static_cast<double>(dataModule->GetDataSize() > 0 ? dataModule->GetDataSize() - 1 : 0);
    if (TryBuildBatchedPresetCurveData(config, dataModule, minX, maxX,
                                       leftAxisData, rightAxisData,
                                       leftAxisCurves, rightAxisCurves)) {
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
            case DataType::SILENT_PHASE_DIFFERENCE:
                {
                    // Разность фаз молчащих передатчиков (R2−R1) по ZERO_dPH
                    dataModule->CalculateSilentPhaseDifferenceData(curve.frequency, curveData,
                                                                  0.0, static_cast<double>(dataModule->GetDataSize() - 1));
                }
                break;
            case DataType::AMPLITUDE_RX1:
            case DataType::AMPLITUDE_RX2:
                {
                    int receiverIndex = (curve.dataType == DataType::AMPLITUDE_RX1) ? 0 : 1;
                    CalculateAmplitudeData(curve.elementIndex, receiverIndex, curve.frequency, dataModule, curveData);
                }
                break;
            case DataType::PHASE_SMT:
                CalculatePhaseSmtData(curve.elementIndex,
                                      curve.frequency,
                                      curve.smtMode,
                                      dataModule,
                                      curveData,
                                      0.0,
                                      static_cast<double>(dataModule->GetDataSize() - 1));
                break;
            case DataType::RHO_SMT:
                CalculateRhoSmtData(curve.elementIndex,
                                    curve.frequency,
                                    curve.smtMode,
                                    dataModule,
                                    curveData,
                                    0.0,
                                    static_cast<double>(dataModule->GetDataSize() - 1));
                break;
            case DataType::SILENT_AMPLITUDE_RX1:
                CalculateSilentAmplitudeData(0, curve.frequency, dataModule, curveData);
                break;
            case DataType::SILENT_AMPLITUDE_RX2:
                CalculateSilentAmplitudeData(1, curve.frequency, dataModule, curveData);
                break;
            case DataType::TEMPERATURE:
                dataModule->GetTemperatureData(curveData);
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

    if (TryBuildBatchedPresetCurveData(config, dataModule, minX, maxX,
                                       leftAxisData, rightAxisData,
                                       leftAxisCurves, rightAxisCurves)) {
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
            case DataType::SILENT_PHASE_DIFFERENCE:
                dataModule->CalculateSilentPhaseDifferenceData(curve.frequency, curveData, minX, maxX);
                break;
            case DataType::AMPLITUDE_RX1:
            case DataType::AMPLITUDE_RX2:
                {
                    int receiverIndex = (curve.dataType == DataType::AMPLITUDE_RX1) ? 0 : 1;
                    dataModule->CalculateAmplitudeData(curve.elementIndex, receiverIndex, curve.frequency, curveData, minX, maxX);
                }
                break;
            case DataType::PHASE_SMT:
                CalculatePhaseSmtData(curve.elementIndex,
                                      curve.frequency,
                                      curve.smtMode,
                                      dataModule,
                                      curveData,
                                      minX,
                                      maxX);
                break;
            case DataType::RHO_SMT:
                CalculateRhoSmtData(curve.elementIndex,
                                    curve.frequency,
                                    curve.smtMode,
                                    dataModule,
                                    curveData,
                                    minX,
                                    maxX);
                break;
            case DataType::SILENT_AMPLITUDE_RX1:
                dataModule->CalculateSilentAmplitudeData(0, curve.frequency, curveData, minX, maxX);
                break;
            case DataType::SILENT_AMPLITUDE_RX2:
                dataModule->CalculateSilentAmplitudeData(1, curve.frequency, curveData, minX, maxX);
                break;
            case DataType::TEMPERATURE:
                dataModule->GetTemperatureData(curveData, minX, maxX);
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
    const int transmitterCount = GetUiTransmitterCount(&selection.transmitters);
    
    // ИЗМЕНЕНО v2.7.0: Сначала добавляем разности фаз (более важны для анализа)
    for (int i = 0; i < transmitterCount; ++i) {
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
    for (int i = 0; i < transmitterCount; ++i) {
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
    const int transmitterCount = GetUiTransmitterCount(&selection.transmitters);
    
    // ИЗМЕНЕНО v2.7.0: Сначала добавляем фазы (более важны для анализа)
    for (int i = 0; i < transmitterCount; ++i) {
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
    for (int i = 0; i < transmitterCount; ++i) {
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
    
    const int transmitterCount = GetUiTransmitterCount(&selection.transmitters);
    for (int txIndex = 0; txIndex < transmitterCount; ++txIndex) {
        if (!selection.transmitters[txIndex]) continue; // Пропускаем невыбранные передатчики

        // Разность фаз (ЛЕВАЯ ось) - разность фаз R1-R2, одна кривая на передатчик.
        CurveDefinition phaseCurve = CreateDifferenceCurve(txIndex, selection.frequency);
        phaseCurve.yAxisGroup = 0; // ЛЕВАЯ ось
        phaseCurve.enabled = true;
        phaseCurve.displayName = GenerateDisplayName(ElementType::TRANSMITTER, txIndex, phaseCurve.dataType);
        phaseCurve.color = AssignSmartColor(curves, txIndex, phaseCurve.yAxisGroup);
        curves.push_back(phaseCurve);
        
        // ИЗМЕНЕНО v2.6.5: Потом добавляем все амплитуды для этого передатчика
        for (int rxIndex = 0; rxIndex < 2; rxIndex++) {
            // Амплитуда (ПРАВАЯ ось)
            CurveDefinition amplitudeCurve = CreateAmplitudeCurve(txIndex, rxIndex, selection.frequency);
            amplitudeCurve.yAxisGroup = 1; // ПРАВАЯ ось
            amplitudeCurve.enabled = true;
            
            // Обновляем отображаемое имя для ясности (локализовано)
            amplitudeCurve.displayName = GenerateDisplayName(ElementType::TRANSMITTER, txIndex, amplitudeCurve.dataType, rxIndex);
            
            // Умное назначение цвета с учетом уже созданных кривых
            amplitudeCurve.color = AssignSmartColor(curves, txIndex, amplitudeCurve.yAxisGroup);
            curves.push_back(amplitudeCurve);
        }
    }
}

void CurveSelectionModule::AddSilentTransmitterCurves(std::vector<CurveDefinition>& curves, 
                                                     const SchematicSelection& selection) const {
    // Молчащие передатчики:
    // - амплитуды: ZERO_AM_RX_1/2 (для R1/R2)
    // - разность фаз (R2−R1): ZERO_dPH (требуются оба приёмника, поэтому показываем только если выбраны оба silent)
    // ИЗМЕНЕНО v2.7.0: Если есть фазы на графике, амплитуды идут на правую ось

    // Учитываем фильтр типов данных (в т.ч. в режиме пресета)
    bool addAmplitudes = (selection.dataFilter == DataTypeFilter::ALL ||
                          selection.dataFilter == DataTypeFilter::AMPLITUDE_ONLY);
    bool addDifferences = (selection.dataFilter == DataTypeFilter::ALL ||
                           selection.dataFilter == DataTypeFilter::DIFFERENCE_ONLY ||
                           selection.dataFilter == DataTypeFilter::PHASE_ONLY);
    
    // Проверяем, есть ли уже фазовые кривые
    bool hasPhases = false;
    for (const auto& curve : curves) {
        if (curve.dataType == DataType::PHASE_SMT || 
            curve.dataType == DataType::PHASE_DIFFERENCE ||
            curve.dataType == DataType::SILENT_PHASE_DIFFERENCE) {
            hasPhases = true;
            break;
        }
    }
    
    if (addDifferences && selection.silentTx.size() >= 2 && selection.silentTx[0] && selection.silentTx[1]) {
        // Разность фаз молчащих передатчиков (R2−R1)
        CurveDefinition silentDiff = CreateSilentPhaseDifferenceCurve(selection.frequency);
        silentDiff.yAxisGroup = 0; // Левая ось для фаз
        silentDiff.enabled = true;
        silentDiff.color = AssignSmartColor(curves, 0, silentDiff.yAxisGroup);
        silentDiff.displayName = GenerateDisplayName(ElementType::SILENT_TX, 0, silentDiff.dataType);
        curves.push_back(silentDiff);

        // Важно: дальше амплитуды должны уйти на правую ось, чтобы не смешивать единицы (mdeg vs mV) на одной оси
        hasPhases = true;
    }

    if (addAmplitudes) {
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
                silentCurve.displayName = GenerateDisplayName(ElementType::SILENT_TX, i, silentCurve.dataType);
                
                curves.push_back(silentCurve);
            }
        }
    }
}

// Методы создания конкретных кривых
CurveDefinition CurveSelectionModule::CreateRatioCurve(int transmitterIndex, FrequencyBand frequency) const {
    std::string id = GenerateCurveId(ElementType::TRANSMITTER, transmitterIndex, 
                                    DataType::AMPLITUDE_RATIO, frequency);
    std::string name = GenerateDisplayName(ElementType::TRANSMITTER, transmitterIndex, 
                                          DataType::AMPLITUDE_RATIO);
    
    // Отношение амплитуд — безразмерная величина (V/V)
    CurveDefinition curve(id, name, "V/V", ElementType::TRANSMITTER, transmitterIndex, 
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

// v2.16.0: Создание кривой УЭС (rho_smt)
CurveDefinition CurveSelectionModule::CreateRhoSmtCurve(int transmitterIndex, FrequencyBand frequency) const {
    std::string id = GenerateCurveId(ElementType::TRANSMITTER, transmitterIndex, 
                                    DataType::RHO_SMT, frequency);
    std::string name = GenerateDisplayName(ElementType::TRANSMITTER, transmitterIndex, 
                                          DataType::RHO_SMT);
    
    CurveDefinition curve(id, name, "Ohm*m", ElementType::TRANSMITTER, transmitterIndex, 
                         DataType::RHO_SMT, frequency);
    curve.yAxisGroup = 0; // Левая ось
    curve.color = GetCurveColorForAxis(transmitterIndex, curve.yAxisGroup);
    curve.lineStyle = GetLineStyle(curve.yAxisGroup);
    
    return curve;
}

// v2.16.0: Создание кривой симметризованных фаз (phase_smt)
CurveDefinition CurveSelectionModule::CreatePhaseSmtCurve(int transmitterIndex, FrequencyBand frequency) const {
    std::string id = GenerateCurveId(ElementType::TRANSMITTER, transmitterIndex, 
                                    DataType::PHASE_SMT, frequency);
    std::string name = GenerateDisplayName(ElementType::TRANSMITTER, transmitterIndex, 
                                          DataType::PHASE_SMT);
    
    CurveDefinition curve(id, name, "mdeg", ElementType::TRANSMITTER, transmitterIndex, 
                         DataType::PHASE_SMT, frequency);
    curve.yAxisGroup = 0; // Левая ось
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

CurveDefinition CurveSelectionModule::CreateSilentPhaseDifferenceCurve(FrequencyBand frequency) const {
    // Для разности фаз молчащих передатчиков используем отдельный DataType.
    // elementIndex фиксируем как 0 (не привязан к конкретному "silent R1/R2", т.к. это R2−R1).
    const int elementIndex = 0;
    std::string id = GenerateCurveId(ElementType::SILENT_TX, elementIndex, DataType::SILENT_PHASE_DIFFERENCE, frequency);
    std::string name = GenerateDisplayName(ElementType::SILENT_TX, elementIndex, DataType::SILENT_PHASE_DIFFERENCE);

    CurveDefinition curve(id, name, "mdeg", ElementType::SILENT_TX, elementIndex, DataType::SILENT_PHASE_DIFFERENCE, frequency);
    curve.yAxisGroup = 0; // Левая ось
    curve.color = GetSpecialDataColor(DataType::SILENT_PHASE_DIFFERENCE);
    curve.lineStyle = GetLineStyle(curve.yAxisGroup);
    return curve;
}

// Вспомогательные методы для работы с цветами
COLORREF CurveSelectionModule::GetCurveColor(int transmitterIndex) const {
    // Старый метод для обратной совместимости - использует цвета левой оси
    return GetCurveColorForAxis(transmitterIndex, 0);
}

COLORREF CurveSelectionModule::GetCurveColorForAxis(int transmitterIndex, int yAxisGroup) const {
    if (IsUiTransmitterIndex(transmitterIndex)) {
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
        case DataType::SILENT_PHASE_DIFFERENCE:
            return SILENT_COLOR;  // Фиолетовый для молчащих передатчиков
        case DataType::TEMPERATURE:
            return TEMPERATURE_COLOR;  // Оранжевый для температуры
        default:
            return RGB(128, 128, 128);  // Серый для неизвестных типов
    }
}

COLORREF CurveSelectionModule::AssignSmartColor(const std::vector<CurveDefinition>& existingCurves, 
                                               int transmitterIndex, int yAxisGroup) const {
    std::vector<COLORREF> usedColors;
    usedColors.reserve(existingCurves.size());
    for (const auto& curve : existingCurves) {
        usedColors.push_back(curve.color);
    }

    const bool hasPreferredColor = IsUiTransmitterIndex(transmitterIndex);
    const COLORREF preferredColor = hasPreferredColor
        ? GetCurveColorForAxis(transmitterIndex, yAxisGroup)
        : RGB(0, 0, 0);

    if (hasPreferredColor && !ContainsColor(usedColors, preferredColor)) {
        return preferredColor;
    }

    if (usedColors.empty()) {
        return EXTENDED_PALETTE[0];
    }

    std::vector<COLORREF> candidates;
    candidates.reserve(SchematicSelection::UI_TRANSMITTER_COUNT * 2 + 12 + 64);
    auto appendCandidate = [&candidates](COLORREF color) {
        if (!ContainsColor(candidates, color)) {
            candidates.push_back(color);
        }
    };

    if (hasPreferredColor) {
        appendCandidate(preferredColor);
    }

    for (int i = 0; i < SchematicSelection::UI_TRANSMITTER_COUNT; ++i) {
        appendCandidate(LEFT_AXIS_COLORS[i]);
        appendCandidate(RIGHT_AXIS_COLORS[i]);
    }

    for (int i = 0; i < 12; ++i) {
        appendCandidate(EXTENDED_PALETTE[i]);
    }

    for (int i = 0; i < 64; ++i) {
        appendCandidate(GenerateAdaptivePaletteColor(i));
    }

    COLORREF bestColor = candidates.empty() ? EXTENDED_PALETTE[0] : candidates.front();
    double bestScore = -1.0;

    for (COLORREF candidateColor : candidates) {
        const double minDistance = GetMinColorDistance(candidateColor, usedColors);
        double score = minDistance;

        if (!ContainsColor(usedColors, candidateColor)) {
            score += 10.0;
        }

        if (hasPreferredColor && candidateColor == preferredColor) {
            score += 18.0;
        }

        if (score > bestScore) {
            bestScore = score;
            bestColor = candidateColor;
        }
    }

    return bestColor;
}

void CurveSelectionModule::RefreshCurveColors(std::vector<CurveDefinition>& curves) const {
    std::vector<CurveDefinition> assignedCurves;
    assignedCurves.reserve(curves.size());

    bool corporateUsed[SchematicSelection::UI_TRANSMITTER_COUNT] = {};

    for (auto& curve : curves) {
        if (curve.dataType == DataType::TEMPERATURE) {
            curve.color = TEMPERATURE_COLOR;
        } else {
            int colorTransmitterIndex =
                (curve.sourceElement == ElementType::TRANSMITTER) ? curve.elementIndex : -1;

            if (colorTransmitterIndex >= 0
                && colorTransmitterIndex < SchematicSelection::UI_TRANSMITTER_COUNT
                && corporateUsed[colorTransmitterIndex]) {
                colorTransmitterIndex = -1;
            }

            curve.color = AssignSmartColor(assignedCurves, colorTransmitterIndex, curve.yAxisGroup);

            if (curve.sourceElement == ElementType::TRANSMITTER
                && curve.elementIndex >= 0
                && curve.elementIndex < SchematicSelection::UI_TRANSMITTER_COUNT) {
                corporateUsed[curve.elementIndex] = true;
            }
        }

        assignedCurves.push_back(curve);
    }
}

int CurveSelectionModule::GetLineStyle(int yAxisGroup) const {
    (void)yAxisGroup;
    // Все кривые должны быть однородными (сплошные линии) независимо от оси.
    return PS_SOLID;
}

// Методы создания пресетов
ChartConfiguration CurveSelectionModule::CreateAllAmplitudesR1Preset(FrequencyBand frequency) const {
    ChartConfiguration config;
    
    // Создаем кривые амплитуд для всех передатчиков T1-T5 на приемнике R1
    for (int i = 0; i < GetUiTransmitterCount(); ++i) {
        CurveDefinition curve = CreateAmplitudeCurve(i, 0, frequency); // 0 = R1
        curve.yAxisGroup = 0; // Левая ось
        curve.enabled = true;
        config.AddCurve(curve);
    }
    
    config.leftAxisUnits = "V";
    config.hasDualAxis = false;
    config.activePreset = PresetType::ALL_AMPLITUDES_R1;
    config.presetDisplayName = GetPresetDisplayName(config.activePreset);
    
    return config;
}

ChartConfiguration CurveSelectionModule::CreateAllAmplitudesR2Preset(FrequencyBand frequency) const {
    ChartConfiguration config;
    
    // Создаем кривые амплитуд для всех передатчиков T1-T5 на приемнике R2
    for (int i = 0; i < GetUiTransmitterCount(); ++i) {
        CurveDefinition curve = CreateAmplitudeCurve(i, 1, frequency); // 1 = R2
        curve.yAxisGroup = 0; // Левая ось
        curve.enabled = true;
        config.AddCurve(curve);
    }
    
    config.leftAxisUnits = "V";
    config.hasDualAxis = false;
    config.activePreset = PresetType::ALL_AMPLITUDES_R2;
    config.presetDisplayName = GetPresetDisplayName(config.activePreset);
    
    return config;
}

ChartConfiguration CurveSelectionModule::CreateAmplitudeRatiosPreset(FrequencyBand frequency) const {
    ChartConfiguration config;
    
    // Создаем кривые отношений амплитуд для всех передатчиков T1-T5
    for (int i = 0; i < GetUiTransmitterCount(); ++i) {
        CurveDefinition curve = CreateRatioCurve(i, frequency);
        curve.yAxisGroup = 0; // Левая ось
        curve.enabled = true;
        config.AddCurve(curve);
    }
    
    config.leftAxisUnits = "mV";
    config.hasDualAxis = false;
    config.activePreset = PresetType::AMPLITUDE_RATIOS;
    config.presetDisplayName = GetPresetDisplayName(config.activePreset);
    
    return config;
}

ChartConfiguration CurveSelectionModule::CreatePhaseDifferencesPreset(FrequencyBand frequency) const {
    ChartConfiguration config;
    
    // Создаем кривые разностей фаз для всех передатчиков T1-T5
    for (int i = 0; i < GetUiTransmitterCount(); ++i) {
        CurveDefinition curve = CreateDifferenceCurve(i, frequency);
        curve.yAxisGroup = 0; // Левая ось
        curve.enabled = true;
        config.AddCurve(curve);
    }
    
    config.leftAxisUnits = "deg";
    config.hasDualAxis = false;
    config.activePreset = PresetType::PHASE_DIFFERENCES;
    config.presetDisplayName = GetPresetDisplayName(config.activePreset);
    
    return config;
}

// v2.16.0: Пресет УЭС (rho_smt) для всех передатчиков T1-T5
ChartConfiguration CurveSelectionModule::CreateRhoSmtPreset(FrequencyBand frequency) const {
    return BuildSmtPresetConfiguration(*this,
                                       frequency,
                                       &CurveSelectionModule::CreateRhoSmtCurve,
                                       "Ohm*m",
                                       PresetType::RHO_SMT);
}

// v2.16.0: Пресет симметризованных фаз (phase_smt) для всех передатчиков T1-T5
ChartConfiguration CurveSelectionModule::CreatePhaseSmtPreset(FrequencyBand frequency) const {
    return BuildSmtPresetConfiguration(*this,
                                       frequency,
                                       &CurveSelectionModule::CreatePhaseSmtCurve,
                                       "mdeg",
                                       PresetType::PHASE_SMT);
}

void CurveSelectionModule::AddSmtSelectionCurves(std::vector<CurveDefinition>& curves,
                                                 FrequencyBand frequency,
                                                 int transmitterCount,
                                                 CurveDefinition (CurveSelectionModule::*createCurve)(int, FrequencyBand) const,
                                                 SmtComputationMode mode) const {
    for (int i = 0; i < transmitterCount; ++i) {
        CurveDefinition curve = (this->*createCurve)(i, frequency);
        curve.smtMode = mode;
        curve.yAxisGroup = 0;
        curve.enabled = true;
        curve.color = AssignSmartColor(curves, i, curve.yAxisGroup);
        curves.push_back(curve);
    }
}

void CurveSelectionModule::FinalizeSilentPresetCurve(CurveDefinition& curve,
                                                     const std::vector<CurveDefinition>& existingCurves,
                                                     int silentIndex) const {
    curve.yAxisGroup = 0;
    curve.enabled = true;
    curve.color = AssignSmartColor(existingCurves, silentIndex, curve.yAxisGroup);
    curve.displayName = GenerateDisplayName(ElementType::SILENT_TX, silentIndex, curve.dataType);
}

void CurveSelectionModule::CalculateRatioData(int transmitterIndex, FrequencyBand frequency,
                                             const DataModule* dataModule, std::vector<float>& outData) const {
    double minX = 0.0;
    double maxX = 0.0;
    if (!TryPrepareFullDataRange(dataModule, minX, maxX)) {
        outData.clear();
        return;
    }

    // Вызываем метод DataModule для вычисления отношений амплитуд
    dataModule->CalculateRatioData(transmitterIndex, frequency, outData, minX, maxX);
}

void CurveSelectionModule::CalculateDifferenceData(int transmitterIndex, FrequencyBand frequency,
                                                  const DataModule* dataModule, std::vector<float>& outData) const {
    double minX = 0.0;
    double maxX = 0.0;
    if (!TryPrepareFullDataRange(dataModule, minX, maxX)) {
        outData.clear();
        return;
    }

    // Вызываем метод DataModule для вычисления разностей фаз
    dataModule->CalculateDifferenceData(transmitterIndex, frequency, outData, minX, maxX);
}

void CurveSelectionModule::CalculateAmplitudeData(int transmitterIndex, int receiverIndex, FrequencyBand frequency,
                                                 const DataModule* dataModule, std::vector<float>& outData) const {
    double minX = 0.0;
    double maxX = 0.0;
    if (!TryPrepareFullDataRange(dataModule, minX, maxX)) {
        outData.clear();
        return;
    }

    // Вызываем метод DataModule для вычисления амплитуд
    dataModule->CalculateAmplitudeData(transmitterIndex, receiverIndex, frequency, outData, minX, maxX);
}

void CurveSelectionModule::CalculatePhaseData(int transmitterIndex, int receiverIndex, FrequencyBand frequency,
                                             const DataModule* dataModule, std::vector<float>& outData) const {
    double minX = 0.0;
    double maxX = 0.0;
    if (!TryPrepareFullDataRange(dataModule, minX, maxX)) {
        outData.clear();
        return;
    }

    // Вызываем метод DataModule для вычисления фаз
    dataModule->CalculatePhaseData(transmitterIndex, receiverIndex, frequency, outData, minX, maxX);
}

void CurveSelectionModule::CalculateSilentAmplitudeData(int receiverIndex, FrequencyBand frequency,
                                                       const DataModule* dataModule, std::vector<float>& outData) const {
    double minX = 0.0;
    double maxX = 0.0;
    if (!TryPrepareFullDataRange(dataModule, minX, maxX)) {
        outData.clear();
        return;
    }

    // Вызываем метод DataModule для вычисления амплитуд молчащих передатчиков
    dataModule->CalculateSilentAmplitudeData(receiverIndex, frequency, outData, minX, maxX);
}

void CurveSelectionModule::CalculatePhaseSmtData(int transmitterIndex, FrequencyBand frequency,
                                                 SmtComputationMode mode,
                                                 const DataModule* dataModule,
                                                 std::vector<float>& outData,
                                                 double minX, double maxX) const {
    outData.clear();
    if (!dataModule || !dataModule->HasData()) {
        return;
    }

    const size_t startIndex = static_cast<size_t>(std::max(0.0, minX));
    const size_t endIndex = static_cast<size_t>(std::min(static_cast<double>(dataModule->GetDataSize()), maxX + 1));
    if (startIndex >= endIndex) {
        return;
    }

    const int freqIdx = (frequency == FrequencyBand::FREQ_400_KHZ) ? 0 : 1;
    outData.reserve(endIndex - startIndex);

    const METROLOGY_DATA* metro = GetComputerSmtMetrology(mode, metrologyModule);

    for (size_t i = startIndex; i < endIndex; ++i) {
        const MeasurementView measurement = dataModule->GetMeasurementView(i);
        float phase = 0.0f;

        if (!TryGetSelectedPhaseSmt(measurement, freqIdx, transmitterIndex, metro, phase)) {
            continue;
        }

        phase = NormalizePhaseLocal(phase);
        outData.push_back(phase * kMdegPerRad);
    }
}

void CurveSelectionModule::CalculateRhoSmtData(int transmitterIndex, FrequencyBand frequency,
                                               SmtComputationMode mode,
                                               const DataModule* dataModule,
                                               std::vector<float>& outData,
                                               double minX, double maxX) const {
    outData.clear();
    if (!dataModule || !dataModule->HasData()) {
        return;
    }

    const size_t startIndex = static_cast<size_t>(std::max(0.0, minX));
    const size_t endIndex = static_cast<size_t>(std::min(static_cast<double>(dataModule->GetDataSize()), maxX + 1));
    if (startIndex >= endIndex) {
        return;
    }

    const int freqIdx = (frequency == FrequencyBand::FREQ_400_KHZ) ? 0 : 1;
    outData.reserve(endIndex - startIndex);

    const METROLOGY_DATA* metro = GetComputerSmtMetrology(mode, metrologyModule);

    for (size_t i = startIndex; i < endIndex; ++i) {
        const MeasurementView measurement = dataModule->GetMeasurementView(i);
        float rho = 0.0f;

        if (!TryGetSelectedRhoSmt(measurement, freqIdx, transmitterIndex, metro, rho)) {
            continue;
        }

        outData.push_back(rho);
    }
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

    const bool silentPhaseDifference = (dataType == DataType::SILENT_PHASE_DIFFERENCE);
    oss << GetElementDisplayToken(element, elementIndex, silentPhaseDifference) << " ";
    
    oss << GetDataTypeDisplayName(dataType);
    
    if (receiverIndex >= 0) {
        oss << " " << GetElementDisplayToken(ElementType::RECEIVER, receiverIndex);
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
        case DataType::SILENT_PHASE_DIFFERENCE:
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
        case DataType::SILENT_PHASE_DIFFERENCE:
            return "deg";
        case DataType::AMPLITUDE_RATIO:
            return "V/V";
        case DataType::TEMPERATURE:
            return "degC";
        case DataType::RHO_SMT:
            return "Ohm*m";
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
        case DataType::SILENT_PHASE_DIFFERENCE:
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





