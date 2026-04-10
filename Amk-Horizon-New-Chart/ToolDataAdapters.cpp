#include "framework.h"
#include "ToolDataAdapters.h"
#include <cstring>

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

// ===== Palette (toolCode) декодирование =====
// toolCode = XYZ: X=тип прибора, Y=кол-во передатчиков, Z=модификация

DeviceType UniversalCanonicalAdapter::DetermineDeviceType(uint32_t toolCode) {
    const int hundredsDigit = static_cast<int>(toolCode / 100u);
    switch (hundredsDigit) {
        case 1: return DeviceType::AUTONOM;
        case 2: return DeviceType::LWD;
        default: return DeviceType::UNKNOWN;
    }
}

int UniversalCanonicalAdapter::DetermineTransmitterCount(uint32_t toolCode) {
    const int tensDigit = static_cast<int>((toolCode / 10u) % 10u);
    if (tensDigit >= 3 && tensDigit <= 5) {
        return tensDigit;
    }
    return kFallbackTransmitterCount;
}

int UniversalCanonicalAdapter::DetermineModification(uint32_t toolCode) {
    return static_cast<int>(toolCode % 10u);
}

ToolDataKind UniversalCanonicalAdapter::DetermineKindFromToolCode(uint32_t toolCode) {
    const DeviceType dt = DetermineDeviceType(toolCode);
    switch (dt) {
        case DeviceType::AUTONOM: return ToolDataKind::AUTONOM;
        case DeviceType::LWD:     return ToolDataKind::LWD;
        default:                  return ToolDataKind::UNKNOWN;
    }
}

ToolCapabilities UniversalCanonicalAdapter::BuildCapabilities(uint32_t toolCode) {
    const int txCount = DetermineTransmitterCount(toolCode);
    const ToolDataKind kind = DetermineKindFromToolCode(toolCode);

    ToolCapabilities cap = {};
    cap.kind = kind;
    cap.transmitterCount = txCount;
    cap.hasT5 = (txCount >= 5);
    cap.hasSeparatePhaseRx = true;
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

std::wstring UniversalCanonicalAdapter::BuildDisplayName(uint32_t toolCode) {
    const DeviceType dt = DetermineDeviceType(toolCode);
    const int txCount = DetermineTransmitterCount(toolCode);

    std::wstring typeName;
    switch (dt) {
        case DeviceType::AUTONOM: typeName = L"Автоном"; break;
        case DeviceType::LWD:     typeName = L"LWD";     break;
        default:                  typeName = L"Неизвестно"; break;
    }

    return typeName + L" " + std::to_wstring(txCount) + L"TX";
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
    adapters.emplace_back(new UniversalCanonicalAdapter());
}
