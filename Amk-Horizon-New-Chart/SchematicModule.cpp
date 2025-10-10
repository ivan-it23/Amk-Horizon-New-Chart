#include "framework.h"
#include "SchematicModule.h"
#include "CurveSelectionModule.h"
#include "Resource.h"
#include <sstream>
#include <commctrl.h>
#include <algorithm>

// Определение цветов для элементов схемы
const COLORREF SchematicModule::TRANSMITTER_COLOR = RGB(180, 100, 100);  // Приглушенный красный
const COLORREF SchematicModule::RECEIVER_COLOR = RGB(100, 180, 100);     // Приглушенный зеленый  
const COLORREF SchematicModule::SILENT_TX_COLOR = RGB(128, 128, 128);    // Серый
const COLORREF SchematicModule::SELECTION_COLOR = RGB(255, 255, 0);      // Желтый для выделения

// Реализация CurveDefinition
CurveDefinition::CurveDefinition() 
    : yAxisGroup(0), enabled(false), color(RGB(0, 0, 0)), lineStyle(PS_SOLID),
      sourceElement(ElementType::TRANSMITTER), elementIndex(0), 
      dataType(DataType::AMPLITUDE_RX1), frequency(FrequencyBand::FREQ_400_KHZ) {
}

CurveDefinition::CurveDefinition(const std::string& id, const std::string& name, 
                               const std::string& units, ElementType element, 
                               int index, DataType type, FrequencyBand freq)
    : id(id), displayName(name), units(units), yAxisGroup(0), enabled(true),
      color(RGB(0, 0, 255)), lineStyle(PS_SOLID), sourceElement(element),
      elementIndex(index), dataType(type), frequency(freq) {
}

// Реализация SchematicSelection
SchematicSelection::SchematicSelection() : frequency(FrequencyBand::FREQ_400_KHZ) {
    transmitters.resize(4, false);  // T1-T4
    receivers.resize(2, false);     // R1-R2
    silentTx.resize(2, false);      // Молчащие для R1 и R2
}

void SchematicSelection::Clear() {
    std::fill(transmitters.begin(), transmitters.end(), false);
    std::fill(receivers.begin(), receivers.end(), false);
    std::fill(silentTx.begin(), silentTx.end(), false);
    frequency = FrequencyBand::FREQ_400_KHZ;
}

bool SchematicSelection::IsValid() const {
    return GetValidationError().empty();
}

int SchematicSelection::GetSelectedReceiversCount() const {
    return static_cast<int>(std::count(receivers.begin(), receivers.end(), true));
}

int SchematicSelection::GetSelectedTransmittersCount() const {
    return static_cast<int>(std::count(transmitters.begin(), transmitters.end(), true));
}

int SchematicSelection::GetSelectedSilentCount() const {
    return static_cast<int>(std::count(silentTx.begin(), silentTx.end(), true));
}

std::string SchematicSelection::GetValidationError() const {
    int receivers = GetSelectedReceiversCount();
    int transmitters = GetSelectedTransmittersCount();
    int silent = GetSelectedSilentCount();
    
    // Должен быть выбран хотя бы один элемент
    if (receivers == 0 && transmitters == 0 && silent == 0) {
        return "Выберите элементы для построения графика";
    }
    
    // Нельзя одновременно выбирать передатчики и приемники
    if (transmitters > 0 && receivers > 0) {
        return "Нельзя одновременно выбирать передатчики и приемники";
    }
    
    // Молчащие передатчики нельзя комбинировать с активными элементами
    if (silent > 0 && (transmitters > 0 || receivers > 0)) {
        return "Молчащие передатчики нельзя комбинировать с активными элементами";
    }
    
    return ""; // Выбор корректен
}

// Реализация ChartConfiguration
ChartConfiguration::ChartConfiguration() : hasDualAxis(false) {
}

void ChartConfiguration::Clear() {
    curves.clear();
    hasDualAxis = false;
    leftAxisUnits.clear();
    rightAxisUnits.clear();
}

bool ChartConfiguration::IsEmpty() const {
    return curves.empty();
}

ChartConfiguration ChartConfiguration::Copy() const {
    ChartConfiguration copy;
    copy.curves = this->curves;
    copy.hasDualAxis = this->hasDualAxis;
    copy.leftAxisUnits = this->leftAxisUnits;
    copy.rightAxisUnits = this->rightAxisUnits;
    return copy;
}

void ChartConfiguration::AddCurve(const CurveDefinition& curve) {
    curves.push_back(curve);
    UpdateAxisConfiguration();
}

void ChartConfiguration::RemoveCurve(const std::string& curveId) {
    curves.erase(std::remove_if(curves.begin(), curves.end(),
        [&curveId](const CurveDefinition& curve) { return curve.id == curveId; }),
        curves.end());
    UpdateAxisConfiguration();
}

void ChartConfiguration::UpdateAxisConfiguration() {
    // Определяем, используются ли обе оси
    bool hasLeftAxis = false, hasRightAxis = false;
    
    for (const auto& curve : curves) {
        if (!curve.enabled) continue;
        
        if (curve.yAxisGroup == 0) {
            hasLeftAxis = true;
            if (leftAxisUnits.empty()) {
                leftAxisUnits = curve.units;
            }
        } else if (curve.yAxisGroup == 1) {
            hasRightAxis = true;
            if (rightAxisUnits.empty()) {
                rightAxisUnits = curve.units;
            }
        }
    }
    
    hasDualAxis = hasLeftAxis && hasRightAxis;
    
    // Очищаем единицы измерения неиспользуемых осей
    if (!hasLeftAxis) leftAxisUnits.clear();
    if (!hasRightAxis) rightAxisUnits.clear();
}

// Реализация SchematicModule
SchematicModule::SchematicModule() : curveSelectionModule(nullptr), targetChartNumber(1) {
}

SchematicModule::~SchematicModule() {
}

bool SchematicModule::ShowSchematicDialog(HWND parent, int chartNumber, ChartConfiguration& config) {
    targetChartNumber = chartNumber;
    
    // Создаем модальное диалоговое окно
    HINSTANCE hInst = GetModuleHandle(NULL);
    
    // Передаем указатель на этот объект через lParam
    INT_PTR result = DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_SCHEMATIC_DIALOG), 
                                   parent, SchematicDialogProc, (LPARAM)this);
    
    if (result == IDOK) {
        // Пользователь нажал "Применить" - обновляем конфигурацию
        // TODO: Здесь будет логика применения настроек
        return true;
    }
    
    return false; // Пользователь отменил или закрыл окно
}

void SchematicModule::RenderSchematic(HDC hdc, const RECT& rect) {
    // Очищаем фон
    HBRUSH bgBrush = CreateSolidBrush(RGB(240, 240, 240));
    FillRect(hdc, &rect, bgBrush);
    DeleteObject(bgBrush);
    
    // Рассчитываем компоновку элементов
    CalculateSchematicLayout(rect);
    
    // Отрисовываем все элементы схемы
    for (const auto& element : schematicElements) {
        std::wstring label = GetElementLabel(element.type, element.index);
        DrawElement(hdc, element.rect, element.type, element.index, element.selected, label);
    }
    
    // Отрисовываем соединительные линии между элементами
    DrawSchematicConnections(hdc, rect);
    
    // Добавляем подписи групп элементов
    DrawGroupLabels(hdc, rect);
}

void SchematicModule::DrawElement(HDC hdc, const RECT& elementRect, ElementType type, 
                                 int index, bool selected, const std::wstring& label) {
    // Определяем цвет элемента
    COLORREF fillColor, borderColor;
    switch (type) {
        case ElementType::TRANSMITTER:
            fillColor = TRANSMITTER_COLOR;
            borderColor = RGB(140, 80, 80);  // Более темная рамка
            break;
        case ElementType::RECEIVER:
            fillColor = RECEIVER_COLOR;
            borderColor = RGB(80, 140, 80);  // Более темная рамка
            break;
        case ElementType::SILENT_TX:
            fillColor = SILENT_TX_COLOR;
            borderColor = RGB(100, 100, 100); // Более темная рамка
            break;
        default:
            fillColor = RGB(200, 200, 200);
            borderColor = RGB(100, 100, 100);
    }
    
    // Создаем кисть для заливки и перо для рамки
    HBRUSH fillBrush = CreateSolidBrush(fillColor);
    HPEN borderPen = CreatePen(PS_SOLID, 2, borderColor);
    
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, fillBrush);
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    
    // Рисуем скругленный прямоугольник для более современного вида
    int cornerRadius = 8;
    RoundRect(hdc, elementRect.left, elementRect.top, elementRect.right, elementRect.bottom,
              cornerRadius, cornerRadius);
    
    // Если элемент выбран, рисуем рамку выделения
    if (selected) {
        DrawSelectionHighlight(hdc, elementRect);
    }
    
    // Восстанавливаем объекты GDI
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(fillBrush);
    DeleteObject(borderPen);
    
    // Настраиваем текст
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(40, 40, 40)); // Темно-серый текст
    
    // Создаем шрифт для подписей элементов
    HFONT elementFont = CreateFont(12, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
    HFONT oldFont = (HFONT)SelectObject(hdc, elementFont);
    
    // Рисуем подпись по центру элемента
    RECT textRect = elementRect;
    DrawText(hdc, label.c_str(), -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    // Восстанавливаем шрифт
    SelectObject(hdc, oldFont);
    DeleteObject(elementFont);
}

bool SchematicModule::HandleElementClick(int x, int y) {
    ElementRect* element = FindElementAt(x, y);
    if (!element) {
        return false;
    }
    
    // Проверяем логику выбора элементов
    bool newState = !element->selected;
    
    // Применяем правила взаимоисключения
    if (newState) {
        // Если выбираем новый элемент, проверяем совместимость
        if (!IsSelectionCompatible(element->type, element->index)) {
            // Очищаем несовместимые выборы
            ClearIncompatibleSelections(element->type);
        }
    }
    
    // Обновляем состояние элемента
    element->selected = newState;
    UpdateSelection(element->type, element->index, newState);
    
    // Обновляем визуальное состояние всех элементов
    SynchronizeElementStates();
    
    return true;
}

void SchematicModule::UpdateSelection(ElementType type, int index, bool selected) {
    switch (type) {
        case ElementType::TRANSMITTER:
            if (index >= 0 && index < 4) {
                currentSelection.transmitters[index] = selected;
            }
            break;
        case ElementType::RECEIVER:
            if (index >= 0 && index < 2) {
                currentSelection.receivers[index] = selected;
            }
            break;
        case ElementType::SILENT_TX:
            if (index >= 0 && index < 2) {
                currentSelection.silentTx[index] = selected;
            }
            break;
    }
}

bool SchematicModule::ValidateCurrentSelection() {
    return currentSelection.IsValid();
}

SchematicSelection SchematicModule::GetCurrentSelection() const {
    return currentSelection;
}

void SchematicModule::SetCurrentSelection(const SchematicSelection& selection) {
    currentSelection = selection;
    
    // Обновляем состояние элементов схемы
    for (auto& element : schematicElements) {
        switch (element.type) {
            case ElementType::TRANSMITTER:
                element.selected = (element.index < currentSelection.transmitters.size()) 
                    ? currentSelection.transmitters[element.index] : false;
                break;
            case ElementType::RECEIVER:
                element.selected = (element.index < currentSelection.receivers.size()) 
                    ? currentSelection.receivers[element.index] : false;
                break;
            case ElementType::SILENT_TX:
                element.selected = (element.index < currentSelection.silentTx.size()) 
                    ? currentSelection.silentTx[element.index] : false;
                break;
        }
    }
}

void SchematicModule::SetCurveSelectionModule(CurveSelectionModule* module) {
    curveSelectionModule = module;
}

void SchematicModule::InitializeSchematicElements(const RECT& schemaRect) {
    schematicElements.clear();
    
    // TODO: Реализовать инициализацию элементов схемы
    // Пока что создаем заглушки для элементов
    
    // Добавляем передатчики T1-T4
    for (int i = 0; i < 4; i++) {
        RECT rect = CalculateElementRect(schemaRect, ElementType::TRANSMITTER, i);
        schematicElements.emplace_back(rect, ElementType::TRANSMITTER, i);
    }
    
    // Добавляем приемники R1-R2
    for (int i = 0; i < 2; i++) {
        RECT rect = CalculateElementRect(schemaRect, ElementType::RECEIVER, i);
        schematicElements.emplace_back(rect, ElementType::RECEIVER, i);
    }
    
    // Добавляем молчащие передатчики
    for (int i = 0; i < 2; i++) {
        RECT rect = CalculateElementRect(schemaRect, ElementType::SILENT_TX, i);
        schematicElements.emplace_back(rect, ElementType::SILENT_TX, i);
    }
}

SchematicModule::ElementRect* SchematicModule::FindElementAt(int x, int y) {
    POINT pt = {x, y};
    for (auto& element : schematicElements) {
        if (PtInRect(&element.rect, pt)) {
            return &element;
        }
    }
    return nullptr;
}

std::wstring SchematicModule::GetElementLabel(ElementType type, int index) const {
    switch (type) {
        case ElementType::TRANSMITTER:
            return L"T" + std::to_wstring(index + 1);
        case ElementType::RECEIVER:
            return L"R" + std::to_wstring(index + 1);
        case ElementType::SILENT_TX:
            return L"Silent R" + std::to_wstring(index + 1);
        default:
            return L"Unknown";
    }
}

std::wstring SchematicModule::GetElementTooltip(ElementType type, int index) const {
    switch (type) {
        case ElementType::TRANSMITTER:
            return L"Передатчик T" + std::to_wstring(index + 1) + L" - нажмите для выбора";
        case ElementType::RECEIVER:
            return L"Приемник R" + std::to_wstring(index + 1) + L" - нажмите для выбора";
        case ElementType::SILENT_TX:
            return L"Молчащий передатчик для R" + std::to_wstring(index + 1) + L" - нажмите для выбора";
        default:
            return L"Неизвестный элемент";
    }
}

void SchematicModule::DrawTransmitter(HDC hdc, const RECT& rect, int index, bool selected) {
    std::wstring label = L"T" + std::to_wstring(index + 1);
    DrawElement(hdc, rect, ElementType::TRANSMITTER, index, selected, label);
}

void SchematicModule::DrawReceiver(HDC hdc, const RECT& rect, int index, bool selected) {
    std::wstring label = L"R" + std::to_wstring(index + 1);
    DrawElement(hdc, rect, ElementType::RECEIVER, index, selected, label);
}

void SchematicModule::DrawSilentTransmitter(HDC hdc, const RECT& rect, int index, bool selected) {
    std::wstring label = L"Silent R" + std::to_wstring(index + 1);
    DrawElement(hdc, rect, ElementType::SILENT_TX, index, selected, label);
}

void SchematicModule::DrawSelectionHighlight(HDC hdc, const RECT& rect) {
    // Создаем анимированную рамку выделения
    HPEN highlightPen = CreatePen(PS_SOLID, 4, SELECTION_COLOR);
    HPEN oldPen = (HPEN)SelectObject(hdc, highlightPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    
    // Рисуем внешнюю рамку выделения
    int offset = 3;
    RoundRect(hdc, rect.left - offset, rect.top - offset, 
              rect.right + offset, rect.bottom + offset, 12, 12);
    
    // Добавляем внутреннюю рамку для большей заметности
    HPEN innerPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255)); // Белая внутренняя рамка
    SelectObject(hdc, innerPen);
    
    offset = 1;
    RoundRect(hdc, rect.left - offset, rect.top - offset, 
              rect.right + offset, rect.bottom + offset, 10, 10);
    
    // Восстанавливаем объекты GDI
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(highlightPen);
    DeleteObject(innerPen);
}

RECT SchematicModule::CalculateElementRect(const RECT& schemaRect, ElementType type, int index) {
    RECT rect;
    int elementWidth = 70, elementHeight = 45;
    int spacing = 15;
    int margin = 30;
    
    // Рассчитываем общую ширину для центрирования
    int totalWidth = schemaRect.right - schemaRect.left;
    int availableWidth = totalWidth - 2 * margin;
    
    switch (type) {
        case ElementType::TRANSMITTER:
            {
                // Передатчики T3, T1, R1, R2, T2, T4 (слева направо)
                int transmitterOrder[4] = {1, 0, 2, 3}; // T1=0, T2=2, T3=1, T4=3 -> порядок T3,T1,T2,T4
                int orderIndex = 0;
                for (int i = 0; i < 4; i++) {
                    if (transmitterOrder[i] == index) {
                        orderIndex = i;
                        break;
                    }
                }
                
                // Позиции: T3(0), T1(1), пропуск для приемников, T2(2), T4(3)
                int xOffset;
                if (orderIndex == 0) xOffset = 0; // T3
                else if (orderIndex == 1) xOffset = 1; // T1  
                else if (orderIndex == 2) xOffset = 4; // T2
                else xOffset = 5; // T4
                
                rect.left = schemaRect.left + margin + xOffset * (elementWidth + spacing);
                rect.top = schemaRect.top + 20;
            }
            break;
            
        case ElementType::RECEIVER:
            {
                // Приемники R1, R2 в центре между передатчиками
                rect.left = schemaRect.left + margin + (2 + index) * (elementWidth + spacing);
                rect.top = schemaRect.top + 80;
            }
            break;
            
        case ElementType::SILENT_TX:
            {
                // Молчащие передатчики справа
                rect.left = schemaRect.left + totalWidth - margin - (2 - index) * (elementWidth + spacing);
                rect.top = schemaRect.top + 140;
            }
            break;
    }
    
    rect.right = rect.left + elementWidth;
    rect.bottom = rect.top + elementHeight;
    
    return rect;
}

void SchematicModule::CalculateSchematicLayout(const RECT& schemaRect) {
    InitializeSchematicElements(schemaRect);
}

// Методы для работы с диалоговым окном
void SchematicModule::InitializeDialogControls(HWND hDlg) {
    // Инициализируем комбобокс частоты
    HWND hFreqCombo = GetDlgItem(hDlg, IDC_FREQUENCY_COMBO);
    if (hFreqCombo) {
        SendMessage(hFreqCombo, CB_ADDSTRING, 0, (LPARAM)L"400 кГц");
        SendMessage(hFreqCombo, CB_ADDSTRING, 0, (LPARAM)L"2000 кГц");
        SendMessage(hFreqCombo, CB_SETCURSEL, 0, 0); // Выбираем 400 кГц по умолчанию
        currentSelection.frequency = FrequencyBand::FREQ_400_KHZ;
    }
    
    // Инициализируем область схемы
    HWND hSchematicArea = GetDlgItem(hDlg, IDC_SCHEMATIC_AREA);
    if (hSchematicArea) {
        RECT schematicRect;
        GetClientRect(hSchematicArea, &schematicRect);
        InitializeSchematicElements(schematicRect);
    }
    
    // Обновляем список доступных кривых
    UpdateAvailableCurves(hDlg);
}

void SchematicModule::UpdateAvailableCurves(HWND hDlg) {
    HWND hCurveList = GetDlgItem(hDlg, IDC_CURVE_LIST);
    if (!hCurveList) return;
    
    // Очищаем ListView
    ListView_DeleteAllItems(hCurveList);
    
    if (!currentSelection.IsValid()) {
        // Показываем сообщение об ошибке выбора
        std::string error = currentSelection.GetValidationError();
        std::wstring werror(error.begin(), error.end());
        
        LVITEM item = {};
        item.mask = LVIF_TEXT;
        item.iItem = 0;
        item.iSubItem = 0;
        item.pszText = const_cast<LPWSTR>(werror.c_str());
        ListView_InsertItem(hCurveList, &item);
        return;
    }
    
    if (!curveSelectionModule) {
        LVITEM item = {};
        item.mask = LVIF_TEXT;
        item.iItem = 0;
        item.iSubItem = 0;
        item.pszText = const_cast<LPWSTR>(L"Модуль выбора кривых не инициализирован");
        ListView_InsertItem(hCurveList, &item);
        return;
    }
    
    // Получаем доступные кривые на основе текущего выбора
    std::vector<CurveDefinition> availableCurves = curveSelectionModule->GetAvailableCurves(currentSelection);
    
    if (availableCurves.empty()) {
        LVITEM item = {};
        item.mask = LVIF_TEXT;
        item.iItem = 0;
        item.iSubItem = 0;
        item.pszText = const_cast<LPWSTR>(L"Нет доступных кривых для данного выбора");
        ListView_InsertItem(hCurveList, &item);
        return;
    }
    
    // Настраиваем ListView для чекбоксов
    ListView_SetExtendedListViewStyle(hCurveList, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
    
    // Добавляем колонку
    LVCOLUMN column = {};
    column.mask = LVCF_TEXT | LVCF_WIDTH;
    column.cx = 350;
    column.pszText = const_cast<LPWSTR>(L"Кривая");
    ListView_InsertColumn(hCurveList, 0, &column);
    
    // Добавляем доступные кривые в ListView
    for (size_t i = 0; i < availableCurves.size(); i++) {
        const auto& curve = availableCurves[i];
        
        std::wstring displayText = std::wstring(curve.displayName.begin(), curve.displayName.end());
        displayText += L" (";
        displayText += std::wstring(curve.units.begin(), curve.units.end());
        displayText += L")";
        
        // Добавляем индикатор оси
        if (curve.yAxisGroup == 0) {
            displayText += L" [Левая ось]";
        } else {
            displayText += L" [Правая ось]";
        }
        
        LVITEM item = {};
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = static_cast<int>(i);
        item.iSubItem = 0;
        item.pszText = const_cast<LPWSTR>(displayText.c_str());
        item.lParam = static_cast<LPARAM>(i); // Сохраняем индекс кривой
        
        int itemIndex = ListView_InsertItem(hCurveList, &item);
        
        // Устанавливаем состояние чекбокса
        ListView_SetCheckState(hCurveList, itemIndex, curve.enabled ? TRUE : FALSE);
    }
    
    // Сохраняем доступные кривые для последующего использования
    currentAvailableCurves = availableCurves;
}

void SchematicModule::HandleFrequencyChange(HWND hDlg) {
    HWND hFreqCombo = GetDlgItem(hDlg, IDC_FREQUENCY_COMBO);
    if (hFreqCombo) {
        int selection = (int)SendMessage(hFreqCombo, CB_GETCURSEL, 0, 0);
        currentSelection.frequency = (selection == 0) ? FrequencyBand::FREQ_400_KHZ : FrequencyBand::FREQ_2000_KHZ;
        
        // Обновляем список доступных кривых
        UpdateAvailableCurves(hDlg);
    }
}

void SchematicModule::HandlePresetButton(HWND hDlg, int presetId) {
    // Очищаем текущий выбор
    currentSelection.Clear();
    
    // Применяем пресет
    switch (presetId) {
        case IDC_PRESET_ALL_AMP_R1:
            currentSelection.receivers[0] = true; // R1
            break;
        case IDC_PRESET_ALL_AMP_R2:
            currentSelection.receivers[1] = true; // R2
            break;
        case IDC_PRESET_ALL_PHASE_R1:
            currentSelection.receivers[0] = true; // R1
            break;
        case IDC_PRESET_ALL_PHASE_R2:
            currentSelection.receivers[1] = true; // R2
            break;
        case IDC_PRESET_RATIOS:
        case IDC_PRESET_DIFFERENCES:
            currentSelection.receivers[0] = true; // R1
            currentSelection.receivers[1] = true; // R2
            break;
    }
    
    // Обновляем отображение
    SetCurrentSelection(currentSelection);
    
    // Перерисовываем схему
    HWND hSchematicArea = GetDlgItem(hDlg, IDC_SCHEMATIC_AREA);
    if (hSchematicArea) {
        InvalidateRect(hSchematicArea, NULL, TRUE);
    }
    
    // Обновляем список кривых
    UpdateAvailableCurves(hDlg);
}

void SchematicModule::HandleCopyFromChart(HWND hDlg) {
    if (!curveSelectionModule) {
        MessageBox(hDlg, L"Модуль выбора кривых не инициализирован", L"Ошибка", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Определяем, с какого графика копировать
    int sourceChart = (targetChartNumber == 1) ? 2 : 1;
    
    // Проверяем, есть ли конфигурация у источника
    if (!curveSelectionModule->HasConfiguration(sourceChart)) {
        std::wstring message = L"График " + std::to_wstring(sourceChart) + L" не имеет настроек кривых для копирования";
        MessageBox(hDlg, message.c_str(), L"Информация", MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    // Спрашиваем подтверждение у пользователя
    std::wstring confirmMessage = L"Скопировать настройки с графика " + std::to_wstring(sourceChart) + 
                                 L" на график " + std::to_wstring(targetChartNumber) + L"?";
    
    int result = MessageBox(hDlg, confirmMessage.c_str(), L"Подтверждение копирования", 
                           MB_YESNO | MB_ICONQUESTION);
    
    if (result == IDYES) {
        // Копируем настройки
        curveSelectionModule->CopySettings(sourceChart, targetChartNumber);
        
        // Получаем скопированную конфигурацию
        ChartConfiguration copiedConfig = curveSelectionModule->GetChartConfiguration(targetChartNumber);
        
        // Обновляем текущие доступные кривые
        currentAvailableCurves = copiedConfig.curves;
        
        // Обновляем отображение списка кривых
        UpdateAvailableCurves(hDlg);
        
        // Показываем сообщение об успехе
        MessageBox(hDlg, L"Настройки успешно скопированы", L"Успех", MB_OK | MB_ICONINFORMATION);
    }
}

void SchematicModule::HandleMouseHover(HWND hDlg, int x, int y) {
    ElementRect* element = FindElementAt(x, y);
    
    if (element) {
        // Устанавливаем курсор "рука" для интерактивных элементов
        SetCursor(LoadCursor(NULL, IDC_HAND));
        
        // Можно добавить показ подсказки с информацией об элементе
        std::wstring tooltip = GetElementTooltip(element->type, element->index);
        // TODO: Показать tooltip (можно использовать SetWindowText для статусной строки)
    } else {
        // Обычный курсор
        SetCursor(LoadCursor(NULL, IDC_ARROW));
    }
}

void SchematicModule::HandleCurveCheckboxChange(HWND hDlg, int itemIndex) {
    if (itemIndex < 0 || itemIndex >= static_cast<int>(currentAvailableCurves.size())) {
        return;
    }
    
    HWND hCurveList = GetDlgItem(hDlg, IDC_CURVE_LIST);
    if (!hCurveList) return;
    
    // Получаем новое состояние чекбокса
    BOOL isChecked = ListView_GetCheckState(hCurveList, itemIndex);
    
    // Обновляем состояние кривой
    currentAvailableCurves[itemIndex].enabled = (isChecked == TRUE);
    
    // Можно добавить дополнительную логику, например:
    // - Проверка совместимости выбранных кривых
    // - Автоматическое назначение осей
    // - Предварительный просмотр конфигурации
    
    ValidateCurrentCurveConfiguration(hDlg);
}

void SchematicModule::ValidateCurrentCurveConfiguration(HWND hDlg) {
    if (!curveSelectionModule) return;
    
    // Создаем временную конфигурацию для валидации
    ChartConfiguration tempConfig;
    tempConfig.curves = currentAvailableCurves;
    tempConfig.UpdateAxisConfiguration();
    
    // Проверяем конфигурацию
    std::string error = curveSelectionModule->GetConfigurationError(tempConfig);
    
    // Обновляем состояние кнопки "Применить"
    HWND hApplyBtn = GetDlgItem(hDlg, IDC_APPLY_SETTINGS);
    if (hApplyBtn) {
        EnableWindow(hApplyBtn, error.empty() ? TRUE : FALSE);
    }
    
    // Можно показать сообщение об ошибке в статусной строке
    if (!error.empty()) {
        // TODO: Показать ошибку в статусной области диалога
        // Пока что просто обновляем заголовок окна
        std::wstring title = L"Схема прибора - График " + std::to_wstring(targetChartNumber);
        if (!error.empty()) {
            title += L" [Ошибка конфигурации]";
        }
        SetWindowText(hDlg, title.c_str());
    }
}

bool SchematicModule::ApplyCurrentCurveConfiguration() {
    if (!curveSelectionModule) return false;
    
    // Создаем конфигурацию из текущих кривых
    ChartConfiguration config;
    config.curves = currentAvailableCurves;
    config.UpdateAxisConfiguration();
    
    // Валидируем конфигурацию
    if (!curveSelectionModule->ValidateConfiguration(config)) {
        return false;
    }
    
    // Применяем конфигурацию к соответствующему графику
    curveSelectionModule->ApplyCurveConfiguration(targetChartNumber, config);
    
    return true;
}

// Дополнительные методы отрисовки схемы
void SchematicModule::DrawSchematicConnections(HDC hdc, const RECT& rect) {
    // Создаем перо для соединительных линий
    HPEN connectionPen = CreatePen(PS_DOT, 1, RGB(120, 120, 120));
    HPEN oldPen = (HPEN)SelectObject(hdc, connectionPen);
    
    // Находим позиции приемников для рисования линий
    RECT r1Rect, r2Rect;
    bool foundR1 = false, foundR2 = false;
    std::vector<RECT> transmitterRects;
    
    for (const auto& element : schematicElements) {
        if (element.type == ElementType::RECEIVER) {
            if (element.index == 0) {
                r1Rect = element.rect;
                foundR1 = true;
            } else if (element.index == 1) {
                r2Rect = element.rect;
                foundR2 = true;
            }
        } else if (element.type == ElementType::TRANSMITTER) {
            transmitterRects.push_back(element.rect);
        }
    }
    
    // Рисуем соединительные линии от передатчиков к приемникам
    if (foundR1 && foundR2 && !transmitterRects.empty()) {
        for (const auto& txRect : transmitterRects) {
            int txCenterX = txRect.left + (txRect.right - txRect.left) / 2;
            int txBottomY = txRect.bottom;
            
            // Рисуем изогнутые линии к приемникам
            int r1CenterX = r1Rect.left + (r1Rect.right - r1Rect.left) / 2;
            int r1TopY = r1Rect.top;
            int r2CenterX = r2Rect.left + (r2Rect.right - r2Rect.left) / 2;
            int r2TopY = r2Rect.top;
            
            // Промежуточная точка для изогнутых линий
            int midY = txBottomY + (r1TopY - txBottomY) / 2;
            
            // Линия к R1 (изогнутая)
            MoveToEx(hdc, txCenterX, txBottomY, NULL);
            LineTo(hdc, txCenterX, midY);
            LineTo(hdc, r1CenterX, midY);
            LineTo(hdc, r1CenterX, r1TopY);
            
            // Линия к R2 (изогнутая)
            MoveToEx(hdc, txCenterX, txBottomY, NULL);
            LineTo(hdc, txCenterX, midY);
            LineTo(hdc, r2CenterX, midY);
            LineTo(hdc, r2CenterX, r2TopY);
        }
    }
    
    // Рисуем стрелки на концах линий для указания направления сигнала
    DrawConnectionArrows(hdc, transmitterRects, r1Rect, r2Rect);
    
    SelectObject(hdc, oldPen);
    DeleteObject(connectionPen);
}

void SchematicModule::DrawGroupLabels(HDC hdc, const RECT& rect) {
    // Устанавливаем шрифт для подписей
    HFONT labelFont = CreateFont(14, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
    HFONT oldFont = (HFONT)SelectObject(hdc, labelFont);
    
    SetTextColor(hdc, RGB(60, 60, 60));
    SetBkMode(hdc, TRANSPARENT);
    
    // Подпись для передатчиков
    RECT transmitterLabelRect = {rect.left + 10, rect.top + 5, rect.right - 10, rect.top + 20};
    DrawText(hdc, L"Передатчики", -1, &transmitterLabelRect, DT_LEFT | DT_SINGLELINE);
    
    // Подпись для приемников
    RECT receiverLabelRect = {rect.left + 10, rect.top + 65, rect.right - 10, rect.top + 80};
    DrawText(hdc, L"Приемники", -1, &receiverLabelRect, DT_LEFT | DT_SINGLELINE);
    
    // Подпись для молчащих передатчиков
    RECT silentLabelRect = {rect.left + 10, rect.top + 125, rect.right - 10, rect.top + 140};
    DrawText(hdc, L"Молчащие передатчики", -1, &silentLabelRect, DT_LEFT | DT_SINGLELINE);
    
    SelectObject(hdc, oldFont);
    DeleteObject(labelFont);
}

void SchematicModule::DrawConnectionArrows(HDC hdc, const std::vector<RECT>& transmitterRects, 
                                          const RECT& r1Rect, const RECT& r2Rect) {
    // Создаем перо для стрелок
    HPEN arrowPen = CreatePen(PS_SOLID, 2, RGB(80, 80, 80));
    HPEN oldPen = (HPEN)SelectObject(hdc, arrowPen);
    HBRUSH arrowBrush = CreateSolidBrush(RGB(80, 80, 80));
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, arrowBrush);
    
    int arrowSize = 6;
    
    // Рисуем стрелки на приемниках (указывают на входящий сигнал)
    int r1CenterX = r1Rect.left + (r1Rect.right - r1Rect.left) / 2;
    int r1TopY = r1Rect.top;
    int r2CenterX = r2Rect.left + (r2Rect.right - r2Rect.left) / 2;
    int r2TopY = r2Rect.top;
    
    // Стрелка на R1
    POINT r1Arrow[3] = {
        {r1CenterX, r1TopY - 2},
        {r1CenterX - arrowSize, r1TopY - arrowSize - 2},
        {r1CenterX + arrowSize, r1TopY - arrowSize - 2}
    };
    Polygon(hdc, r1Arrow, 3);
    
    // Стрелка на R2
    POINT r2Arrow[3] = {
        {r2CenterX, r2TopY - 2},
        {r2CenterX - arrowSize, r2TopY - arrowSize - 2},
        {r2CenterX + arrowSize, r2TopY - arrowSize - 2}
    };
    Polygon(hdc, r2Arrow, 3);
    
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(arrowBrush);
    DeleteObject(arrowPen);
}

// Методы для интеллектуальной обработки выбора элементов
bool SchematicModule::IsSelectionCompatible(ElementType type, int index) const {
    // Проверяем правила совместимости выбора элементов
    
    // Правило 1: Нельзя одновременно выбирать передатчики и приемники
    if (type == ElementType::TRANSMITTER) {
        return currentSelection.GetSelectedReceiversCount() == 0 && 
               currentSelection.GetSelectedSilentCount() == 0;
    }
    
    if (type == ElementType::RECEIVER) {
        return currentSelection.GetSelectedTransmittersCount() == 0 && 
               currentSelection.GetSelectedSilentCount() == 0;
    }
    
    // Правило 2: Молчащие передатчики нельзя комбинировать с активными элементами
    if (type == ElementType::SILENT_TX) {
        return currentSelection.GetSelectedTransmittersCount() == 0 && 
               currentSelection.GetSelectedReceiversCount() == 0;
    }
    
    return true;
}

void SchematicModule::ClearIncompatibleSelections(ElementType newType) {
    // Очищаем несовместимые выборы в зависимости от нового типа
    
    if (newType == ElementType::TRANSMITTER) {
        // Очищаем приемники и молчащие передатчики
        std::fill(currentSelection.receivers.begin(), currentSelection.receivers.end(), false);
        std::fill(currentSelection.silentTx.begin(), currentSelection.silentTx.end(), false);
    }
    else if (newType == ElementType::RECEIVER) {
        // Очищаем передатчики и молчащие передатчики
        std::fill(currentSelection.transmitters.begin(), currentSelection.transmitters.end(), false);
        std::fill(currentSelection.silentTx.begin(), currentSelection.silentTx.end(), false);
    }
    else if (newType == ElementType::SILENT_TX) {
        // Очищаем все активные элементы
        std::fill(currentSelection.transmitters.begin(), currentSelection.transmitters.end(), false);
        std::fill(currentSelection.receivers.begin(), currentSelection.receivers.end(), false);
    }
}

void SchematicModule::SynchronizeElementStates() {
    // Синхронизируем визуальное состояние элементов с логическим состоянием
    for (auto& element : schematicElements) {
        switch (element.type) {
            case ElementType::TRANSMITTER:
                if (element.index < currentSelection.transmitters.size()) {
                    element.selected = currentSelection.transmitters[element.index];
                }
                break;
            case ElementType::RECEIVER:
                if (element.index < currentSelection.receivers.size()) {
                    element.selected = currentSelection.receivers[element.index];
                }
                break;
            case ElementType::SILENT_TX:
                if (element.index < currentSelection.silentTx.size()) {
                    element.selected = currentSelection.silentTx[element.index];
                }
                break;
        }
    }
}

// Процедура диалогового окна схемы прибора
INT_PTR CALLBACK SchematicDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    static SchematicModule* pSchematic = nullptr;
    
    switch (message) {
        case WM_INITDIALOG:
            {
                // Получаем указатель на объект SchematicModule
                pSchematic = reinterpret_cast<SchematicModule*>(lParam);
                SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
                
                // Устанавливаем заголовок окна
                std::wstring title = L"Схема прибора - График " + std::to_wstring(pSchematic->targetChartNumber);
                SetWindowText(hDlg, title.c_str());
                
                // Инициализируем элементы управления
                pSchematic->InitializeDialogControls(hDlg);
                
                // Центрируем окно
                RECT parentRect, dialogRect;
                HWND hParent = GetParent(hDlg);
                GetWindowRect(hParent, &parentRect);
                GetWindowRect(hDlg, &dialogRect);
                
                int x = parentRect.left + (parentRect.right - parentRect.left - 
                       (dialogRect.right - dialogRect.left)) / 2;
                int y = parentRect.top + (parentRect.bottom - parentRect.top - 
                       (dialogRect.bottom - dialogRect.top)) / 2;
                
                SetWindowPos(hDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                
                return (INT_PTR)TRUE;
            }
            
        case WM_PAINT:
            {
                if (pSchematic) {
                    PAINTSTRUCT ps;
                    HDC hdc = BeginPaint(hDlg, &ps);
                    
                    // Получаем область для отрисовки схемы
                    HWND hSchematicArea = GetDlgItem(hDlg, IDC_SCHEMATIC_AREA);
                    if (hSchematicArea) {
                        RECT schematicRect;
                        GetClientRect(hSchematicArea, &schematicRect);
                        MapWindowPoints(hSchematicArea, hDlg, (POINT*)&schematicRect, 2);
                        
                        // Проверяем, пересекается ли область схемы с областью перерисовки
                        RECT intersectRect;
                        if (IntersectRect(&intersectRect, &schematicRect, &ps.rcPaint)) {
                            // Отрисовываем схему прибора только если нужно
                            pSchematic->RenderSchematic(hdc, schematicRect);
                        }
                    }
                    
                    EndPaint(hDlg, &ps);
                }
                return (INT_PTR)TRUE;
            }
            
        case WM_LBUTTONDOWN:
            {
                if (pSchematic) {
                    int x = LOWORD(lParam);
                    int y = HIWORD(lParam);
                    
                    // Проверяем, попал ли клик в область схемы
                    HWND hSchematicArea = GetDlgItem(hDlg, IDC_SCHEMATIC_AREA);
                    if (hSchematicArea) {
                        RECT schematicRect;
                        GetClientRect(hSchematicArea, &schematicRect);
                        MapWindowPoints(hSchematicArea, hDlg, (POINT*)&schematicRect, 2);
                        
                        POINT pt = {x, y};
                        if (PtInRect(&schematicRect, pt)) {
                            // Преобразуем координаты относительно области схемы
                            x -= schematicRect.left;
                            y -= schematicRect.top;
                            
                            if (pSchematic->HandleElementClick(x, y)) {
                                // Обновляем отображение схемы
                                InvalidateRect(hDlg, &schematicRect, TRUE);
                                
                                // Обновляем список доступных кривых
                                pSchematic->UpdateAvailableCurves(hDlg);
                            }
                        }
                    }
                }
                return (INT_PTR)TRUE;
            }
            
        case WM_MOUSEMOVE:
            {
                if (pSchematic) {
                    int x = LOWORD(lParam);
                    int y = HIWORD(lParam);
                    
                    // Проверяем, находится ли курсор над элементом схемы
                    HWND hSchematicArea = GetDlgItem(hDlg, IDC_SCHEMATIC_AREA);
                    if (hSchematicArea) {
                        RECT schematicRect;
                        GetClientRect(hSchematicArea, &schematicRect);
                        MapWindowPoints(hSchematicArea, hDlg, (POINT*)&schematicRect, 2);
                        
                        POINT pt = {x, y};
                        if (PtInRect(&schematicRect, pt)) {
                            // Преобразуем координаты относительно области схемы
                            x -= schematicRect.left;
                            y -= schematicRect.top;
                            
                            // Обновляем курсор в зависимости от того, над каким элементом находимся
                            pSchematic->HandleMouseHover(hDlg, x, y);
                        }
                    }
                }
                return (INT_PTR)TRUE;
            }
            
        case WM_NOTIFY:
            {
                if (pSchematic) {
                    LPNMHDR pnmh = (LPNMHDR)lParam;
                    if (pnmh->idFrom == IDC_CURVE_LIST && pnmh->code == LVN_ITEMCHANGED) {
                        LPNMLISTVIEW pnmlv = (LPNMLISTVIEW)lParam;
                        
                        // Проверяем, изменилось ли состояние чекбокса
                        if ((pnmlv->uChanged & LVIF_STATE) && 
                            ((pnmlv->uNewState & LVIS_STATEIMAGEMASK) != (pnmlv->uOldState & LVIS_STATEIMAGEMASK))) {
                            
                            pSchematic->HandleCurveCheckboxChange(hDlg, pnmlv->iItem);
                        }
                    }
                }
                return (INT_PTR)TRUE;
            }
            
        case WM_COMMAND:
            {
                int wmId = LOWORD(wParam);
                
                switch (wmId) {
                    case IDC_APPLY_SETTINGS:
                    case IDOK:
                        if (pSchematic && pSchematic->ValidateCurrentSelection()) {
                            // Применяем конфигурацию кривых
                            if (pSchematic->ApplyCurrentCurveConfiguration()) {
                                EndDialog(hDlg, IDOK);
                            } else {
                                MessageBox(hDlg, L"Ошибка применения конфигурации кривых", L"Ошибка", MB_OK | MB_ICONERROR);
                            }
                        } else {
                            // Показываем сообщение об ошибке
                            if (pSchematic) {
                                std::string error = pSchematic->GetCurrentSelection().GetValidationError();
                                std::wstring werror(error.begin(), error.end());
                                MessageBox(hDlg, werror.c_str(), L"Ошибка выбора", MB_OK | MB_ICONWARNING);
                            }
                        }
                        return (INT_PTR)TRUE;
                        
                    case IDCANCEL:
                        EndDialog(hDlg, IDCANCEL);
                        return (INT_PTR)TRUE;
                        
                    case IDC_FREQUENCY_COMBO:
                        if (HIWORD(wParam) == CBN_SELCHANGE && pSchematic) {
                            pSchematic->HandleFrequencyChange(hDlg);
                        }
                        return (INT_PTR)TRUE;
                        
                    // Обработка быстрых пресетов
                    case IDC_PRESET_ALL_AMP_R1:
                    case IDC_PRESET_ALL_AMP_R2:
                    case IDC_PRESET_ALL_PHASE_R1:
                    case IDC_PRESET_ALL_PHASE_R2:
                    case IDC_PRESET_RATIOS:
                    case IDC_PRESET_DIFFERENCES:
                        if (pSchematic) {
                            pSchematic->HandlePresetButton(hDlg, wmId);
                        }
                        return (INT_PTR)TRUE;
                        
                    case IDC_COPY_FROM_CHART:
                        if (pSchematic) {
                            pSchematic->HandleCopyFromChart(hDlg);
                        }
                        return (INT_PTR)TRUE;
                }
                break;
            }
    }
    return (INT_PTR)FALSE;
}