#include "framework.h"
#include "DeviceLogPanel.h"
#include "DeviceExchangeLog.h"
#include "UIModule.h"
#include "LayoutModule.h"
#include "resource.h"
#include <algorithm>
#include <commctrl.h>
#include <string>
#include <vector>

#ifndef _RICHEDIT_VER
#define _RICHEDIT_VER 0x0500
#endif
#include <richedit.h>

static constexpr wchar_t kExpandedClassName[] = L"AmkHorizonDeviceLogExpandedFrame";

static HINSTANCE g_hInst = nullptr;
static HWND g_hMain = nullptr;
static HFONT g_hAppUiFont = nullptr;

static HFONT LogPanelUiFont() {
    return g_hAppUiFont ? g_hAppUiFont : reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
}

static HWND g_hDockHost = nullptr;
static HWND g_hDockEdit = nullptr;
static HWND g_hDockExpandBtn = nullptr;
static HWND g_hDockClearBtn = nullptr;
static HWND g_hDockRawCheck = nullptr;
static HWND g_hDockCloseBtn = nullptr;

static HWND g_hExpandedRoot = nullptr;
static HWND g_hExpandedEdit = nullptr;
static HWND g_hExpandedClearBtn = nullptr;
static HWND g_hExpandedCollapseBtn = nullptr;
static HWND g_hExpandedRawCheck = nullptr;

static bool g_dockVisible = false;
static bool g_expandedVisible = false;

static bool g_classesRegistered = false;

static HMODULE g_hMsftedit = nullptr;
static bool g_dockUsesRichEdit = false;
static bool g_expandedUsesRichEdit = false;

// Последний текст, уже отданный полям EDIT; для чистого «дописывания в конец» без SetWindowText (без сброса прокрутки).
static std::wstring g_lastSyncedLogText;
// Число строк модели лога после последнего синка (инкремент Rich Edit: новые видимые строки).
static size_t g_lastSyncedLineCount = 0;
// Первый RefreshBothEdits вызывается до нормального размера панели (хост 10×10) — после UpdateLayout прокручиваем вниз один раз.
static bool g_forceDockScrollToBottomOnce = false;

static bool EnsureMsfteditLoaded() {
    if (g_hMsftedit) {
        return true;
    }
    g_hMsftedit = LoadLibraryW(L"Msftedit.dll");
    return g_hMsftedit != nullptr;
}

static HWND CreateLogEditChild(HWND parent, int controlId, bool* outUsesRichEdit) {
    *outUsesRichEdit = false;
    const HMENU id = reinterpret_cast<HMENU>(static_cast<INT_PTR>(controlId));
    if (EnsureMsfteditLoaded()) {
        HWND h = CreateWindowExW(WS_EX_CLIENTEDGE, L"RichEdit50W", L"",
            WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL,
            0, 0, 1, 1, parent, id, g_hInst, nullptr);
        if (h && IsWindow(h)) {
            SendMessageW(h, EM_SETREADONLY, TRUE, 0);
            SendMessageW(h, EM_SETLIMITTEXT, 1024 * 1024, 0);
            HFONT hf = LogPanelUiFont();
            SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(hf), TRUE);
            *outUsesRichEdit = true;
            return h;
        }
    }
    HWND h = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_WANTRETURN,
        0, 0, 1, 1, parent, id, g_hInst, nullptr);
    if (h) {
        SendMessageW(h, EM_SETLIMITTEXT, 1024 * 1024, 0);
        HFONT hf = LogPanelUiFont();
        SendMessageW(h, WM_SETFONT, reinterpret_cast<WPARAM>(hf), TRUE);
    }
    return h;
}

static void EditRedrawEnd(HWND hEdit);

static void RichEditApplyFullLog(HWND hRich, const std::vector<DeviceLogLine>& lines, bool pinBottom, int savedFirstLine) {
    if (!hRich || !IsWindow(hRich)) {
        return;
    }
    SendMessageW(hRich, WM_SETREDRAW, FALSE, 0);
    SendMessageW(hRich, EM_SETREADONLY, FALSE, 0);
    SetWindowTextW(hRich, L"");

    CHARFORMAT2W cfBold = {};
    cfBold.cbSize = sizeof(cfBold);
    cfBold.dwMask = CFM_BOLD | CFM_WEIGHT;
    cfBold.dwEffects = CFE_BOLD;
    cfBold.wWeight = static_cast<WORD>(FW_BOLD);
    CHARFORMAT2W cfNorm = {};
    cfNorm.cbSize = sizeof(cfNorm);
    cfNorm.dwMask = CFM_BOLD | CFM_WEIGHT;
    cfNorm.dwEffects = 0;
    cfNorm.wWeight = static_cast<WORD>(FW_NORMAL);

    SendMessageW(hRich, EM_SETSEL, 0, 0);
    SendMessageW(hRich, EM_SETCHARFORMAT, SCF_ALL, reinterpret_cast<LPARAM>(&cfNorm));

    for (const auto& ln : lines) {
        const int pos = GetWindowTextLengthW(hRich);
        SendMessageW(hRich, EM_SETSEL, static_cast<WPARAM>(pos), static_cast<LPARAM>(pos));
        SendMessageW(hRich, EM_SETCHARFORMAT, SCF_SELECTION,
            reinterpret_cast<LPARAM>(ln.bold ? &cfBold : &cfNorm));
        const std::wstring chunk = ln.text + L"\r\n";
        SendMessageW(hRich, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(chunk.c_str()));
    }
    SendMessageW(hRich, EM_SETREADONLY, TRUE, 0);

    if (pinBottom) {
        SendMessageW(hRich, WM_VSCROLL, SB_BOTTOM, 0);
        const int len = GetWindowTextLengthW(hRich);
        SendMessageW(hRich, EM_SETSEL, static_cast<WPARAM>(len), static_cast<LPARAM>(len));
        SendMessageW(hRich, EM_SCROLLCARET, 0, 0);
    } else if (savedFirstLine > 0) {
        const int total = static_cast<int>(SendMessageW(hRich, EM_GETLINECOUNT, 0, 0));
        const int scrollTo = (total > 0) ? (std::min)(savedFirstLine, (std::max)(0, total - 1)) : 0;
        SendMessageW(hRich, EM_LINESCROLL, 0, scrollTo);
    }
    EditRedrawEnd(hRich);
}

// Инкремент в конец Rich Edit с пер-строчным bold (без полной пересборки всего буфера).
static void RichEditAppendLinesFrom(HWND hRich, const std::vector<DeviceLogLine>& lines, size_t startIdx,
    bool pinBottom, int savedFirstLine) {
    if (!hRich || !IsWindow(hRich) || startIdx >= lines.size()) {
        return;
    }
    SendMessageW(hRich, WM_SETREDRAW, FALSE, 0);
    SendMessageW(hRich, EM_SETREADONLY, FALSE, 0);

    CHARFORMAT2W cfBold = {};
    cfBold.cbSize = sizeof(cfBold);
    cfBold.dwMask = CFM_BOLD | CFM_WEIGHT;
    cfBold.dwEffects = CFE_BOLD;
    cfBold.wWeight = static_cast<WORD>(FW_BOLD);
    CHARFORMAT2W cfNorm = {};
    cfNorm.cbSize = sizeof(cfNorm);
    cfNorm.dwMask = CFM_BOLD | CFM_WEIGHT;
    cfNorm.dwEffects = 0;
    cfNorm.wWeight = static_cast<WORD>(FW_NORMAL);

    for (size_t i = startIdx; i < lines.size(); ++i) {
        const int pos = GetWindowTextLengthW(hRich);
        SendMessageW(hRich, EM_SETSEL, static_cast<WPARAM>(pos), static_cast<LPARAM>(pos));
        SendMessageW(hRich, EM_SETCHARFORMAT, SCF_SELECTION,
            reinterpret_cast<LPARAM>(lines[i].bold ? &cfBold : &cfNorm));
        const std::wstring chunk = lines[i].text + L"\r\n";
        SendMessageW(hRich, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(chunk.c_str()));
    }
    SendMessageW(hRich, EM_SETREADONLY, TRUE, 0);

    if (pinBottom) {
        SendMessageW(hRich, WM_VSCROLL, SB_BOTTOM, 0);
        const int len = GetWindowTextLengthW(hRich);
        SendMessageW(hRich, EM_SETSEL, static_cast<WPARAM>(len), static_cast<LPARAM>(len));
        SendMessageW(hRich, EM_SCROLLCARET, 0, 0);
    } else if (savedFirstLine > 0) {
        const int total = static_cast<int>(SendMessageW(hRich, EM_GETLINECOUNT, 0, 0));
        const int scrollTo = (total > 0) ? (std::min)(savedFirstLine, (std::max)(0, total - 1)) : 0;
        SendMessageW(hRich, EM_LINESCROLL, 0, scrollTo);
    }
    EditRedrawEnd(hRich);
}

static int EditGetVisibleLineCount(HWND hEdit) {
    RECT rcFmt{};
    SendMessageW(hEdit, EM_GETRECT, 0, reinterpret_cast<LPARAM>(&rcFmt));
    HFONT hf = reinterpret_cast<HFONT>(SendMessageW(hEdit, WM_GETFONT, 0, 0));
    HDC hdc = GetDC(hEdit);
    if (hdc) {
        if (hf) {
            SelectObject(hdc, hf);
        }
        TEXTMETRICW tm{};
        if (GetTextMetricsW(hdc, &tm) && tm.tmHeight > 0) {
            ReleaseDC(hEdit, hdc);
            const int h = tm.tmHeight;
            const int areaH = static_cast<int>(rcFmt.bottom - rcFmt.top);
            return (std::max)(1, areaH / h);
        }
        ReleaseDC(hEdit, hdc);
    }
    return 1;
}

// Пользователь смотрит «хвост» лога: ползунок у нижнего края (GetScrollInfo надёжнее, чем только номер строки).
static bool EditIsPinnedToBottom(HWND hEdit) {
    if (!hEdit || !IsWindow(hEdit)) {
        return true;
    }
    SCROLLINFO si = {};
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_POS | SIF_RANGE | SIF_PAGE;
    if (GetScrollInfo(hEdit, SB_VERT, &si)) {
        const int page = static_cast<int>(si.nPage);
        const int maxRange = static_cast<int>(si.nMax);
        if (maxRange > 0 && page > 0) {
            const int maxPos = maxRange - page + 1;
            if (maxPos > 0) {
                return static_cast<int>(si.nPos) >= maxPos - 2;
            }
            return true;
        }
    }
    const int total = static_cast<int>(SendMessageW(hEdit, EM_GETLINECOUNT, 0, 0));
    if (total <= 0) {
        return true;
    }
    const int firstVis = static_cast<int>(SendMessageW(hEdit, EM_GETFIRSTVISIBLELINE, 0, 0));
    const int vis = EditGetVisibleLineCount(hEdit);
    return firstVis + vis >= total - 1;
}


static void EditRedrawEnd(HWND hEdit) {
    SendMessageW(hEdit, WM_SETREDRAW, TRUE, 0);
    RedrawWindow(hEdit, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE);
}

static void ScrollLogEditToBottom(HWND hEdit) {
    if (!hEdit || !IsWindow(hEdit)) {
        return;
    }
    SendMessageW(hEdit, WM_VSCROLL, SB_BOTTOM, 0);
    const int len = GetWindowTextLengthW(hEdit);
    SendMessageW(hEdit, EM_SETSEL, static_cast<WPARAM>(len), static_cast<LPARAM>(len));
    SendMessageW(hEdit, EM_SCROLLCARET, 0, 0);
}

// Инкремент допустим только если оба поля уже совпадают с последним синком (иначе, например, пустое развёрнутое окно получило бы только «хвост»).
static bool EditsMatchLastSyncedLength() {
    if (g_hDockEdit && IsWindow(g_hDockEdit)) {
        if (GetWindowTextLengthW(g_hDockEdit) != static_cast<int>(g_lastSyncedLogText.size())) {
            return false;
        }
    }
    if (g_hExpandedEdit && IsWindow(g_hExpandedEdit)) {
        if (GetWindowTextLengthW(g_hExpandedEdit) != static_cast<int>(g_lastSyncedLogText.size())) {
            return false;
        }
    }
    return true;
}

static void EditApplyFullText(HWND hEdit, const wchar_t* text) {
    if (!hEdit || !IsWindow(hEdit)) {
        return;
    }
    const bool pinBottom = EditIsPinnedToBottom(hEdit);
    const int savedFirstLine = static_cast<int>(SendMessageW(hEdit, EM_GETFIRSTVISIBLELINE, 0, 0));

    SendMessageW(hEdit, WM_SETREDRAW, FALSE, 0);
    SetWindowTextW(hEdit, text);

    if (pinBottom) {
        // SetWindowText сбрасывает прокрутку; для многострочного EDIT EM_SETSEL(-1,-1) часто не опускает вид.
        SendMessageW(hEdit, WM_VSCROLL, SB_BOTTOM, 0);
        const int len = GetWindowTextLengthW(hEdit);
        SendMessageW(hEdit, EM_SETSEL, static_cast<WPARAM>(len), static_cast<LPARAM>(len));
        SendMessageW(hEdit, EM_SCROLLCARET, 0, 0);
    } else if (savedFirstLine > 0) {
        const int total = static_cast<int>(SendMessageW(hEdit, EM_GETLINECOUNT, 0, 0));
        const int scrollTo = (total > 0) ? (std::min)(savedFirstLine, (std::max)(0, total - 1)) : 0;
        SendMessageW(hEdit, EM_LINESCROLL, 0, scrollTo);
    }
    EditRedrawEnd(hEdit);
}

// Дописывание в конец без SetWindowText — прокрутка не сбрасывается на первую строку; вместе с «низом» даёт плавный рост лога.
static void EditAppendDeltaPinnedBottom(HWND hEdit, const wchar_t* delta) {
    if (!hEdit || !IsWindow(hEdit) || !delta || !delta[0]) {
        return;
    }
    SendMessageW(hEdit, WM_SETREDRAW, FALSE, 0);
    const int len = GetWindowTextLengthW(hEdit);
    SendMessageW(hEdit, EM_SETSEL, static_cast<WPARAM>(len), static_cast<LPARAM>(len));
    SendMessageW(hEdit, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(delta));
    SendMessageW(hEdit, WM_VSCROLL, SB_BOTTOM, 0);
    const int len2 = GetWindowTextLengthW(hEdit);
    SendMessageW(hEdit, EM_SETSEL, static_cast<WPARAM>(len2), static_cast<LPARAM>(len2));
    SendMessageW(hEdit, EM_SCROLLCARET, 0, 0);
    EditRedrawEnd(hEdit);
}

static void RichEditAppendPlainDeltaPinnedBottom(HWND hRich, const wchar_t* delta) {
    if (!hRich || !IsWindow(hRich) || !delta || !delta[0]) {
        return;
    }
    CHARFORMAT2W cfNorm = {};
    cfNorm.cbSize = sizeof(cfNorm);
    cfNorm.dwMask = CFM_BOLD | CFM_WEIGHT;
    cfNorm.dwEffects = 0;
    cfNorm.wWeight = static_cast<WORD>(FW_NORMAL);
    SendMessageW(hRich, WM_SETREDRAW, FALSE, 0);
    SendMessageW(hRich, EM_SETREADONLY, FALSE, 0);
    const int len = GetWindowTextLengthW(hRich);
    SendMessageW(hRich, EM_SETSEL, static_cast<WPARAM>(len), static_cast<LPARAM>(len));
    SendMessageW(hRich, EM_SETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&cfNorm));
    SendMessageW(hRich, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(delta));
    SendMessageW(hRich, EM_SETREADONLY, TRUE, 0);
    SendMessageW(hRich, WM_VSCROLL, SB_BOTTOM, 0);
    const int len2 = GetWindowTextLengthW(hRich);
    SendMessageW(hRich, EM_SETSEL, static_cast<WPARAM>(len2), static_cast<LPARAM>(len2));
    SendMessageW(hRich, EM_SCROLLCARET, 0, 0);
    EditRedrawEnd(hRich);
}

static std::wstring BuildTextFromLines(const std::vector<DeviceLogLine>& lines) {
    std::wstring out;
    out.reserve(lines.size() * 60);
    bool first = true;
    for (const auto& ln : lines) {
        if (!first) out += L"\r\n";
        first = false;
        out += ln.text;
    }
    return out;
}

static void RefreshBothEdits() {
    const std::vector<DeviceLogLine> lines = DeviceExchangeLogGetLines();
    const std::wstring text = BuildTextFromLines(lines);

    const bool dockRich = g_dockUsesRichEdit && g_hDockEdit && IsWindow(g_hDockEdit);
    const bool expRich = g_expandedUsesRichEdit && g_hExpandedEdit && IsWindow(g_hExpandedEdit);
    const bool anyRichEdit = dockRich || expRich;

    const bool textExtends = text.size() > g_lastSyncedLogText.size()
        && (g_lastSyncedLogText.empty() || text.compare(0, g_lastSyncedLogText.size(), g_lastSyncedLogText) == 0)
        && EditsMatchLastSyncedLength();

    const bool dockPin = !g_hDockEdit || !IsWindow(g_hDockEdit) || EditIsPinnedToBottom(g_hDockEdit);
    const bool expPin = !g_hExpandedEdit || !IsWindow(g_hExpandedEdit) || EditIsPinnedToBottom(g_hExpandedEdit);

    const int savedDockFirst = (g_hDockEdit && IsWindow(g_hDockEdit))
        ? static_cast<int>(SendMessageW(g_hDockEdit, EM_GETFIRSTVISIBLELINE, 0, 0))
        : 0;
    const int savedExpFirst = (g_hExpandedEdit && IsWindow(g_hExpandedEdit))
        ? static_cast<int>(SendMessageW(g_hExpandedEdit, EM_GETFIRSTVISIBLELINE, 0, 0))
        : 0;

    bool richNewLinesArePlainOnly = false;
    if (textExtends && lines.size() > g_lastSyncedLineCount) {
        richNewLinesArePlainOnly = true;
        for (size_t i = g_lastSyncedLineCount; i < lines.size(); ++i) {
            if (lines[i].bold) {
                richNewLinesArePlainOnly = false;
                break;
            }
        }
    }

    // Обычный EDIT: инкремент без полной замены (в т.ч. первое заполнение с пустого).
    if (!anyRichEdit && textExtends && dockPin && expPin) {
        const std::wstring delta = text.substr(g_lastSyncedLogText.size());
        if (!delta.empty()) {
            if (g_hDockEdit && IsWindow(g_hDockEdit)) {
                EditAppendDeltaPinnedBottom(g_hDockEdit, delta.c_str());
            }
            if (g_hExpandedEdit && IsWindow(g_hExpandedEdit)) {
                EditAppendDeltaPinnedBottom(g_hExpandedEdit, delta.c_str());
            }
        }
        g_lastSyncedLogText = text;
        g_lastSyncedLineCount = lines.size();
        return;
    }

    // Rich Edit: дописывание в конец без полной пересборки (plain — одним куском; с bold — по строкам модели).
    if (anyRichEdit && textExtends && dockPin && expPin && lines.size() > g_lastSyncedLineCount && !lines.empty()) {
        const size_t startIdx = g_lastSyncedLineCount;
        if (richNewLinesArePlainOnly) {
            const std::wstring delta = text.substr(g_lastSyncedLogText.size());
            if (!delta.empty()) {
                if (dockRich) {
                    RichEditAppendPlainDeltaPinnedBottom(g_hDockEdit, delta.c_str());
                }
                if (expRich) {
                    RichEditAppendPlainDeltaPinnedBottom(g_hExpandedEdit, delta.c_str());
                }
            }
        } else {
            if (dockRich) {
                RichEditAppendLinesFrom(g_hDockEdit, lines, startIdx, true, 0);
            }
            if (expRich) {
                RichEditAppendLinesFrom(g_hExpandedEdit, lines, startIdx, true, 0);
            }
        }
        g_lastSyncedLogText = text;
        g_lastSyncedLineCount = lines.size();
        return;
    }

    if (dockRich) {
        RichEditApplyFullLog(g_hDockEdit, lines, dockPin, savedDockFirst);
    } else if (g_hDockEdit && IsWindow(g_hDockEdit)) {
        EditApplyFullText(g_hDockEdit, text.c_str());
    }

    if (expRich) {
        RichEditApplyFullLog(g_hExpandedEdit, lines, expPin, savedExpFirst);
    } else if (g_hExpandedEdit && IsWindow(g_hExpandedEdit)) {
        EditApplyFullText(g_hExpandedEdit, text.c_str());
    }
    g_lastSyncedLogText = text;
    g_lastSyncedLineCount = lines.size();
}

LRESULT CALLBACK DockHostSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
    UINT_PTR, DWORD_PTR dwRefData) {
    if (msg == WM_COMMAND) {
        SendMessageW(reinterpret_cast<HWND>(dwRefData), msg, wParam, lParam);
        return 0;
    }
    if (msg == WM_NCDESTROY) {
        RemoveWindowSubclass(hwnd, DockHostSubclassProc, 1);
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

static void LayoutDockChildren() {
    if (!g_hDockHost || !IsWindow(g_hDockHost)) {
        return;
    }
    RECT rc{};
    GetClientRect(g_hDockHost, &rc);
    const int btnH = 22;
    const int pad = 4;
    const int gap = 4;
    const int expandW = 28;
    const int clearW = 72;
    const int closeW = 22;
    int x = pad;
    if (g_hDockExpandBtn) {
        SetWindowPos(g_hDockExpandBtn, nullptr, x, pad, expandW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
        x += expandW + gap;
    }
    if (g_hDockClearBtn) {
        SetWindowPos(g_hDockClearBtn, nullptr, x, pad, clearW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
        x += clearW + gap;
    }
    if (g_hDockRawCheck) {
        const int checkW = std::max(10, static_cast<int>(rc.right) - pad - x - closeW - gap);
        SetWindowPos(g_hDockRawCheck, nullptr, x, pad, checkW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    if (g_hDockCloseBtn) {
        SetWindowPos(g_hDockCloseBtn, nullptr, static_cast<int>(rc.right) - pad - closeW, pad, closeW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    const int topEdit = pad + btnH + 4;
    const int editH = std::max(0, static_cast<int>(rc.bottom) - topEdit - pad);
    if (g_hDockEdit) {
        SetWindowPos(g_hDockEdit, nullptr, pad, topEdit, std::max(10, static_cast<int>(rc.right) - 2 * pad), std::max(10, editH),
            SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

static void LayoutExpandedChildren(HWND hRoot) {
    RECT rc{};
    GetClientRect(hRoot, &rc);
    const int btnH = 26;
    const int pad = 8;
    const int gap = 8;
    const int clearW = 90;
    const int collapseW = 100;
    int x = pad;
    if (g_hExpandedClearBtn) {
        SetWindowPos(g_hExpandedClearBtn, nullptr, x, pad, clearW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
        x += clearW + gap;
    }
    if (g_hExpandedCollapseBtn) {
        SetWindowPos(g_hExpandedCollapseBtn, nullptr, x, pad, collapseW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
        x += collapseW + gap;
    }
    if (g_hExpandedRawCheck) {
        const int checkW = std::max(10, static_cast<int>(rc.right) - pad - x);
        SetWindowPos(g_hExpandedRawCheck, nullptr, x, pad, checkW, btnH, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    const int topEdit = pad + btnH + 8;
    if (g_hExpandedEdit) {
        SetWindowPos(g_hExpandedEdit, nullptr, pad, topEdit, std::max(10, static_cast<int>(rc.right) - 2 * pad),
            std::max(10, static_cast<int>(rc.bottom) - topEdit - pad), SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

LRESULT CALLBACK ExpandedFrameWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
        HINSTANCE hi = cs->hInstance;
        g_hExpandedClearBtn = CreateWindowExW(0, L"BUTTON", L"Очистить",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 1, 1, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_DEVICE_LOG_CLEAR)),
            hi, nullptr);
        g_hExpandedCollapseBtn = CreateWindowExW(0, L"BUTTON", L"Свернуть",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 1, 1, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_DEVICE_LOG_COLLAPSE)),
            hi, nullptr);
        g_hExpandedRawCheck = CreateWindowExW(0, L"BUTTON", L"Сырые данные",
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 0, 0, 1, 1, hWnd,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_DEVICE_LOG_RAW_CHECK)), hi, nullptr);
        SendMessageW(g_hExpandedRawCheck, BM_SETCHECK,
            DeviceExchangeLogGetShowRawData() ? BST_CHECKED : BST_UNCHECKED, 0);
        g_hExpandedEdit = CreateLogEditChild(hWnd, IDC_DEVICE_LOG_EDIT_EXPANDED, &g_expandedUsesRichEdit);
        HFONT hf = LogPanelUiFont();
        SendMessageW(g_hExpandedClearBtn, WM_SETFONT, reinterpret_cast<WPARAM>(hf), TRUE);
        SendMessageW(g_hExpandedCollapseBtn, WM_SETFONT, reinterpret_cast<WPARAM>(hf), TRUE);
        SendMessageW(g_hExpandedRawCheck, WM_SETFONT, reinterpret_cast<WPARAM>(hf), TRUE);
        RefreshBothEdits();
        return 0;
    }
    case WM_SIZE:
        LayoutExpandedChildren(hWnd);
        return 0;
    case WM_COMMAND:
        if (LOWORD(wParam) == IDC_DEVICE_LOG_CLEAR && HIWORD(wParam) == BN_CLICKED) {
            DeviceExchangeLogClear();
            RefreshBothEdits();
            return 0;
        }
        if (LOWORD(wParam) == IDC_DEVICE_LOG_COLLAPSE && HIWORD(wParam) == BN_CLICKED) {
            DestroyWindow(hWnd);
            return 0;
        }
        if (LOWORD(wParam) == IDC_DEVICE_LOG_RAW_CHECK && HIWORD(wParam) == BN_CLICKED) {
            DeviceLogPanel_OnRawDataToggled(reinterpret_cast<HWND>(lParam));
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hWnd);
        return 0;
    case WM_DESTROY:
        if (hWnd == g_hExpandedRoot) {
            g_hExpandedRoot = nullptr;
            g_hExpandedEdit = nullptr;
            g_hExpandedClearBtn = nullptr;
            g_hExpandedCollapseBtn = nullptr;
            g_hExpandedRawCheck = nullptr;
            g_expandedUsesRichEdit = false;
            g_expandedVisible = false;
            DeviceExchangeLogSetPanelVisible(g_dockVisible);
            if (g_dockVisible && g_hDockHost && IsWindow(g_hDockHost)) {
                ShowWindow(g_hDockHost, SW_SHOW);
                RefreshBothEdits();
            }
        }
        break;
    default:
        break;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

static void RegisterExpandedClass(HINSTANCE hInst) {
    if (g_classesRegistered) {
        return;
    }
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_DBLCLKS;
    wc.lpfnWndProc = ExpandedFrameWndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_BTNFACE + 1);
    wc.lpszClassName = kExpandedClassName;
    const ATOM a = RegisterClassExW(&wc);
    if (!a && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return;
    }
    g_classesRegistered = true;
}

static void DestroyDockControls() {
    if (g_hMain && IsWindow(g_hMain)) {
        KillTimer(g_hMain, IDT_DEVICE_LOG_REFRESH);
    }
    if (g_hDockHost && IsWindow(g_hDockHost)) {
        DestroyWindow(g_hDockHost);
    }
    g_hDockHost = nullptr;
    g_hDockEdit = nullptr;
    g_hDockExpandBtn = nullptr;
    g_hDockClearBtn = nullptr;
    g_hDockCloseBtn = nullptr;
    g_hDockRawCheck = nullptr;
    g_dockUsesRichEdit = false;
    g_lastSyncedLogText.clear();
    g_lastSyncedLineCount = 0;
    g_forceDockScrollToBottomOnce = false;
}

static void CreateDockControls() {
    if (g_hDockHost && IsWindow(g_hDockHost)) {
        return;
    }
    // WS_POPUP + координаты экрана: верх совпадает с верхом окна (включая заголовок),
    // низ задаётся в UpdateLayout (чуть ниже верхней границы графика, выше подписи легенды).
    g_hDockHost = CreateWindowExW(WS_EX_CONTROLPARENT, L"STATIC", L"",
        WS_POPUP | WS_CLIPCHILDREN | WS_BORDER, 0, 0, 10, 10, g_hMain, nullptr, g_hInst, nullptr);
    SetWindowSubclass(g_hDockHost, DockHostSubclassProc, 1, reinterpret_cast<DWORD_PTR>(g_hMain));

    g_hDockExpandBtn = CreateWindowExW(0, L"BUTTON", L"\u2922",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 1, 1, g_hDockHost,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_DEVICE_LOG_EXPAND)), g_hInst, nullptr);
    g_hDockClearBtn = CreateWindowExW(0, L"BUTTON", L"Очистить",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 1, 1, g_hDockHost,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_DEVICE_LOG_CLEAR)), g_hInst, nullptr);

    g_hDockRawCheck = CreateWindowExW(0, L"BUTTON", L"Сырые данные",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX, 0, 0, 1, 1, g_hDockHost,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_DEVICE_LOG_RAW_CHECK)), g_hInst, nullptr);
    SendMessageW(g_hDockRawCheck, BM_SETCHECK,
        DeviceExchangeLogGetShowRawData() ? BST_CHECKED : BST_UNCHECKED, 0);

    g_hDockCloseBtn = CreateWindowExW(0, L"BUTTON", L"\u2715",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP, 0, 0, 1, 1, g_hDockHost,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(IDC_DEVICE_LOG_CLOSE)), g_hInst, nullptr);

    g_hDockEdit = CreateLogEditChild(g_hDockHost, IDC_DEVICE_LOG_EDIT, &g_dockUsesRichEdit);

    HFONT hf = LogPanelUiFont();
    SendMessageW(g_hDockExpandBtn, WM_SETFONT, reinterpret_cast<WPARAM>(hf), TRUE);
    SendMessageW(g_hDockClearBtn, WM_SETFONT, reinterpret_cast<WPARAM>(hf), TRUE);
    SendMessageW(g_hDockCloseBtn, WM_SETFONT, reinterpret_cast<WPARAM>(hf), TRUE);
    SendMessageW(g_hDockRawCheck, WM_SETFONT, reinterpret_cast<WPARAM>(hf), TRUE);

    RefreshBothEdits();
    LayoutDockChildren();
    SetWindowPos(g_hDockHost, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW | SWP_NOACTIVATE);
    // Реальный размер задаётся в DeviceLogPanel_UpdateLayout; после него нужно опустить скролл (см. g_forceDockScrollToBottomOnce).
    g_forceDockScrollToBottomOnce = true;
}

static void ShowExpandedWindow() {
    if (g_hExpandedRoot && IsWindow(g_hExpandedRoot)) {
        SetForegroundWindow(g_hExpandedRoot);
        return;
    }

    static constexpr int kExpandedLogDefaultW = 600;
    static constexpr int kExpandedLogDefaultH = 550;
    const int w = kExpandedLogDefaultW;
    const int h = kExpandedLogDefaultH;

    HMONITOR mon = MonitorFromWindow(g_hMain, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi = {};
    mi.cbSize = sizeof(mi);
    int x = 0, y = 0;
    if (mon && GetMonitorInfo(mon, &mi)) {
        const RECT& work = mi.rcWork;
        x = work.left + (work.right - work.left - w) / 2;
        y = work.top + (work.bottom - work.top - h) / 2;
    }

    g_hExpandedRoot = CreateWindowExW(WS_EX_DLGMODALFRAME | WS_EX_APPWINDOW, kExpandedClassName, L"Лог обмена с прибором",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_CLIPCHILDREN,
        x, y, w, h, g_hMain, nullptr, g_hInst, nullptr);
    if (g_hExpandedRoot) {
        if (g_hMain && IsWindow(g_hMain)) {
            HICON hIconBig = (HICON)SendMessageW(g_hMain, WM_GETICON, ICON_BIG, 0);
            if (!hIconBig) {
                hIconBig = (HICON)GetClassLongPtr(g_hMain, GCLP_HICON);
            }
            if (hIconBig) {
                SendMessageW(g_hExpandedRoot, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);
            }
            HICON hIconSmall = (HICON)SendMessageW(g_hMain, WM_GETICON, ICON_SMALL, 0);
            if (!hIconSmall) {
                hIconSmall = (HICON)GetClassLongPtr(g_hMain, GCLP_HICONSM);
            }
            if (hIconSmall) {
                SendMessageW(g_hExpandedRoot, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);
            }
        }
        ShowWindow(g_hExpandedRoot, SW_SHOW);
        g_expandedVisible = true;
        DeviceExchangeLogSetPanelVisible(true);
        if (g_hDockHost && IsWindow(g_hDockHost)) {
            ShowWindow(g_hDockHost, SW_HIDE);
        }
    }
}

void DeviceLogPanel_Init(HWND hMain, HINSTANCE hInst, HFONT hAppFont) {
    g_hMain = hMain;
    g_hInst = hInst;
    g_hAppUiFont = hAppFont;
    EnsureMsfteditLoaded();
    RegisterExpandedClass(hInst);
}

void DeviceLogPanel_Shutdown(HWND) {
    g_hAppUiFont = nullptr;
    DeviceExchangeLogSetPanelVisible(false);
    DeviceExchangeLogFileClose();
    if (g_hMain && IsWindow(g_hMain)) {
        KillTimer(g_hMain, IDT_DEVICE_LOG_REFRESH);
    }
    if (g_hExpandedRoot && IsWindow(g_hExpandedRoot)) {
        DestroyWindow(g_hExpandedRoot);
    }
    g_hExpandedRoot = nullptr;
    DestroyDockControls();
    g_dockVisible = false;
    g_hMain = nullptr;
    if (g_hMsftedit) {
        FreeLibrary(g_hMsftedit);
        g_hMsftedit = nullptr;
    }
}

void DeviceLogPanel_Toggle(HWND hMain) {
    UNREFERENCED_PARAMETER(hMain);
    if (g_dockVisible) {
        g_dockVisible = false;
        if (g_hExpandedRoot && IsWindow(g_hExpandedRoot)) {
            DestroyWindow(g_hExpandedRoot);
        }
        DestroyDockControls();
        DeviceExchangeLogSetPanelVisible(false);
        return;
    }
    g_dockVisible = true;
    DeviceExchangeLogSetPanelVisible(true);
    CreateDockControls();
    ShowWindow(g_hDockHost, SW_SHOW);
    RefreshBothEdits();
}

bool DeviceLogPanel_IsDockVisible() {
    return g_dockVisible;
}

void DeviceLogPanel_UpdateLayout(HWND hMain, UIModule* ui, const LayoutModule* layout) {
    g_hMain = hMain;
    if (!g_dockVisible || !g_hDockHost || !IsWindow(g_hDockHost)) {
        return;
    }
    if (!ui || !layout) {
        return;
    }

    HWND hConnect = ui->GetConnectToggleBtn();
    HWND hCursorIcon = ui->GetChart1CursorIconHwnd();
    if (!hConnect || !hCursorIcon) {
        return;
    }

    RECT rcMain{};
    GetWindowRect(hMain, &rcMain);

    RECT rcConnect{};
    RECT rcIcon{};
    GetWindowRect(hConnect, &rcConnect);
    GetWindowRect(hCursorIcon, &rcIcon);

    // Горизонталь: экранные координаты — зазор между «Включить/Отключить» и иконкой курсора
    const int leftScreen = rcConnect.right + 6;
    const int rightScreen = rcIcon.left - 6;
    const int width = rightScreen - leftScreen;

    const RECT chart1 = layout->GetChart1Rect();
    // ChartModule: подписи легенды на chartRect.top + 35 — нижний край лога чуть выше, с небольшим заходом на область графика
    static constexpr int kLegendYFromChartTop = 35;
    static constexpr int kPixelsAboveLegendLine = 3;
    const int bottomClientY = static_cast<int>(chart1.top) + kLegendYFromChartTop - kPixelsAboveLegendLine;
    POINT ptBottom = { chart1.left, bottomClientY };
    ClientToScreen(hMain, &ptBottom);
    const int topScreen = rcMain.top;
    const int height = ptBottom.y - topScreen;

    const int kMinW = 80;
    const int kMinH = 40;
    if (width < kMinW || height < kMinH) {
        ShowWindow(g_hDockHost, SW_HIDE);
        return;
    }

    SetWindowPos(g_hDockHost, HWND_TOP, leftScreen, topScreen, width, height, SWP_NOACTIVATE);
    LayoutDockChildren();

    if (g_hDockEdit && IsWindow(g_hDockEdit)) {
        if (g_forceDockScrollToBottomOnce) {
            ScrollLogEditToBottom(g_hDockEdit);
            g_forceDockScrollToBottomOnce = false;
        } else if (EditIsPinnedToBottom(g_hDockEdit)) {
            ScrollLogEditToBottom(g_hDockEdit);
        }
    }
}

void DeviceLogPanel_OnRefresh(HWND) {
    RefreshBothEdits();
}

// Точка входа из главного окна: развернуть из dock-кнопки
void DeviceLogPanel_OnExpandCommand() {
    ShowExpandedWindow();
}

void DeviceLogPanel_OnRawDataToggled(HWND hCheckbox) {
    if (!hCheckbox || !IsWindow(hCheckbox)) {
        return;
    }
    const bool checked = (SendMessageW(hCheckbox, BM_GETCHECK, 0, 0) == BST_CHECKED);
    DeviceExchangeLogSetShowRawData(checked);
    if (hCheckbox != g_hDockRawCheck && g_hDockRawCheck && IsWindow(g_hDockRawCheck)) {
        SendMessageW(g_hDockRawCheck, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
    }
    if (hCheckbox != g_hExpandedRawCheck && g_hExpandedRawCheck && IsWindow(g_hExpandedRawCheck)) {
        SendMessageW(g_hExpandedRawCheck, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
    }
    g_lastSyncedLogText.clear();
    g_lastSyncedLineCount = 0;
    RefreshBothEdits();
}

// Статическая переменная для отслеживания видимости чекбокса
static bool g_rawCheckboxVisible = true;

void DeviceLogPanel_SetRawDataCheckboxVisible(bool visible) {
    g_rawCheckboxVisible = visible;
    
    // Обновляем видимость чекбокса в dock-панели
    if (g_hDockRawCheck && IsWindow(g_hDockRawCheck)) {
        ShowWindow(g_hDockRawCheck, visible ? SW_SHOW : SW_HIDE);
    }
    
    // Обновляем видимость чекбокса в развёрнутом окне
    if (g_hExpandedRawCheck && IsWindow(g_hExpandedRawCheck)) {
        ShowWindow(g_hExpandedRawCheck, visible ? SW_SHOW : SW_HIDE);
    }
    
    // Пересчитываем layout, чтобы элементы заняли освободившееся/добавленное пространство
    if (g_hDockHost && IsWindow(g_hDockHost)) {
        LayoutDockChildren();
    }
    if (g_hExpandedRoot && IsWindow(g_hExpandedRoot)) {
        LayoutExpandedChildren(g_hExpandedRoot);
    }
}

bool DeviceLogPanel_IsRawDataCheckboxVisible() {
    return g_rawCheckboxVisible;
}
