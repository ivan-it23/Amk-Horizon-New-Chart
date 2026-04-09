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

ToolDataKind Lwd3TxCanonicalAdapter::GetKind() const {
    return ToolDataKind::LWD_3TX;
}

ToolCapabilities Lwd3TxCanonicalAdapter::GetCapabilities() const {
    ToolCapabilities capabilities = {};
    capabilities.kind = ToolDataKind::LWD_3TX;
    capabilities.transmitterCount = 3;
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

bool Lwd3TxCanonicalAdapter::CanHandleSignature(uint32_t signature) const {
    return ToolDataAdapterRegistry::DecodeToolCode(signature) == kToolCodeLwd3TxCanonical;
}

bool Lwd3TxCanonicalAdapter::SupportsFileFormat(FileFormat format) const {
    return format == FileFormat::FORMAT_BIN || format == FileFormat::FORMAT_BIV || format == FileFormat::FORMAT_DEV;
}

size_t Lwd3TxCanonicalAdapter::GetRawFrameSize() const {
    return sizeof(GP_TOOL_DATA);
}

bool Lwd3TxCanonicalAdapter::ParseRawFrame(const void* rawFrame, size_t rawFrameSize, ParsedToolFrame& outFrame) const {
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
    if (ToolDataAdapterRegistry::DecodeToolCode(signature) == 241u) {
        return true;
    }
    // Дополнительные известные 4Tx signature, не декодируемые как toolCode==241
    return signature == 0x3143ADD4u ||
           signature == 0x31443454u ||
           signature == 0x31444453u ||
           signature == 0x31443453u;
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
    const IToolDataAdapter* adapter = Get().FindAdapterBySignature(signature);
    if (adapter) {
        const ToolDataKind kind = adapter->GetKind();
        if (kind != ToolDataKind::UNKNOWN) {
            return kind;
        }
    }
    // UniversalCanonicalAdapter возвращает UNKNOWN из GetKind() —
    // определяем kind динамически из toolCode.
    const uint32_t toolCode = DecodeToolCode(signature);
    return UniversalCanonicalAdapter::DetermineKindFromToolCode(toolCode);
}

// ===== Универсальный Адаптер =====

int UniversalCanonicalAdapter::DetermineTransmitterCount(uint32_t toolCode) {
    // Десятки toolCode кодируют количество зондов:
    //   x3x -> 3Tx, x4x -> 4Tx, x5x -> 5Tx
    const int tensDigit = static_cast<int>((toolCode / 10u) % 10u);
    if (tensDigit >= 3 && tensDigit <= 5) {
        return tensDigit;
    }
    return 5; // fallback: 5Tx
}

ToolDataKind UniversalCanonicalAdapter::DetermineKindFromToolCode(uint32_t toolCode) {
    const int txCount = DetermineTransmitterCount(toolCode);
    switch (txCount) {
        case 3: return ToolDataKind::LWD_3TX;
        case 4: return ToolDataKind::LWD_4TX;
        default: return ToolDataKind::AUTONOM_5TX;
    }
}

ToolCapabilities UniversalCanonicalAdapter::BuildCapabilities(uint32_t toolCode) {
    const int txCount = DetermineTransmitterCount(toolCode);
    const ToolDataKind kind = DetermineKindFromToolCode(toolCode);

    ToolCapabilities cap = {};
    cap.kind = kind;
    cap.transmitterCount = txCount;
    cap.hasT5 = (txCount >= 5);
    cap.hasSeparatePhaseRx = (txCount < 5);
    cap.hasDeltaPhase = true;
    cap.hasZeroAmplitudeRx1 = true;
    cap.hasZeroAmplitudeRx2 = true;
    cap.hasZeroDeltaPhase = true;
    cap.hasSondeRhoSmt = true;
    cap.hasSondePhaseSmt = true;
    cap.supportsBinContainer = true;
    cap.supportsBivContainer = true;
    cap.supportsDevContainer = true;
    cap.supportsComPolling = true;
    cap.supportsSilentData = true;
    return cap;
}

ToolDataKind UniversalCanonicalAdapter::GetKind() const {
    return ToolDataKind::UNKNOWN; // определяется динамически в ParseRawFrame
}

ToolCapabilities UniversalCanonicalAdapter::GetCapabilities() const {
    return {}; // определяется динамически в ParseRawFrame
}

bool UniversalCanonicalAdapter::CanHandleSignature(uint32_t signature) const {
    // Catch-all: принимает любую сигнатуру с разумным toolCode (> 0)
    const uint32_t toolCode = ToolDataAdapterRegistry::DecodeToolCode(signature);
    return toolCode > 0;
}

bool UniversalCanonicalAdapter::SupportsFileFormat(FileFormat format) const {
    return format == FileFormat::FORMAT_BIN || format == FileFormat::FORMAT_BIV || format == FileFormat::FORMAT_DEV;
}

size_t UniversalCanonicalAdapter::GetRawFrameSize() const {
    return sizeof(GP_TOOL_DATA);
}

bool UniversalCanonicalAdapter::ParseRawFrame(const void* rawFrame, size_t rawFrameSize, ParsedToolFrame& outFrame) const {
    outFrame = {};
    if (!rawFrame || rawFrameSize != sizeof(GP_TOOL_DATA)) {
        return false;
    }

    GP_TOOL_DATA canonicalData = {};
    memcpy(&canonicalData, rawFrame, sizeof(canonicalData));

    const uint32_t toolCode = ToolDataAdapterRegistry::DecodeToolCode(canonicalData.signature);
    outFrame.kind = DetermineKindFromToolCode(toolCode);
    outFrame.capabilities = BuildCapabilities(toolCode);
    outFrame.canonicalData = canonicalData;
    return true;
}

// ===== ToolDataAdapterRegistry =====

ToolDataAdapterRegistry::ToolDataAdapterRegistry() {
    adapters.emplace_back(new Autonom5TxAdapter());         // 151, 152
    adapters.emplace_back(new Lwd3TxCanonicalAdapter());    // 231
    adapters.emplace_back(new UniversalCanonicalAdapter());  // 241, 242, любые новые — canonical passthrough
    adapters.emplace_back(new Lwd4TxAdapter());             // legacy fallback (для старых файлов, не достигается через FindAdapterBySignature)
}
