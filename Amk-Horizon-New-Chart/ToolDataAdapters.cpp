#include "framework.h"
#include "ToolDataAdapters.h"
#include <cmath>
#include <cstring>

namespace {
float NormalizePhaseValue(float phaseRadians) {
    const float pi = 3.14159265358979323846f;
    while (phaseRadians > pi) {
        phaseRadians -= 2.0f * pi;
    }
    while (phaseRadians <= -pi) {
        phaseRadians += 2.0f * pi;
    }
    return phaseRadians;
}

int LegacyLinearIndex(int freqIdx, int txIdx) {
    return (freqIdx == 0) ? txIdx : (4 + txIdx);
}
}

ToolDataKind Autonom5TxAdapter::GetKind() const {
    return ToolDataKind::AUTONOM_5TX;
}

ToolCapabilities Autonom5TxAdapter::GetCapabilities() const {
    ToolCapabilities capabilities = {};
    capabilities.kind = ToolDataKind::AUTONOM_5TX;
    capabilities.transmitterCount = 5;
    capabilities.hasT5 = true;
    capabilities.hasSeparatePhaseRx = false;
    capabilities.hasDeltaPhase = true;
    capabilities.hasZeroAmplitudeRx1 = true;
    capabilities.hasZeroAmplitudeRx2 = true;
    capabilities.hasZeroDeltaPhase = true;
    capabilities.hasSondeRhoSmt = true;
    capabilities.hasSondePhaseSmt = true;
    capabilities.supportsBinContainer = true;
    capabilities.supportsBivContainer = true;
    capabilities.supportsDevContainer = true;
    capabilities.supportsComPolling = true;
    capabilities.supportsSilentData = true;
    return capabilities;
}

bool Autonom5TxAdapter::CanHandleSignature(uint32_t signature) const {
    const uint32_t toolCode = ToolDataAdapterRegistry::DecodeToolCode(signature);
    return toolCode == 151u || toolCode == 152u;
}

bool Autonom5TxAdapter::SupportsFileFormat(FileFormat format) const {
    return format == FileFormat::FORMAT_BIN || format == FileFormat::FORMAT_BIV || format == FileFormat::FORMAT_DEV;
}

size_t Autonom5TxAdapter::GetRawFrameSize() const {
    return sizeof(GP_TOOL_DATA);
}

bool Autonom5TxAdapter::ParseRawFrame(const void* rawFrame, size_t rawFrameSize, ParsedToolFrame& outFrame) const {
    outFrame = {};
    if (!rawFrame || rawFrameSize != sizeof(GP_TOOL_DATA)) {
        return false;
    }

    GP_TOOL_DATA canonicalData = {};
    memcpy(&canonicalData, rawFrame, sizeof(canonicalData));

    outFrame.kind = GetKind();
    outFrame.capabilities = GetCapabilities();
    outFrame.canonicalData = canonicalData;
    return true;
}

ToolDataKind Lwd4TxAdapter::GetKind() const {
    return ToolDataKind::LWD_4TX;
}

ToolCapabilities Lwd4TxAdapter::GetCapabilities() const {
    ToolCapabilities capabilities = {};
    capabilities.kind = ToolDataKind::LWD_4TX;
    capabilities.transmitterCount = 4;
    capabilities.hasT5 = false;
    capabilities.hasSeparatePhaseRx = true;
    capabilities.hasDeltaPhase = true;
    capabilities.hasZeroAmplitudeRx1 = true;
    capabilities.hasZeroAmplitudeRx2 = true;
    capabilities.hasZeroDeltaPhase = true;
    capabilities.hasSondeRhoSmt = true;
    capabilities.hasSondePhaseSmt = true;
    capabilities.supportsBinContainer = true;
    capabilities.supportsBivContainer = true;
    capabilities.supportsDevContainer = true;
    capabilities.supportsComPolling = true;
    capabilities.supportsSilentData = true;
    return capabilities;
}

bool Lwd4TxAdapter::CanHandleSignature(uint32_t signature) const {
    return ToolDataAdapterRegistry::DecodeToolCode(signature) == 241u;
}

bool Lwd4TxAdapter::SupportsFileFormat(FileFormat format) const {
    return format == FileFormat::FORMAT_BIN || format == FileFormat::FORMAT_BIV || format == FileFormat::FORMAT_DEV;
}

size_t Lwd4TxAdapter::GetRawFrameSize() const {
    return sizeof(LEGACY_GP_TOOL_DATA);
}

bool Lwd4TxAdapter::ParseRawFrame(const void* rawFrame, size_t rawFrameSize, ParsedToolFrame& outFrame) const {
    outFrame = {};
    if (!rawFrame || rawFrameSize != sizeof(LEGACY_GP_TOOL_DATA)) {
        return false;
    }

    LEGACY_GP_TOOL_DATA legacyData = {};
    memcpy(&legacyData, rawFrame, sizeof(legacyData));

    GP_TOOL_DATA canonicalData = {};
    canonicalData.signature = legacyData.signature;
    canonicalData.condition = legacyData.condition;
    canonicalData.frame = legacyData.frame;
    canonicalData.temperature = legacyData.temperature;

    for (int freqIdx = 0; freqIdx < 2; ++freqIdx) {
        for (int txIdx = 0; txIdx < 4; ++txIdx) {
            const int linearIndex = LegacyLinearIndex(freqIdx, txIdx);
            canonicalData.rho_smt[freqIdx][txIdx] = legacyData.Ro_smt[linearIndex];
            canonicalData.phase_smt[freqIdx][txIdx] = legacyData.PH_smt[linearIndex];
            canonicalData.AM_RX_1[freqIdx][txIdx] = legacyData.AM_RX_1[linearIndex];
            canonicalData.AM_RX_2[freqIdx][txIdx] = legacyData.AM_RX_2[linearIndex];

            float delta = legacyData.PH_RX_2[linearIndex] - legacyData.PH_RX_1[linearIndex];
            if (std::fabs(delta) < 40.0f) {
                delta = NormalizePhaseValue(delta);
            } else {
                delta = 0.0f;
            }
            canonicalData.DELTA_PH[freqIdx][txIdx] = delta;
        }

        canonicalData.ZERO_AM_RX_1[freqIdx] = legacyData.AM_RX_1[8 + freqIdx];
        canonicalData.ZERO_AM_RX_2[freqIdx] = legacyData.AM_RX_2[8 + freqIdx];

        float zeroDelta = legacyData.PH_RX_2[8 + freqIdx] - legacyData.PH_RX_1[8 + freqIdx];
        if (std::fabs(zeroDelta) < 40.0f) {
            zeroDelta = NormalizePhaseValue(zeroDelta);
        } else {
            zeroDelta = 0.0f;
        }
        canonicalData.ZERO_dPH[freqIdx] = zeroDelta;
    }

    outFrame.kind = GetKind();
    outFrame.capabilities = GetCapabilities();
    outFrame.canonicalData = canonicalData;
    return true;
}

const ToolDataAdapterRegistry& ToolDataAdapterRegistry::Get() {
    static ToolDataAdapterRegistry instance;
    return instance;
}

const IToolDataAdapter* ToolDataAdapterRegistry::FindAdapterBySignature(uint32_t signature) const {
    for (const auto& adapter : adapters) {
        if (adapter && adapter->CanHandleSignature(signature)) {
            return adapter.get();
        }
    }
    return nullptr;
}

bool ToolDataAdapterRegistry::ParseRawFrame(const void* rawFrame, size_t rawFrameSize, ParsedToolFrame& outFrame) const {
    outFrame = {};
    if (!rawFrame || rawFrameSize < sizeof(uint32_t)) {
        return false;
    }

    uint32_t signature = 0;
    memcpy(&signature, rawFrame, sizeof(signature));

    const IToolDataAdapter* adapter = FindAdapterBySignature(signature);
    if (!adapter) {
        return false;
    }

    return adapter->ParseRawFrame(rawFrame, rawFrameSize, outFrame);
}

bool ToolDataAdapterRegistry::IsSupportedSignature(uint32_t signature) const {
    return FindAdapterBySignature(signature) != nullptr;
}

uint32_t ToolDataAdapterRegistry::DecodeToolCode(uint32_t signature) {
    const uint32_t low20 = (signature << 12) >> 12;
    return low20 / 1000u;
}

ToolDataKind ToolDataAdapterRegistry::DetermineToolDataKind(uint32_t signature) {
    const uint32_t toolCode = DecodeToolCode(signature);
    if (toolCode == 151u) {
        return ToolDataKind::AUTONOM_5TX;
    }
    if (toolCode == 152u) {
        return ToolDataKind::AUTONOM_5TX_SDR;
    }
    if (toolCode == 241u) {
        return ToolDataKind::LWD_4TX;
    }
    return ToolDataKind::UNKNOWN;
}

ToolDataAdapterRegistry::ToolDataAdapterRegistry() {
    adapters.emplace_back(new Autonom5TxAdapter());
    adapters.emplace_back(new Lwd4TxAdapter());
}
