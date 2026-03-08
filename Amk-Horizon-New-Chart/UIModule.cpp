#include "framework.h"
#include "UIModule.h"

namespace {
bool IsUiTransmitterIndex(int transmitterIdx) {
    return transmitterIdx >= 0 && transmitterIdx < UIModule::UI_TRANSMITTER_COUNT;
}

void SetTransmitterEnabled(std::vector<bool>& enabledTransmitters, int transmitterIdx, bool enabled) {
    if (IsUiTransmitterIndex(transmitterIdx) && transmitterIdx < static_cast<int>(enabledTransmitters.size())) {
        enabledTransmitters[static_cast<size_t>(transmitterIdx)] = enabled;
    }
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
    , hChart1SurfCheck(nullptr)
    , hChart1SurfEdit(nullptr)
    , hChart2SurfCheck(nullptr)
    , hChart2SurfEdit(nullptr)
    , hChart1ShowNotesCheck(nullptr)
    , hChart2ShowNotesCheck(nullptr)
    , hTooltip(nullptr)
    , dualMode(false) {
    chart1EnabledTransmitters.resize(UI_TRANSMITTER_COUNT, true);
    chart2EnabledTransmitters.resize(UI_TRANSMITTER_COUNT, true);
}

UIModule::~UIModule() {
}

void UIModule::CreateControls(HWND hWnd, HINSTANCE hInst) {
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
    
    // Кнопка "Reset Zoom" для Chart 1 - Owner-Draw для цветной иконки (увеличен размер)
    hResetZoom1Btn = CreateWindow(L"BUTTON", L"",
        WS_VISIBLE | WS_CHILD | BS_OWNERDRAW,
        xRangeStartX + 206, yPos - 2, 65, 24, hWnd, (HMENU)IDC_RESET_ZOOM1_BTN, hInst, NULL);

    // === Элементы управления режимом Surf для Chart 1 (v2.14.0) ===
    // ПЕРЕМЕЩЕНО: Surf теперь ПЕРЕД Cursor (v2.15.1)
    int surfStartX = xRangeStartX + 275;
    
    // Checkbox "Surf"
    hChart1SurfCheck = CreateWindow(L"BUTTON", L"Surf",
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        surfStartX, yPos, 50, 20, hWnd, (HMENU)IDC_CHART1_SURF_CHECK, hInst, NULL);
    
    // Поле ввода количества точек (по умолчанию 100)
    hChart1SurfEdit = CreateWindow(L"EDIT", L"100",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER | ES_CENTER,
        surfStartX + 55, yPos - 2, 45, 22, hWnd, (HMENU)IDC_CHART1_SURF_EDIT, hInst, NULL);

    // Чекбокс отображения заметок (Chart 1), компактный и на уровне заголовка графика
    hChart1ShowNotesCheck = CreateWindow(L"BUTTON", L"\x041F\x043E\x043A\x0430\x0437\x0430\x0442\x044C \x0437\x0430\x043C\x0435\x0442\x043A\x0438",
        WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        0, 0, 128, 18, hWnd, (HMENU)IDC_CHART1_SHOW_NOTES_CHECK, hInst, NULL);
    SendMessage(hChart1ShowNotesCheck, BM_SETCHECK, BST_CHECKED, 0);
    SendMessage(hChart1ShowNotesCheck, WM_SETFONT, (WPARAM)GetStockObject(ANSI_VAR_FONT), TRUE);

    // Элементы отображения координат курсора для Chart 1 (компактный дизайн v2.15.1)
    // ПЕРЕМЕЩЕНО: Cursor теперь КРАЙНИЙ СПРАВА (v2.15.1)
    // Формат: 📍 [X] : [Y1 unit] | [Y2 unit]
    int cursorStartX = surfStartX + 105;  // После Surf элементов (убрана метка "pts")
    
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
        xRange2StartX + 178, 298, 65, 24, hWnd, (HMENU)IDC_RESET_ZOOM2_BTN, hInst, NULL);

    // === Элементы управления режимом Surf для Chart 2 (v2.14.0) ===
    // ПЕРЕМЕЩЕНО: Surf теперь ПЕРЕД Cursor (v2.15.1)
    int surf2StartX = xRange2StartX + 247;
    
    // Checkbox "Surf" для Chart 2 (изначально скрыт)
    hChart2SurfCheck = CreateWindow(L"BUTTON", L"Surf",
        WS_CHILD | BS_AUTOCHECKBOX,
        surf2StartX, 300, 50, 20, hWnd, (HMENU)IDC_CHART2_SURF_CHECK, hInst, NULL);
    
    // Поле ввода количества точек для Chart 2 (по умолчанию 100)
    hChart2SurfEdit = CreateWindow(L"EDIT", L"100",
        WS_CHILD | WS_BORDER | ES_NUMBER | ES_CENTER,
        surf2StartX + 55, 298, 45, 22, hWnd, (HMENU)IDC_CHART2_SURF_EDIT, hInst, NULL);

    // Чекбокс отображения заметок (Chart 2), изначально скрыт
    hChart2ShowNotesCheck = CreateWindow(L"BUTTON", L"\x041F\x043E\x043A\x0430\x0437\x0430\x0442\x044C \x0437\x0430\x043C\x0435\x0442\x043A\x0438",
        WS_CHILD | BS_AUTOCHECKBOX,
        0, 0, 128, 18, hWnd, (HMENU)IDC_CHART2_SHOW_NOTES_CHECK, hInst, NULL);
    SendMessage(hChart2ShowNotesCheck, BM_SETCHECK, BST_CHECKED, 0);
    SendMessage(hChart2ShowNotesCheck, WM_SETFONT, (WPARAM)GetStockObject(ANSI_VAR_FONT), TRUE);

    // Элементы отображения координат курсора для Chart 2 (компактный дизайн v2.15.1, изначально скрыты)
    // ПЕРЕМЕЩЕНО: Cursor теперь КРАЙНИЙ СПРАВА (v2.15.1)
    // Формат: 📍 [X] : [Y1 unit] | [Y2 unit]
    int cursor2StartX = surf2StartX + 105;  // После Surf элементов (убрана метка "pts")
    
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

    // Настройка частей статусной строки (4 части)
    int statusBarParts[4] = { 150, 350, 550, -1 }; // -1 означает "до конца"
    SendMessage(hStatusBar, SB_SETPARTS, 4, (LPARAM)statusBarParts);
    
    // Установка начального текста
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"Ready");
    SendMessage(hStatusBar, SB_SETTEXT, 1, (LPARAM)L"");
    SendMessage(hStatusBar, SB_SETTEXT, 2, (LPARAM)L"");
    SendMessage(hStatusBar, SB_SETTEXT, 3, (LPARAM)L"No file loaded");
    
    // Создание tooltips для кнопок с иконками (Chart 1)
    CreateTooltip(hWnd, hXRangeApplyBtn, L"Применить");
    CreateTooltip(hWnd, hXRangeResetBtn, L"Сбросить");
    CreateTooltip(hWnd, hSaveRange1Btn, L"Загрузить");
    CreateTooltip(hWnd, hResetZoom1Btn, L"Сбросить Zoom");
    
    // Создание tooltips для кнопок с иконками (Chart 2)
    CreateTooltip(hWnd, hXRange2ApplyBtn, L"Применить");
    CreateTooltip(hWnd, hXRange2ResetBtn, L"Сбросить");
    CreateTooltip(hWnd, hResetZoom2Btn, L"Сбросить Zoom");

    // Tooltips для осевого undo
    CreateTooltip(hWnd, hChart1HUndoBtn, L"Undo X zoom step");
    CreateTooltip(hWnd, hChart1VUndoBtn, L"Undo Y zoom step");
    CreateTooltip(hWnd, hChart2HUndoBtn, L"Undo X zoom step");
    CreateTooltip(hWnd, hChart2VUndoBtn, L"Undo Y zoom step");

    // Tooltip для иконки перекрестия (клик сбрасывает перекрестие на графике)
    CreateTooltip(hWnd, hCursor1Icon, L"Clear crosshair");
    CreateTooltip(hWnd, hCursor2Icon, L"Clear crosshair");

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

    // Показываем/скрываем чекбокс отображения заметок для Chart 2
    ShowWindow(hChart2ShowNotesCheck, showFlag);
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
    SetWindowPos(hResetZoom2Btn, HWND_TOP, xRange2StartX + 178, yPos - 2, 65, 24, SWP_NOSIZE);
    
    // Позиционируем элементы Surf для Chart 2 (v2.15.1 - Surf ПЕРЕД Cursor)
    int surf2StartX = xRange2StartX + 247;
    SetWindowPos(hChart2SurfCheck, HWND_TOP, surf2StartX, yPos, 50, 20, SWP_NOSIZE);
    SetWindowPos(hChart2SurfEdit, HWND_TOP, surf2StartX + 55, yPos - 2, 45, 22, SWP_NOSIZE);
    
    // Позиционируем элементы координат курсора для Chart 2 (v2.15.1 - Cursor КРАЙНИЙ СПРАВА)
    int cursor2StartX = surf2StartX + 105;  // После Surf элементов (убрана метка "pts")
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
    const int CHECK_WIDTH = 128;
    const int CHECK_HEIGHT = 18;
    const int RIGHT_MARGIN = 10;
    const int TOP_OFFSET = 8;

    if (hChart1ShowNotesCheck) {
        int x1 = chart1Rect.right - RIGHT_MARGIN - CHECK_WIDTH;
        int y1 = chart1Rect.top + TOP_OFFSET;
        SetWindowPos(hChart1ShowNotesCheck, HWND_TOP, x1, y1, CHECK_WIDTH, CHECK_HEIGHT, SWP_NOZORDER);
    }

    if (hChart2ShowNotesCheck) {
        if (dualMode) {
            int x2 = chart2Rect.right - RIGHT_MARGIN - CHECK_WIDTH;
            int y2 = chart2Rect.top + TOP_OFFSET;
            SetWindowPos(hChart2ShowNotesCheck, HWND_TOP, x2, y2, CHECK_WIDTH, CHECK_HEIGHT, SWP_NOZORDER);
        } else {
            ShowWindow(hChart2ShowNotesCheck, SW_HIDE);
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
    
    SendMessage(hStatusBar, SB_SETTEXT, 0, (LPARAM)L"Ready");
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
    // Реализация перенесена в диалог
}

void UIModule::SetSerialPolling(bool polling) {
    // Реализация перенесена в диалог
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
        case IDC_RESET_ZOOM1_BTN:
        case IDC_XRANGE2_APPLY_BTN:
        case IDC_XRANGE2_RESET_BTN:
        case IDC_RESET_ZOOM2_BTN:
        case IDC_CURSOR1_ICON:  // Иконка маркера для Chart 1
        case IDC_CURSOR2_ICON:  // Иконка маркера для Chart 2
            return true;
        default:
            return false;
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
        case IDC_RESET_ZOOM1_BTN:
        case IDC_RESET_ZOOM2_BTN:
            iconText = L"\u21BA";          // ↺
            labelText = L"Zoom";
            break;
        default:
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
        
        // Создаём шрифт для текста "Zoom" — жирный, как у других элементов панели
        HFONT hTextFont = CreateFont(
            15,                        // Высота шрифта (как у стандартных кнопок)
            0, 0, 0,
            FW_SEMIBOLD,               // Полужирный — как у стандартных элементов
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            L"Segoe UI"
        );
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
