#include "framework.h"
#include "DeviceLogPayloadFormat.h"
#include "DeviceExchangeLog.h"
#include "ToolDataAdapters.h"
#include "ToolDataTypes.h"
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <string>

namespace {

static void AppendFmt(std::wstring& s, const wchar_t* fmt, ...) {
    wchar_t buf[1024];
    va_list args;
    va_start(args, fmt);
    _vsnwprintf_s(buf, static_cast<size_t>(_countof(buf)), _TRUNCATE, fmt, args);
    va_end(args);
    if (!s.empty()) {
        s += L"\r\n";
    }
    s += buf;
}

static std::wstring Fw(float x) {
    wchar_t b[80];
    swprintf_s(b, L"%.6g", static_cast<double>(x));
    return std::wstring(b);
}

static int NormalizeTxCount(int txFromCaps) {
    if (txFromCaps <= 0) {
        return 5;
    }
    return (std::min)(5, (std::max)(1, txFromCaps));
}

static std::wstring FormatGpToolDataVariantA(const GP_TOOL_DATA& d, int transmitterCount) {
    const int nTx = NormalizeTxCount(transmitterCount);
    std::wstring s;
    AppendFmt(s, L"[GP_TOOL_DATA — разбор полей]");
    AppendFmt(s, L"");
    AppendFmt(s, L"УЭС (rho_smt), Ом·м");
    for (int fi = 0; fi < 2; ++fi) {
        const wchar_t* freqLabel = (fi == 0) ? L"400 кГц" : L"2000 кГц";
        std::wstring line = std::wstring(L"  ") + freqLabel + L":";
        for (int t = 0; t < nTx; ++t) {
            line += L"  T" + std::to_wstring(t + 1) + L"=" + Fw(d.rho_smt[fi][t]);
        }
        if (!s.empty()) {
            s += L"\r\n";
        }
        s += line;
    }

    AppendFmt(s, L"");
    AppendFmt(s, L"Симметризованные фазы (phase_smt)");
    for (int fi = 0; fi < 2; ++fi) {
        const wchar_t* freqLabel = (fi == 0) ? L"400 кГц" : L"2000 кГц";
        std::wstring line = std::wstring(L"  ") + freqLabel + L":";
        for (int t = 0; t < nTx; ++t) {
            line += L"  T" + std::to_wstring(t + 1) + L"=" + Fw(d.phase_smt[fi][t]);
        }
        if (!s.empty()) {
            s += L"\r\n";
        }
        s += line;
    }

    AppendFmt(s, L"");
    AppendFmt(s, L"Амплитуды приёмника 1 (AM_RX_1)");
    for (int fi = 0; fi < 2; ++fi) {
        const wchar_t* freqLabel = (fi == 0) ? L"400 кГц" : L"2000 кГц";
        std::wstring line = std::wstring(L"  ") + freqLabel + L":";
        for (int t = 0; t < nTx; ++t) {
            line += L"  T" + std::to_wstring(t + 1) + L"=" + Fw(d.AM_RX_1[fi][t]);
        }
        if (!s.empty()) {
            s += L"\r\n";
        }
        s += line;
    }

    AppendFmt(s, L"");
    AppendFmt(s, L"Амплитуды приёмника 2 (AM_RX_2)");
    for (int fi = 0; fi < 2; ++fi) {
        const wchar_t* freqLabel = (fi == 0) ? L"400 кГц" : L"2000 кГц";
        std::wstring line = std::wstring(L"  ") + freqLabel + L":";
        for (int t = 0; t < nTx; ++t) {
            line += L"  T" + std::to_wstring(t + 1) + L"=" + Fw(d.AM_RX_2[fi][t]);
        }
        if (!s.empty()) {
            s += L"\r\n";
        }
        s += line;
    }

    AppendFmt(s, L"");
    AppendFmt(s, L"Разность фаз (DELTA_PH)");
    for (int fi = 0; fi < 2; ++fi) {
        const wchar_t* freqLabel = (fi == 0) ? L"400 кГц" : L"2000 кГц";
        std::wstring line = std::wstring(L"  ") + freqLabel + L":";
        for (int t = 0; t < nTx; ++t) {
            line += L"  T" + std::to_wstring(t + 1) + L"=" + Fw(d.DELTA_PH[fi][t]);
        }
        if (!s.empty()) {
            s += L"\r\n";
        }
        s += line;
    }

    AppendFmt(s, L"");
    AppendFmt(s, L"Молчание (молчащие передатчики)");
    AppendFmt(s, L"  ZERO_AM_RX_1:  400 кГц=%.6g  2000 кГц=%.6g",
        static_cast<double>(d.ZERO_AM_RX_1[0]), static_cast<double>(d.ZERO_AM_RX_1[1]));
    AppendFmt(s, L"  ZERO_AM_RX_2:  400 кГц=%.6g  2000 кГц=%.6g",
        static_cast<double>(d.ZERO_AM_RX_2[0]), static_cast<double>(d.ZERO_AM_RX_2[1]));
    AppendFmt(s, L"  ZERO_dPH:      400 кГц=%.6g  2000 кГц=%.6g",
        static_cast<double>(d.ZERO_dPH[0]), static_cast<double>(d.ZERO_dPH[1]));

    return s;
}

} // namespace

void DeviceLogTryAppendVariantAForDataPayload240(const uint8_t* payload, size_t payloadLen) {
    if (!payload || payloadLen != sizeof(GP_TOOL_DATA)) {
        return;
    }

    ParsedToolFrame parsed = {};
    if (!ToolDataAdapterRegistry::Get().ParseRawFrame(payload, payloadLen, parsed)) {
        return;
    }

    const int tx = parsed.capabilities.transmitterCount > 0
        ? parsed.capabilities.transmitterCount
        : 5;

    const GP_TOOL_DATA& d = parsed.canonicalData;


    DeviceExchangeLogAppendRaw(L"");
    DeviceExchangeLogAppendRaw(L"");

    DeviceExchangeLogAppendRaw(L"───────────────────────────────────────────────────────────────────");

    wchar_t boldHeader[384];
    swprintf_s(boldHeader,
        L"▶ Кадр #%u | signature=0x%08X | condition=%u | T=%.6g °C",
        static_cast<unsigned>(d.frame),
        d.signature,
        d.condition,
        static_cast<double>(d.temperature));
    DeviceExchangeLogAppendRawBold(boldHeader);

    const std::wstring text = FormatGpToolDataVariantA(d, tx);
    DeviceExchangeLogAppendRaw(text);
}
