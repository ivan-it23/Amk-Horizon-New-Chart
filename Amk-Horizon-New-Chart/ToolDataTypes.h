#pragma once

#include <cstdint>

#pragma pack(push, 1)
struct LEGACY_GP_TOOL_DATA {
    uint32_t signature;
    uint32_t condition;
    float Ro_smt[8];
    float PH_smt[8];
    uint32_t frame;
    float temperature;
    float AM_RX_1[10];
    float AM_RX_2[10];
    float PH_RX_1[10];
    float PH_RX_2[10];
};

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

static_assert(sizeof(LEGACY_GP_TOOL_DATA) == 240, "LEGACY_GP_TOOL_DATA must be exactly 240 bytes");
static_assert(sizeof(GP_TOOL_DATA) == 240, "GP_TOOL_DATA must be exactly 240 bytes");

enum class FileFormat {
    FORMAT_BIN,
    FORMAT_BIV,
    FORMAT_DEV,
    FORMAT_UNKNOWN
};

enum class ToolDataKind {
    UNKNOWN,
    AUTONOM_5TX,
    AUTONOM_5TX_SDR,
    LWD_4TX
};

static constexpr int kLegacyFallbackTransmitterCount = 5;

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
