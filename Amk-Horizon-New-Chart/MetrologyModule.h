#pragma once

#include "framework.h"
#include "SerialModule.h"
#include <cstdint>
#include <string>

class MetrologyModule {
public:
    struct SignatureInfo {
        uint32_t sondeType = 0;          // decoded as (((sig<<12)>>12)/1000)
        uint32_t low20 = 0;              // (sig & 0xFFFFF)
        uint32_t signatureForPalette = 0; // low20/1000
    };

    MetrologyModule();

    void Clear();

    bool HasData() const { return hasData; }
    bool IsDirty() const { return dirty; }

    const METROLOGY_DATA& Get() const { return current; }
    METROLOGY_DATA& GetMutable(); // marks dirty

    void Set(const METROLOGY_DATA& data, bool markDirty = false);

    const std::wstring& GetCurrentFilePath() const { return currentFilePath; }
    void SetCurrentFilePath(const std::wstring& path) { currentFilePath = path; }

    uint32_t GetSelectedKey() const { return selectedKey; }
    void SetSelectedKey(uint32_t key) { selectedKey = key; dirty = true; }

    // Stable file format: exactly 240 bytes METROLOGY_DATA
    bool LoadFromFile(const std::wstring& path, std::wstring& outError);
    bool SaveToFile(const std::wstring& path, std::wstring& outError) const;

    static SignatureInfo DecodeSignature(uint32_t signature);
    static bool IsSupportedAutonom5TxSignature(uint32_t signature);

    // Builds signature in the same "low20 = sondeType*1000 + suffix" style.
    // For this project we only support AUTONOM 5Tx types (151 or 152).
    static uint32_t BuildSignature(uint32_t sondeType151or152, int mod1to9, int number0to99, uint32_t existingSignature);

    static std::wstring SignatureToBinaryString(uint32_t signature);

private:
    METROLOGY_DATA current{};
    bool hasData = false;
    bool dirty = false;
    std::wstring currentFilePath;
    uint32_t selectedKey = 0;
};

