#pragma once

#include "ToolDataTypes.h"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct ParsedToolFrame {
    ToolDataKind kind = ToolDataKind::UNKNOWN;
    ToolCapabilities capabilities = {};
    GP_TOOL_DATA canonicalData = {};
};

class IToolDataAdapter {
public:
    virtual ~IToolDataAdapter() = default;

    virtual ToolDataKind GetKind() const = 0;
    virtual ToolCapabilities GetCapabilities() const = 0;
    virtual bool CanHandleSignature(uint32_t signature) const = 0;
    virtual bool SupportsFileFormat(FileFormat format) const = 0;
    virtual size_t GetRawFrameSize() const = 0;
    virtual bool ParseRawFrame(const void* rawFrame, size_t rawFrameSize, ParsedToolFrame& outFrame) const = 0;
};

// Универсальный canonical-адаптер. Единственный адаптер в системе.
// Все приборы шлют canonical GP_TOOL_DATA (240 байт).
// Тип прибора, количество передатчиков и модификация определяются из palette (toolCode = XYZ):
//   X = тип прибора (1=Автоном, 2=LWD)
//   Y = количество передатчиков (3, 4, 5)
//   Z = модификация
class UniversalCanonicalAdapter : public IToolDataAdapter {
public:
    ToolDataKind GetKind() const override;
    ToolCapabilities GetCapabilities() const override;
    bool CanHandleSignature(uint32_t signature) const override;
    bool SupportsFileFormat(FileFormat format) const override;
    size_t GetRawFrameSize() const override;
    bool ParseRawFrame(const void* rawFrame, size_t rawFrameSize, ParsedToolFrame& outFrame) const override;

    // === Palette (toolCode) декодирование ===
    // toolCode = XYZ: X=тип, Y=txCount, Z=модификация
    static DeviceType  DetermineDeviceType(uint32_t toolCode);
    static int         DetermineTransmitterCount(uint32_t toolCode);
    static int         DetermineModification(uint32_t toolCode);
    static ToolDataKind DetermineKindFromToolCode(uint32_t toolCode);
    static ToolCapabilities BuildCapabilities(uint32_t toolCode);

    // Динамическое имя прибора для отображения в UI/логах
    static std::wstring BuildDisplayName(uint32_t toolCode);
};

class ToolDataAdapterRegistry {
public:
    static const ToolDataAdapterRegistry& Get();

    const IToolDataAdapter* FindAdapterBySignature(uint32_t signature) const;
    bool ParseRawFrame(const void* rawFrame, size_t rawFrameSize, ParsedToolFrame& outFrame) const;
    bool IsSupportedSignature(uint32_t signature) const;

    static uint32_t DecodeToolCode(uint32_t signature);
    static ToolDataKind DetermineToolDataKind(uint32_t signature);

private:
    ToolDataAdapterRegistry();

    std::vector<std::unique_ptr<IToolDataAdapter>> adapters;
};
