#include "framework.h"
#include "UIModule.h"
#include <shellapi.h>

namespace {

constexpr int kStatusPartFolder = 3;
constexpr UINT_PTR kStatusFolderSubclassId = 2;

LRESULT CALLBACK StatusBarFolderSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    if (msg == WM_LBUTTONUP) {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);
        RECT rc;
        if (SendMessage(hwnd, SB_GETRECT, kStatusPartFolder, (LPARAM)&rc)) {
            POINT pt = { x, y };
            if (PtInRect(&rc, pt)) {
                HWND hMain = reinterpret_cast<HWND>(dwRefData);
                if (hMain && IsWindow(hMain)) {
                    PostMessage(hMain, WM_APP_OPEN_CURRENT_FILE_FOLDER, 0, 0);
                }
                return 0;
            }
        }
    }
    if (msg == WM_NCDESTROY) {
        RemoveWindowSubclass(hwnd, StatusBarFolderSubclassProc, uIdSubclass);
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}

bool IsUiTransmitterIndex(int transmitterIdx) {
    return transmitterIdx >= 0 && transmitterIdx < UIModule::UI_TRANSMITTER_COUNT;
}

void SetTransmitterEnabled(std::vector<bool>& enabledTransmitters, int transmitterIdx, bool enabled) {
    if (IsUiTransmitterIndex(transmitterIdx) && transmitterIdx < static_cast<int>(enabledTransmitters.size())) {
        enabledTransmitters[static_cast<size_t>(transmitterIdx)] = enabled;
    }
}

// ПКМ по кнопке «Включить» — открыть диалог «Подключение к прибору» (как в меню настроек).
LRESULT CALLBACK ConnectToggleSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
    UINT_PTR uIdSubclass, DWORD_PTR dwRefData) {
    // Owner-draw: ПКМ обрабатываем по WM_RBUTTONUP (иначе возможен дубль с WM_CONTEXTMENU).
    if (msg == WM_RBUTTONUP) {
        HWND hParent = reinterpret_cast<HWND>(dwRefData);
        if (hParent && IsWindow(hParent)) {
            PostMessage(hParent, WM_APP_OPEN_SERIAL_CONNECTION, 0, 0);
        }
        return 0;
    }
    if (msg == WM_NCDESTROY) {
        RemoveWindowSubclass(hwnd, ConnectToggleSubclassProc, uIdSubclass);
    }
    return DefSubclassProc(hwnd, msg, wParam, lParam);
}
}

UIModule::UIModule()
    : hDualModeCheck(nullptr)
    , hChart1SchematicBtn(nullptr)
    , hChart2SchematicBtn(nullptr)
    , hChart1HScrollBar(nullptr)
    , hChart2HScrollBar(nullptr)
    , hChart1VScrollBar(nullptr)
    , hChart2VScrollBar(nullptr)
    , hChart1HUndoBtn(nullptr)
    , hChart1VUndoBtn(nullptr)
    , hChart2HUndoBtn(nullptr)
    , hChart2VUndoBtn(nullptr)
    , hXRangeFromLabel(nullptr)
    , hXRangeFromEdit(nullptr)
    , hXRangeToLabel(nullptr)
    , hXRangeToEdit(nullptr)
    , hXRangeApplyBtn(nullptr)
    , hXRangeResetBtn(nullptr)
    , hXRange2FromLabel(nullptr)
    , hXRange2FromEdit(nullptr)
    , hXRange2ToLabel(nullptr)
    , hXRange2ToEdit(nullptr)
    , hXRange2ApplyBtn(nullptr)
    , hXRange2ResetBtn(nullptr)
    , hSaveRange1Btn(nullptr)
    , hCutRange1Btn(nullptr)
    , hResetZoom1Btn(nullptr)
    , hResetZoom2Btn(nullptr)
    , hCursor1Icon(nullptr)
    , hCursor1XEdit(nullptr)
    , hCursor1Separator(nullptr)
    , hCursor1Y1Edit(nullptr)
    , hCursor1Y2Separator(nullptr)
    , hCursor1Y2Edit(nullptr)
    , hCursor2Icon(nullptr)
    , hCursor2XEdit(nullptr)
    , hCursor2Separator(nullptr)
    , hCursor2Y1Edit(nullptr)
    , hCursor2Y2Separator(nullptr)
    , hCursor2Y2Edit(nullptr)
    , hStatusBar(nullptr)
    , hStatusBarFolderIcon(nullptr)
    , hChart2OpenFileIcon(nullptr)
    , hChart1SurfCheck(nullptr)
    , hChart1SurfEdit(nullptr)
    , hChart2SurfCheck(nullptr)
    , hChart2SurfEdit(nullptr)
    , hChart2SettingsBtn(nullptr)
    , hChart2OpenFileBtn(nullptr)
    , hChart1UndoZoomBtn(nullptr)
    , hChart2UndoZoomBtn(nullptr)
    , hChart1ShowNotesCheck(nullptr)
    , hChart2ShowNotesCheck(nullptr)
    , hChart1YZoomTargetCombo(nullptr)
    , hChart2YZoomTargetCombo(nullptr)
    , hChart1YZoomLabel(nullptr)
    , hChart2YZoomLabel(nullptr)
    , hPollingToggleBtn(nullptr)
    , pollingToggleActive(false)
    , pollingToggleEnabled(false)
    , hConnectToggleBtn(nullptr)
    , connectToggleActive(false)
    , connectToggleEnabled(false)
    , connectToggleReturnMode(false)
    , hTooltip(nullptr)
    , dualMode(false) {
    chart1EnabledTransmitters.resize(UI_TRANSMITTER_COUNT, true);
    chart2EnabledTransmitters.resize(UI_TRANSMITTER_COUNT, true);
}

UIModule::~UIModule() {
    if (hStatusBarFolderIcon) {
        DestroyIcon(hStatusBarFolderIcon);
        hStatusBarFolderIcon = nullptr;
    }
    if (hChart2OpenFileIcon) {
        DestroyIcon(hChart2OpenFileIcon);
        hChart2OpenFileIcon = nullptr;
    }
    if (hAppFont) {
        DeleteObject(hAppFont);
        hAppFont = nullptr;
    }
    if (hAppFontBold) {
        DeleteObject(hAppFontBold);
        hAppFontBold = nullptr;
    }
}

void UIModule::CreateControls(HWND hWnd, HINSTANCE hInst) {
    // === Единый шрифт приложения (Segoe UI, системный размер) ===
    {
        NONCLIENTMETRICS ncm = {};
        ncm.cbSize = sizeof(ncm);
        if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0)) {
            hAppFont = CreateFontIndirect(&ncm.lfMessageFont);
        }
        if (!hAppFont) {
            hAppFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        }
    }
    // === Жирный вариант единого шрифта (для акцентных контролов) ===
    {
        LOGFONT lfBold = {};
        if (hAppFont && GetObject(hAppFont, sizeof(lfBold), &lfBold)) {
            lfBold.lfWeight = FW_SEMIBOLD;
            hAppFontBold = CreateFontIndirect(&lfBold);
        }
        if (!hAppFontBold) {
            hAppFontBold = hAppFont;
        }
    }

    // Создание чекбокса Dual Mode (Двойной режим)
    hDualModeCheck = CreateWindow(L"BUTTON", L"\x0414\x0432\x043E\x0439\x043D\x043E\x0439 \x0440\x0435\x0436\x0438\x043C", 
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        10, 10, 130, 20, hWnd, (HMENU)IDC_DUAL_MODE_CHECK, hInst, NULL);
    
    // Создание кнопки Device Schematic для первого графика (Схема прибора)
    hChart1SchematicBtn = CreateWindow(L"BUTTON", L"\x0421\x0445\x0435\x043C\x0430 \x043F\x0440\x0438\x0431\x043E\x0440\x0430",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        145, 8, 120, 22, hWnd, (HMENU)IDC_CHART1_SCHEMATIC_BTN, hInst, NULL);
    
    // Создание кнопки Device Schematic для второго графика (изначально скрыта)
    hChart2SchematicBtn = CreateWindow(L"BUTTON", L"\x0421\x0445\x0435\x043C\x0430 \x043F\x0440\x0438\x0431\x043E\x0440\x0430",
        WS_CHILD | BS_PUSHBUTTON,
        10, 300, 120, 22, hWnd, (HMENU)IDC_CHART2_SCHEMATIC_BTN, hInst, NULL);

    // Создание горизонтальных скроллбаров (изначально скрыты)
    hChart1HScrollBar = CreateWindow(L"SCROLLBAR", NULL,
        WS_CHILD | SBS_HORZ,
        0, 0, 100, 20, hWnd, (HMENU)IDC_CHART1_HSCROLL, hInst, NULL);
    
    hChart2HScrollBar = CreateWindow(L"SCROLLBAR", NULL,
        WS_CHILD | SBS_HORZ,
        0, 0, 100, 20, hWnd, (HMENU)IDC_CHART2_HSCROLL, hInst, NULL);
    
    // Создание вертикальных скроллбаров (изначально скрыты)
    hChart1VScrollBar = CreateWindow(L"SCROLLBAR", NULL,
        WS_CHILD | SBS_VERT,
        0, 0, 20, 100, hWnd, (HMENU)IDC_CHART1_VSCROLL, hInst, NULL);
    
    hChart2VScrollBar = CreateWindow(L"SCROLLBAR", NULL,
        WS_CHILD | SBS_VERT,
        0, 0, 20, 100, hWnd, (HMENU)IDC_CHART2_VSCROLL, hInst, NULL);

    // Кнопки осевого undo ('-') у скроллбаров (изначально скрыты)
    hChart1HUndoBtn = CreateWindow(L"BUTTON", L"-",
        WS_CHILD | BS_PUSHBUTTON,
        0, 0, 16, 16, hWnd, (HMENU)IDC_CHART1_HUNDO_BTN, hInst, NULL);

    hChart1VUndoBtn = CreateWindow(L"BUTTON", L"-",
        WS_CHILD | BS_PUSHBUTTON,
        0, 0, 16, 16, hWnd, (HMENU)IDC_CHART1_VUNDO_BTN, hInst, NULL);

    hChart2HUndoBtn = CreateWindow(L"BUTTON", L"-",
        WS_CHILD | BS_PUSHBUTTON,
        0, 0, 16, 16, hWnd, (HMENU)IDC_CHART2_HUNDO_BTN, hInst, NULL);

    hChart2VUndoBtn = CreateWindow(L"BUTTON", L"-",
        WS_CHILD | BS_PUSHBUTTON,
        0, 0, 16, 16, hWnd, (HMENU)IDC_CHART2_VUNDO_BTN, hInst, NULL);

    // Создание элементов управления X Range в панели инструментов
    // Размещаем их справа от кнопки "Схема прибора"
    // Компактный формат: [From] - [To] [✓] [↺] [💾]
    int xRangeStartX = 275; // Начальная позиция X Range элементов (после кнопки "Схема прибора" + отступ)
    int yPos = 10;          // Вертикальная позиция (та же, что у других элементов)
    
    // Метка "X Range:" - скрыта (nullptr), оставляем для совместимости
    hXRangeFromLabel = nullptr;
    
    // Поле ввода "From" (компактное, 50px)
    hXRangeFromEdit = CreateWindow(L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
        xRangeStartX, yPos - 2, 50, 22, hWnd, (HMENU)IDC_XRANGE_FROM_EDIT, hInst, NULL);
    
    // Метка "-" (разделитель)
    hXRangeToLabel = CreateWindow(L"STATIC", L"-",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        xRangeStartX + 52, yPos, 12, 20, hWnd, (HMENU)IDC_XRANGE_TO_LABEL, hInst, NULL);
    
    // Поле ввода "To" (компактное, 50px)
    hXRangeToEdit = CreateWindow(L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
        xRangeStartX + 66, yPos - 2, 50, 22, hWnd, (HMENU)IDC_XRANGE_TO_EDIT, hInst, NULL);
    
    // Кнопка "Apply" - иконка ✓ (галочка) - Owner-Draw для цветной иконки
    hXRangeApplyBtn = CreateWindow(L"BUTTON", L"",
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        xRangeStartX + 120, yPos - 2, 26, 24, hWnd, (HMENU)IDC_XRANGE_APPLY_BTN, hInst, NULL);
    
    // Кнопка "Reset" - иконка ↺ (стрелка по кругу) - Owner-Draw для цветной иконки
    hXRangeResetBtn = CreateWindow(L"BUTTON", L"",
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        xRangeStartX + 148, yPos - 2, 26, 24, hWnd, (HMENU)IDC_XRANGE_RESET_BTN, hInst, NULL);
    
    // Кнопка "Save Range" для Chart 1 - Owner-Draw для цветной иконки
    hSaveRange1Btn = CreateWindow(L"BUTTON", L"",
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        xRangeStartX + 176, yPos - 2, 26, 24, hWnd, (HMENU)IDC_SAVE_RANGE1_BTN, hInst, NULL);

    hCutRange1Btn = CreateWindow(L"BUTTON", L"",
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        xRangeStartX + 206, yPos - 2, 26, 24, hWnd, (HMENU)IDC_CUT_RANGE1_BTN, hInst, NULL);
    
    // Кнопка "Reset Zoom" для Chart 1 - Owner-Draw для цветной иконки (увеличен размер)
    hResetZoom1Btn = CreateWindow(L"BUTTON", L"",
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        xRangeStartX + 244, yPos - 2, 65, 24, hWnd, (HMENU)IDC_RESET_ZOOM1_BTN, hInst, NULL);

    // === Элементы управления режимом Surf для Chart 1 (v2.14.0) ===
    // ПЕРЕМЕЩЕНО: Surf теперь ПЕРЕД Cursor (v2.15.1)
    int surfStartX = xRangeStartX + 321;
    
    // Checkbox «Поток» (режим Surf)
    hChart1SurfCheck = CreateWindow(L"BUTTON", L"\x041F\x043E\x0442\x043E\x043A",
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        surfStartX, yPos, 58, 20, hWnd, (HMENU)IDC_CHART1_SURF_CHECK, hInst, NULL);
    
    // Поле ввода количества точек (по умолчанию 100)
    hChart1SurfEdit = CreateWindow(L"EDIT", L"100",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER | ES_CENTER,
        surfStartX + 63, yPos - 2, 45, 22, hWnd, (HMENU)IDC_CHART1_SURF_EDIT, hInst, NULL);

    // === Кнопка-переключатель Старт/Стоп опроса (панель Chart 1) ===
    int pollingBtnX = surfStartX + 120;  // 12px зазор после поля окна «Поток»
    hPollingToggleBtn = CreateWindow(L"BUTTON", L"",
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW | WS_DISABLED,
        pollingBtnX, yPos - 2, 32, 24, hWnd, (HMENU)IDC_POLLING_TOGGLE_BTN, hInst, NULL);
    CreateTooltip(hWnd, hPollingToggleBtn, L"\u0421\u0442\u0430\u0440\u0442/\u0421\u0442\u043E\u043F \u043E\u043F\u0440\u043E\u0441\u0430 \u043F\u0440\u0438\u0431\u043E\u0440\u0430");

    // === Кнопка-переключатель Включить/Отключить прибор (панель Chart 1) ===
    int connectBtnX = pollingBtnX + 44;  // 12px зазор после кнопки опроса (32 + 12)
    hConnectToggleBtn = CreateWindow(L"BUTTON", L"",
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        connectBtnX, yPos - 2, 80, 24, hWnd, (HMENU)IDC_CONNECT_TOGGLE_BTN, hInst, NULL);
    SetWindowSubclass(hConnectToggleBtn, ConnectToggleSubclassProc, 1, reinterpret_cast<DWORD_PTR>(hWnd));
    CreateTooltip(hWnd, hConnectToggleBtn,
        L"\u0412\u043A\u043B\u044E\u0447\u0435\u043D\u0438\u0435/\u043E\u0442\u043A\u043B\u044E\u0447\u0435\u043D\u0438\u0435 \u043F\u0440\u0438\u0431\u043E\u0440\u0430. "
        L"\u041F\u041A\u041C \u2014 \u043D\u0430\u0441\u0442\u0440\u043E\u0439\u043A\u0438 \u043F\u043E\u0434\u043A\u043B\u044E\u0447\u0435\u043D\u0438\u044F.");

    // === Переключатель цели Y-zoom для Chart 1 (v2.18.0) ===
    // Размещаем рядом с кнопкой undo zoom на графике, изначально скрыт (показывается только при dual-axis)
    hChart1YZoomLabel = CreateWindow(L"STATIC", L"Zoom \x043E\x0441\x0438 Y:",
        WS_CHILD | SS_RIGHT,
        0, 0, 75, 18, hWnd, (HMENU)IDC_CHART1_YZOOM_LABEL, hInst, NULL);
    hChart1YZoomTargetCombo = CreateWindow(L"COMBOBOX", L"",
        WS_CHILD | CBS_DROPDOWNLIST | CBS_HASSTRINGS,
        0, 0, 120, 100, hWnd, (HMENU)IDC_CHART1_YZOOM_TARGET_COMBO, hInst, NULL);
    SendMessage(hChart1YZoomTargetCombo, CB_ADDSTRING, 0, (LPARAM)L"\x041F\x043E \x0434\x0432\x0443\x043C \x043E\x0441\x044F\x043C");
    SendMessage(hChart1YZoomTargetCombo, CB_ADDSTRING, 0, (LPARAM)L"\x041F\x043E \x043B\x0435\x0432\x043E\x0439 \x043E\x0441\x0438");
    SendMessage(hChart1YZoomTargetCombo, CB_ADDSTRING, 0, (LPARAM)L"\x041F\x043E \x043F\x0440\x0430\x0432\x043E\x0439 \x043E\x0441\x0438");
    SendMessage(hChart1YZoomTargetCombo, CB_SETCURSEL, 0, 0);

    hChart1UndoZoomBtn = CreateWindow(L"BUTTON", L"",
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW | WS_DISABLED,
        0, 0, 22, 20, hWnd, (HMENU)IDC_CHART1_UNDO_ZOOM_BTN, hInst, NULL);

    // Чекбокс отображения заметок (Chart 1), компактный и на уровне заголовка графика
    hChart1ShowNotesCheck = CreateWindow(L"BUTTON", L"\x041F\x043E\x043A\x0430\x0437\x0430\x0442\x044C \x0437\x0430\x043C\x0435\x0442\x043A\x0438",
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        0, 0, 128, 18, hWnd, (HMENU)IDC_CHART1_SHOW_NOTES_CHECK, hInst, NULL);
    SendMessage(hChart1ShowNotesCheck, BM_SETCHECK, BST_CHECKED, 0);

    // Элементы отображения координат курсора для Chart 1 (компактный дизайн v2.15.1)
    // ПЕРЕМЕЩЕНО: Cursor теперь КРАЙНИЙ СПРАВА (v2.15.1)
    // Формат: 📍 [X] : [Y1 unit] | [Y2 unit]
    int cursorStartX = surfStartX + 113;  // После «Поток» + поле N (YZoom combo на графике)
    
    // Иконка маркера (Owner-Draw для красного цвета)
    // v2.14.2: Добавлен SS_NOTIFY для обработки клика (сброс перекрестия)
    hCursor1Icon = CreateWindow(L"STATIC", L"",
        WS_VISIBLE | WS_CHILD | SS_OWNERDRAW | SS_NOTIFY,
        cursorStartX, yPos, 18, 20, hWnd, (HMENU)IDC_CURSOR1_ICON, hInst, NULL);
    
    // Поле X (компактное, 45px)
    hCursor1XEdit = CreateWindow(L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_READONLY | ES_CENTER,
        cursorStartX + 22, yPos - 2, 45, 22, hWnd, (HMENU)IDC_CURSOR1_X_EDIT, hInst, NULL);
    
    // Разделитель ":"
    hCursor1Separator = CreateWindow(L"STATIC", L":",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        cursorStartX + 69, yPos, 10, 20, hWnd, (HMENU)IDC_CURSOR1_SEPARATOR, hInst, NULL);
    
    // Поле Y1 с единицами (95px для значения + единицы)
    hCursor1Y1Edit = CreateWindow(L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_READONLY | ES_CENTER,
        cursorStartX + 81, yPos - 2, 95, 22, hWnd, (HMENU)IDC_CURSOR1_Y1_EDIT, hInst, NULL);
    
    // Разделитель "|" для второй оси (изначально скрыт)
    hCursor1Y2Separator = CreateWindow(L"STATIC", L"|",
        WS_CHILD | SS_CENTER,
        cursorStartX + 178, yPos, 10, 20, hWnd, (HMENU)IDC_CURSOR1_Y2_SEPARATOR, hInst, NULL);
    
    // Поле Y2 с единицами для второй оси (изначально скрыто, 95px)
    hCursor1Y2Edit = CreateWindow(L"EDIT", L"",
        WS_CHILD | WS_BORDER | ES_READONLY | ES_CENTER,
        cursorStartX + 190, yPos - 2, 95, 22, hWnd, (HMENU)IDC_CURSOR1_Y2_EDIT, hInst, NULL);

    // Создание элементов управления X Range для Chart 2 (изначально скрыты)
    // Размещаем их рядом с кнопкой "Схема прибора" для Chart 2
    // Компактный формат: [From] - [To] [✓] [↺]
    int xRange2StartX = 140; // Позиция после кнопки "Схема прибора" для Chart 2
    
    // Метка "X Range:" для Chart 2 - скрыта (nullptr), оставляем для совместимости
    hXRange2FromLabel = nullptr;
    
    // Поле ввода "From" для Chart 2 (компактное, 50px)
    hXRange2FromEdit = CreateWindow(L"EDIT", L"",
        WS_CHILD | WS_BORDER | ES_NUMBER,
        xRange2StartX, 298, 50, 22, hWnd, (HMENU)IDC_XRANGE2_FROM_EDIT, hInst, NULL);
    
    // Метка "-" для Chart 2 (разделитель)
    hXRange2ToLabel = CreateWindow(L"STATIC", L"-",
        WS_CHILD | SS_CENTER,
        xRange2StartX + 52, 300, 12, 20, hWnd, (HMENU)IDC_XRANGE2_TO_LABEL, hInst, NULL);
    
    // Поле ввода "To" для Chart 2 (компактное, 50px)
    hXRange2ToEdit = CreateWindow(L"EDIT", L"",
        WS_CHILD | WS_BORDER | ES_NUMBER,
        xRange2StartX + 66, 298, 50, 22, hWnd, (HMENU)IDC_XRANGE2_TO_EDIT, hInst, NULL);
    
    // Кнопка "Apply" для Chart 2 - Owner-Draw для цветной иконки
    hXRange2ApplyBtn = CreateWindow(L"BUTTON", L"",
        WS_CHILD | BS_OWNERDRAW,
        xRange2StartX + 120, 298, 26, 24, hWnd, (HMENU)IDC_XRANGE2_APPLY_BTN, hInst, NULL);
    
    // Кнопка "Reset" для Chart 2 - Owner-Draw для цветной иконки
    hXRange2ResetBtn = CreateWindow(L"BUTTON", L"",
        WS_CHILD | BS_OWNERDRAW,
        xRange2StartX + 148, 298, 26, 24, hWnd, (HMENU)IDC_XRANGE2_RESET_BTN, hInst, NULL);
    
    // Кнопка "Reset Zoom" для Chart 2 (изначально скрыта) - Owner-Draw (увеличен размер)
    hResetZoom2Btn = CreateWindow(L"BUTTON", L"",
        WS_CHILD | BS_OWNERDRAW,
        xRange2StartX + 186, 298, 65, 24, hWnd, (HMENU)IDC_RESET_ZOOM2_BTN, hInst, NULL);

    // === Элементы управления режимом Surf для Chart 2 (v2.14.0) ===
    // ПЕРЕМЕЩЕНО: Surf теперь ПЕРЕД Cursor (v2.15.1)
    int surf2StartX = xRange2StartX + 263;
    
    // Checkbox «Поток» для Chart 2 (изначально скрыт)
    hChart2SurfCheck = CreateWindow(L"BUTTON", L"\x041F\x043E\x0442\x043E\x043A",
        WS_CHILD | BS_AUTOCHECKBOX,
        surf2StartX, 300, 58, 20, hWnd, (HMENU)IDC_CHART2_SURF_CHECK, hInst, NULL);
    
    // Поле ввода количества точек для Chart 2 (по умолчанию 100)
    hChart2SurfEdit = CreateWindow(L"EDIT", L"100",
        WS_CHILD | WS_BORDER | ES_NUMBER | ES_CENTER,
        surf2StartX + 63, 298, 45, 22, hWnd, (HMENU)IDC_CHART2_SURF_EDIT, hInst, NULL);

    hChart2SettingsBtn = CreateWindow(L"BUTTON", L"",
        WS_CHILD | BS_OWNERDRAW,
        surf2StartX + 113, 298, 24, 22, hWnd, (HMENU)IDC_CHART2_SETTINGS_BTN, hInst, NULL);

    hChart2OpenFileBtn = CreateWindow(L"BUTTON", L"",
        WS_CHILD | BS_OWNERDRAW,
        surf2StartX + 140, 298, 24, 22, hWnd, (HMENU)IDC_CHART2_OPEN_FILE_BTN, hInst, NULL);

    // === Переключатель цели Y-zoom для Chart 2 (v2.18.0) ===
    // Размещаем рядом с кнопкой undo zoom на графике, изначально скрыт (показывается только при dual-axis)
    hChart2YZoomLabel = CreateWindow(L"STATIC", L"Zoom \x043E\x0441\x0438 Y:",
        WS_CHILD | SS_RIGHT,
        0, 0, 75, 18, hWnd, (HMENU)IDC_CHART2_YZOOM_LABEL, hInst, NULL);
    hChart2YZoomTargetCombo = CreateWindow(L"COMBOBOX", L"",
        WS_CHILD | CBS_DROPDOWNLIST | CBS_HASSTRINGS,
        0, 0, 120, 100, hWnd, (HMENU)IDC_CHART2_YZOOM_TARGET_COMBO, hInst, NULL);
    SendMessage(hChart2YZoomTargetCombo, CB_ADDSTRING, 0, (LPARAM)L"\x041F\x043E \x0434\x0432\x0443\x043C \x043E\x0441\x044F\x043C");
    SendMessage(hChart2YZoomTargetCombo, CB_ADDSTRING, 0, (LPARAM)L"\x041F\x043E \x043B\x0435\x0432\x043E\x0439 \x043E\x0441\x0438");
    SendMessage(hChart2YZoomTargetCombo, CB_ADDSTRING, 0, (LPARAM)L"\x041F\x043E \x043F\x0440\u0430\x0432\x043E\x0439 \x043E\x0441\x0438");
    SendMessage(hChart2YZoomTargetCombo, CB_SETCURSEL, 0, 0);

    hChart2UndoZoomBtn = CreateWindow(L"BUTTON", L"",
        WS_CHILD | BS_OWNERDRAW | WS_DISABLED,
        0, 0, 22, 20, hWnd, (HMENU)IDC_CHART2_UNDO_ZOOM_BTN, hInst, NULL);

    // Чекбокс отображения заметок (Chart 2), изначально скрыт
    hChart2ShowNotesCheck = CreateWindow(L"BUTTON", L"\x041F\x043E\x043A\x0430\x0437\x0430\x0442\x044C \x0437\x0430\x043C\x0435\x0442\x043A\x0438",
        WS_CHILD | BS_AUTOCHECKBOX,
        0, 0, 128, 18, hWnd, (HMENU)IDC_CHART2_SHOW_NOTES_CHECK, hInst, NULL);
    SendMessage(hChart2ShowNotesCheck, BM_SETCHECK, BST_CHECKED, 0);

    // Элементы отображения координат курсора для Chart 2 (компактный дизайн v2.15.1, изначально скрыты)
    // ПЕРЕМЕЩЕНО: Cursor теперь КРАЙНИЙ СПРАВА (v2.15.1)
    // Формат: 📍 [X] : [Y1 unit] | [Y2 unit]
    int cursor2StartX = surf2StartX + 169;  // После «Поток», поля N и кнопок режима Chart 2
    
    // Иконка маркера (Owner-Draw для красного цвета)
    // v2.14.2: Добавлен SS_NOTIFY для обработки клика (сброс перекрестия)
    hCursor2Icon = CreateWindow(L"STATIC", L"",
        WS_CHILD | SS_OWNERDRAW | SS_NOTIFY,
        cursor2StartX, 300, 18, 20, hWnd, (HMENU)IDC_CURSOR2_ICON, hInst, NULL);
    
    // Поле X (компактное, 45px)
    hCursor2XEdit = CreateWindow(L"EDIT", L"",
        WS_CHILD | WS_BORDER | ES_READONLY | ES_CENTER,
        cursor2StartX + 22, 298, 45, 22, hWnd, (HMENU)IDC_CURSOR2_X_EDIT, hInst, NULL);
    
    // Разделитель ":"
    hCursor2Separator = CreateWindow(L"STATIC", L":",
        WS_CHILD | SS_CENTER,
        cursor2StartX + 69, 300, 10, 20, hWnd, (HMENU)IDC_CURSOR2_SEPARATOR, hInst, NULL);
    
    // Поле Y1 с единицами (95px для значения + единицы)
    hCursor2Y1Edit = CreateWindow(L"EDIT", L"",
        WS_CHILD | WS_BORDER | ES_READONLY | ES_CENTER,
        cursor2StartX + 81, 298, 95, 22, hWnd, (HMENU)IDC_CURSOR2_Y1_EDIT, hInst, NULL);
    
    // Разделитель "|" для второй оси (изначально скрыт)
    hCursor2Y2Separator = CreateWindow(L"STATIC", L"|",
        WS_CHILD | SS_CENTER,
        cursor2StartX + 178, 300, 10, 20, hWnd, (HMENU)IDC_CURSOR2_Y2_SEPARATOR, hInst, NULL);
    
    // Поле Y2 с единицами для второй оси (изначально скрыто, 95px)
    hCursor2Y2Edit = CreateWindow(L"EDIT", L"",
        WS_CHILD | WS_BORDER | ES_READONLY | ES_CENTER,
        cursor2StartX + 190, 298, 95, 22, hWnd, (HMENU)IDC_CURSOR2_Y2_EDIT, hInst, NULL);

    // Создание статусной строки
    hStatusBar = CreateWindowEx(
        0,                              // Расширенный стиль
        STATUSCLASSNAME,                // Класс статусной строки
        NULL,                           // Текст
        WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP, // Стили
        0, 0, 0, 0,                     // Позиция и размер (автоматически)
        hWnd,                           // Родительское окно
        (HMENU)IDC_STATUSBAR,           // ID
        hInst,                          // Instance
        NULL);                          // Дополнительные параметры

    // Части статусной строки: координаты (0–2), иконка папки (3), путь/имя файла (4)
    int statusBarParts[5] = { 150, 350, 550, 574, -1 };
    SendMessage(hStatusBar, SB_SETPARTS, 5, (LPARAM)statusBarParts);
    
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"Готов");
    SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)L"");
    SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)L"");
    SendMessage(hStatusBar, SB_SETTEXT, kStatusPartFolder, (LPARAM)L"");
    SendMessage(hStatusBar, SB_SETTEXT, 4, (LPARAM)L"Файл не загружен");
    SetWindowSubclass(hStatusBar, StatusBarFolderSubclassProc, kStatusFolderSubclassId,
                      reinterpret_cast<DWORD_PTR>(hWnd));
    SetStatusBarFolderIconVisible(false);
    
    // Создание tooltips для кнопок с иконками (Chart 1)
    CreateTooltip(hWnd, hXRangeApplyBtn, L"Применить");
    CreateTooltip(hWnd, hXRangeResetBtn, L"Сбросить");
    CreateTooltip(hWnd, hSaveRange1Btn, L"Загрузить");
    CreateTooltip(hWnd, hCutRange1Btn, L"Вырезать отрезок");
    CreateTooltip(hWnd, hResetZoom1Btn, L"Сбросить Zoom");
    CreateTooltip(hWnd, hChart1UndoZoomBtn, L"Отменить последний zoom");
    
    // Создание tooltips для кнопок с иконками (Chart 2)
    CreateTooltip(hWnd, hXRange2ApplyBtn, L"Применить");
    CreateTooltip(hWnd, hXRange2ResetBtn, L"Сбросить");
    CreateTooltip(hWnd, hResetZoom2Btn, L"Сбросить Zoom");
    CreateTooltip(hWnd, hChart2UndoZoomBtn, L"Отменить последний zoom");
    CreateTooltip(hWnd, hChart2SettingsBtn, L"Режим данных графика 2");
    CreateTooltip(hWnd, hChart2OpenFileBtn, L"Открыть файл для графика 2");

    // Tooltips для осевого undo
    CreateTooltip(hWnd, hChart1HUndoBtn, L"Undo X zoom step");
    CreateTooltip(hWnd, hChart1VUndoBtn, L"Undo Y zoom step");
    CreateTooltip(hWnd, hChart2HUndoBtn, L"Undo X zoom step");
    CreateTooltip(hWnd, hChart2VUndoBtn, L"Undo Y zoom step");

    // Tooltip для иконки перекрестия (клик сбрасывает перекрестие на графике)
    CreateTooltip(hWnd, hCursor1Icon, L"Clear crosshair");
    CreateTooltip(hWnd, hCursor2Icon, L"Clear crosshair");

    // === Применяем единый шрифт ко ВСЕМ контролам главного окна ===
    {
        const HWND allControls[] = {
            hDualModeCheck,
            hChart1SchematicBtn, hChart2SchematicBtn,
            hXRangeFromEdit, hXRangeToLabel, hXRangeToEdit,
            hXRangeApplyBtn, hXRangeResetBtn,
            hSaveRange1Btn, hCutRange1Btn,
            hResetZoom1Btn, hResetZoom2Btn,
            hXRange2FromEdit, hXRange2ToLabel, hXRange2ToEdit,
            hXRange2ApplyBtn, hXRange2ResetBtn,
            hChart1SurfCheck, hChart1SurfEdit,
            hChart2SurfCheck, hChart2SurfEdit,
            hChart2SettingsBtn, hChart2OpenFileBtn,
            hChart1HUndoBtn, hChart1VUndoBtn,
            hChart2HUndoBtn, hChart2VUndoBtn,
            hChart1YZoomLabel, hChart1YZoomTargetCombo,
            hChart2YZoomLabel, hChart2YZoomTargetCombo,
            hChart1UndoZoomBtn, hChart2UndoZoomBtn,
            hChart1ShowNotesCheck, hChart2ShowNotesCheck,
            hCursor1Separator, hCursor1Y2Separator,
            hCursor2Separator, hCursor2Y2Separator,
            hPollingToggleBtn, hConnectToggleBtn,
        };
        for (HWND hCtrl : allControls) {
            if (hCtrl) SendMessage(hCtrl, WM_SETFONT, (WPARAM)hAppFont, TRUE);
        }
    }

    // === Жирный шрифт для акцентных контролов ===
    {
        const HWND boldControls[] = {
            hDualModeCheck,
            hChart1SchematicBtn, hChart2SchematicBtn,
            hChart1SurfCheck, hChart2SurfCheck,
            hCursor1XEdit, hCursor1Y1Edit, hCursor1Y2Edit,
            hCursor2XEdit, hCursor2Y1Edit, hCursor2Y2Edit,
        };
        for (HWND hCtrl : boldControls) {
            if (hCtrl) SendMessage(hCtrl, WM_SETFONT, (WPARAM)hAppFontBold, TRUE);
        }
    }

    // Инициализируем "пустое" состояние координат
    ClearClickCoordinates(1);
    ClearClickCoordinates(2);
}

void UIModule::UpdateControlsVisibility(bool showChart2Controls) {
    int showFlag = showChart2Controls ? SW_SHOW : SW_HIDE;
    ShowWindow(hChart2SchematicBtn, showFlag);
    
    // Показываем/скрываем элементы управления X Range для Chart 2
    // hXRange2FromLabel теперь nullptr (метка убрана)
    if (hXRange2FromLabel) ShowWindow(hXRange2FromLabel, showFlag);
    ShowWindow(hXRange2FromEdit, showFlag);
    ShowWindow(hXRange2ToLabel, showFlag);
    ShowWindow(hXRange2ToEdit, showFlag);
    ShowWindow(hXRange2ApplyBtn, showFlag);
    ShowWindow(hXRange2ResetBtn, showFlag);
    ShowWindow(hResetZoom2Btn, showFlag);

    // Кнопки undo осей для Chart 2 управляются отдельно ChartManager,
    // но в single-mode должны быть скрыты.
    if (!showChart2Controls) {
        ShowWindow(hChart2HUndoBtn, SW_HIDE);
        ShowWindow(hChart2VUndoBtn, SW_HIDE);
    }
    
    // Показываем/скрываем элементы координат курсора для Chart 2
    ShowWindow(hCursor2Icon, showFlag);
    ShowWindow(hCursor2XEdit, showFlag);
    ShowWindow(hCursor2Separator, showFlag);
    ShowWindow(hCursor2Y1Edit, showFlag);
    // Y2 элементы управляются отдельно через ShowDualAxisControls
    
    // Показываем/скрываем элементы Surf для Chart 2 (v2.14.0)
    ShowWindow(hChart2SurfCheck, showFlag);
    ShowWindow(hChart2SurfEdit, showFlag);
    ShowWindow(hChart2SettingsBtn, showFlag);
    if (!showChart2Controls) {
        ShowWindow(hChart2OpenFileBtn, SW_HIDE);
    }

    // Показываем/скрываем чекбокс отображения заметок для Chart 2
    ShowWindow(hChart2UndoZoomBtn, showFlag);
    ShowWindow(hChart2ShowNotesCheck, showFlag);

    // v2.18.0: YZoom target combo и label для Chart 2 управляются отдельно через ShowYZoomTargetCombo,
    // но в single-mode должны быть скрыты.
    if (!showChart2Controls) {
        ShowWindow(hChart2YZoomTargetCombo, SW_HIDE);
        if (hChart2YZoomLabel) ShowWindow(hChart2YZoomLabel, SW_HIDE);
    }
}

void UIModule::PositionChart2Controls(int panel2Y) {
    // v2.14.2: Высота кнопки 22 (выравнивание с Chart 1)
    SetWindowPos(hChart2SchematicBtn, HWND_TOP, 10, panel2Y, 140, 22, SWP_NOZORDER);
    
    // Позиционируем элементы управления X Range для Chart 2
    // Компактный формат: [From] - [To] [✓] [↺] [Reset Zoom]
    int xRange2StartX = 160;
    int yPos = panel2Y + 2; // Выравниваем с кнопкой Device Schematic
    
    // hXRange2FromLabel теперь nullptr (метка убрана)
    if (hXRange2FromLabel) SetWindowPos(hXRange2FromLabel, HWND_TOP, xRange2StartX, yPos, 60, 20, SWP_NOSIZE);
    SetWindowPos(hXRange2FromEdit, HWND_TOP, xRange2StartX, yPos - 2, 50, 22, SWP_NOSIZE);
    SetWindowPos(hXRange2ToLabel, HWND_TOP, xRange2StartX + 52, yPos, 12, 20, SWP_NOSIZE);
    SetWindowPos(hXRange2ToEdit, HWND_TOP, xRange2StartX + 66, yPos - 2, 50, 22, SWP_NOSIZE);
    SetWindowPos(hXRange2ApplyBtn, HWND_TOP, xRange2StartX + 120, yPos - 2, 26, 24, SWP_NOSIZE);
    SetWindowPos(hXRange2ResetBtn, HWND_TOP, xRange2StartX + 148, yPos - 2, 26, 24, SWP_NOSIZE);
    SetWindowPos(hResetZoom2Btn, HWND_TOP, xRange2StartX + 186, yPos - 2, 65, 24, SWP_NOSIZE);
    
    // Позиционируем элементы Surf для Chart 2 (v2.15.1 - Surf ПЕРЕД Cursor)
    int surf2StartX = xRange2StartX + 263;
    SetWindowPos(hChart2SurfCheck, HWND_TOP, surf2StartX, yPos, 58, 20, SWP_NOSIZE);
    SetWindowPos(hChart2SurfEdit, HWND_TOP, surf2StartX + 63, yPos - 2, 45, 22, SWP_NOSIZE);
    SetWindowPos(hChart2SettingsBtn, HWND_TOP, surf2StartX + 113, yPos - 2, 24, 22, SWP_NOSIZE);
    SetWindowPos(hChart2OpenFileBtn, HWND_TOP, surf2StartX + 140, yPos - 2, 24, 22, SWP_NOSIZE);
    
    // Позиционируем элементы координат курсора для Chart 2 (v2.15.1 - Cursor КРАЙНИЙ СПРАВА)
    int cursor2StartX = surf2StartX + 169;  // После «Поток», поля N и кнопок режима Chart 2
    SetWindowPos(hCursor2Icon, HWND_TOP, cursor2StartX, yPos, 20, 20, SWP_NOSIZE);
    SetWindowPos(hCursor2XEdit, HWND_TOP, cursor2StartX + 22, yPos - 2, 45, 22, SWP_NOSIZE);
    SetWindowPos(hCursor2Separator, HWND_TOP, cursor2StartX + 69, yPos, 10, 20, SWP_NOSIZE);
    SetWindowPos(hCursor2Y1Edit, HWND_TOP, cursor2StartX + 81, yPos - 2, 95, 22, SWP_NOSIZE);
    SetWindowPos(hCursor2Y2Separator, HWND_TOP, cursor2StartX + 178, yPos, 10, 20, SWP_NOSIZE);
    SetWindowPos(hCursor2Y2Edit, HWND_TOP, cursor2StartX + 190, yPos - 2, 95, 22, SWP_NOSIZE);
}

void UIModule::ToggleDualMode() {
    dualMode = !dualMode;
    SendMessage(hDualModeCheck, BM_SETCHECK, dualMode ? BST_CHECKED : BST_UNCHECKED, 0);
}



void UIModule::SetChart1TransmitterEnabled(int transmitterIdx, bool enabled) {
    SetTransmitterEnabled(chart1EnabledTransmitters, transmitterIdx, enabled);
}

void UIModule::PositionNotesVisibilityControls(const RECT& chart1Rect, const RECT& chart2Rect, bool dualMode) {
    const int BUTTON_WIDTH = 22;
    const int BUTTON_HEIGHT = 20;
    const int CHECK_WIDTH = 128;
    const int CHECK_HEIGHT = 18;
    const int COMBO_WIDTH = 120;
    const int COMBO_HEIGHT = 20;
    const int LABEL_WIDTH = 75;
    const int LABEL_HEIGHT = 18;
    const int BUTTON_GAP = 4;
    const int RIGHT_MARGIN = 10;
    const int TOP_OFFSET = 8;

    // === Chart 1 ===
    if (hChart1ShowNotesCheck) {
        int x1 = chart1Rect.right - RIGHT_MARGIN - CHECK_WIDTH;
        int y1 = chart1Rect.top + TOP_OFFSET;
        SetWindowPos(hChart1ShowNotesCheck, HWND_TOP, x1, y1, CHECK_WIDTH, CHECK_HEIGHT, SWP_NOZORDER);

        int undoX1 = x1 - BUTTON_GAP - BUTTON_WIDTH;
        if (hChart1UndoZoomBtn) {
            SetWindowPos(hChart1UndoZoomBtn, HWND_TOP, undoX1, y1 - 1, BUTTON_WIDTH, BUTTON_HEIGHT, SWP_NOZORDER);
        }

        // v2.18.x: Позиционируем ComboBox Y-zoom target и метку слева от кнопки undo
        if (hChart1YZoomTargetCombo) {
            int comboX1 = undoX1 - BUTTON_GAP - COMBO_WIDTH;
            SetWindowPos(hChart1YZoomTargetCombo, HWND_TOP, comboX1, y1 - 2, COMBO_WIDTH, COMBO_HEIGHT, SWP_NOZORDER);
            if (hChart1YZoomLabel) {
                SetWindowPos(hChart1YZoomLabel, HWND_TOP, comboX1 - LABEL_WIDTH - 2, y1, LABEL_WIDTH, LABEL_HEIGHT, SWP_NOZORDER);
            }
        }
    }

    // === Chart 2 ===
    if (hChart2ShowNotesCheck) {
        if (dualMode) {
            int x2 = chart2Rect.right - RIGHT_MARGIN - CHECK_WIDTH;
            int y2 = chart2Rect.top + TOP_OFFSET;
            SetWindowPos(hChart2ShowNotesCheck, HWND_TOP, x2, y2, CHECK_WIDTH, CHECK_HEIGHT, SWP_NOZORDER);

            int undoX2 = x2 - BUTTON_GAP - BUTTON_WIDTH;
            if (hChart2UndoZoomBtn) {
                SetWindowPos(hChart2UndoZoomBtn, HWND_TOP, undoX2, y2 - 1, BUTTON_WIDTH, BUTTON_HEIGHT, SWP_NOZORDER);
            }

            // v2.18.x: Позиционируем ComboBox Y-zoom target и метку слева от кнопки undo
            if (hChart2YZoomTargetCombo) {
                int comboX2 = undoX2 - BUTTON_GAP - COMBO_WIDTH;
                SetWindowPos(hChart2YZoomTargetCombo, HWND_TOP, comboX2, y2 - 2, COMBO_WIDTH, COMBO_HEIGHT, SWP_NOZORDER);
                if (hChart2YZoomLabel) {
                    SetWindowPos(hChart2YZoomLabel, HWND_TOP, comboX2 - LABEL_WIDTH - 2, y2, LABEL_WIDTH, LABEL_HEIGHT, SWP_NOZORDER);
                }
            }
        } else {
            ShowWindow(hChart2UndoZoomBtn, SW_HIDE);
            ShowWindow(hChart2ShowNotesCheck, SW_HIDE);
            if (hChart2YZoomTargetCombo) ShowWindow(hChart2YZoomTargetCombo, SW_HIDE);
            if (hChart2YZoomLabel) ShowWindow(hChart2YZoomLabel, SW_HIDE);
        }
    }
}

bool UIModule::IsNotesVisible(int chartNumber) const {
    HWND hCheck = nullptr;

    if (chartNumber == 1) {
        hCheck = hChart1ShowNotesCheck;
    } else if (chartNumber == 2) {
        hCheck = hChart2ShowNotesCheck;
    }

    if (!hCheck) return true;
    return SendMessage(hCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

void UIModule::SetNotesVisible(int chartNumber, bool visible) {
    HWND hCheck = nullptr;

    if (chartNumber == 1) {
        hCheck = hChart1ShowNotesCheck;
    } else if (chartNumber == 2) {
        hCheck = hChart2ShowNotesCheck;
    }

    if (hCheck) {
        SendMessage(hCheck, BM_SETCHECK, visible ? BST_CHECKED : BST_UNCHECKED, 0);
    }
}

void UIModule::SetUndoZoomStepEnabled(int chartNumber, bool enabled) {
    HWND hButton = nullptr;

    if (chartNumber == 1) {
        hButton = hChart1UndoZoomBtn;
    } else if (chartNumber == 2) {
        hButton = hChart2UndoZoomBtn;
    }

    if (hButton) {
        EnableWindow(hButton, enabled ? TRUE : FALSE);
    }
}

void UIModule::SetChart2TransmitterEnabled(int transmitterIdx, bool enabled) {
    SetTransmitterEnabled(chart2EnabledTransmitters, transmitterIdx, enabled);
}

bool UIModule::HandleCommand(WPARAM wParam, LPARAM lParam, HWND hWnd) {
    int wmId = LOWORD(wParam);
    int wmEvent = HIWORD(wParam);
    bool needsRedraw = false;

    switch (wmId) {
    case IDC_DUAL_MODE_CHECK:
        if (wmEvent == BN_CLICKED) {
            ToggleDualMode();
            needsRedraw = true;
        }
        break;
    
    case IDC_XRANGE_APPLY_BTN:
        if (wmEvent == BN_CLICKED) {
            // Обработка будет в ChartManager
            needsRedraw = true;
        }
        break;
    
    case IDC_XRANGE_RESET_BTN:
        if (wmEvent == BN_CLICKED) {
            // Обработка будет в ChartManager
            needsRedraw = true;
        }
        break;
    
    case IDC_XRANGE2_APPLY_BTN:
        if (wmEvent == BN_CLICKED) {
            // Обработка будет в ChartManager
            needsRedraw = true;
        }
        break;
    
    case IDC_XRANGE2_RESET_BTN:
        if (wmEvent == BN_CLICKED) {
            // Обработка будет в ChartManager
            needsRedraw = true;
        }
        break;

    case IDC_CHART1_SHOW_NOTES_CHECK:
    case IDC_CHART2_SHOW_NOTES_CHECK:
        if (wmEvent == BN_CLICKED) {
            // Скрываем focus cues, чтобы не показывать пунктирную рамку вокруг текста чекбокса.
            // Состояние чекбокса при этом сохраняется штатно (BS_AUTOCHECKBOX).
            HWND hControl = (HWND)lParam;
            if (hControl) {
                SendMessage(hControl, WM_UPDATEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS), 0);
            }
            needsRedraw = true;
        }
        break;

    }
    return needsRedraw;
}



// Методы для управления скроллбарами
void UIModule::ShowScrollBars(int chartNumber, bool show) {
    int showFlag = show ? SW_SHOW : SW_HIDE;
    
    if (chartNumber == 1) {
        ShowWindow(hChart1HScrollBar, showFlag);
        ShowWindow(hChart1VScrollBar, showFlag);
        if (!show) {
            ShowWindow(hChart1HUndoBtn, SW_HIDE);
            ShowWindow(hChart1VUndoBtn, SW_HIDE);
        }
    } else if (chartNumber == 2) {
        ShowWindow(hChart2HScrollBar, showFlag);
        ShowWindow(hChart2VScrollBar, showFlag);
        if (!show) {
            ShowWindow(hChart2HUndoBtn, SW_HIDE);
            ShowWindow(hChart2VUndoBtn, SW_HIDE);
        }
    }
}

void UIModule::UpdateScrollBarPositions(int chartNumber, const RECT& chartRect) {
    const int SCROLLBAR_SIZE = 15; // Уменьшенный размер скроллбара
    const int UNDO_BUTTON_SIZE = 16;
    const int MARGIN_LEFT = 50;    // Отступ слева (для Y-оси)
    const int MARGIN_RIGHT = 20;   // Отступ справа
    const int MARGIN_BOTTOM = 5;   // Отступ снизу от края графика
    const int MARGIN_TOP = 60;     // Отступ сверху (для заголовка)
    
    if (chartNumber == 1) {
        // Горизонтальный скроллбар располагается ВНУТРИ области графика, в нижней части
        SetWindowPos(hChart1HScrollBar, HWND_TOP, 
                     chartRect.left + MARGIN_LEFT, 
                     chartRect.bottom - SCROLLBAR_SIZE - MARGIN_BOTTOM,
                     chartRect.right - chartRect.left - MARGIN_LEFT - MARGIN_RIGHT - SCROLLBAR_SIZE, 
                     SCROLLBAR_SIZE,
                     SWP_NOZORDER);

        SetWindowPos(hChart1HUndoBtn, HWND_TOP,
                     chartRect.left + MARGIN_LEFT - UNDO_BUTTON_SIZE - 2,
                     chartRect.bottom - SCROLLBAR_SIZE - MARGIN_BOTTOM,
                     UNDO_BUTTON_SIZE,
                     UNDO_BUTTON_SIZE,
                     SWP_NOZORDER);
        
        // Вертикальный скроллбар располагается справа от графика
        SetWindowPos(hChart1VScrollBar, HWND_TOP,
                     chartRect.right - SCROLLBAR_SIZE - MARGIN_RIGHT,
                     chartRect.top + MARGIN_TOP,
                     SCROLLBAR_SIZE,
                     chartRect.bottom - chartRect.top - MARGIN_TOP - MARGIN_BOTTOM - SCROLLBAR_SIZE,
                     SWP_NOZORDER);

        SetWindowPos(hChart1VUndoBtn, HWND_TOP,
                     chartRect.right - SCROLLBAR_SIZE - MARGIN_RIGHT,
                     chartRect.top + MARGIN_TOP - UNDO_BUTTON_SIZE - 2,
                     UNDO_BUTTON_SIZE,
                     UNDO_BUTTON_SIZE,
                     SWP_NOZORDER);
    } else if (chartNumber == 2) {
        // Горизонтальный скроллбар для второго графика
        SetWindowPos(hChart2HScrollBar, HWND_TOP,
                     chartRect.left + MARGIN_LEFT, 
                     chartRect.bottom - SCROLLBAR_SIZE - MARGIN_BOTTOM,
                     chartRect.right - chartRect.left - MARGIN_LEFT - MARGIN_RIGHT - SCROLLBAR_SIZE, 
                     SCROLLBAR_SIZE,
                     SWP_NOZORDER);

        SetWindowPos(hChart2HUndoBtn, HWND_TOP,
                     chartRect.left + MARGIN_LEFT - UNDO_BUTTON_SIZE - 2,
                     chartRect.bottom - SCROLLBAR_SIZE - MARGIN_BOTTOM,
                     UNDO_BUTTON_SIZE,
                     UNDO_BUTTON_SIZE,
                     SWP_NOZORDER);
        
        // Вертикальный скроллбар для второго графика
        SetWindowPos(hChart2VScrollBar, HWND_TOP,
                     chartRect.right - SCROLLBAR_SIZE - MARGIN_RIGHT,
                     chartRect.top + MARGIN_TOP,
                     SCROLLBAR_SIZE,
                     chartRect.bottom - chartRect.top - MARGIN_TOP - MARGIN_BOTTOM - SCROLLBAR_SIZE,
                     SWP_NOZORDER);

        SetWindowPos(hChart2VUndoBtn, HWND_TOP,
                     chartRect.right - SCROLLBAR_SIZE - MARGIN_RIGHT,
                     chartRect.top + MARGIN_TOP - UNDO_BUTTON_SIZE - 2,
                     UNDO_BUTTON_SIZE,
                     UNDO_BUTTON_SIZE,
                     SWP_NOZORDER);
    }
}

void UIModule::ShowAxisUndoButtons(int chartNumber, bool showX, bool showY) {
    if (chartNumber == 1) {
        ShowWindow(hChart1HUndoBtn, showX ? SW_SHOW : SW_HIDE);
        ShowWindow(hChart1VUndoBtn, showY ? SW_SHOW : SW_HIDE);
    } else if (chartNumber == 2) {
        ShowWindow(hChart2HUndoBtn, showX ? SW_SHOW : SW_HIDE);
        ShowWindow(hChart2VUndoBtn, showY ? SW_SHOW : SW_HIDE);
    }
}

void UIModule::SetScrollBarRange(int chartNumber, int min, int max, int page, int pos) {
    HWND hScrollBar = nullptr;
    
    if (chartNumber == 1) {
        hScrollBar = hChart1HScrollBar;
    } else if (chartNumber == 2) {
        hScrollBar = hChart2HScrollBar;
    }
    
    if (hScrollBar) {
        SCROLLINFO si = {};
        si.cbSize = sizeof(SCROLLINFO);
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMin = min;
        si.nMax = max;
        si.nPage = page;
        si.nPos = pos;
        SetScrollInfo(hScrollBar, SB_CTL, &si, TRUE);
    }
}

HWND UIModule::GetHScrollBar(int chartNumber) const {
    if (chartNumber == 1) return hChart1HScrollBar;
    if (chartNumber == 2) return hChart2HScrollBar;
    return nullptr;
}

HWND UIModule::GetVScrollBar(int chartNumber) const {
    if (chartNumber == 1) return hChart1VScrollBar;
    if (chartNumber == 2) return hChart2VScrollBar;
    return nullptr;
}

void UIModule::SetVScrollBarRange(int chartNumber, int min, int max, int page, int pos) {
    HWND hScrollBar = nullptr;
    
    if (chartNumber == 1) {
        hScrollBar = hChart1VScrollBar;
    } else if (chartNumber == 2) {
        hScrollBar = hChart2VScrollBar;
    }
    
    if (hScrollBar) {
        SCROLLINFO si = {};
        si.cbSize = sizeof(SCROLLINFO);
        si.fMask = SIF_RANGE | SIF_PAGE | SIF_POS;
        si.nMin = min;
        si.nMax = max;
        si.nPage = page;
        si.nPos = pos;
        SetScrollInfo(hScrollBar, SB_CTL, &si, TRUE);
    }
}

// Методы для управления X Range
void UIModule::UpdateXRangeControls(int chartNumber, double minX, double maxX, bool isCustomRange) {
    HWND hFromEdit, hToEdit, hResetBtn;
    
    if (chartNumber == 1) {
        hFromEdit = hXRangeFromEdit;
        hToEdit = hXRangeToEdit;
        hResetBtn = hXRangeResetBtn;
    } else if (chartNumber == 2) {
        hFromEdit = hXRange2FromEdit;
        hToEdit = hXRange2ToEdit;
        hResetBtn = hXRange2ResetBtn;
    } else {
        return;
    }
    
    if (!hFromEdit || !hToEdit) return;
    
    // Обновляем поля ввода текущими значениями (целые числа)
    wchar_t fromText[32], toText[32];
    swprintf_s(fromText, L"%.0f", minX);
    swprintf_s(toText, L"%.0f", maxX);
    
    SetWindowText(hFromEdit, fromText);
    SetWindowText(hToEdit, toText);
    
    // Включаем/отключаем кнопку Reset в зависимости от состояния
    EnableWindow(hResetBtn, isCustomRange);
}

void UIModule::GetXRangeValues(int chartNumber, double& fromX, double& toX) const {
    HWND hFromEdit, hToEdit;
    
    if (chartNumber == 1) {
        hFromEdit = hXRangeFromEdit;
        hToEdit = hXRangeToEdit;
    } else if (chartNumber == 2) {
        hFromEdit = hXRange2FromEdit;
        hToEdit = hXRange2ToEdit;
    } else {
        fromX = toX = 0.0;
        return;
    }
    
    if (!hFromEdit || !hToEdit) {
        fromX = toX = 0.0;
        return;
    }
    
    wchar_t fromText[32], toText[32];
    GetWindowText(hFromEdit, fromText, 32);
    GetWindowText(hToEdit, toText, 32);
    
    fromX = _wtof(fromText);
    toX = _wtof(toText);
}

void UIModule::SetXRangeValues(int chartNumber, double fromX, double toX) {
    HWND hFromEdit, hToEdit;
    
    if (chartNumber == 1) {
        hFromEdit = hXRangeFromEdit;
        hToEdit = hXRangeToEdit;
    } else if (chartNumber == 2) {
        hFromEdit = hXRange2FromEdit;
        hToEdit = hXRange2ToEdit;
    } else {
        return;
    }
    
    if (!hFromEdit || !hToEdit) return;
    
    wchar_t fromText[32], toText[32];
    swprintf_s(fromText, L"%.0f", fromX);
    swprintf_s(toText, L"%.0f", toX);
    
    SetWindowText(hFromEdit, fromText);
    SetWindowText(hToEdit, toText);
}

void UIModule::SetXRangeFromValue(int chartNumber, double fromX) {
    HWND hFromEdit = nullptr;

    if (chartNumber == 1) {
        hFromEdit = hXRangeFromEdit;
    } else if (chartNumber == 2) {
        hFromEdit = hXRange2FromEdit;
    } else {
        return;
    }

    if (!hFromEdit) return;

    wchar_t fromText[32];
    swprintf_s(fromText, L"%.0f", fromX);
    SetWindowText(hFromEdit, fromText);
}

void UIModule::SetXRangeToValue(int chartNumber, double toX) {
    HWND hToEdit = nullptr;

    if (chartNumber == 1) {
        hToEdit = hXRangeToEdit;
    } else if (chartNumber == 2) {
        hToEdit = hXRange2ToEdit;
    } else {
        return;
    }

    if (!hToEdit) return;

    wchar_t toText[32];
    swprintf_s(toText, L"%.0f", toX);
    SetWindowText(hToEdit, toText);
}

void UIModule::SetXRangeResetEnabled(int chartNumber, bool enabled) {
    HWND hResetBtn = nullptr;

    if (chartNumber == 1) {
        hResetBtn = hXRangeResetBtn;
    } else if (chartNumber == 2) {
        hResetBtn = hXRange2ResetBtn;
    } else {
        return;
    }

    if (!hResetBtn) return;

    EnableWindow(hResetBtn, enabled ? TRUE : FALSE);
}

void UIModule::ResetXRangeControls(int chartNumber) {
    HWND hFromEdit, hToEdit, hResetBtn;
    
    if (chartNumber == 1) {
        hFromEdit = hXRangeFromEdit;
        hToEdit = hXRangeToEdit;
        hResetBtn = hXRangeResetBtn;
    } else if (chartNumber == 2) {
        hFromEdit = hXRange2FromEdit;
        hToEdit = hXRange2ToEdit;
        hResetBtn = hXRange2ResetBtn;
    } else {
        return;
    }
    
    if (!hFromEdit || !hToEdit) return;
    
    // Очищаем поля ввода
    SetWindowText(hFromEdit, L"");
    SetWindowText(hToEdit, L"");
    
    // Отключаем кнопку Reset
    EnableWindow(hResetBtn, FALSE);
}

bool UIModule::IsXRangeFromEmpty(int chartNumber) const {
    HWND hFromEdit = nullptr;
    
    if (chartNumber == 1) {
        hFromEdit = hXRangeFromEdit;
    } else if (chartNumber == 2) {
        hFromEdit = hXRange2FromEdit;
    } else {
        return true;
    }
    
    if (!hFromEdit) return true;
    
    wchar_t fromText[32];
    GetWindowText(hFromEdit, fromText, 32);
    
    for (int i = 0; fromText[i] != L'\0'; i++) {
        if (fromText[i] != L' ' && fromText[i] != L'\t') {
            return false;
        }
    }
    return true;
}

bool UIModule::IsXRangeToEmpty(int chartNumber) const {
    HWND hToEdit = nullptr;
    
    if (chartNumber == 1) {
        hToEdit = hXRangeToEdit;
    } else if (chartNumber == 2) {
        hToEdit = hXRange2ToEdit;
    } else {
        return true;
    }
    
    if (!hToEdit) return true;
    
    wchar_t toText[32];
    GetWindowText(hToEdit, toText, 32);
    
    // Проверяем, пустая ли строка или содержит только пробелы
    for (int i = 0; toText[i] != L'\0'; i++) {
        if (toText[i] != L' ' && toText[i] != L'\t') {
            return false;  // Найден непробельный символ
        }
    }
    return true;  // Строка пустая или содержит только пробелы
}


// Методы для управления статусной строкой
void UIModule::UpdateStatusBar(const std::wstring& text) {
    if (hStatusBar) {
        SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)text.c_str());
    }
}

void UIModule::UpdateStatusBarCoordinates(double x, double y1, const std::wstring& y1Unit,
                                         double y2, const std::wstring& y2Unit,
                                         bool hasDualAxis) {
    if (!hStatusBar) return;

    // Форматирование X координаты (целое число)
    wchar_t xText[64];
    swprintf_s(xText, L"X: %.0f", x);
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)xText);

    // Форматирование Y координат
    if (hasDualAxis && !y2Unit.empty()) {
        // Двойные оси: Y₁ (левая ось) и Y₂ (правая ось)
        wchar_t y1Text[128];
        wchar_t y2Text[128];
        
        // Левая ось (фазы)
        if (y1Unit == L"mdeg") {
            swprintf_s(y1Text, L"Y\u2081: %.0f %s", y1, y1Unit.c_str());
        } else {
            swprintf_s(y1Text, L"Y\u2081: %.3f %s", y1, y1Unit.c_str());
        }
        
        // Правая ось (амплитуды)
        if (y2Unit == L"mV") {
            swprintf_s(y2Text, L"Y\u2082: %.3f %s", y2, y2Unit.c_str());
        } else {
            swprintf_s(y2Text, L"Y\u2082: %.3f %s", y2, y2Unit.c_str());
        }
        
        SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)y1Text);
        SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)y2Text);
    } else {
        // Одна ось
        wchar_t yText[128];
        
        if (y1Unit == L"mdeg") {
            swprintf_s(yText, L"Y: %.0f %s", y1, y1Unit.c_str());
        } else if (y1Unit == L"mV") {
            swprintf_s(yText, L"Y: %.3f %s", y1, y1Unit.c_str());
        } else if (y1Unit == L"V/V") {
            swprintf_s(yText, L"Y: %.4f %s", y1, y1Unit.c_str());
        } else if (y1Unit == L"°C") {
            swprintf_s(yText, L"Y: %.2f %s", y1, y1Unit.c_str());
        } else {
            swprintf_s(yText, L"Y: %.3f %s", y1, y1Unit.c_str());
        }
        
        SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)yText);
        SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)L"");
    }
}

void UIModule::ClearStatusBarCoordinates() {
    if (!hStatusBar) return;
    
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"Готов");
    SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)L"");
    SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)L"");
}

// Обновление размера статусной строки при изменении размера окна
void UIModule::ResizeStatusBar() {
    if (hStatusBar) {
        // Отправляем сообщение WM_SIZE статусной строке для автоматического изменения размера
        SendMessage(hStatusBar, WM_SIZE, 0, 0);
    }
}

void UIModule::SetStatusBarFolderIconVisible(bool visible) {
    if (!hStatusBar) {
        return;
    }
    if (visible) {
        if (!hStatusBarFolderIcon) {
            SHSTOCKICONINFO sii = {};
            sii.cbSize = sizeof(sii);
            if (SUCCEEDED(SHGetStockIconInfo(SIID_FOLDER, SHGFI_ICON | SHGFI_SMALLICON, &sii))) {
                hStatusBarFolderIcon = sii.hIcon;
            }
        }
        SendMessage(hStatusBar, SB_SETICON, kStatusPartFolder, (LPARAM)hStatusBarFolderIcon);
        SendMessage(hStatusBar, SB_SETTEXT, kStatusPartFolder, (LPARAM)L"");
    } else {
        SendMessage(hStatusBar, SB_SETICON, kStatusPartFolder, (LPARAM)nullptr);
        SendMessage(hStatusBar, SB_SETTEXT, kStatusPartFolder, (LPARAM)L"");
    }
}

// Обновление координат курсора при клике на график (v2.15.1 - поддержка двойных осей)
void UIModule::UpdateClickCoordinates(int chartNumber, double x, double y1, const std::wstring& y1Unit,
                                      double y2, const std::wstring& y2Unit, bool hasDualAxis) {
    HWND hXEdit, hY1Edit, hY2Edit;
    HWND hIcon = nullptr;
    
    if (chartNumber == 1) {
        hXEdit = hCursor1XEdit;
        hY1Edit = hCursor1Y1Edit;
        hY2Edit = hCursor1Y2Edit;
        hIcon = hCursor1Icon;
    } else if (chartNumber == 2) {
        hXEdit = hCursor2XEdit;
        hY1Edit = hCursor2Y1Edit;
        hY2Edit = hCursor2Y2Edit;
        hIcon = hCursor2Icon;
    } else {
        return;
    }
    
    if (!hXEdit || !hY1Edit) return;
    
    // Форматируем X координату (целое число - индекс кадра)
    wchar_t xText[32];
    swprintf_s(xText, L"%.0f", x);
    SetWindowText(hXEdit, xText);
    
    // Форматируем Y1 координату с единицами измерения
    wchar_t y1Text[64];
    if (y1Unit == L"mdeg") {
        swprintf_s(y1Text, L"%.0f %s", y1, y1Unit.c_str());
    } else if (y1Unit == L"mV") {
        swprintf_s(y1Text, L"%.2f %s", y1, y1Unit.c_str());  // 2 знака для mV
    } else if (y1Unit == L"V/V") {
        swprintf_s(y1Text, L"%.4f %s", y1, y1Unit.c_str());
    } else if (y1Unit == L"°C" || y1Unit == L"degC") {
        swprintf_s(y1Text, L"%.2f °C", y1);
    } else {
        swprintf_s(y1Text, L"%.2f %s", y1, y1Unit.c_str());
    }
    SetWindowText(hY1Edit, y1Text);
    
    // Показываем/скрываем элементы для второй оси
    ShowDualAxisControls(chartNumber, hasDualAxis);
    
    // Форматируем Y2 координату если есть двойные оси
    if (hasDualAxis && hY2Edit && !y2Unit.empty()) {
        wchar_t y2Text[64];
        if (y2Unit == L"mdeg") {
            swprintf_s(y2Text, L"%.0f %s", y2, y2Unit.c_str());
        } else if (y2Unit == L"mV") {
            swprintf_s(y2Text, L"%.2f %s", y2, y2Unit.c_str());  // 2 знака для mV
        } else if (y2Unit == L"V/V") {
            swprintf_s(y2Text, L"%.4f %s", y2, y2Unit.c_str());
        } else {
            swprintf_s(y2Text, L"%.2f %s", y2, y2Unit.c_str());
        }
        SetWindowText(hY2Edit, y2Text);
    }

    // Перерисовываем иконку "перекрестие" (активное состояние)
    if (hIcon) {
        InvalidateRect(hIcon, NULL, TRUE);
    }
}

// Показать/скрыть элементы для второй оси Y
void UIModule::ShowDualAxisControls(int chartNumber, bool show) {
    int showFlag = show ? SW_SHOW : SW_HIDE;
    
    if (chartNumber == 1) {
        ShowWindow(hCursor1Y2Separator, showFlag);
        ShowWindow(hCursor1Y2Edit, showFlag);
    } else if (chartNumber == 2) {
        ShowWindow(hCursor2Y2Separator, showFlag);
        ShowWindow(hCursor2Y2Edit, showFlag);
    }
}

void UIModule::ClearClickCoordinates(int chartNumber) {
    HWND hXEdit, hY1Edit, hY2Edit;
    HWND hIcon = nullptr;
    
    if (chartNumber == 1) {
        hXEdit = hCursor1XEdit;
        hY1Edit = hCursor1Y1Edit;
        hY2Edit = hCursor1Y2Edit;
        hIcon = hCursor1Icon;
    } else if (chartNumber == 2) {
        hXEdit = hCursor2XEdit;
        hY1Edit = hCursor2Y1Edit;
        hY2Edit = hCursor2Y2Edit;
        hIcon = hCursor2Icon;
    } else {
        return;
    }
    
    // Показываем явное "пустое" состояние вместо пустых строк
    const wchar_t* kEmptyValue = L"\u2014"; // —
    if (hXEdit) SetWindowText(hXEdit, kEmptyValue);
    if (hY1Edit) SetWindowText(hY1Edit, kEmptyValue);
    if (hY2Edit) SetWindowText(hY2Edit, kEmptyValue);
    
    // Скрываем элементы второй оси
    ShowDualAxisControls(chartNumber, false);

    // Перерисовываем иконку "перекрестие" (неактивное состояние)
    if (hIcon) {
        InvalidateRect(hIcon, NULL, TRUE);
    }
}

// Позиционирование элементов координат к правому краю окна (v2.15.1)
void UIModule::PositionCursorControls(int windowWidth, int panel2Y) {
    // Константы размеров элементов
    const int MARGIN_RIGHT = 10;      // Отступ от правого края
    const int ICON_WIDTH = 18;        // Ширина иконки
    const int X_EDIT_WIDTH = 45;      // Ширина поля X
    const int SEPARATOR_WIDTH = 10;   // Ширина разделителя
    const int Y_EDIT_WIDTH = 95;      // Ширина поля Y
    const int ELEMENT_HEIGHT = 22;    // Высота элементов
    const int LABEL_HEIGHT = 20;      // Высота меток
    
    // Расчёт позиций от правого края для Chart 1
    // Полная ширина блока: иконка + X + : + Y1 + | + Y2 = 18 + 45 + 10 + 95 + 10 + 95 = 273px
    int yPos = 10;  // Вертикальная позиция для Chart 1
    
    // Y2 поле (крайнее правое)
    int y2EditX = windowWidth - MARGIN_RIGHT - Y_EDIT_WIDTH;
    SetWindowPos(hCursor1Y2Edit, HWND_TOP, y2EditX, yPos - 2, Y_EDIT_WIDTH, ELEMENT_HEIGHT, SWP_NOZORDER);
    
    // Разделитель | 
    int y2SepX = y2EditX - SEPARATOR_WIDTH;
    SetWindowPos(hCursor1Y2Separator, HWND_TOP, y2SepX, yPos, SEPARATOR_WIDTH, LABEL_HEIGHT, SWP_NOZORDER);
    
    // Y1 поле
    int y1EditX = y2SepX - Y_EDIT_WIDTH;
    SetWindowPos(hCursor1Y1Edit, HWND_TOP, y1EditX, yPos - 2, Y_EDIT_WIDTH, ELEMENT_HEIGHT, SWP_NOZORDER);
    
    // Разделитель :
    int sepX = y1EditX - SEPARATOR_WIDTH;
    SetWindowPos(hCursor1Separator, HWND_TOP, sepX, yPos, SEPARATOR_WIDTH, LABEL_HEIGHT, SWP_NOZORDER);
    
    // X поле
    int xEditX = sepX - X_EDIT_WIDTH;
    SetWindowPos(hCursor1XEdit, HWND_TOP, xEditX, yPos - 2, X_EDIT_WIDTH, ELEMENT_HEIGHT, SWP_NOZORDER);
    
    // Иконка
    int iconX = xEditX - ICON_WIDTH - 2;
    SetWindowPos(hCursor1Icon, HWND_TOP, iconX, yPos, ICON_WIDTH, LABEL_HEIGHT, SWP_NOZORDER);
    
    // Позиционирование для Chart 2 (если panel2Y > 0, значит Dual Mode активен)
    if (panel2Y > 0) {
        int yPos2 = panel2Y + 2;
        
        // Y2 поле
        SetWindowPos(hCursor2Y2Edit, HWND_TOP, y2EditX, yPos2 - 2, Y_EDIT_WIDTH, ELEMENT_HEIGHT, SWP_NOZORDER);
        
        // Разделитель |
        SetWindowPos(hCursor2Y2Separator, HWND_TOP, y2SepX, yPos2, SEPARATOR_WIDTH, LABEL_HEIGHT, SWP_NOZORDER);
        
        // Y1 поле
        SetWindowPos(hCursor2Y1Edit, HWND_TOP, y1EditX, yPos2 - 2, Y_EDIT_WIDTH, ELEMENT_HEIGHT, SWP_NOZORDER);
        
        // Разделитель :
        SetWindowPos(hCursor2Separator, HWND_TOP, sepX, yPos2, SEPARATOR_WIDTH, LABEL_HEIGHT, SWP_NOZORDER);
        
        // X поле
        SetWindowPos(hCursor2XEdit, HWND_TOP, xEditX, yPos2 - 2, X_EDIT_WIDTH, ELEMENT_HEIGHT, SWP_NOZORDER);
        
        // Иконка
        SetWindowPos(hCursor2Icon, HWND_TOP, iconX, yPos2, ICON_WIDTH, LABEL_HEIGHT, SWP_NOZORDER);
    }
}

// === Методы для работы с COM-портом (v2.13.0) ===
// Элементы управления COM-портом теперь в отдельном диалоге IDD_SERIAL_STATUS
// Эти методы оставлены пустыми для совместимости API

void UIModule::CreateSerialControls(HWND hWnd, HINSTANCE hInst) {
    // Элементы управления COM-портом теперь в меню "Прибор" и диалоге
    // Этот метод больше не создаёт элементы на панели инструментов
    hSerialPortCombo = nullptr;
    hSerialConnectBtn = nullptr;
    hSerialStatusLabel = nullptr;
    hSerialStartBtn = nullptr;
    hSerialStopBtn = nullptr;
    hSerialClearBtn = nullptr;
    hSerialFrameLabel = nullptr;
    hSerialPacketsLabel = nullptr;
}

void UIModule::UpdateSerialPortList(const std::vector<std::wstring>& ports) {
    // Реализация перенесена в диалог
}

std::wstring UIModule::GetSelectedPort() const {
    return L"";  // Реализация перенесена в диалог
}

void UIModule::SetSerialConnected(bool connected) {
    // Обновляем состояние toggle-кнопки опроса
    UpdatePollingToggleState(connected, pollingToggleActive);
    // Обновляем состояние toggle-кнопки подключения
    UpdateConnectToggleState(connected, true);
}

void UIModule::SetSerialPolling(bool polling) {
    // Обновляем состояние toggle-кнопки опроса
    pollingToggleActive = polling;
    if (hPollingToggleBtn) {
        InvalidateRect(hPollingToggleBtn, NULL, TRUE);
    }
}

void UIModule::UpdatePollingToggleState(bool connected, bool polling) {
    pollingToggleActive = polling;
    pollingToggleEnabled = connected;
    if (hPollingToggleBtn) {
        EnableWindow(hPollingToggleBtn, connected ? TRUE : FALSE);
        InvalidateRect(hPollingToggleBtn, NULL, TRUE);
    }
}

void UIModule::UpdateConnectToggleState(bool connected, bool hasAvailablePorts) {
    connectToggleActive = connected;
    connectToggleEnabled = hasAvailablePorts || connected; // Активна если есть порты или уже подключены
    if (hConnectToggleBtn) {
        EnableWindow(hConnectToggleBtn, (connectToggleEnabled || connectToggleReturnMode) ? TRUE : FALSE);
        InvalidateRect(hConnectToggleBtn, NULL, TRUE);
    }
    // При подключении также обновляем состояние кнопки опроса
    if (connected) {
        UpdatePollingToggleState(true, pollingToggleActive);
    } else {
        UpdatePollingToggleState(false, false);
    }
}

void UIModule::SetConnectToggleReturnMode(bool returnMode) {
    connectToggleReturnMode = returnMode;
    if (hConnectToggleBtn) {
        EnableWindow(hConnectToggleBtn, TRUE);
        InvalidateRect(hConnectToggleBtn, NULL, TRUE);
    }
}

void UIModule::UpdateSerialStatus(const std::wstring& status) {
    // Реализация перенесена в диалог
}

void UIModule::UpdateSerialFrameInfo(uint32_t frameNumber, uint32_t packetsReceived, uint32_t packetsErrors) {
    // Реализация перенесена в диалог
}

// === Методы для работы с режимом Surf (v2.14.0) ===

void UIModule::CreateSurfControls(HWND hWnd, HINSTANCE hInst) {
    // Контролы создаются в CreateControls(), этот метод для совместимости
}

void UIModule::UpdateSurfControlsVisibility(bool showChart2Controls) {
    int showFlag = showChart2Controls ? SW_SHOW : SW_HIDE;
    ShowWindow(hChart2SurfCheck, showFlag);
    ShowWindow(hChart2SurfEdit, showFlag);
    ShowWindow(hChart2SettingsBtn, showFlag);
    if (!showChart2Controls) {
        ShowWindow(hChart2OpenFileBtn, SW_HIDE);
    }
}

void UIModule::PositionSurfControls(int chart2PanelY) {
    // Позиционирование выполняется в PositionChart2Controls()
}

bool UIModule::IsSurfEnabled(int chartNumber) const {
    HWND hCheck = nullptr;
    
    if (chartNumber == 1) {
        hCheck = hChart1SurfCheck;
    } else if (chartNumber == 2) {
        hCheck = hChart2SurfCheck;
    }
    
    if (!hCheck) return false;
    
    return SendMessage(hCheck, BM_GETCHECK, 0, 0) == BST_CHECKED;
}

void UIModule::SetSurfEnabled(int chartNumber, bool enabled) {
    HWND hCheck = nullptr;
    
    if (chartNumber == 1) {
        hCheck = hChart1SurfCheck;
    } else if (chartNumber == 2) {
        hCheck = hChart2SurfCheck;
    }
    
    if (hCheck) {
        SendMessage(hCheck, BM_SETCHECK, enabled ? BST_CHECKED : BST_UNCHECKED, 0);
    }
}

int UIModule::GetSurfWindowSize(int chartNumber) const {
    HWND hEdit = nullptr;
    
    if (chartNumber == 1) {
        hEdit = hChart1SurfEdit;
    } else if (chartNumber == 2) {
        hEdit = hChart2SurfEdit;
    }
    
    if (!hEdit) return 100; // Значение по умолчанию
    
    wchar_t text[32];
    GetWindowText(hEdit, text, 32);
    
    int value = _wtoi(text);
    return (value > 0) ? value : 100; // Минимум 1, по умолчанию 100
}

void UIModule::SetSurfWindowSize(int chartNumber, int windowSize) {
    HWND hEdit = nullptr;
    
    if (chartNumber == 1) {
        hEdit = hChart1SurfEdit;
    } else if (chartNumber == 2) {
        hEdit = hChart2SurfEdit;
    }
    
    if (hEdit) {
        wchar_t text[32];
        swprintf_s(text, L"%d", windowSize);
        SetWindowText(hEdit, text);
    }
}

void UIModule::SetChart2IndependentModeUi(bool independent) {
    if (hChart2OpenFileBtn) {
        ShowWindow(hChart2OpenFileBtn, independent && dualMode ? SW_SHOW : SW_HIDE);
    }
    if (hChart2SettingsBtn) {
        InvalidateRect(hChart2SettingsBtn, NULL, TRUE);
    }
    if (hChart2OpenFileBtn) {
        // Перерисовываем область родителя, занимаемую кнопкой, чтобы убрать
        // остатки рамки после её скрытия. Саму скрытую кнопку InvalidateRect
        // не приведёт к обновлению фона под ней.
        HWND hParent = GetParent(hChart2OpenFileBtn);
        if (hParent) {
            RECT rc;
            GetWindowRect(hChart2OpenFileBtn, &rc);
            ScreenToClient(hParent, (POINT*)&rc.left);
            ScreenToClient(hParent, (POINT*)&rc.right);
            InvalidateRect(hParent, &rc, TRUE);
        }
    }
}

void UIModule::UpdateStatusBarSurfInfo(bool surf1Enabled, int surf1Window, bool surf2Enabled, int surf2Window) {
    if (!hStatusBar) return;
    
    // Формируем строку статуса Surf
    wchar_t surfText[128];
    
    if (surf1Enabled && surf2Enabled) {
        swprintf_s(surfText, L"Surf: C1 ON (%d) | C2 ON (%d)", surf1Window, surf2Window);
    } else if (surf1Enabled) {
        swprintf_s(surfText, L"Surf: C1 ON (%d) | C2 OFF", surf1Window);
    } else if (surf2Enabled) {
        swprintf_s(surfText, L"Surf: C1 OFF | C2 ON (%d)", surf2Window);
    } else {
        swprintf_s(surfText, L"Surf: OFF");
    }
    
    // Обновляем часть 2 статусной строки (индекс 2)
    SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)surfText);
}

// Создание tooltip для элемента управления
void UIModule::CreateTooltip(HWND hWnd, HWND hControl, const wchar_t* text) {
    if (!hControl) return;
    
    // Создаём tooltip окно если ещё не создано
    if (!hTooltip) {
        hTooltip = CreateWindowEx(
            WS_EX_TOPMOST,
            TOOLTIPS_CLASS,
            NULL,
            WS_POPUP | TTS_ALWAYSTIP | TTS_NOPREFIX,
            CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, CW_USEDEFAULT,
            hWnd,
            NULL,
            GetModuleHandle(NULL),
            NULL);
        
        if (!hTooltip) return;
        
        // Устанавливаем tooltip поверх всех окон
        SetWindowPos(hTooltip, HWND_TOPMOST, 0, 0, 0, 0, 
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

        // Явно активируем tooltip и разрешаем перенос длинного текста
        SendMessage(hTooltip, TTM_ACTIVATE, TRUE, 0);
        SendMessage(hTooltip, TTM_SETMAXTIPWIDTH, 0, 400);
    }
    
    // Добавляем инструмент (элемент управления) к tooltip
    TOOLINFOW ti = { 0 };
#ifdef TTTOOLINFOW_V2_SIZE
    ti.cbSize = TTTOOLINFOW_V2_SIZE;
#else
    ti.cbSize = sizeof(TOOLINFOW);
#endif
    ti.uFlags = TTF_SUBCLASS;
    ti.hwnd = hControl;
    ti.uId = 1;
    GetClientRect(hControl, &ti.rect);
    ti.lpszText = const_cast<LPWSTR>(text);

    SendMessageW(hTooltip, TTM_ADDTOOLW, 0, (LPARAM)&ti);
}

// Проверка, является ли элемент Owner-Draw иконкой
bool UIModule::IsIconButton(UINT ctrlId) const {
    switch (ctrlId) {
        case IDC_XRANGE_APPLY_BTN:
        case IDC_XRANGE_RESET_BTN:
        case IDC_SAVE_RANGE1_BTN:
        case IDC_CUT_RANGE1_BTN:
        case IDC_CHART1_UNDO_ZOOM_BTN:
        case IDC_CHART2_UNDO_ZOOM_BTN:
        case IDC_RESET_ZOOM1_BTN:
        case IDC_XRANGE2_APPLY_BTN:
        case IDC_XRANGE2_RESET_BTN:
        case IDC_RESET_ZOOM2_BTN:
        case IDC_CHART2_SETTINGS_BTN:
        case IDC_CHART2_OPEN_FILE_BTN:
        case IDC_CURSOR1_ICON:  // Иконка маркера для Chart 1
        case IDC_CURSOR2_ICON:  // Иконка маркера для Chart 2
        case IDC_POLLING_TOGGLE_BTN:  // Кнопка-переключатель Старт/Стоп опроса
        case IDC_CONNECT_TOGGLE_BTN:  // Кнопка-переключатель Подключить/Отключить
            return true;
        default:
            return false;
    }
}

// Отрисовка фоновых зон группировки элементов панели инструментов
void UIModule::DrawToolbarGroupBackgrounds(HDC hdc) {
    if (!hdc) return;

    // Вспомогательная лямбда: получить RECT контрола в клиентских координатах родителя
    auto getControlRect = [](HWND hCtrl) -> RECT {
        RECT rc = {};
        if (!hCtrl || !IsWindowVisible(hCtrl)) return rc;
        GetWindowRect(hCtrl, &rc);
        MapWindowPoints(HWND_DESKTOP, GetParent(hCtrl), reinterpret_cast<POINT*>(&rc), 2);
        return rc;
    };

    // Вспомогательная лямбда: объединить RECT (расширить bounds, если контрол видим)
    auto unionRect = [](RECT& bounds, const RECT& rc, bool& first) {
        if (rc.left == 0 && rc.right == 0 && rc.top == 0 && rc.bottom == 0) return;
        if (first) {
            bounds = rc;
            first = false;
        } else {
            if (rc.left < bounds.left)     bounds.left = rc.left;
            if (rc.top < bounds.top)       bounds.top = rc.top;
            if (rc.right > bounds.right)   bounds.right = rc.right;
            if (rc.bottom > bounds.bottom) bounds.bottom = rc.bottom;
        }
    };

    // Параметры отрисовки зон
    const COLORREF zoneBg     = RGB(240, 243, 247);
    const COLORREF zoneBorder = RGB(210, 214, 219);
    const int padding  = 4;   // Отступ вокруг группы
    const int radius   = 6;   // Скругление углов

    // Лямбда для отрисовки одной зоны
    auto drawZone = [&](const RECT& bounds) {
        if (bounds.left >= bounds.right || bounds.top >= bounds.bottom) return;

        RECT zr = bounds;
        InflateRect(&zr, padding, padding);

        HBRUSH hFill = CreateSolidBrush(zoneBg);
        HPEN hPen = CreatePen(PS_SOLID, 1, zoneBorder);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hFill);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

        RoundRect(hdc, zr.left, zr.top, zr.right, zr.bottom, radius, radius);

        SelectObject(hdc, hOldBrush);
        SelectObject(hdc, hOldPen);
        DeleteObject(hFill);
        DeleteObject(hPen);
    };

    // === Группа 1 (Chart 1): Отрезок — [From] - [To] [✓] [↺] [↓] [✂] ===
    {
        RECT bounds = {};
        bool first = true;
        unionRect(bounds, getControlRect(hXRangeFromEdit), first);
        unionRect(bounds, getControlRect(hXRangeToLabel), first);
        unionRect(bounds, getControlRect(hXRangeToEdit), first);
        unionRect(bounds, getControlRect(hXRangeApplyBtn), first);
        unionRect(bounds, getControlRect(hXRangeResetBtn), first);
        unionRect(bounds, getControlRect(hSaveRange1Btn), first);
        unionRect(bounds, getControlRect(hCutRange1Btn), first);
        if (!first) drawZone(bounds);
    }

    // === Группа 2 (Chart 1): Навигация — [↺ Zoom] ===
    {
        RECT bounds = {};
        bool first = true;
        unionRect(bounds, getControlRect(hResetZoom1Btn), first);
        if (!first) drawZone(bounds);
    }

    // === Группа 3 (Chart 1): Серф — [Surf] [pts] ===
    {
        RECT bounds = {};
        bool first = true;
        unionRect(bounds, getControlRect(hChart1SurfCheck), first);
        unionRect(bounds, getControlRect(hChart1SurfEdit), first);
        if (!first) drawZone(bounds);
    }

    // === Группа 4 (Chart 1): Подключение — [Подключить / Отключить] ===
    {
        RECT bounds = {};
        bool first = true;
        unionRect(bounds, getControlRect(hConnectToggleBtn), first);
        if (!first) drawZone(bounds);
    }

    // === Группа 5 (Chart 1): Опрос — [▶ Старт / ■ Стоп] ===
    {
        RECT bounds = {};
        bool first = true;
        unionRect(bounds, getControlRect(hPollingToggleBtn), first);
        if (!first) drawZone(bounds);
    }

    // === Группа 5 (Chart 1): Курсор — [📍] [X] : [Y1] | [Y2] ===
    {
        RECT bounds = {};
        bool first = true;
        unionRect(bounds, getControlRect(hCursor1Icon), first);
        unionRect(bounds, getControlRect(hCursor1XEdit), first);
        unionRect(bounds, getControlRect(hCursor1Separator), first);
        unionRect(bounds, getControlRect(hCursor1Y1Edit), first);
        unionRect(bounds, getControlRect(hCursor1Y2Separator), first);
        unionRect(bounds, getControlRect(hCursor1Y2Edit), first);
        if (!first) drawZone(bounds);
    }

    // === Chart 2 (только в dual mode) ===
    if (dualMode) {
        // Группа 1 (Chart 2): Отрезок — [From] - [To] [✓] [↺]
        {
            RECT bounds = {};
            bool first = true;
            unionRect(bounds, getControlRect(hXRange2FromEdit), first);
            unionRect(bounds, getControlRect(hXRange2ToLabel), first);
            unionRect(bounds, getControlRect(hXRange2ToEdit), first);
            unionRect(bounds, getControlRect(hXRange2ApplyBtn), first);
            unionRect(bounds, getControlRect(hXRange2ResetBtn), first);
            if (!first) drawZone(bounds);
        }

        // Группа 2 (Chart 2): Навигация — [↺ Zoom]
        {
            RECT bounds = {};
            bool first = true;
            unionRect(bounds, getControlRect(hResetZoom2Btn), first);
            if (!first) drawZone(bounds);
        }

        // Группа 3 (Chart 2): Серф — [Surf] [pts]
        {
            RECT bounds = {};
            bool first = true;
            unionRect(bounds, getControlRect(hChart2SurfCheck), first);
            unionRect(bounds, getControlRect(hChart2SurfEdit), first);
            unionRect(bounds, getControlRect(hChart2SettingsBtn), first);
            unionRect(bounds, getControlRect(hChart2OpenFileBtn), first);
            if (!first) drawZone(bounds);
        }

        // Группа 4 (Chart 2): Курсор — [📍] [X] : [Y1] | [Y2]
        {
            RECT bounds = {};
            bool first = true;
            unionRect(bounds, getControlRect(hCursor2Icon), first);
            unionRect(bounds, getControlRect(hCursor2XEdit), first);
            unionRect(bounds, getControlRect(hCursor2Separator), first);
            unionRect(bounds, getControlRect(hCursor2Y1Edit), first);
            unionRect(bounds, getControlRect(hCursor2Y2Separator), first);
            unionRect(bounds, getControlRect(hCursor2Y2Edit), first);
            if (!first) drawZone(bounds);
        }
    }
}

// Отрисовка Owner-Draw кнопки с крупной иконкой
void UIModule::DrawIconButton(LPDRAWITEMSTRUCT lpDrawItem) {
    if (!lpDrawItem) return;
    
    HDC hdc = lpDrawItem->hDC;
    RECT rc = lpDrawItem->rcItem;
    UINT ctrlId = lpDrawItem->CtlID;
    UINT state = lpDrawItem->itemState;
    
    // Определяем текст иконки
    const wchar_t* iconText = L"";
    const wchar_t* labelText = L"";
    
    // Специальная обработка для иконки маркера курсора
    if (ctrlId == IDC_CURSOR1_ICON || ctrlId == IDC_CURSOR2_ICON) {
        // Иконка перекрестия: нейтральный "офисный" вид, активная/неактивная индикация.
        const COLORREF bg = GetSysColor(COLOR_WINDOW);
        const COLORREF border = RGB(210, 214, 219);
        const COLORREF active = RGB(200, 50, 50);
        const COLORREF inactive = RGB(150, 150, 150);

        auto isActive = [&]() -> bool {
            HWND hX = (ctrlId == IDC_CURSOR1_ICON) ? hCursor1XEdit : hCursor2XEdit;
            if (!hX) return false;
            wchar_t buf[32] = {};
            GetWindowText(hX, buf, 32);
            // Активно, если поле X не пустое и не "—"
            return (buf[0] != L'\0' && wcscmp(buf, L"\u2014") != 0);
        };

        const COLORREF stroke = isActive() ? active : inactive;

        // Фон
        HBRUSH hBgBrush = CreateSolidBrush(bg);
        FillRect(hdc, &rc, hBgBrush);
        DeleteObject(hBgBrush);

        // Небольшая "кнопка" под иконкой (скругленный прямоугольник)
        RECT r = rc;
        InflateRect(&r, -1, -1);

        HBRUSH hFill = CreateSolidBrush(RGB(255, 255, 255));
        HPEN hBorderPen = CreatePen(PS_SOLID, 1, border);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hFill);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hBorderPen);
        RoundRect(hdc, r.left, r.top, r.right, r.bottom, 6, 6);
        SelectObject(hdc, hOldBrush);
        SelectObject(hdc, hOldPen);
        DeleteObject(hFill);
        DeleteObject(hBorderPen);

        // Перекрестие внутри
        const int w = (r.right - r.left);
        const int h = (r.bottom - r.top);
        const int cx = (r.left + r.right) / 2;
        const int cy = (r.top + r.bottom) / 2;
        int radius = (w < h ? w : h) / 2 - 4;
        if (radius < 2) radius = 2;

        HPEN hPen = CreatePen(PS_SOLID, 1, stroke);
        HPEN hOld = (HPEN)SelectObject(hdc, hPen);
        HBRUSH hNoBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

        // Кольцо
        Ellipse(hdc, cx - radius, cy - radius, cx + radius + 1, cy + radius + 1);
        // Линии перекрестия
        MoveToEx(hdc, cx - radius, cy, NULL);
        LineTo(hdc, cx + radius + 1, cy);
        MoveToEx(hdc, cx, cy - radius, NULL);
        LineTo(hdc, cx, cy + radius + 1);

        // Центр-точка (маленькая)
        HBRUSH hDotBrush = CreateSolidBrush(stroke);
        HBRUSH hOldDotBrush = (HBRUSH)SelectObject(hdc, hDotBrush);
        HPEN hOldDotPen = (HPEN)SelectObject(hdc, GetStockObject(NULL_PEN));
        const int dotR = 2;
        Ellipse(hdc, cx - dotR, cy - dotR, cx + dotR + 1, cy + dotR + 1);

        // Cleanup
        SelectObject(hdc, hOldDotPen);
        SelectObject(hdc, hOldDotBrush);
        DeleteObject(hDotBrush);

        SelectObject(hdc, hNoBrush);
        SelectObject(hdc, hOld);
        DeleteObject(hPen);

        return;
    }
    
    // === Специальная отрисовка toggle-кнопки Подключить/Отключить/→К прибору ===
    if (ctrlId == IDC_CONNECT_TOGGLE_BTN) {
        const bool isActive = connectToggleActive;
        const bool isReturn = connectToggleReturnMode;
        const wchar_t* btnText = isReturn ? L"\u2192\u041A \u043F\u0440\u0438\u0431\u043E\u0440\u0443"
                               : isActive ? L"\u041E\u0442\u043A\u043B\u044E\u0447\u0438\u0442\u044C"
                               : L"\u0412\u043A\u043B\u044E\u0447\u0438\u0442\u044C";
        
        COLORREF bgColor, textColor, borderColor;
        if (state & ODS_DISABLED) {
            bgColor = RGB(240, 240, 240);
            textColor = RGB(160, 160, 160);
            borderColor = RGB(200, 200, 200);
        } else if (isReturn) {
            // Режим "→К прибору" — синий акцент
            bgColor = (state & ODS_SELECTED) ? RGB(30, 100, 180) : RGB(40, 120, 210);
            textColor = RGB(255, 255, 255);
            borderColor = (state & ODS_SELECTED) ? RGB(20, 80, 150) : RGB(30, 100, 180);
        } else if (state & ODS_SELECTED) {
            bgColor = isActive ? RGB(180, 50, 50) : RGB(40, 130, 40);
            textColor = RGB(255, 255, 255);
            borderColor = isActive ? RGB(140, 30, 30) : RGB(20, 100, 20);
        } else {
            bgColor = isActive ? RGB(210, 60, 60) : RGB(50, 160, 50);
            textColor = RGB(255, 255, 255);
            borderColor = isActive ? RGB(180, 40, 40) : RGB(30, 130, 30);
        }
        
        // Фон со скруглением
        HBRUSH hFill = CreateSolidBrush(bgColor);
        HPEN hPen = CreatePen(PS_SOLID, 1, borderColor);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hFill);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 6, 6);
        SelectObject(hdc, hOldBrush);
        SelectObject(hdc, hOldPen);
        DeleteObject(hFill);
        DeleteObject(hPen);
        
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, textColor);
        
        int offsetX = (state & ODS_SELECTED) ? 1 : 0;
        int offsetY = (state & ODS_SELECTED) ? 1 : 0;
        
        // Текст ("Подключить" или "Отключить") — на базе единого шрифта, полужирный
        LOGFONT lfBtn = {};
        if (hAppFont && GetObject(hAppFont, sizeof(lfBtn), &lfBtn)) {
            lfBtn.lfWeight = FW_SEMIBOLD;
        } else {
            wcscpy_s(lfBtn.lfFaceName, L"Segoe UI");
            lfBtn.lfHeight = 13;
            lfBtn.lfWeight = FW_SEMIBOLD;
            lfBtn.lfQuality = CLEARTYPE_QUALITY;
        }
        HFONT hTextFont = CreateFontIndirect(&lfBtn);
        HFONT hOldFont = (HFONT)SelectObject(hdc, hTextFont);
        
        RECT textRect = rc;
        textRect.left += offsetX;
        textRect.top += offsetY;
        DrawText(hdc, btnText, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        
        SelectObject(hdc, hOldFont);
        DeleteObject(hTextFont);
        
        if (state & ODS_FOCUS) {
            RECT focusRect = rc;
            InflateRect(&focusRect, -3, -3);
            DrawFocusRect(hdc, &focusRect);
        }
        return;
    }
    
    // === Специальная отрисовка toggle-кнопки Старт/Стоп опроса ===
    if (ctrlId == IDC_POLLING_TOGGLE_BTN) {
        const bool isActive = pollingToggleActive;
        const wchar_t* btnIcon = isActive ? L"\u25A0" : L"\u25B6";  // ■ или ▶
        
        COLORREF bgColor, textColor, borderColor;
        if (state & ODS_DISABLED) {
            bgColor = RGB(240, 240, 240);
            textColor = RGB(160, 160, 160);
            borderColor = RGB(200, 200, 200);
        } else if (state & ODS_SELECTED) {
            bgColor = isActive ? RGB(180, 50, 50) : RGB(40, 130, 40);
            textColor = RGB(255, 255, 255);
            borderColor = isActive ? RGB(140, 30, 30) : RGB(20, 100, 20);
        } else {
            bgColor = isActive ? RGB(210, 60, 60) : RGB(50, 160, 50);
            textColor = RGB(255, 255, 255);
            borderColor = isActive ? RGB(180, 40, 40) : RGB(30, 130, 30);
        }
        
        // Фон со скруглением
        HBRUSH hFill = CreateSolidBrush(bgColor);
        HPEN hPen = CreatePen(PS_SOLID, 1, borderColor);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hFill);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, 6, 6);
        SelectObject(hdc, hOldBrush);
        SelectObject(hdc, hOldPen);
        DeleteObject(hFill);
        DeleteObject(hPen);
        
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, textColor);
        
        int offsetX = (state & ODS_SELECTED) ? 1 : 0;
        int offsetY = (state & ODS_SELECTED) ? 1 : 0;
        
        // Иконка (▶ или ■) - только иконка, без текста
        HFONT hIconFont = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Symbol");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hIconFont);
        
        RECT iconRect = rc;
        iconRect.left += offsetX;
        iconRect.top += offsetY;
        DrawText(hdc, btnIcon, -1, &iconRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        
        SelectObject(hdc, hOldFont);
        DeleteObject(hIconFont);
        
        if (state & ODS_FOCUS) {
            RECT focusRect = rc;
            InflateRect(&focusRect, -3, -3);
            DrawFocusRect(hdc, &focusRect);
        }
        return;
    }
    
    switch (ctrlId) {
        case IDC_XRANGE_APPLY_BTN:
        case IDC_XRANGE2_APPLY_BTN:
            iconText = L"\u2714";          // ✔
            break;
        case IDC_XRANGE_RESET_BTN:
        case IDC_XRANGE2_RESET_BTN:
            iconText = L"\u21BA";          // ↺
            break;
        case IDC_SAVE_RANGE1_BTN:
            iconText = L"\u2193";          // ↓
            break;
        case IDC_CUT_RANGE1_BTN:
            iconText = L"\u2702";          // ✂
            break;
        case IDC_CHART1_UNDO_ZOOM_BTN:
        case IDC_CHART2_UNDO_ZOOM_BTN:
            iconText = L"\u21B6";          // ↶
            break;
        case IDC_RESET_ZOOM1_BTN:
        case IDC_RESET_ZOOM2_BTN:
            iconText = L"\u21BA";          // ↺
            labelText = L"Zoom";
            break;
        case IDC_CHART2_SETTINGS_BTN:
            iconText = L"\u2699";          // ⚙
            break;
        case IDC_CHART2_OPEN_FILE_BTN:
            // Иконка папки рисуется отдельно через GDI ниже (специальный блок).
            break;
        default:
            return;
    }

    // === Специальная отрисовка кнопки «Открыть файл для графика 2» (системная иконка папки) ===
    if (ctrlId == IDC_CHART2_OPEN_FILE_BTN) {
        // Фон кнопки
        COLORREF bgColor;
        if (state & ODS_SELECTED) {
            bgColor = RGB(200, 200, 200);
        } else if (state & ODS_DISABLED) {
            bgColor = RGB(240, 240, 240);
        } else {
            bgColor = GetSysColor(COLOR_BTNFACE);
        }

        HBRUSH hBrush = CreateSolidBrush(bgColor);
        FillRect(hdc, &rc, hBrush);
        DeleteObject(hBrush);

        // Иконка в статусной строке плоская, поэтому рамку не рисуем.
        // Фон уже залит системным цветом кнопки, чтобы визуально вписаться в панель.

        // Загружаем системную иконку папки (такую же, как в статусной строке)
        if (!hChart2OpenFileIcon) {
            SHSTOCKICONINFO sii = {};
            sii.cbSize = sizeof(sii);
            if (SUCCEEDED(SHGetStockIconInfo(SIID_FOLDER, SHGFI_ICON | SHGFI_SMALLICON, &sii))) {
                hChart2OpenFileIcon = sii.hIcon;
            }
        }

        // Рисуем иконку по центру кнопки
        if (hChart2OpenFileIcon) {
            const int offsetX = (state & ODS_SELECTED) ? 1 : 0;
            const int offsetY = (state & ODS_SELECTED) ? 1 : 0;

            ICONINFO ii = {};
            int iconWidth = 16;
            int iconHeight = 16;
            if (GetIconInfo(hChart2OpenFileIcon, &ii)) {
                if (ii.hbmColor) {
                    BITMAP bm = {};
                    if (GetObject(ii.hbmColor, sizeof(bm), &bm)) {
                        iconWidth = bm.bmWidth;
                        iconHeight = bm.bmHeight;
                    }
                    DeleteObject(ii.hbmColor);
                }
                if (ii.hbmMask) {
                    DeleteObject(ii.hbmMask);
                }
            }

            const int x = rc.left + ((rc.right - rc.left) - iconWidth) / 2 + offsetX;
            const int y = rc.top + ((rc.bottom - rc.top) - iconHeight) / 2 + offsetY;
            DrawIconEx(hdc, x, y, hChart2OpenFileIcon, iconWidth, iconHeight, 0, nullptr, DI_NORMAL);
        }

        // Фокус
        if (state & ODS_FOCUS) {
            RECT focusRect = rc;
            InflateRect(&focusRect, -3, -3);
            DrawFocusRect(hdc, &focusRect);
        }
        return;
    }

    // Цвет текста — стандартный системный
    COLORREF textColor = GetSysColor(COLOR_BTNTEXT);
    
    // Фон кнопки
    COLORREF bgColor;
    if (state & ODS_SELECTED) {
        bgColor = RGB(200, 200, 200);  // Нажатая кнопка
    } else if (state & ODS_DISABLED) {
        bgColor = RGB(240, 240, 240);  // Отключённая кнопка
        textColor = RGB(160, 160, 160);
    } else {
        bgColor = GetSysColor(COLOR_BTNFACE);  // Обычный фон
    }
    
    // Заливка фона
    HBRUSH hBrush = CreateSolidBrush(bgColor);
    FillRect(hdc, &rc, hBrush);
    DeleteObject(hBrush);
    
    // Рамка кнопки
    if (state & ODS_SELECTED) {
        DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT);
    } else {
        DrawEdge(hdc, &rc, EDGE_RAISED, BF_RECT);
    }
    
    // Создаём крупный жирный шрифт для иконки
    HFONT hIconFont = CreateFont(
        22,                        // Высота шрифта (крупный)
        0,                         // Ширина (авто)
        0, 0,                      // Углы
        FW_BOLD,                   // Жирный шрифт для лучшей видимости
        FALSE, FALSE, FALSE,       // Курсив, подчёркивание, зачёркивание
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,         // Качественный рендеринг
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI Symbol"         // Шрифт с хорошей поддержкой Unicode символов
    );
    
    HFONT hOldFont = (HFONT)SelectObject(hdc, hIconFont);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, textColor);
    
    // Смещение при нажатии
    int offsetX = (state & ODS_SELECTED) ? 1 : 0;
    int offsetY = (state & ODS_SELECTED) ? 1 : 0;
    
    // Отрисовка иконки и текста
    if (wcslen(labelText) > 0) {
        // Кнопка с иконкой и текстом (Reset Zoom)
        RECT iconRect = rc;
        iconRect.right = rc.left + 24;
        iconRect.left += offsetX;
        iconRect.top += offsetY;
        DrawText(hdc, iconText, -1, &iconRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        
        // Текст "Zoom" — на базе единого шрифта приложения, полужирный
        LOGFONT lfZoom = {};
        if (hAppFont && GetObject(hAppFont, sizeof(lfZoom), &lfZoom)) {
            lfZoom.lfWeight = FW_SEMIBOLD;
        } else {
            wcscpy_s(lfZoom.lfFaceName, L"Segoe UI");
            lfZoom.lfHeight = 15;
            lfZoom.lfWeight = FW_SEMIBOLD;
            lfZoom.lfQuality = CLEARTYPE_QUALITY;
        }
        HFONT hTextFont = CreateFontIndirect(&lfZoom);
        SelectObject(hdc, hTextFont);
        
        RECT textRect = rc;
        textRect.left = rc.left + 22 + offsetX;
        textRect.top += offsetY;
        DrawText(hdc, labelText, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        
        DeleteObject(hTextFont);
    } else {
        // Только иконка
        RECT iconRect = rc;
        iconRect.left += offsetX;
        iconRect.top += offsetY;
        DrawText(hdc, iconText, -1, &iconRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    
    // Восстановление и удаление шрифта
    SelectObject(hdc, hOldFont);
    DeleteObject(hIconFont);
    
    // Фокус
    if (state & ODS_FOCUS) {
        RECT focusRect = rc;
        InflateRect(&focusRect, -3, -3);
        DrawFocusRect(hdc, &focusRect);
    }
}

// === Реализация методов переключателя цели Y-zoom (v2.18.0) ===

int UIModule::GetYZoomTargetIndex(int chartNumber) const {
    HWND hCombo = (chartNumber == 1) ? hChart1YZoomTargetCombo : hChart2YZoomTargetCombo;
    if (!hCombo) return 0;
    int idx = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
    return (idx >= 0 && idx <= 2) ? idx : 0;
}

void UIModule::SetYZoomTargetIndex(int chartNumber, int index) {
    HWND hCombo = (chartNumber == 1) ? hChart1YZoomTargetCombo : hChart2YZoomTargetCombo;
    if (!hCombo) return;
    if (index < 0 || index > 2) index = 0;
    SendMessage(hCombo, CB_SETCURSEL, (WPARAM)index, 0);
}

void UIModule::ShowYZoomTargetCombo(int chartNumber, bool show) {
    HWND hCombo = (chartNumber == 1) ? hChart1YZoomTargetCombo : hChart2YZoomTargetCombo;
    HWND hLabel = (chartNumber == 1) ? hChart1YZoomLabel : hChart2YZoomLabel;
    if (hCombo) ShowWindow(hCombo, show ? SW_SHOW : SW_HIDE);
    if (hLabel) ShowWindow(hLabel, show ? SW_SHOW : SW_HIDE);
}

HWND UIModule::GetYZoomTargetCombo(int chartNumber) const {
    return (chartNumber == 1) ? hChart1YZoomTargetCombo : hChart2YZoomTargetCombo;
}
