#pragma once

#include "ToolDataTypes.h"
#include <cstddef>
#include <cstdint>
#include <memory>
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

class Autonom5TxAdapter : public IToolDataAdapter {
public:
    ToolDataKind GetKind() const override;
    ToolCapabilities GetCapabilities() const override;
    bool CanHandleSignature(uint32_t signature) const override;
    bool SupportsFileFormat(FileFormat format) const override;
    size_t GetRawFrameSize() const override;
    bool ParseRawFrame(const void* rawFrame, size_t rawFrameSize, ParsedToolFrame& outFrame) const override;
};

// LWD с canonical GP_TOOL_DATA на проводе (как у AUTONOM 5Tx): неиспользуемые передатчики — нули в слотах T4/T5.
class Lwd3TxCanonicalAdapter : public IToolDataAdapter {
public:
    ToolDataKind GetKind() const override;
    ToolCapabilities GetCapabilities() const override;
    bool CanHandleSignature(uint32_t signature) const override;
    bool SupportsFileFormat(FileFormat format) const override;
    size_t GetRawFrameSize() const override;
    bool ParseRawFrame(const void* rawFrame, size_t rawFrameSize, ParsedToolFrame& outFrame) const override;
};

class Lwd4TxAdapter : public IToolDataAdapter {
public:
    ToolDataKind GetKind() const override;
    ToolCapabilities GetCapabilities() const override;
    bool CanHandleSignature(uint32_t signature) const override;
    bool SupportsFileFormat(FileFormat format) const override;
    size_t GetRawFrameSize() const override;
    bool ParseRawFrame(const void* rawFrame, size_t rawFrameSize, ParsedToolFrame& outFrame) const override;
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
