#pragma once

#include <cstdint>

#pragma pack(push, 1)
struct GP_TOOL_DATA {
    uint32_t signature;
    uint32_t condition;
    uint32_t frame;
    float temperature;
    float rho_smt[2][5];
    float phase_smt[2][5];
    float AM_RX_1[2][5];
    float ZERO_AM_RX_1[2];
    float AM_RX_2[2][5];
    float ZERO_AM_RX_2[2];
    float DELTA_PH[2][5];
    float ZERO_dPH[2];
};
#pragma pack(pop)

static_assert(sizeof(GP_TOOL_DATA) == 240, "GP_TOOL_DATA must be exactly 240 bytes");

enum class FileFormat {
    FORMAT_BIN,
    FORMAT_BIV,
    FORMAT_DEV,
    FORMAT_UNKNOWN
};

// Тип прибора: определяется из первой цифры palette (toolCode).
//   1 = AUTONOM, 2 = LWD. Расширяемо для будущих типов.
enum class DeviceType {
    UNKNOWN = 0,
    AUTONOM = 1,   // Автономный прибор
    LWD     = 2    // LWD (Logging While Drilling)
};

// ToolDataKind — комбинация типа прибора + числа передатчиков.
// Определяется динамически из palette (toolCode = XYZ):
//   X = DeviceType (1=AUTONOM, 2=LWD)
//   Y = transmitterCount (3, 4, 5)
//   Z = modification
// Enum сохраняется для обратной совместимости структур.
enum class ToolDataKind {
    UNKNOWN,
    AUTONOM,     // Автономный прибор (любое число передатчиков)
    LWD          // LWD прибор (любое число передатчиков)
};

static constexpr int kFallbackTransmitterCount = 5;

struct ToolCapabilities {
    ToolDataKind kind = ToolDataKind::UNKNOWN;
    int transmitterCount = 0;
    bool hasT5 = false;
    bool hasSeparatePhaseRx = false;
    bool hasDeltaPhase = false;
    bool hasZeroAmplitudeRx1 = false;
    bool hasZeroAmplitudeRx2 = false;
    bool hasZeroDeltaPhase = false;
    bool hasSondeRhoSmt = false;
    bool hasSondePhaseSmt = false;
    bool supportsBinContainer = false;
    bool supportsBivContainer = false;
    bool supportsDevContainer = false;
    bool supportsComPolling = false;
    bool supportsSilentData = false;
};

struct ToolRecordMetadata {
    ToolDataKind kind = ToolDataKind::UNKNOWN;
    ToolCapabilities capabilities = {};
};
