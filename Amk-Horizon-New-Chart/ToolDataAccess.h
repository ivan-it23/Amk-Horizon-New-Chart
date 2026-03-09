#pragma once

#include "ToolDataAdapters.h"

inline bool TryExtractCanonicalData(const ParsedToolFrame& parsedFrame, GP_TOOL_DATA& outData) {
    outData = parsedFrame.canonicalData;
    return true;
}

class ToolDataAccessView {
public:
    ToolDataAccessView()
        : record(nullptr)
        , capabilities() {
    }

    explicit ToolDataAccessView(const GP_TOOL_DATA& inRecord)
        : record(&inRecord)
        , capabilities() {
        const IToolDataAdapter* adapter = ToolDataAdapterRegistry::Get().FindAdapterBySignature(inRecord.signature);
        if (adapter) {
            capabilities = adapter->GetCapabilities();
        }
    }

    ToolDataAccessView(const GP_TOOL_DATA& inRecord, const ToolCapabilities& inCapabilities)
        : record(&inRecord)
        , capabilities(inCapabilities) {
    }

    ToolDataAccessView(const GP_TOOL_DATA& inRecord, const ToolRecordMetadata& inMetadata)
        : record(&inRecord)
        , capabilities(inMetadata.capabilities) {
    }

    bool IsValid() const {
        return record != nullptr;
    }

    ToolDataKind GetKind() const {
        return capabilities.kind;
    }

    const ToolCapabilities& GetCapabilities() const {
        return capabilities;
    }

    int GetTransmitterCount() const {
        return (capabilities.transmitterCount > 0) ? capabilities.transmitterCount : kLegacyFallbackTransmitterCount;
    }

    bool SupportsTransmitter(int transmitterIndex) const {
        return transmitterIndex >= 0 && transmitterIndex < GetTransmitterCount();
    }

    bool TryGetDeltaPhase(int freqIdx, int transmitterIndex, float& outValue) const {
        if (!record || !SupportsTransmitter(transmitterIndex) || !capabilities.hasDeltaPhase || freqIdx < 0 || freqIdx > 1) {
            return false;
        }
        outValue = record->DELTA_PH[freqIdx][transmitterIndex];
        return true;
    }

    bool TryGetAmplitudeRx1(int freqIdx, int transmitterIndex, float& outValue) const {
        if (!record || !SupportsTransmitter(transmitterIndex) || freqIdx < 0 || freqIdx > 1) {
            return false;
        }
        outValue = record->AM_RX_1[freqIdx][transmitterIndex];
        return true;
    }

    bool TryGetAmplitudeRx2(int freqIdx, int transmitterIndex, float& outValue) const {
        if (!record || !SupportsTransmitter(transmitterIndex) || freqIdx < 0 || freqIdx > 1) {
            return false;
        }
        outValue = record->AM_RX_2[freqIdx][transmitterIndex];
        return true;
    }

    bool TryGetPhaseSmt(int freqIdx, int transmitterIndex, float& outValue) const {
        if (!record || !SupportsTransmitter(transmitterIndex) || !capabilities.hasSondePhaseSmt || freqIdx < 0 || freqIdx > 1) {
            return false;
        }
        outValue = record->phase_smt[freqIdx][transmitterIndex];
        return true;
    }

    bool TryGetRhoSmt(int freqIdx, int transmitterIndex, float& outValue) const {
        if (!record || !SupportsTransmitter(transmitterIndex) || !capabilities.hasSondeRhoSmt || freqIdx < 0 || freqIdx > 1) {
            return false;
        }
        outValue = record->rho_smt[freqIdx][transmitterIndex];
        return true;
    }

    bool TryGetSilentAmplitudeRx1(int freqIdx, float& outValue) const {
        if (!record || !capabilities.hasZeroAmplitudeRx1 || freqIdx < 0 || freqIdx > 1) {
            return false;
        }
        outValue = record->ZERO_AM_RX_1[freqIdx];
        return true;
    }

    bool TryGetSilentAmplitudeRx2(int freqIdx, float& outValue) const {
        if (!record || !capabilities.hasZeroAmplitudeRx2 || freqIdx < 0 || freqIdx > 1) {
            return false;
        }
        outValue = record->ZERO_AM_RX_2[freqIdx];
        return true;
    }

    bool TryGetSilentDeltaPhase(int freqIdx, float& outValue) const {
        if (!record || !capabilities.hasZeroDeltaPhase || freqIdx < 0 || freqIdx > 1) {
            return false;
        }
        outValue = record->ZERO_dPH[freqIdx];
        return true;
    }

    float GetTemperature() const {
        return record ? record->temperature : 0.0f;
    }

private:
    const GP_TOOL_DATA* record;
    ToolCapabilities capabilities;
};

class MeasurementView {
public:
    MeasurementView()
        : record(nullptr)
        , metadata() {
    }

    explicit MeasurementView(const GP_TOOL_DATA& inRecord)
        : record(&inRecord)
        , metadata() {
        const IToolDataAdapter* adapter = ToolDataAdapterRegistry::Get().FindAdapterBySignature(inRecord.signature);
        if (adapter) {
            metadata.kind = adapter->GetKind();
            metadata.capabilities = adapter->GetCapabilities();
        }
    }

    MeasurementView(const GP_TOOL_DATA& inRecord, const ToolRecordMetadata& inMetadata)
        : record(&inRecord)
        , metadata(inMetadata) {
    }

    bool IsValid() const {
        return record != nullptr;
    }

    const GP_TOOL_DATA* GetRecord() const {
        return record;
    }

    const ToolRecordMetadata& GetMetadata() const {
        return metadata;
    }

    ToolDataKind GetKind() const {
        return metadata.kind;
    }

    const ToolCapabilities& GetCapabilities() const {
        return metadata.capabilities;
    }

    uint32_t GetSignature() const {
        return record ? record->signature : 0;
    }

    float GetTemperature() const {
        return record ? record->temperature : 0.0f;
    }

    ToolDataAccessView GetAccessView() const {
        if (!record) {
            return ToolDataAccessView();
        }

        return ToolDataAccessView(*record, metadata);
    }

private:
    const GP_TOOL_DATA* record;
    ToolRecordMetadata metadata;
};
