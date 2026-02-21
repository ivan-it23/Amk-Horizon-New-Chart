#include "framework.h"
#include "MetrologyModule.h"

#include <fstream>
#include <bitset>
#include <sstream>

MetrologyModule::MetrologyModule() {
    ZeroMemory(&current, sizeof(current));
}

void MetrologyModule::Clear() {
    ZeroMemory(&current, sizeof(current));
    hasData = false;
    dirty = false;
    currentFilePath.clear();
    selectedKey = 0;
}

METROLOGY_DATA& MetrologyModule::GetMutable() {
    dirty = true;
    hasData = true;
    return current;
}

void MetrologyModule::Set(const METROLOGY_DATA& data, bool markDirty) {
    current = data;
    hasData = true;
    if (markDirty) dirty = true;
}

bool MetrologyModule::LoadFromFile(const std::wstring& path, std::wstring& outError) {
    outError.clear();

    std::ifstream f(path, std::ios::binary);
    if (!f) {
        outError = L"Не удалось открыть файл.";
        return false;
    }

    // Read signature first (4 bytes)
    uint32_t sig = 0;
    f.read(reinterpret_cast<char*>(&sig), sizeof(sig));
    if (!f) {
        outError = L"Не удалось прочитать signature из файла.";
        return false;
    }

    if (!IsSupportedAutonom5TxSignature(sig)) {
        SignatureInfo info = DecodeSignature(sig);
        std::wstringstream ss;
        ss << L"Неподдерживаемый тип прибора по signature.\n"
           << L"signature=0x" << std::hex << sig << std::dec << L"\n"
           << L"sonde_type=" << info.sondeType << L"\n\n"
           << L"Поддерживается только AUTONOM 5Tx (151/152).";
        outError = ss.str();
        return false;
    }

    // Rewind and read whole struct (stable format: 240 bytes)
    f.clear();
    f.seekg(0, std::ios::beg);

    METROLOGY_DATA tmp{};
    f.read(reinterpret_cast<char*>(&tmp), sizeof(tmp));
    if (!f) {
        outError = L"Файл имеет неверный размер или повреждён (ожидается 240 байт).";
        return false;
    }

    // Ensure there's no extra bytes? (optional) We'll accept larger but read first 240.
    current = tmp;
    hasData = true;
    dirty = false;
    currentFilePath = path;
    return true;
}

bool MetrologyModule::SaveToFile(const std::wstring& path, std::wstring& outError) const {
    outError.clear();

    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) {
        outError = L"Не удалось открыть файл для записи.";
        return false;
    }

    f.write(reinterpret_cast<const char*>(&current), sizeof(current));
    if (!f) {
        outError = L"Ошибка записи файла.";
        return false;
    }

    return true;
}

MetrologyModule::SignatureInfo MetrologyModule::DecodeSignature(uint32_t signature) {
    SignatureInfo info;
    info.low20 = (signature << 12) >> 12; // keep only low 20 bits
    info.sondeType = info.low20 / 1000;
    info.signatureForPalette = info.sondeType; // per doc: low20/1000
    return info;
}

bool MetrologyModule::IsSupportedAutonom5TxSignature(uint32_t signature) {
    SignatureInfo info = DecodeSignature(signature);
    return (info.sondeType == 151u || info.sondeType == 152u);
}

uint32_t MetrologyModule::BuildSignature(uint32_t sondeType151or152, int mod1to9, int number0to99, uint32_t existingSignature) {
    if (sondeType151or152 != 151u && sondeType151or152 != 152u) {
        sondeType151or152 = 151u;
    }

    if (mod1to9 < 1) mod1to9 = 1;
    if (mod1to9 > 9) mod1to9 = 9;
    if (number0to99 < 0) number0to99 = 0;
    if (number0to99 > 999) number0to99 = 999;

    uint32_t high12 = existingSignature & 0xFFF00000u;

    // Совместимо со старым приложением:
    // low20 = tool_type*100000 + tool_N_Tx*10000 + tool_mod*1000 + tool_number
    // Для AUTONOM 5Tx: tool_type=1, tool_N_Tx=5 => база 150000.
    const uint32_t toolType = 1u;
    const uint32_t ntx = 5u;
    uint32_t low20 = toolType * 100000u + ntx * 10000u + static_cast<uint32_t>(mod1to9 * 1000 + number0to99);

    // Для SDR варианта сохраняем совместимость по старой схеме как AUTONOM 5Tx.
    (void)sondeType151or152;

    low20 &= 0x000FFFFFu;
    return high12 | low20;
}

std::wstring MetrologyModule::SignatureToBinaryString(uint32_t signature) {
    std::bitset<32> bits(signature);
    std::wstring s;
    s.reserve(32);
    for (int i = 31; i >= 0; --i) {
        s.push_back(bits.test(i) ? L'1' : L'0');
    }
    return s;
}

