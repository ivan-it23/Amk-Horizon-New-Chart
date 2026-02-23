// Amk-Horizon-New-Chart.cpp : Определяет точку входа для приложения.
//

#include "framework.h"
#include "Amk-Horizon-New-Chart.h"
#include "SerialModule.h"  // v2.13.0: Для IDT_SERIAL_POLL
#include "MetrologyModule.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cfloat>
#include <cmath>
#include <vector>
#include <string>
#include <commdlg.h>
#include <commctrl.h>
#include <ctime>

#define MAX_LOADSTRING 100

struct MetrologyDialogContext;

struct NotesJournalDialogContext {
    ChartManager* manager = nullptr;
    HWND hMainWnd = nullptr;
    std::wstring notesPath;
    std::vector<ChartNote> notes;
    int targetChartNumber = 1;
};

extern HWND g_hNotesJournalDialog;
extern NotesJournalDialogContext* g_notesJournalCtx;

static bool IsEmphasizedDeviceMenuItem(UINT id) {
    return id == IDM_DEVICE_CONNECT || id == IDM_DEVICE_METROLOGY;
}

static std::wstring FormatNoteTimestamp(uint64_t utcMs) {
    if (utcMs == 0) return L"-";
    time_t sec = static_cast<time_t>(utcMs / 1000ULL);
    tm localTm{};
    if (localtime_s(&localTm, &sec) != 0) {
        return L"-";
    }

    wchar_t buf[64];
    if (wcsftime(buf, _countof(buf), L"%d.%m.%Y %H:%M:%S", &localTm) == 0) {
        return L"-";
    }
    return std::wstring(buf);
}

static std::wstring BuildSnapshotSummary(const ChartNote& note) {
    XAxisType xAxisType = XAxisType::FRAME_NUMBER;
    PresetType preset = PresetType::NONE;
    std::vector<NotesModule::SnapshotCurveItem> curves;

    if (!NotesModule::ParseConfigSnapshot(note.configSnapshot, xAxisType, preset, curves)) {
        return L"(ошибка чтения snapshot)";
    }

    std::wstringstream ss;
    ss << L"X=" << (xAxisType == XAxisType::TEMPERATURE ? L"Температура" : L"Кадр");
    ss << L"; Кривых=" << curves.size();
    return ss.str();
}

static std::wstring BuildNotePreview(const std::wstring& text) {
    std::wstring out = text;
    for (wchar_t& ch : out) {
        if (ch == L'\r' || ch == L'\n') ch = L' ';
    }
    if (out.size() > 80) {
        out = out.substr(0, 80);
        out += L"...";
    }
    return out;
}

static void NotesJournal_InitListColumns(HWND hList) {
    if (!hList) return;

    LVCOLUMN col{};
    col.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;

    col.pszText = const_cast<LPWSTR>(L"Дата");
    col.cx = 130;
    col.iSubItem = 0;
    ListView_InsertColumn(hList, 0, &col);

    col.pszText = const_cast<LPWSTR>(L"Пресет");
    col.cx = 140;
    col.iSubItem = 1;
    ListView_InsertColumn(hList, 1, &col);

    col.pszText = const_cast<LPWSTR>(L"Конфигурация");
    col.cx = 150;
    col.iSubItem = 2;
    ListView_InsertColumn(hList, 2, &col);

    col.pszText = const_cast<LPWSTR>(L"Заметка");
    col.cx = 250;
    col.iSubItem = 3;
    ListView_InsertColumn(hList, 3, &col);
}

static bool NotesJournal_Refresh(HWND hDlg) {
    NotesJournalDialogContext* ctx = reinterpret_cast<NotesJournalDialogContext*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
    if (!ctx || !ctx->manager) return false;

    std::vector<ChartNote> notes;
    std::wstring notesPath;
    if (!ctx->manager->GetNotesForCurrentSource(notes, notesPath)) {
        ctx->notes.clear();
        ctx->notesPath.clear();

        HWND hList = GetDlgItem(hDlg, IDC_NOTES_JOURNAL_LIST);
        if (hList) {
            ListView_DeleteAllItems(hList);
        }

        SetDlgItemText(hDlg,
                       IDC_NOTES_INFO_LABEL,
                       L"Нет активного файла/записи для журнала заметок.");
        return false;
    }

    ctx->notes = std::move(notes);
    ctx->notesPath = std::move(notesPath);
    ctx->targetChartNumber = ctx->manager->GetActiveChartNumber();

    HWND hList = GetDlgItem(hDlg, IDC_NOTES_JOURNAL_LIST);
    if (!hList) return false;

    ListView_DeleteAllItems(hList);

    for (size_t i = 0; i < ctx->notes.size(); ++i) {
        const ChartNote& note = ctx->notes[i];
        std::wstring ts = FormatNoteTimestamp(note.createdUtcMs);
        std::wstring preset = note.presetName.empty() ? L"(без пресета)" : note.presetName;
        std::wstring cfg = BuildSnapshotSummary(note);
        std::wstring preview = BuildNotePreview(note.text);

        LVITEM item{};
        item.mask = LVIF_TEXT;
        item.iItem = static_cast<int>(i);
        item.iSubItem = 0;
        item.pszText = const_cast<LPWSTR>(ts.c_str());
        int row = ListView_InsertItem(hList, &item);
        if (row >= 0) {
            ListView_SetItemText(hList, row, 1, const_cast<LPWSTR>(preset.c_str()));
            ListView_SetItemText(hList, row, 2, const_cast<LPWSTR>(cfg.c_str()));
            ListView_SetItemText(hList, row, 3, const_cast<LPWSTR>(preview.c_str()));
        }
    }

    wchar_t info[768];
    swprintf_s(info,
               L"Источник: %s | Заметок: %zu | График применения: %d",
               ctx->notesPath.c_str(),
               ctx->notes.size(),
               ctx->targetChartNumber);
    SetDlgItemText(hDlg, IDC_NOTES_INFO_LABEL, info);
    return true;
}

static bool NotesJournal_ApplySelected(HWND hDlg) {
    NotesJournalDialogContext* ctx = reinterpret_cast<NotesJournalDialogContext*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
    if (!ctx || !ctx->manager || !ctx->hMainWnd) return false;

    HWND hList = GetDlgItem(hDlg, IDC_NOTES_JOURNAL_LIST);
    if (!hList) return false;

    int sel = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
    if (sel < 0 || static_cast<size_t>(sel) >= ctx->notes.size()) {
        MessageBox(hDlg,
                   L"Выберите заметку в списке.",
                   L"Журнал заметок",
                   MB_OK | MB_ICONINFORMATION);
        return false;
    }

    ctx->targetChartNumber = ctx->manager->GetActiveChartNumber();
    bool ok = ctx->manager->ApplyNoteConfiguration(ctx->hMainWnd,
                                                   ctx->notes[static_cast<size_t>(sel)],
                                                   ctx->targetChartNumber);
    if (ok) {
        InvalidateRect(ctx->hMainWnd, nullptr, FALSE);
    }
    return ok;
}

static INT_PTR CALLBACK NotesJournalDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam);

            NotesJournalDialogContext* ctx = reinterpret_cast<NotesJournalDialogContext*>(lParam);
            if (!ctx) return (INT_PTR)FALSE;

            HWND hList = GetDlgItem(hDlg, IDC_NOTES_JOURNAL_LIST);
            if (hList) {
                ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
                NotesJournal_InitListColumns(hList);
            }

            NotesJournal_Refresh(hDlg);
            return (INT_PTR)TRUE;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == IDC_NOTES_REFRESH_BTN) {
                NotesJournal_Refresh(hDlg);
                return (INT_PTR)TRUE;
            }
            if (id == IDC_NOTES_APPLY_BTN) {
                NotesJournal_ApplySelected(hDlg);
                return (INT_PTR)TRUE;
            }
            if (id == IDOK || id == IDCANCEL) {
                ShowWindow(hDlg, SW_HIDE);
                return (INT_PTR)TRUE;
            }
            break;
        }

        case WM_NOTIFY: {
            LPNMHDR hdr = reinterpret_cast<LPNMHDR>(lParam);
            if (hdr && hdr->idFrom == IDC_NOTES_JOURNAL_LIST && hdr->code == NM_DBLCLK) {
                NotesJournal_ApplySelected(hDlg);
                return (INT_PTR)TRUE;
            }
            break;
        }

        case WM_CLOSE:
            ShowWindow(hDlg, SW_HIDE);
            return (INT_PTR)TRUE;

        case WM_DESTROY:
            {
                NotesJournalDialogContext* ctx = reinterpret_cast<NotesJournalDialogContext*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
                if (ctx) {
                    SetWindowLongPtr(hDlg, GWLP_USERDATA, 0);
                    delete ctx;
                }
                g_notesJournalCtx = nullptr;
            }
            g_hNotesJournalDialog = nullptr;
            return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}

static void EnableEmphasizedDeviceMenuItems(HWND hWnd) {
    HMENU hMenu = GetMenu(hWnd);
    if (!hMenu) return;

    HMENU hDeviceMenu = GetSubMenu(hMenu, 1); // "Прибор"
    if (!hDeviceMenu) return;

    const UINT ids[] = { IDM_DEVICE_CONNECT, IDM_DEVICE_METROLOGY };
    for (UINT id : ids) {
        MENUITEMINFO mii{};
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_FTYPE;
        if (GetMenuItemInfo(hDeviceMenu, id, FALSE, &mii)) {
            mii.fType |= MFT_OWNERDRAW;
            SetMenuItemInfo(hDeviceMenu, id, FALSE, &mii);
        }
    }
}

static HFONT CreateBoldMenuFont() {
    NONCLIENTMETRICS ncm{};
    ncm.cbSize = sizeof(ncm);
    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0)) {
        LOGFONT lf = ncm.lfMenuFont;
        lf.lfWeight = FW_BOLD;
        return CreateFontIndirect(&lf);
    }

    LOGFONT lf{};
    HDC hScreenDC = GetDC(nullptr);
    int dpiY = hScreenDC ? GetDeviceCaps(hScreenDC, LOGPIXELSY) : 96;
    if (hScreenDC) ReleaseDC(nullptr, hScreenDC);
    lf.lfHeight = -MulDiv(9, dpiY, 72);
    lf.lfWeight = FW_BOLD;
    wcscpy_s(lf.lfFaceName, L"Segoe UI");
    return CreateFontIndirect(&lf);
}

// Глобальные переменные:
HINSTANCE hInst;                                // текущий экземпляр
WCHAR szTitle[MAX_LOADSTRING];                  // Текст строки заголовка
WCHAR szWindowClass[MAX_LOADSTRING];            // имя класса главного окна
ChartManager* g_chartManager = nullptr;         // Менеджер графиков
HWND g_hSerialDialog = nullptr;                 // v2.13.0: Handle диалога COM-порта
HWND g_hMetrologyDialog = nullptr;              // Handle диалога метрологии (немодальный)
MetrologyDialogContext* g_metrologyDialogCtx = nullptr; // Контекст диалога метрологии (живёт пока жив диалог)
HWND g_hNotesJournalDialog = nullptr;           // Handle окна журнала заметок (немодальный)

NotesJournalDialogContext* g_notesJournalCtx = nullptr;

// === Метрология (AUTONOM 5Tx) ===
struct MetrologyDialogContext {
    MetrologyModule* metro = nullptr;
    SerialModule* serial = nullptr;
    HWND hMainWnd = nullptr;
};

static void Metro_SetLFReadOnly(HWND hDlg, bool readOnly) {
    const int ids[] = {
        IDC_METRO_L1_T1, IDC_METRO_L1_T2, IDC_METRO_L1_T3, IDC_METRO_L1_T4, IDC_METRO_L1_T5,
        IDC_METRO_L2_T1, IDC_METRO_L2_T2, IDC_METRO_L2_T3, IDC_METRO_L2_T4, IDC_METRO_L2_T5,
        IDC_METRO_F1, IDC_METRO_F2
    };
    for (int id : ids) {
        HWND h = GetDlgItem(hDlg, id);
        if (h) SendMessage(h, EM_SETREADONLY, (WPARAM)(readOnly ? TRUE : FALSE), 0);
    }
}

static void Metro_SetEditUInt(HWND hDlg, int id, uint32_t v) {
    wchar_t buf[64];
    swprintf_s(buf, L"%u", v);
    SetDlgItemText(hDlg, id, buf);
}

static void Metro_SetEditInt(HWND hDlg, int id, int32_t v) {
    wchar_t buf[64];
    swprintf_s(buf, L"%d", v);
    SetDlgItemText(hDlg, id, buf);
}

static bool Metro_GetEditInt(HWND hDlg, int id, int& out) {
    wchar_t buf[128];
    GetDlgItemText(hDlg, id, buf, _countof(buf));
    if (wcslen(buf) == 0) { out = 0; return true; }
    wchar_t* end = nullptr;
    long v = wcstol(buf, &end, 10);
    if (end == buf) return false;
    out = (int)v;
    return true;
}

static bool Metro_GetEditIntOptional(HWND hDlg, int id, int& out, bool& hasValue) {
    wchar_t buf[128];
    GetDlgItemText(hDlg, id, buf, _countof(buf));

    if (wcslen(buf) == 0) {
        hasValue = false;
        return true;
    }

    wchar_t* end = nullptr;
    long v = wcstol(buf, &end, 10);
    if (end == buf) {
        hasValue = false;
        return false;
    }

    hasValue = true;
    out = (int)v;
    return true;
}

static void Metro_FillSelectedKeyCheckboxes(HWND hDlg, uint32_t key) {
    for (int bit = 0; bit <= 25; ++bit) {
        int id = IDC_METRO_KEY_0 + bit;
        SendMessage(GetDlgItem(hDlg, id), BM_SETCHECK, (key & (1u << bit)) ? BST_CHECKED : BST_UNCHECKED, 0);
    }
}

static uint32_t Metro_ReadSelectedKeyCheckboxes(HWND hDlg) {
    uint32_t key = 0;
    for (int bit = 0; bit <= 25; ++bit) {
        int id = IDC_METRO_KEY_0 + bit;
        if (SendMessage(GetDlgItem(hDlg, id), BM_GETCHECK, 0, 0) == BST_CHECKED) {
            key |= (1u << bit);
        }
    }
    return key;
}

static void Metro_UpdateSignaturePanel(HWND hDlg, const METROLOGY_DATA& metro) {
    SetDlgItemText(hDlg, IDC_METRO_SIGNATURE32_EDIT, MetrologyModule::SignatureToBinaryString(metro.signature).c_str());
    auto info = MetrologyModule::DecodeSignature(metro.signature);
    Metro_SetEditUInt(hDlg, IDC_METRO_SIGNATUREPAL_EDIT, info.signatureForPalette);

    // Формат старого/рабочего приложения (актуален для AUTONOM 5Tx):
    // buff=low20, mod=(buff%10000)/1000, number=buff%1000
    int mod = static_cast<int>((info.low20 % 10000u) / 1000u);
    int number = static_cast<int>(info.low20 % 1000u);

    HWND hMod = GetDlgItem(hDlg, IDC_METRO_MOD_COMBO);
    if (hMod) {
        if (mod >= 1 && mod <= 9) {
            SendMessage(hMod, CB_SETCURSEL, mod - 1, 0);
        } else {
            SendMessage(hMod, CB_SETCURSEL, 0, 0);
        }
    }
    Metro_SetEditInt(hDlg, IDC_METRO_NUMBER_EDIT, number);

    Metro_SetEditUInt(hDlg, IDC_METRO_SERIAL_EDIT, metro.serial);
}

static void Metro_FillFormFromStruct(HWND hDlg, const METROLOGY_DATA& m) {
    const int l1Ids[] = { IDC_METRO_L1_T1, IDC_METRO_L1_T2, IDC_METRO_L1_T3, IDC_METRO_L1_T4, IDC_METRO_L1_T5 };
    const int l2Ids[] = { IDC_METRO_L2_T1, IDC_METRO_L2_T2, IDC_METRO_L2_T3, IDC_METRO_L2_T4, IDC_METRO_L2_T5 };
    for (int i = 0; i < 5; ++i) {
        Metro_SetEditUInt(hDlg, l1Ids[i], m.L1[i]);
        Metro_SetEditUInt(hDlg, l2Ids[i], m.L2[i]);
    }

    Metro_SetEditUInt(hDlg, IDC_METRO_F1, m.F[0]);
    Metro_SetEditUInt(hDlg, IDC_METRO_F2, m.F[1]);

    const int airIds[] = {
        IDC_METRO_AIR_1, IDC_METRO_AIR_2, IDC_METRO_AIR_3, IDC_METRO_AIR_4, IDC_METRO_AIR_5,
        IDC_METRO_AIR_6, IDC_METRO_AIR_7, IDC_METRO_AIR_8, IDC_METRO_AIR_9, IDC_METRO_AIR_10
    };
    const int minIds[] = {
        IDC_METRO_MIN_1, IDC_METRO_MIN_2, IDC_METRO_MIN_3, IDC_METRO_MIN_4, IDC_METRO_MIN_5,
        IDC_METRO_MIN_6, IDC_METRO_MIN_7, IDC_METRO_MIN_8, IDC_METRO_MIN_9, IDC_METRO_MIN_10
    };
    for (int i = 0; i < 10; ++i) {
        Metro_SetEditInt(hDlg, airIds[i], m.Air_zz[i]);
        Metro_SetEditInt(hDlg, minIds[i], m.Air_zz_amt[i]);
    }

    Metro_UpdateSignaturePanel(hDlg, m);
}

static bool Metro_FillStructFromForm(HWND hDlg, METROLOGY_DATA& out, std::wstring& outError) {
    outError.clear();

    const int l1Ids[] = { IDC_METRO_L1_T1, IDC_METRO_L1_T2, IDC_METRO_L1_T3, IDC_METRO_L1_T4, IDC_METRO_L1_T5 };
    const int l2Ids[] = { IDC_METRO_L2_T1, IDC_METRO_L2_T2, IDC_METRO_L2_T3, IDC_METRO_L2_T4, IDC_METRO_L2_T5 };

    for (int i = 0; i < 5; ++i) {
        int v = 0;
        bool hasValue = false;

        if (!Metro_GetEditIntOptional(hDlg, l1Ids[i], v, hasValue) || (hasValue && (v < 0 || v > 65535))) {
            outError = L"Некорректное значение L1.";
            return false;
        }
        if (hasValue) out.L1[i] = (uint16_t)v;

        if (!Metro_GetEditIntOptional(hDlg, l2Ids[i], v, hasValue) || (hasValue && (v < 0 || v > 65535))) {
            outError = L"Некорректное значение L2.";
            return false;
        }
        if (hasValue) out.L2[i] = (uint16_t)v;
    }

    int f1 = 0, f2 = 0;
    bool hasF1 = false, hasF2 = false;
    if (!Metro_GetEditIntOptional(hDlg, IDC_METRO_F1, f1, hasF1) || (hasF1 && (f1 < 0 || f1 > 65535))) {
        outError = L"Некорректное значение F1.";
        return false;
    }
    if (!Metro_GetEditIntOptional(hDlg, IDC_METRO_F2, f2, hasF2) || (hasF2 && (f2 < 0 || f2 > 65535))) {
        outError = L"Некорректное значение F2.";
        return false;
    }
    if (hasF1) out.F[0] = (uint16_t)f1;
    if (hasF2) out.F[1] = (uint16_t)f2;

    const int airIds[] = {
        IDC_METRO_AIR_1, IDC_METRO_AIR_2, IDC_METRO_AIR_3, IDC_METRO_AIR_4, IDC_METRO_AIR_5,
        IDC_METRO_AIR_6, IDC_METRO_AIR_7, IDC_METRO_AIR_8, IDC_METRO_AIR_9, IDC_METRO_AIR_10
    };
    const int minIds[] = {
        IDC_METRO_MIN_1, IDC_METRO_MIN_2, IDC_METRO_MIN_3, IDC_METRO_MIN_4, IDC_METRO_MIN_5,
        IDC_METRO_MIN_6, IDC_METRO_MIN_7, IDC_METRO_MIN_8, IDC_METRO_MIN_9, IDC_METRO_MIN_10
    };
    for (int i = 0; i < 10; ++i) {
        int v = 0;
        bool hasValue = false;

        if (!Metro_GetEditIntOptional(hDlg, airIds[i], v, hasValue) || (hasValue && (v < -32768 || v > 32767))) {
            outError = L"Некорректное значение AIR.";
            return false;
        }
        if (hasValue) out.Air_zz[i] = (int16_t)v;

        if (!Metro_GetEditIntOptional(hDlg, minIds[i], v, hasValue) || (hasValue && (v < -32768 || v > 32767))) {
            outError = L"Некорректное значение MIN.";
            return false;
        }
        if (hasValue) out.Air_zz_amt[i] = (int16_t)v;
    }

    int serial = 0;
    bool hasSerial = false;
    if (!Metro_GetEditIntOptional(hDlg, IDC_METRO_SERIAL_EDIT, serial, hasSerial) || (hasSerial && serial < 0)) {
        outError = L"Некорректный serial.";
        return false;
    }
    if (hasSerial) out.serial = (uint32_t)serial;

    // signature: we build only AUTONOM 5Tx (151) by UI mod+number, preserving high 12 bits.
    int modSel = (int)SendMessage(GetDlgItem(hDlg, IDC_METRO_MOD_COMBO), CB_GETCURSEL, 0, 0);
    int mod = (modSel == CB_ERR) ? 1 : (modSel + 1);
    int number = 0;
    if (!Metro_GetEditInt(hDlg, IDC_METRO_NUMBER_EDIT, number)) number = 0;
    out.signature = MetrologyModule::BuildSignature(151u, mod, number, out.signature);
    return true;
}

static INT_PTR CALLBACK MetrologyDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam);
            MetrologyDialogContext* ctx = reinterpret_cast<MetrologyDialogContext*>(lParam);

            // Показываем окно как отдельную кнопку в панели задач (в т.ч. при сворачивании).
            LONG_PTR exStyle = GetWindowLongPtr(hDlg, GWL_EXSTYLE);
            exStyle |= WS_EX_APPWINDOW;
            exStyle &= ~WS_EX_TOOLWINDOW;
            SetWindowLongPtr(hDlg, GWL_EXSTYLE, exStyle);
            SetWindowPos(hDlg, nullptr, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

            // Наследуем иконки от главного окна, чтобы кнопка в taskbar была узнаваемой.
            if (ctx && ctx->hMainWnd) {
                HICON hIconBig = (HICON)SendMessage(ctx->hMainWnd, WM_GETICON, ICON_BIG, 0);
                if (!hIconBig) hIconBig = (HICON)GetClassLongPtr(ctx->hMainWnd, GCLP_HICON);
                if (hIconBig) SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);

                HICON hIconSmall = (HICON)SendMessage(ctx->hMainWnd, WM_GETICON, ICON_SMALL, 0);
                if (!hIconSmall) hIconSmall = (HICON)GetClassLongPtr(ctx->hMainWnd, GCLP_HICONSM);
                if (hIconSmall) SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);
            }

            // Init combos (kept mostly informational; we support AUTONOM 5Tx only)
            HWND hType = GetDlgItem(hDlg, IDC_METRO_TYPE_COMBO);
            if (hType) {
                SendMessage(hType, CB_ADDSTRING, 0, (LPARAM)L"АВТОНОМ");
                SendMessage(hType, CB_ADDSTRING, 0, (LPARAM)L"LWD");
                SendMessage(hType, CB_ADDSTRING, 0, (LPARAM)L"КАРТОГРАФ");
                SendMessage(hType, CB_SETCURSEL, 0, 0);
            }
            HWND hNtx = GetDlgItem(hDlg, IDC_METRO_NTX_COMBO);
            if (hNtx) {
                SendMessage(hNtx, CB_ADDSTRING, 0, (LPARAM)L"3");
                SendMessage(hNtx, CB_ADDSTRING, 0, (LPARAM)L"4");
                SendMessage(hNtx, CB_ADDSTRING, 0, (LPARAM)L"5");
                SendMessage(hNtx, CB_SETCURSEL, 2, 0);
            }
            HWND hMod = GetDlgItem(hDlg, IDC_METRO_MOD_COMBO);
            if (hMod) {
                for (int i = 1; i <= 9; ++i) {
                    wchar_t b[8];
                    swprintf_s(b, L"%d", i);
                    SendMessage(hMod, CB_ADDSTRING, 0, (LPARAM)b);
                }
                SendMessage(hMod, CB_SETCURSEL, 0, 0);
            }

            CheckDlgButton(hDlg, IDC_METRO_READONLY_CHECK, BST_CHECKED);
            Metro_SetLFReadOnly(hDlg, true);

            if (ctx && ctx->metro && ctx->metro->HasData()) {
                Metro_FillFormFromStruct(hDlg, ctx->metro->Get());
                Metro_FillSelectedKeyCheckboxes(hDlg, ctx->metro->GetSelectedKey());
            } else if (ctx && ctx->metro) {
                METROLOGY_DATA blank{};
                ctx->metro->Set(blank, false);
                Metro_FillFormFromStruct(hDlg, blank);
                Metro_FillSelectedKeyCheckboxes(hDlg, 0);
            }

            return (INT_PTR)TRUE;
        }

        case WM_COMMAND: {
            const int id = LOWORD(wParam);
            MetrologyDialogContext* ctx = reinterpret_cast<MetrologyDialogContext*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
            if (!ctx || !ctx->metro) {
                if (id == IDOK || id == IDCANCEL) {
                    ShowWindow(hDlg, SW_HIDE);
                    return (INT_PTR)TRUE;
                }
                return (INT_PTR)FALSE;
            }

            if (id == IDC_METRO_READONLY_CHECK) {
                bool checked = (IsDlgButtonChecked(hDlg, IDC_METRO_READONLY_CHECK) == BST_CHECKED);
                Metro_SetLFReadOnly(hDlg, checked);
                return (INT_PTR)TRUE;
            }

            if (id == IDC_METRO_BTN_OPEN_FILE) {
                wchar_t fileName[MAX_PATH] = {0};
                OPENFILENAME ofn = {};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hDlg;
                ofn.lpstrFilter = L"Metrology (*.bin)\0*.bin\0All Files\0*.*\0";
                ofn.lpstrFile = fileName;
                ofn.nMaxFile = _countof(fileName);
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
                ofn.lpstrDefExt = L"bin";
                if (GetOpenFileName(&ofn)) {
                    std::wstring err;
                    if (ctx->metro->LoadFromFile(fileName, err)) {
                        Metro_FillFormFromStruct(hDlg, ctx->metro->Get());
                        MessageBox(hDlg, L"Метрология загружена из файла.", L"METROLOGY", MB_OK | MB_ICONINFORMATION);
                    } else {
                        MessageBox(hDlg, err.c_str(), L"Ошибка", MB_OK | MB_ICONERROR);
                    }
                }
                return (INT_PTR)TRUE;
            }

            if (id == IDC_METRO_BTN_SAVE_FILE) {
                METROLOGY_DATA tmp = ctx->metro->Get();
                std::wstring err;
                if (!Metro_FillStructFromForm(hDlg, tmp, err)) {
                    MessageBox(hDlg, err.c_str(), L"Ошибка", MB_OK | MB_ICONWARNING);
                    return (INT_PTR)TRUE;
                }

                wchar_t fileName[MAX_PATH] = {0};
                OPENFILENAME ofn = {};
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hDlg;
                ofn.lpstrFilter = L"Metrology (*.bin)\0*.bin\0All Files\0*.*\0";
                ofn.lpstrFile = fileName;
                ofn.nMaxFile = _countof(fileName);
                ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;
                ofn.lpstrDefExt = L"bin";
                if (GetSaveFileName(&ofn)) {
                    ctx->metro->Set(tmp, false);
                    std::wstring saveErr;
                    if (ctx->metro->SaveToFile(fileName, saveErr)) {
                        ctx->metro->SetCurrentFilePath(fileName);
                        MessageBox(hDlg, L"Метрология сохранена в файл.", L"METROLOGY", MB_OK | MB_ICONINFORMATION);
                    } else {
                        MessageBox(hDlg, saveErr.c_str(), L"Ошибка", MB_OK | MB_ICONERROR);
                    }
                }
                return (INT_PTR)TRUE;
            }

            if (id == IDC_METRO_BTN_READ_SONDE) {
                if (!ctx->serial || !ctx->serial->IsConnected()) {
                    MessageBox(hDlg, L"Нет подключения к прибору.", L"METROLOGY", MB_OK | MB_ICONWARNING);
                    return (INT_PTR)TRUE;
                }
                METROLOGY_DATA m{};
                SerialResult r = ctx->serial->RequestMetrology(m);
                if (r == SerialResult::SUCCESS) {
                    ctx->metro->Set(m, false);
                    Metro_FillFormFromStruct(hDlg, m);
                    MessageBox(hDlg, L"Метрология считана из зонда.", L"METROLOGY", MB_OK | MB_ICONINFORMATION);
                } else {
                    std::wstring msg = L"Ошибка: " + SerialModule::ResultToString(r);
                    MessageBox(hDlg, msg.c_str(), L"METROLOGY", MB_OK | MB_ICONERROR);
                }
                return (INT_PTR)TRUE;
            }

            if (id == IDC_METRO_BTN_WRITE_SONDE) {
                if (!ctx->serial || !ctx->serial->IsConnected()) {
                    MessageBox(hDlg, L"Нет подключения к прибору.", L"METROLOGY", MB_OK | MB_ICONWARNING);
                    return (INT_PTR)TRUE;
                }
                int res = MessageBox(hDlg,
                                     L"Записать метрологию в зонд?\nПо умолчанию Cancel.",
                                     L"Подтверждение",
                                     MB_OKCANCEL | MB_ICONWARNING | MB_DEFBUTTON2);
                if (res != IDOK) return (INT_PTR)TRUE;

                METROLOGY_DATA tmp = ctx->metro->Get();
                std::wstring err;
                if (!Metro_FillStructFromForm(hDlg, tmp, err)) {
                    MessageBox(hDlg, err.c_str(), L"Ошибка", MB_OK | MB_ICONWARNING);
                    return (INT_PTR)TRUE;
                }

                // Will be implemented in SerialModule (CMD 0x12)
                SerialResult wr = ctx->serial->WriteMetrology(tmp);
                if (wr == SerialResult::SUCCESS) {
                    ctx->metro->Set(tmp, false);
                    MessageBox(hDlg, L"Метрология записана в зонд.", L"METROLOGY", MB_OK | MB_ICONINFORMATION);
                } else {
                    std::wstring msg = L"Ошибка: " + SerialModule::ResultToString(wr);
                    MessageBox(hDlg, msg.c_str(), L"METROLOGY", MB_OK | MB_ICONERROR);
                }
                return (INT_PTR)TRUE;
            }

            if (id == IDC_METRO_BTN_WRITE_CMD) {
                if (!ctx->serial || !ctx->serial->IsConnected()) {
                    MessageBox(hDlg, L"Нет подключения к прибору.", L"METROLOGY", MB_OK | MB_ICONWARNING);
                    return (INT_PTR)TRUE;
                }
                uint32_t key = Metro_ReadSelectedKeyCheckboxes(hDlg);
                ctx->metro->SetSelectedKey(key);

                SerialResult rr = ctx->serial->SetSelectedKey(key);
                if (rr == SerialResult::SUCCESS) {
                    MessageBox(hDlg, L"Команда (selected_key) отправлена.", L"METROLOGY", MB_OK | MB_ICONINFORMATION);
                } else {
                    std::wstring msg = L"Ошибка: " + SerialModule::ResultToString(rr);
                    MessageBox(hDlg, msg.c_str(), L"METROLOGY", MB_OK | MB_ICONERROR);
                }
                return (INT_PTR)TRUE;
            }

            if (id == IDOK || id == IDCANCEL) {
                ShowWindow(hDlg, SW_HIDE);
                return (INT_PTR)TRUE;
            }
            break;
        }

        case WM_CLOSE:
            ShowWindow(hDlg, SW_HIDE);
            return (INT_PTR)TRUE;

        case WM_DESTROY:
            {
                MetrologyDialogContext* ctx = reinterpret_cast<MetrologyDialogContext*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
                if (ctx) {
                    SetWindowLongPtr(hDlg, GWLP_USERDATA, 0);
                    delete ctx;
                }
                g_metrologyDialogCtx = nullptr;
            }
            g_hMetrologyDialog = nullptr;
            return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}

// === Заметки на графике (v2.16.x) ===
struct NoteDialogContext {
    std::wstring text;
    bool ok = false;
};

static INT_PTR CALLBACK NoteDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam);
            NoteDialogContext* ctx = reinterpret_cast<NoteDialogContext*>(lParam);
            if (ctx) {
                SetDlgItemText(hDlg, IDC_NOTE_TEXT_EDIT, ctx->text.c_str());
                SendMessage(GetDlgItem(hDlg, IDC_NOTE_TEXT_EDIT), EM_SETSEL, 0, -1);
                SetFocus(GetDlgItem(hDlg, IDC_NOTE_TEXT_EDIT));
                return (INT_PTR)FALSE; // focus set manually
            }
            return (INT_PTR)TRUE;
        }
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == IDOK) {
                NoteDialogContext* ctx = reinterpret_cast<NoteDialogContext*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
                if (ctx) {
                    wchar_t buf[4096];
                    GetDlgItemText(hDlg, IDC_NOTE_TEXT_EDIT, buf, _countof(buf));
                    ctx->text = buf;
                    ctx->ok = true;
                }
                EndDialog(hDlg, IDOK);
                return (INT_PTR)TRUE;
            }
            if (id == IDCANCEL) {
                EndDialog(hDlg, IDCANCEL);
                return (INT_PTR)TRUE;
            }
            break;
        }
    }
    return (INT_PTR)FALSE;
}

// Отправить объявления функций, включенных в этот модуль кода:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    SerialStatusDialogProc(HWND, UINT, WPARAM, LPARAM);  // v2.13.0

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Разместите код здесь.

    // Инициализация глобальных строк
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_AMKHORIZONNEWCHART, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Выполнить инициализацию приложения:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_AMKHORIZONNEWCHART));

    MSG msg;

    // Цикл основного сообщения:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  ФУНКЦИЯ: MyRegisterClass()
//
//  ЦЕЛЬ: Регистрирует класс окна.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_AMKHORIZONNEWCHART));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_AMKHORIZONNEWCHART);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   ФУНКЦИЯ: InitInstance(HINSTANCE, int)
//
//   ЦЕЛЬ: Сохраняет маркер экземпляра и создает главное окно
//
//   КОММЕНТАРИИ:
//
//        В этой функции маркер экземпляра сохраняется в глобальной переменной, а также
//        создается и выводится главное окно программы.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Сохранить маркер экземпляра в глобальной переменной

   // Инициализация Common Controls для статусной строки
   INITCOMMONCONTROLSEX icex;
   icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
   icex.dwICC = ICC_BAR_CLASSES; // Включаем статусную строку
   InitCommonControlsEx(&icex);

   // v2.14.2: Увеличен начальный размер окна (800x600 → 1000x600) для корректного отображения всех элементов
   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
      CW_USEDEFAULT, 0, 1000, 600, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   EnableEmphasizedDeviceMenuItems(hWnd);

   // Создаем менеджер графиков
   g_chartManager = new ChartManager();
   g_chartManager->CreateControls(hWnd, hInstance);

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  ФУНКЦИЯ: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  ЦЕЛЬ: Обрабатывает сообщения в главном окне.
//
//  WM_COMMAND  - обработать меню приложения
//  WM_PAINT    - Отрисовка главного окна
//  WM_DESTROY  - отправить сообщение о выходе и вернуться
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Разобрать выбор в меню:
            switch (wmId)
            {
            case IDM_ABOUT:
                DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                break;
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            
            // === Команды меню "Прибор" (v2.13.0) ===
            case IDM_DEVICE_CONNECT:
                // Открываем диалог подключения к прибору
                if (!g_hSerialDialog || !IsWindow(g_hSerialDialog)) {
                    g_hSerialDialog = CreateDialog(hInst, MAKEINTRESOURCE(IDD_SERIAL_STATUS), 
                                                   hWnd, SerialStatusDialogProc);
                    if (g_hSerialDialog) {
                        ShowWindow(g_hSerialDialog, SW_SHOW);
                    }
                } else {
                    // Диалог уже создан: показываем/восстанавливаем и выводим на передний план
                    if (IsIconic(g_hSerialDialog)) {
                        ShowWindow(g_hSerialDialog, SW_RESTORE);
                    } else {
                        ShowWindow(g_hSerialDialog, SW_SHOW);
                    }
                    SetForegroundWindow(g_hSerialDialog);
                }
                break;
                
            case IDM_DEVICE_DISCONNECT:
                if (g_chartManager) {
                    g_chartManager->HandleSerialDisconnect(hWnd);
                }
                break;
                
            case IDM_DEVICE_START:
                if (g_chartManager) {
                    g_chartManager->HandleSerialStart(hWnd);
                }
                break;
                
            case IDM_DEVICE_STOP:
                if (g_chartManager) {
                    g_chartManager->HandleSerialStop(hWnd);
                }
                break;
                
            case IDM_DEVICE_CLEAR:
                if (g_chartManager) {
                    g_chartManager->HandleSerialClear(hWnd);
                }
                break;

            case IDM_DEVICE_METROLOGY:
                if (g_chartManager) {
                    if (!g_hMetrologyDialog || !IsWindow(g_hMetrologyDialog)) {
                        if (g_metrologyDialogCtx) {
                            delete g_metrologyDialogCtx;
                            g_metrologyDialogCtx = nullptr;
                        }

                        g_metrologyDialogCtx = new MetrologyDialogContext();
                        g_metrologyDialogCtx->metro = g_chartManager->GetMetrologyModule();
                        g_metrologyDialogCtx->serial = g_chartManager->GetSerialModule();
                        g_metrologyDialogCtx->hMainWnd = hWnd;

                        g_hMetrologyDialog = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_METROLOGY_DIALOG),
                                                              nullptr, MetrologyDialogProc, (LPARAM)g_metrologyDialogCtx);
                        if (g_hMetrologyDialog) {
                            ShowWindow(g_hMetrologyDialog, SW_SHOW);
                        } else {
                            delete g_metrologyDialogCtx;
                            g_metrologyDialogCtx = nullptr;
                        }
                    } else {
                        if (IsIconic(g_hMetrologyDialog)) {
                            ShowWindow(g_hMetrologyDialog, SW_RESTORE);
                        } else {
                            ShowWindow(g_hMetrologyDialog, SW_SHOW);
                        }
                        SetForegroundWindow(g_hMetrologyDialog);
                    }
                }
                break;
            
            // v2.15.0: Команды записи в файл
            case IDM_DEVICE_RECORD_FOLDER:
                if (g_chartManager) {
                    SerialModule* serial = g_chartManager->GetSerialModule();
                    if (serial) {
                        std::wstring folder = SerialModule::BrowseForFolder(hWnd);
                        if (!folder.empty()) {
                            serial->SetRecordFolder(folder);
                        }
                    }
                }
                break;
                
            case IDM_DEVICE_OPEN_FOLDER:
                if (g_chartManager) {
                    SerialModule* serial = g_chartManager->GetSerialModule();
                    if (serial) {
                        serial->OpenRecordFolder();
                    }
                }
                break;

            case IDM_NOTES_JOURNAL:
                if (g_chartManager) {
                    if (!g_hNotesJournalDialog || !IsWindow(g_hNotesJournalDialog)) {
                        if (g_notesJournalCtx) {
                            delete g_notesJournalCtx;
                            g_notesJournalCtx = nullptr;
                        }

                        g_notesJournalCtx = new NotesJournalDialogContext();
                        g_notesJournalCtx->manager = g_chartManager;
                        g_notesJournalCtx->hMainWnd = hWnd;

                        g_hNotesJournalDialog = CreateDialogParam(hInst,
                                                                  MAKEINTRESOURCE(IDD_NOTES_JOURNAL_DIALOG),
                                                                  nullptr,
                                                                  NotesJournalDialogProc,
                                                                  (LPARAM)g_notesJournalCtx);
                        if (g_hNotesJournalDialog) {
                            ShowWindow(g_hNotesJournalDialog, SW_SHOW);
                        } else {
                            delete g_notesJournalCtx;
                            g_notesJournalCtx = nullptr;
                        }
                    } else {
                        SendMessage(g_hNotesJournalDialog, WM_COMMAND, MAKEWPARAM(IDC_NOTES_REFRESH_BTN, BN_CLICKED), 0);
                        if (IsIconic(g_hNotesJournalDialog)) {
                            ShowWindow(g_hNotesJournalDialog, SW_RESTORE);
                        } else {
                            ShowWindow(g_hNotesJournalDialog, SW_SHOW);
                        }
                        SetForegroundWindow(g_hNotesJournalDialog);
                    }
                }
                break;
            
            case IDM_OPEN_FILE:
                {
                    OPENFILENAME ofn;
                    WCHAR szFile[260] = { 0 };
                    
                    ZeroMemory(&ofn, sizeof(ofn));
                    ofn.lStructSize = sizeof(ofn);
                    ofn.hwndOwner = hWnd;
                    ofn.lpstrFile = szFile;
                    ofn.nMaxFile = sizeof(szFile);
                    // v2.15.1: Добавлена поддержка форматов .biv и .DEV
                    ofn.lpstrFilter = L"All Data Files\0*.bin;*.biv;*.DEV\0Binary Files (*.bin)\0*.bin\0BIV Files (*.biv)\0*.biv\0DEV Files (*.DEV)\0*.DEV\0All Files\0*.*\0";
                    ofn.nFilterIndex = 1;
                    ofn.lpstrFileTitle = NULL;
                    ofn.nMaxFileTitle = 0;
                    ofn.lpstrInitialDir = NULL;
                    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                    
                    if (GetOpenFileName(&ofn) && g_chartManager)
                    {
                        g_chartManager->LoadDataFromFile(szFile);
                        InvalidateRect(hWnd, NULL, FALSE);
                    }
                }
                break;
            // Обработка кнопок схемы прибора
            case IDC_CHART1_SCHEMATIC_BTN:
                if (g_chartManager)
                {
                    g_chartManager->ShowSchematicDialog(hWnd, 1);
                    InvalidateRect(hWnd, NULL, FALSE);
                }
                break;
            case IDC_CHART2_SCHEMATIC_BTN:
                if (g_chartManager)
                {
                    g_chartManager->ShowSchematicDialog(hWnd, 2);
                    InvalidateRect(hWnd, NULL, FALSE);
                }
                break;
            // Обработка кнопки сохранения диапазона
            case IDC_SAVE_RANGE1_BTN:
                if (g_chartManager)
                {
                    g_chartManager->HandleSaveRange(hWnd, 1);
                }
                break;
            
            // === Обработка режима Surf (v2.14.0) ===
            case IDC_CHART1_SURF_CHECK:
                if (g_chartManager)
                {
                    g_chartManager->HandleSurfToggle(hWnd, 1);
                    InvalidateRect(hWnd, NULL, FALSE);
                }
                break;
            case IDC_CHART2_SURF_CHECK:
                if (g_chartManager)
                {
                    g_chartManager->HandleSurfToggle(hWnd, 2);
                    InvalidateRect(hWnd, NULL, FALSE);
                }
                break;
            case IDC_CHART1_SURF_EDIT:
                if (HIWORD(wParam) == EN_CHANGE && g_chartManager)
                {
                    // Обновляем диапазон Surf при изменении значения
                    g_chartManager->UpdateSurfRange(1);
                    g_chartManager->UpdateSurfStatusBar();
                    InvalidateRect(hWnd, NULL, FALSE);
                }
                break;
            case IDC_CHART2_SURF_EDIT:
                if (HIWORD(wParam) == EN_CHANGE && g_chartManager)
                {
                    // Обновляем диапазон Surf при изменении значения
                    g_chartManager->UpdateSurfRange(2);
                    g_chartManager->UpdateSurfStatusBar();
                    InvalidateRect(hWnd, NULL, FALSE);
                }
                break;
            
            // === v2.14.2: Сброс перекрестия при клике на иконку 📍 ===
            case IDC_CURSOR1_ICON:
                if (HIWORD(wParam) == STN_CLICKED && g_chartManager)
                {
                    g_chartManager->ClearCrosshair(hWnd, 1);
                }
                break;
            case IDC_CURSOR2_ICON:
                if (HIWORD(wParam) == STN_CLICKED && g_chartManager)
                {
                    g_chartManager->ClearCrosshair(hWnd, 2);
                }
                break;
            
            default:
                if (g_chartManager)
                {
                    if (g_chartManager->HandleCommand(wParam, lParam))
                    {
                        InvalidateRect(hWnd, NULL, FALSE);
                    }
                }
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_SIZE:
        if (g_chartManager)
        {
            g_chartManager->UpdateLayout(hWnd);
            InvalidateRect(hWnd, NULL, FALSE);
        }
        break;
    case WM_LBUTTONDOWN:
        if (g_chartManager)
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            g_chartManager->HandleMouseDown(hWnd, x, y);
        }
        break;
    case WM_MOUSEMOVE:
        if (g_chartManager)
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            g_chartManager->HandleMouseMove(hWnd, x, y);
            g_chartManager->HandleMouseMoveForCoordinates(hWnd, x, y); // Обновление координат в статусной строке
        }
        break;
    case WM_LBUTTONUP:
        if (g_chartManager)
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            g_chartManager->HandleMouseUp(hWnd, x, y);
        }
        break;
    case WM_RBUTTONDOWN:
        if (g_chartManager)
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            g_chartManager->HandleRightClick(hWnd, x, y);
        }
        break;
    case WM_CONTEXTMENU:
        // v2.16.x: ПКМ по иконке 📍 открывает окно заметки
        if (g_chartManager) {
            HWND hTarget = (HWND)wParam;
            int ctrlId = hTarget ? GetDlgCtrlID(hTarget) : 0;
            int chartNumber = 0;
            if (ctrlId == IDC_CURSOR1_ICON) chartNumber = 1;
            else if (ctrlId == IDC_CURSOR2_ICON) chartNumber = 2;

            if (chartNumber != 0) {
                NoteDialogContext ctx;
                ctx.text = L"";
                ctx.ok = false;
                DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_NOTE_DIALOG), hWnd, NoteDialogProc, (LPARAM)&ctx);
                if (ctx.ok) {
                    g_chartManager->AddNoteAtCrosshair(hWnd, chartNumber, ctx.text);
                }
                return 0;
            }
        }
        break;
    case WM_HSCROLL:
    case WM_VSCROLL:
        if (g_chartManager)
        {
            g_chartManager->HandleScrollEvent(hWnd, wParam, lParam);
        }
        break;
    case WM_TIMER:
        // v2.13.0: Обработка таймера опроса COM-порта
        if (wParam == IDT_SERIAL_POLL && g_chartManager)
        {
            g_chartManager->GetSerialModule()->OnTimer();
            
            // Обновляем диалог статуса, если он открыт
            if (g_hSerialDialog && IsWindow(g_hSerialDialog)) {
                SerialModule* serial = g_chartManager->GetSerialModule();
                if (serial) {
                    wchar_t frameText[32], packetsText[64];
                    swprintf_s(frameText, L"%u", serial->GetLastFrameNumber());
                    swprintf_s(packetsText, L"%u / %u", serial->GetPacketsReceived(), serial->GetPacketsErrors());
                    
                    SetDlgItemText(g_hSerialDialog, IDC_SERIAL_FRAME_LABEL, frameText);
                    SetDlgItemText(g_hSerialDialog, IDC_SERIAL_PACKETS_LABEL, packetsText);
                    
                    // v2.15.0: Обновляем статус записи
                    if (serial->IsRecordingEnabled() && serial->IsRecordingActive()) {
                        std::wstring filePath = serial->GetRecordFilePath();
                        size_t lastSlash = filePath.find_last_of(L"\\/");
                        std::wstring fileName = (lastSlash != std::wstring::npos) 
                            ? filePath.substr(lastSlash + 1) : filePath;
                        SetDlgItemText(g_hSerialDialog, IDC_SERIAL_RECORD_STATUS, fileName.c_str());
                        
                        wchar_t framesText[32];
                        swprintf_s(framesText, L"%u кадров", serial->GetRecordedFrames());
                        SetDlgItemText(g_hSerialDialog, IDC_SERIAL_RECORD_FRAMES, framesText);
                    }
                }
            }
        }
        break;
    case WM_USER + 100: // Пользовательское сообщение для обновления графика после применения настроек схемы
        if (g_chartManager)
        {
            int chartNumber = static_cast<int>(wParam);
            if (chartNumber >= 1 && chartNumber <= 2)
            {
                // Обновляем конкретный график после применения настроек
                InvalidateRect(hWnd, NULL, FALSE);
            }
        }
        break;
    case WM_MEASUREITEM:
        {
            LPMEASUREITEMSTRUCT lpMeasure = (LPMEASUREITEMSTRUCT)lParam;
            if (lpMeasure && lpMeasure->CtlType == ODT_MENU && IsEmphasizedDeviceMenuItem((UINT)lpMeasure->itemID)) {
                HMENU hMenu = GetMenu(hWnd);
                HMENU hDeviceMenu = hMenu ? GetSubMenu(hMenu, 1) : nullptr;
                wchar_t text[128] = {0};
                if (hDeviceMenu) {
                    GetMenuString(hDeviceMenu, lpMeasure->itemID, text, _countof(text), MF_BYCOMMAND);
                }

                HDC hdc = GetDC(hWnd);
                HFONT hBoldFont = CreateBoldMenuFont();
                HFONT hOldFont = hBoldFont ? (HFONT)SelectObject(hdc, hBoldFont) : nullptr;

                SIZE sz{};
                GetTextExtentPoint32(hdc, text, (int)wcslen(text), &sz);
                lpMeasure->itemWidth = sz.cx + 24;
                lpMeasure->itemHeight = sz.cy + 6;

                if (hOldFont) SelectObject(hdc, hOldFont);
                if (hBoldFont) DeleteObject(hBoldFont);
                ReleaseDC(hWnd, hdc);
                return TRUE;
            }
        }
        break;
    case WM_DRAWITEM:
        // Обработка Owner-Draw кнопок с цветными иконками
        {
            LPDRAWITEMSTRUCT lpDrawItem = (LPDRAWITEMSTRUCT)lParam;

            if (lpDrawItem && lpDrawItem->CtlType == ODT_MENU && IsEmphasizedDeviceMenuItem((UINT)lpDrawItem->itemID)) {
                wchar_t text[128] = {0};
                HMENU hMenu = GetMenu(hWnd);
                HMENU hDeviceMenu = hMenu ? GetSubMenu(hMenu, 1) : nullptr;
                if (hDeviceMenu) {
                    GetMenuString(hDeviceMenu, lpDrawItem->itemID, text, _countof(text), MF_BYCOMMAND);
                }

                COLORREF bgColor = (lpDrawItem->itemState & ODS_SELECTED) ? GetSysColor(COLOR_HIGHLIGHT) : GetSysColor(COLOR_MENU);
                COLORREF textColor = (lpDrawItem->itemState & ODS_SELECTED) ? GetSysColor(COLOR_HIGHLIGHTTEXT) : GetSysColor(COLOR_MENUTEXT);

                HBRUSH bgBrush = CreateSolidBrush(bgColor);
                FillRect(lpDrawItem->hDC, &lpDrawItem->rcItem, bgBrush);
                DeleteObject(bgBrush);

                SetBkMode(lpDrawItem->hDC, TRANSPARENT);
                SetTextColor(lpDrawItem->hDC, textColor);

                HFONT hBoldFont = CreateBoldMenuFont();
                HFONT hOldFont = hBoldFont ? (HFONT)SelectObject(lpDrawItem->hDC, hBoldFont) : nullptr;

                RECT rcText = lpDrawItem->rcItem;
                rcText.left += 12;
                DrawText(lpDrawItem->hDC, text, -1, &rcText, DT_SINGLELINE | DT_VCENTER | DT_LEFT | DT_HIDEPREFIX);

                if (lpDrawItem->itemState & ODS_FOCUS) {
                    DrawFocusRect(lpDrawItem->hDC, &lpDrawItem->rcItem);
                }

                if (hOldFont) SelectObject(lpDrawItem->hDC, hOldFont);
                if (hBoldFont) DeleteObject(hBoldFont);
                return TRUE;
            }

            if (lpDrawItem && g_chartManager) {
                UIModule* uiModule = g_chartManager->GetUIModule();
                if (uiModule && uiModule->IsIconButton(lpDrawItem->CtlID)) {
                    uiModule->DrawIconButton(lpDrawItem);
                    return TRUE;
                }
            }
        }
        break;
    case WM_ERASEBKGND:
        // Предотвращаем стирание фона для устранения мигания при скроллинге
        // Фон будет отрисован в WM_PAINT через double buffering
        return 1;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            
            if (g_chartManager)
            {
                // Получаем размеры клиентской области
                RECT clientRect;
                GetClientRect(hWnd, &clientRect);
                int width = clientRect.right - clientRect.left;
                int height = clientRect.bottom - clientRect.top;
                
                // Создаем буфер в памяти для double buffering (устранение мигания)
                HDC hdcMem = CreateCompatibleDC(hdc);
                HBITMAP hBitmap = CreateCompatibleBitmap(hdc, width, height);
                HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);
                
                // Заполняем фон буфера цветом окна
                HBRUSH hBrush = (HBRUSH)(COLOR_WINDOW + 1);
                FillRect(hdcMem, &clientRect, hBrush);
                
                // Рисуем графики в буфер
                g_chartManager->DrawChart1(hdcMem);
                if (g_chartManager->IsDualMode())
                {
                    g_chartManager->DrawChart2(hdcMem);
                }
                
                // Копируем буфер на экран одной операцией
                BitBlt(hdc, 0, 0, width, height, hdcMem, 0, 0, SRCCOPY);
                
                // Освобождаем ресурсы
                SelectObject(hdcMem, hOldBitmap);
                DeleteObject(hBitmap);
                DeleteDC(hdcMem);
            }
            
            EndPaint(hWnd, &ps);
        }
        break;
    case WM_DESTROY:
        g_hMetrologyDialog = nullptr;
        if (g_chartManager)
        {
            delete g_chartManager;
            g_chartManager = nullptr;
        }
        PostQuitMessage(0);
        break;
    
    // v2.14.2: Установка минимального размера окна для корректного отображения элементов
    case WM_GETMINMAXINFO:
        {
            MINMAXINFO* mmi = (MINMAXINFO*)lParam;
            mmi->ptMinTrackSize.x = 900;  // Минимальная ширина
            mmi->ptMinTrackSize.y = 400;  // Минимальная высота
        }
        break;
    
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Обработчик сообщений для окна "О программе".
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        return (INT_PTR)TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}

// v2.13.0: Обработчик сообщений для диалога подключения к прибору
// v2.15.0: Добавлена поддержка записи в файл
INT_PTR CALLBACK SerialStatusDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HWND hPortCombo = nullptr;
    static HWND hStatusLabel = nullptr;
    static HWND hFrameLabel = nullptr;
    static HWND hPacketsLabel = nullptr;
    static HWND hConnectBtn = nullptr;
    static HWND hStartBtn = nullptr;
    static HWND hStopBtn = nullptr;
    static HWND hRecordCheck = nullptr;
    static HWND hRecordStatus = nullptr;
    static HWND hRecordFrames = nullptr;
    
    switch (message)
    {
    case WM_INITDIALOG:
        {
            // Получаем handles элементов управления
            hPortCombo = GetDlgItem(hDlg, IDC_SERIAL_PORT_COMBO);
            hStatusLabel = GetDlgItem(hDlg, IDC_SERIAL_STATUS_LABEL);
            hFrameLabel = GetDlgItem(hDlg, IDC_SERIAL_FRAME_LABEL);
            hPacketsLabel = GetDlgItem(hDlg, IDC_SERIAL_PACKETS_LABEL);
            hConnectBtn = GetDlgItem(hDlg, IDC_SERIAL_CONNECT_BTN);
            hStartBtn = GetDlgItem(hDlg, IDC_SERIAL_START_BTN);
            hStopBtn = GetDlgItem(hDlg, IDC_SERIAL_STOP_BTN);
            hRecordCheck = GetDlgItem(hDlg, IDC_SERIAL_RECORD_CHECK);
            hRecordStatus = GetDlgItem(hDlg, IDC_SERIAL_RECORD_STATUS);
            hRecordFrames = GetDlgItem(hDlg, IDC_SERIAL_RECORD_FRAMES);
            
            // Заполняем список COM-портов
            if (hPortCombo) {
                SendMessage(hPortCombo, CB_RESETCONTENT, 0, 0);
                std::vector<std::wstring> ports = SerialModule::GetAvailablePorts();
                for (const auto& port : ports) {
                    SendMessage(hPortCombo, CB_ADDSTRING, 0, (LPARAM)port.c_str());
                }
                if (!ports.empty()) {
                    SendMessage(hPortCombo, CB_SETCURSEL, 0, 0);
                }
            }
            
            // v2.15.0: Инициализация чекбокса записи
            if (g_chartManager && hRecordCheck) {
                SerialModule* serial = g_chartManager->GetSerialModule();
                if (serial) {
                    SendMessage(hRecordCheck, BM_SETCHECK, 
                               serial->IsRecordingEnabled() ? BST_CHECKED : BST_UNCHECKED, 0);
                }
            }
            
            // Обновляем состояние кнопок
            if (g_chartManager) {
                bool connected = g_chartManager->IsSerialConnected();
                bool polling = g_chartManager->IsSerialPolling();
                
                SetWindowText(hStartBtn, connected ? L"Подключить" : L"Подключить");
                EnableWindow(hStartBtn, !connected);
                EnableWindow(hStopBtn, connected);
                
                if (connected) {
                    SetWindowText(hStatusLabel, polling ? L"Опрос..." : L"Подключено");
                } else {
                    SetWindowText(hStatusLabel, L"Отключено");
                }
            }
            
            return (INT_PTR)TRUE;
        }
        
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_SERIAL_CONNECT_BTN:
            // Кнопка "Обновить" - обновляем список портов
            if (hPortCombo) {
                SendMessage(hPortCombo, CB_RESETCONTENT, 0, 0);
                std::vector<std::wstring> ports = SerialModule::GetAvailablePorts();
                for (const auto& port : ports) {
                    SendMessage(hPortCombo, CB_ADDSTRING, 0, (LPARAM)port.c_str());
                }
                if (!ports.empty()) {
                    SendMessage(hPortCombo, CB_SETCURSEL, 0, 0);
                }
            }
            return (INT_PTR)TRUE;
            
        case IDC_SERIAL_START_BTN:
            // Кнопка "Подключить"
            if (g_chartManager && hPortCombo) {
                // Получаем выбранный порт
                int sel = (int)SendMessage(hPortCombo, CB_GETCURSEL, 0, 0);
                if (sel != CB_ERR) {
                    wchar_t portName[32];
                    SendMessage(hPortCombo, CB_GETLBTEXT, sel, (LPARAM)portName);
                    
                    // Подключаемся
                    SerialModule* serial = g_chartManager->GetSerialModule();
                    if (serial) {
                        SerialResult result = serial->Connect(portName);
                        
                        if (result == SerialResult::SUCCESS) {
                            SetWindowText(hStatusLabel, L"Подключено");
                            EnableWindow(hStartBtn, FALSE);
                            EnableWindow(hStopBtn, TRUE);
                            
                            // v2.15.0: Сначала запрашиваем метрологию (до начала записи!)
                            // Это нужно для правильного именования файла записи
                            METROLOGY_DATA metro;
                            // NOTE: при открытии COM-порта Arduino часто перезагружается из-за DTR/RTS,
                            // поэтому первый запрос метрологии может таймаутиться. Делаем несколько попыток.
                            SerialResult metroRes = SerialResult::TIMEOUT;
                            for (int attempt = 0; attempt < 4; ++attempt) {
                                metroRes = serial->RequestMetrology(metro);
                                if (metroRes == SerialResult::SUCCESS) {
                                    break;
                                }
                                // Первая пауза под Arduino reset, последующие — короткие ретраи
                                Sleep(attempt == 0 ? 1200 : 300);
                            }

                            if (metroRes == SerialResult::SUCCESS) {
                                wchar_t info[128];
                                swprintf_s(info, L"S/N: %u\nD: %u mm", metro.serial, metro.D_sonde_mm);
                                SetDlgItemText(hDlg, IDC_SERIAL_INFO_LABEL, info);
                            } else {
                                // Не блокируем работу: данные могут идти и без метрологии, но файл будет AMK_Unknown...
                                std::wstring msg = L"Метрология не получена: " + SerialModule::ResultToString(metroRes);
                                SetDlgItemText(hDlg, IDC_SERIAL_INFO_LABEL, msg.c_str());
                            }
                            
                            // v2.14.1: Очищаем данные перед началом опроса
                            // Режимы работы с файлом и COM-портом взаимоисключающие
                            HWND hMainWnd = GetParent(hDlg);
                            g_chartManager->HandleSerialClear(hMainWnd);
                            SetWindowText(hFrameLabel, L"0");
                            SetWindowText(hPacketsLabel, L"0 / 0");
                            
                            // Запускаем опрос (после получения метрологии)
                            serial->StartPolling(hMainWnd, 2000);
                            SetWindowText(hStatusLabel, L"Опрос...");
                        } else {
                            std::wstring errorMsg = L"Ошибка: " + SerialModule::ResultToString(result);
                            SetWindowText(hStatusLabel, errorMsg.c_str());
                        }
                    }
                } else {
                    MessageBox(hDlg, L"Выберите COM-порт", L"Ошибка", MB_OK | MB_ICONWARNING);
                }
            }
            return (INT_PTR)TRUE;
            
        case IDC_SERIAL_STOP_BTN:
            // Кнопка "Отключить"
            if (g_chartManager) {
                SerialModule* serial = g_chartManager->GetSerialModule();
                if (serial) {
                    serial->StopPolling();
                    serial->Disconnect();
                    
                    SetWindowText(hStatusLabel, L"Отключено");
                    EnableWindow(hStartBtn, TRUE);
                    EnableWindow(hStopBtn, FALSE);
                    SetDlgItemText(hDlg, IDC_SERIAL_INFO_LABEL, L"");
                }
            }
            return (INT_PTR)TRUE;
            
        case IDC_SERIAL_CLEAR_BTN:
            // Кнопка "Очистить данные"
            if (g_chartManager) {
                HWND hMainWnd = GetParent(hDlg);
                g_chartManager->HandleSerialClear(hMainWnd);
                SetWindowText(hFrameLabel, L"0");
                SetWindowText(hPacketsLabel, L"0 / 0");
            }
            return (INT_PTR)TRUE;
        
        // v2.15.0: Обработка элементов записи в файл
        case IDC_SERIAL_RECORD_CHECK:
            if (g_chartManager) {
                SerialModule* serial = g_chartManager->GetSerialModule();
                if (serial) {
                    bool checked = (SendMessage(hRecordCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);
                    serial->SetRecordingEnabled(checked);
                    
                    if (!checked) {
                        SetWindowText(hRecordStatus, L"Запись отключена");
                    }
                }
            }
            return (INT_PTR)TRUE;
            
        case IDC_SERIAL_BROWSE_BTN:
            // Кнопка выбора папки
            if (g_chartManager) {
                SerialModule* serial = g_chartManager->GetSerialModule();
                if (serial) {
                    std::wstring folder = SerialModule::BrowseForFolder(hDlg);
                    if (!folder.empty()) {
                        serial->SetRecordFolder(folder);
                    }
                }
            }
            return (INT_PTR)TRUE;
            
        case IDC_SERIAL_OPEN_FOLDER_BTN:
            // Кнопка открытия папки
            if (g_chartManager) {
                SerialModule* serial = g_chartManager->GetSerialModule();
                if (serial) {
                    serial->OpenRecordFolder();
                }
            }
            return (INT_PTR)TRUE;
            
        case IDCANCEL:
            // "Закрыть" (и Esc): скрываем окно вместо уничтожения,
            // чтобы его можно было быстро открыть снова из меню "Прибор"
            ShowWindow(hDlg, SW_HIDE);
            return (INT_PTR)TRUE;
        }
        break;
        
    case WM_CLOSE:
        // Крестик: скрываем вместо уничтожения (см. IDCANCEL выше)
        ShowWindow(hDlg, SW_HIDE);
        return (INT_PTR)TRUE;

    case WM_TIMER:
        // Обновляем статистику в диалоге
        if (g_chartManager) {
            SerialModule* serial = g_chartManager->GetSerialModule();
            if (serial && serial->IsConnected()) {
                wchar_t frameText[32], packetsText[64];
                swprintf_s(frameText, L"%u", serial->GetLastFrameNumber());
                swprintf_s(packetsText, L"%u / %u", serial->GetPacketsReceived(), serial->GetPacketsErrors());
                
                SetWindowText(hFrameLabel, frameText);
                SetWindowText(hPacketsLabel, packetsText);
                
                // v2.15.0: Обновляем статус записи
                if (hRecordStatus && hRecordFrames) {
                    if (serial->IsRecordingEnabled() && serial->IsRecordingActive()) {
                        // Показываем только имя файла (без пути)
                        std::wstring filePath = serial->GetRecordFilePath();
                        size_t lastSlash = filePath.find_last_of(L"\\/");
                        std::wstring fileName = (lastSlash != std::wstring::npos) 
                            ? filePath.substr(lastSlash + 1) : filePath;
                        SetWindowText(hRecordStatus, fileName.c_str());
                        
                        wchar_t framesText[32];
                        swprintf_s(framesText, L"%u кадров", serial->GetRecordedFrames());
                        SetWindowText(hRecordFrames, framesText);
                    } else if (!serial->IsRecordingEnabled()) {
                        SetWindowText(hRecordStatus, L"Запись отключена");
                        SetWindowText(hRecordFrames, L"0 кадров");
                    } else {
                        SetWindowText(hRecordStatus, L"Ожидание данных...");
                    }
                }
            }
        }
        return (INT_PTR)TRUE;
        
    case WM_DESTROY:
        g_hSerialDialog = nullptr;
        return (INT_PTR)TRUE;
    }
    
    return (INT_PTR)FALSE;
}
