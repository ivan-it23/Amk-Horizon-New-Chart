#include "framework.h"
#include "SchematicModule.h"
#include "CurveSelectionModule.h"
#include "Resource.h"
#include <sstream>
#include <commctrl.h>
#include <algorithm>
#include <map>

// Определение цветов для элементов схемы
const COLORREF SchematicModule::TRANSMITTER_COLOR = RGB(180, 100, 100);  // Приглушенный красный
const COLORREF SchematicModule::RECEIVER_COLOR = RGB(100, 180, 100);     // Приглушенный зеленый  
const COLORREF SchematicModule::SILENT_TX_COLOR = RGB(128, 128, 128);    // Серый
const COLORREF SchematicModule::SELECTION_COLOR = RGB(255, 255, 0);      // Желтый для выделения

// Константы для рамки основных элементов
const COLORREF FRAME_BACKGROUND = RGB(245, 245, 245);  // Светло-серый фон
const COLORREF FRAME_BORDER = RGB(180, 180, 180);      // Тёмно-серая граница
const int FRAME_PADDING = 15;                          // Отступ внутри рамки
const int FRAME_BORDER_WIDTH = 1;                      // Толщина границы
const int FRAME_CORNER_RADIUS = 8;                     // Радиус скругления углов

// Константы для градиентов элементов
// Передатчики (Transmitters)
const COLORREF TX_GRADIENT_START = RGB(200, 120, 120);
const COLORREF TX_GRADIENT_END = RGB(170, 90, 90);
const COLORREF TX_ACTIVE_GRADIENT_START = RGB(230, 130, 130);
const COLORREF TX_ACTIVE_GRADIENT_END = RGB(200, 100, 100);
const COLORREF TX_HOVER_GRADIENT_START = RGB(215, 125, 125);
const COLORREF TX_HOVER_GRADIENT_END = RGB(185, 95, 95);

// Приемники (Receivers)
const COLORREF RX_GRADIENT_START = RGB(120, 200, 120);
const COLORREF RX_GRADIENT_END = RGB(90, 170, 90);
const COLORREF RX_ACTIVE_GRADIENT_START = RGB(130, 230, 130);
const COLORREF RX_ACTIVE_GRADIENT_END = RGB(100, 200, 100);
const COLORREF RX_HOVER_GRADIENT_START = RGB(125, 215, 125);
const COLORREF RX_HOVER_GRADIENT_END = RGB(95, 185, 95);

// Молчащие передатчики (Silent)
const COLORREF SILENT_GRADIENT_START = RGB(160, 160, 160);
const COLORREF SILENT_GRADIENT_END = RGB(130, 130, 130);
const COLORREF SILENT_ACTIVE_GRADIENT_START = RGB(180, 180, 180);
const COLORREF SILENT_ACTIVE_GRADIENT_END = RGB(150, 150, 150);
const COLORREF SILENT_HOVER_GRADIENT_START = RGB(170, 170, 170);
const COLORREF SILENT_HOVER_GRADIENT_END = RGB(140, 140, 140);

// Константы для тени
const int SHADOW_OFFSET_X = 2;
const int SHADOW_OFFSET_Y = 2;
const COLORREF SHADOW_COLOR = RGB(200, 200, 200);  // Светло-серая тень

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
SchematicSelection::SchematicSelection() : frequency(FrequencyBand::FREQ_400_KHZ), dataFilter(DataTypeFilter::ALL) {
    transmitters.resize(4, false);  // T1-T4
    receivers.resize(2, false);     // R1-R2
    silentTx.resize(2, false);      // Молчащие для R1 и R2
}

void SchematicSelection::Clear() {
    std::fill(transmitters.begin(), transmitters.end(), false);
    std::fill(receivers.begin(), receivers.end(), false);
    std::fill(silentTx.begin(), silentTx.end(), false);
    frequency = FrequencyBand::FREQ_400_KHZ;
    dataFilter = DataTypeFilter::ALL;
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
    
    // Для пресетов (dataFilter != ALL) валидация всегда проходит
    // Кривые генерируются на основе фильтра, а не выбранных элементов
    if (dataFilter != DataTypeFilter::ALL) {
        return ""; // Пресет всегда валиден
    }
    
    // Для ручного выбора (dataFilter == ALL) проверяем выбранные элементы
    // Должен быть выбран хотя бы один элемент
    if (receivers == 0 && transmitters == 0 && silent == 0) {
        return "Выберите элементы для построения графика";
    }
    
    // Нельзя одновременно выбирать передатчики и приемники
    if (transmitters > 0 && receivers > 0) {
        return "Нельзя одновременно выбирать передатчики и приемники";
    }
    
    // ИСПРАВЛЕНО: Разрешаем комбинации молчащих передатчиков с активными элементами
    // Запрещена только комбинация приемников R1+R2 с молчащими передатчиками
    if (silent > 0 && receivers == 2) {
        return "Нельзя одновременно выбирать оба приемника и молчащие передатчики";
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
SchematicModule::SchematicModule() 
    : curveSelectionModule(nullptr), targetChartNumber(1), hoveredElement(nullptr) {
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

void SchematicModule::DrawElementShadow(HDC hdc, const RECT& rect) {
    // Создаём смещённый прямоугольник для тени
    RECT shadowRect = {
        rect.left + SHADOW_OFFSET_X,
        rect.top + SHADOW_OFFSET_Y,
        rect.right + SHADOW_OFFSET_X,
        rect.bottom + SHADOW_OFFSET_Y
    };
    
    // Рисуем полупрозрачную тень
    HBRUSH shadowBrush = CreateSolidBrush(SHADOW_COLOR);
    HPEN shadowPen = CreatePen(PS_SOLID, 1, SHADOW_COLOR);
    
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, shadowBrush);
    HPEN oldPen = (HPEN)SelectObject(hdc, shadowPen);
    
    // Рисуем скруглённый прямоугольник для тени
    RoundRect(hdc, shadowRect.left, shadowRect.top,
              shadowRect.right, shadowRect.bottom, 8, 8);
    
    // Восстанавливаем объекты GDI
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(shadowBrush);
    DeleteObject(shadowPen);
}

void SchematicModule::DrawGradientRect(HDC hdc, const RECT& rect, COLORREF colorStart, COLORREF colorEnd) {
    // Простая эмуляция вертикального градиента через GDI
    // Рисуем множество горизонтальных линий с плавным переходом цвета
    
    int height = rect.bottom - rect.top;
    if (height <= 0) return;
    
    // Извлекаем компоненты цветов
    int r1 = GetRValue(colorStart), g1 = GetGValue(colorStart), b1 = GetBValue(colorStart);
    int r2 = GetRValue(colorEnd), g2 = GetGValue(colorEnd), b2 = GetBValue(colorEnd);
    
    // Рисуем градиент построчно
    for (int y = 0; y < height; y++) {
        // Вычисляем промежуточный цвет
        float ratio = (float)y / (float)height;
        int r = r1 + (int)((r2 - r1) * ratio);
        int g = g1 + (int)((g2 - g1) * ratio);
        int b = b1 + (int)((b2 - b1) * ratio);
        
        COLORREF color = RGB(r, g, b);
        HPEN pen = CreatePen(PS_SOLID, 1, color);
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        
        // Рисуем горизонтальную линию
        MoveToEx(hdc, rect.left, rect.top + y, NULL);
        LineTo(hdc, rect.right, rect.top + y);
        
        SelectObject(hdc, oldPen);
        DeleteObject(pen);
    }
}

void SchematicModule::RenderSchematic(HDC hdc, const RECT& rect) {
    // Используем двойную буферизацию для плавной отрисовки без мерцания
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    
    // Создаём буфер в памяти
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);
    
    // Смещаем координаты для рисования в буфере
    SetViewportOrgEx(memDC, -rect.left, -rect.top, NULL);
    
    // Очищаем фон в буфере
    HBRUSH bgBrush = CreateSolidBrush(RGB(240, 240, 240));
    FillRect(memDC, &rect, bgBrush);
    DeleteObject(bgBrush);
    
    // Рассчитываем компоновку элементов (только если нужно)
    if (schematicElements.empty()) {
        CalculateSchematicLayout(rect);
    }
    
    // Отрисовываем рамку для основных элементов (T1-T4, R1-R2)
    if (deviceFrameRect.right > deviceFrameRect.left && 
        deviceFrameRect.bottom > deviceFrameRect.top) {
        DrawDeviceFrame(memDC, deviceFrameRect);
    }
    
    // Отрисовываем все элементы схемы
    for (const auto& element : schematicElements) {
        std::wstring label = GetElementLabel(element.type, element.index);
        DrawElement(memDC, element.rect, element.type, element.index, element.selected, element.hovered, label);
    }
    
    // Отрисовываем соединительные линии между элементами
    DrawSchematicConnections(memDC, rect);
    
    // Добавляем подписи групп элементов
    DrawGroupLabels(memDC, rect);
    
    // Копируем буфер на экран
    BitBlt(hdc, rect.left, rect.top, width, height, memDC, rect.left, rect.top, SRCCOPY);
    
    // Очищаем ресурсы
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

void SchematicModule::DrawElement(HDC hdc, const RECT& elementRect, ElementType type, 
                                 int index, bool selected, bool hovered, const std::wstring& label) {
    // Определяем цвета градиента и рамки в зависимости от типа и состояния
    COLORREF gradientStart, gradientEnd, borderColor;
    int borderWidth = 2;
    
    // Выбираем цвета в зависимости от типа элемента и его состояния
    switch (type) {
        case ElementType::TRANSMITTER:
            if (selected) {
                gradientStart = TX_ACTIVE_GRADIENT_START;
                gradientEnd = TX_ACTIVE_GRADIENT_END;
                borderColor = RGB(180, 60, 60);
                borderWidth = 3;
            } else if (hovered) {
                gradientStart = TX_HOVER_GRADIENT_START;
                gradientEnd = TX_HOVER_GRADIENT_END;
                borderColor = RGB(160, 70, 70);
            } else {
                gradientStart = TX_GRADIENT_START;
                gradientEnd = TX_GRADIENT_END;
                borderColor = RGB(140, 80, 80);
            }
            break;
            
        case ElementType::RECEIVER:
            if (selected) {
                gradientStart = RX_ACTIVE_GRADIENT_START;
                gradientEnd = RX_ACTIVE_GRADIENT_END;
                borderColor = RGB(60, 180, 60);
                borderWidth = 3;
            } else if (hovered) {
                gradientStart = RX_HOVER_GRADIENT_START;
                gradientEnd = RX_HOVER_GRADIENT_END;
                borderColor = RGB(70, 160, 70);
            } else {
                gradientStart = RX_GRADIENT_START;
                gradientEnd = RX_GRADIENT_END;
                borderColor = RGB(80, 140, 80);
            }
            break;
            
        case ElementType::SILENT_TX:
            if (selected) {
                gradientStart = SILENT_ACTIVE_GRADIENT_START;
                gradientEnd = SILENT_ACTIVE_GRADIENT_END;
                borderColor = RGB(80, 80, 80);
                borderWidth = 3;
            } else if (hovered) {
                gradientStart = SILENT_HOVER_GRADIENT_START;
                gradientEnd = SILENT_HOVER_GRADIENT_END;
                borderColor = RGB(90, 90, 90);
            } else {
                gradientStart = SILENT_GRADIENT_START;
                gradientEnd = SILENT_GRADIENT_END;
                borderColor = RGB(100, 100, 100);
            }
            break;
            
        default:
            gradientStart = RGB(200, 200, 200);
            gradientEnd = RGB(180, 180, 180);
            borderColor = RGB(100, 100, 100);
    }
    
    // Рисуем тень элемента
    DrawElementShadow(hdc, elementRect);
    
    // Рисуем градиентную заливку
    DrawGradientRect(hdc, elementRect, gradientStart, gradientEnd);
    
    // Рисуем рамку поверх градиента
    HPEN borderPen = CreatePen(PS_SOLID, borderWidth, borderColor);
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    
    // Рисуем скругленный прямоугольник для рамки
    int cornerRadius = 8;
    RoundRect(hdc, elementRect.left, elementRect.top, elementRect.right, elementRect.bottom,
              cornerRadius, cornerRadius);
    
    // Восстанавливаем объекты GDI
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
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
    
    // ОТЛАДКА: Выводим информацию о клике
    char debugMsg[256];
    sprintf_s(debugMsg, "=== HandleElementClick ===\n");
    OutputDebugStringA(debugMsg);
    sprintf_s(debugMsg, "Click: Type=%d, Index=%d, CurrentlySelected=%s\n", 
             static_cast<int>(element->type), element->index,
             element->selected ? "true" : "false");
    OutputDebugStringA(debugMsg);
    
    // Проверяем логику выбора элементов
    bool newState = !element->selected;
    
    // ИСПРАВЛЕНО: Применяем правила взаимоисключения ТОЛЬКО если выбираем новый элемент
    // И ТОЛЬКО если есть реальная несовместимость
    if (newState && !IsSelectionCompatible(element->type, element->index)) {
        sprintf_s(debugMsg, "Selection not compatible, clearing incompatible selections\n");
        OutputDebugStringA(debugMsg);
        
        // Очищаем несовместимые выборы
        ClearIncompatibleSelections(element->type);
    }
    
    // Обновляем состояние элемента
    element->selected = newState;
    UpdateSelection(element->type, element->index, newState);
    
    // При ручном выборе элементов сбрасываем фильтр на ALL
    currentSelection.dataFilter = DataTypeFilter::ALL;
    
    // Обновляем визуальное состояние всех элементов
    SynchronizeElementStates();
    
    // ОТЛАДКА: Выводим итоговое состояние
    OutputDebugStringA("Final selection state after click:\n");
    for (int i = 0; i < 4; i++) {
        sprintf_s(debugMsg, "  T%d: %s\n", i + 1, currentSelection.transmitters[i] ? "selected" : "not selected");
        OutputDebugStringA(debugMsg);
    }
    for (int i = 0; i < 2; i++) {
        sprintf_s(debugMsg, "  R%d: %s\n", i + 1, currentSelection.receivers[i] ? "selected" : "not selected");
        OutputDebugStringA(debugMsg);
        sprintf_s(debugMsg, "  Silent R%d: %s\n", i + 1, currentSelection.silentTx[i] ? "selected" : "not selected");
        OutputDebugStringA(debugMsg);
    }
    OutputDebugStringA("=== END HandleElementClick ===\n");
    
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

void SchematicModule::UpdateHoverState(int x, int y) {
    // Находим элемент под курсором
    ElementRect* newHovered = FindElementAt(x, y);
    
    // Если hover состояние изменилось
    if (newHovered != hoveredElement) {
        // Сбрасываем предыдущий hover
        if (hoveredElement) {
            hoveredElement->hovered = false;
        }
        
        // Устанавливаем новый hover
        if (newHovered) {
            newHovered->hovered = true;
        }
        
        hoveredElement = newHovered;
        
        // Перерисовка будет вызвана из обработчика WM_MOUSEMOVE
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
    DrawElement(hdc, rect, ElementType::TRANSMITTER, index, selected, false, label);
}

void SchematicModule::DrawReceiver(HDC hdc, const RECT& rect, int index, bool selected) {
    std::wstring label = L"R" + std::to_wstring(index + 1);
    DrawElement(hdc, rect, ElementType::RECEIVER, index, selected, false, label);
}

void SchematicModule::DrawSilentTransmitter(HDC hdc, const RECT& rect, int index, bool selected) {
    std::wstring label = L"Silent R" + std::to_wstring(index + 1);
    DrawElement(hdc, rect, ElementType::SILENT_TX, index, selected, false, label);
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
    const int elementWidth = 70;
    const int elementHeight = 45;
    const int spacing = 12;  // Отступ между элементами
    const int frameToSilentSpacing = 25;  // Отступ между рамкой и молчащими
    
    // Рассчитываем ширину рамки (6 элементов: T1-T4, R1-R2)
    const int frameElementsCount = 6;
    const int frameWidth = frameElementsCount * elementWidth + (frameElementsCount - 1) * spacing + 2 * FRAME_PADDING;
    
    // Рассчитываем общую ширину (рамка + отступ + 2 молчащих)
    const int totalWidth = frameWidth + frameToSilentSpacing + 2 * elementWidth + spacing;
    
    // Центрируем всю компоновку
    const int startX = schemaRect.left + (schemaRect.right - schemaRect.left - totalWidth) / 2;
    const int centerY = schemaRect.top + (schemaRect.bottom - schemaRect.top - elementHeight) / 2;
    
    // Определяем позицию элемента в горизонтальной линии
    // ПРАВИЛЬНЫЙ порядок: T3(0), T1(1), R1(2), R2(3), T2(4), T4(5), Silent R1(6), Silent R2(7)
    int positionIndex = -1;
    int currentX = startX + FRAME_PADDING;  // Начинаем внутри рамки
    
    switch (type) {
        case ElementType::TRANSMITTER:
            {
                // Передатчики в порядке: T3(pos 0), T1(pos 1), T2(pos 4), T4(pos 5)
                switch (index) {
                    case 0: positionIndex = 1; break; // T1 -> позиция 1
                    case 1: positionIndex = 4; break; // T2 -> позиция 4
                    case 2: positionIndex = 0; break; // T3 -> позиция 0
                    case 3: positionIndex = 5; break; // T4 -> позиция 5
                }
                currentX += positionIndex * (elementWidth + spacing);
            }
            break;
            
        case ElementType::RECEIVER:
            {
                // Приемники R1-R2 в центре (позиции 2-3)
                positionIndex = 2 + index;  // R1=2, R2=3
                currentX += positionIndex * (elementWidth + spacing);
            }
            break;
            
        case ElementType::SILENT_TX:
            {
                // Молчащие передатчики вне рамки (позиции 6-7)
                positionIndex = 6 + index;  // Silent R1=6, Silent R2=7
                // Начинаем после рамки
                currentX = startX + frameWidth + frameToSilentSpacing + index * (elementWidth + spacing);
            }
            break;
    }
    
    // Вычисляем координаты
    if (positionIndex >= 0) {
        rect.left = currentX;
        rect.top = centerY;
        rect.right = rect.left + elementWidth;
        rect.bottom = rect.top + elementHeight;
    } else {
        // Fallback на случай ошибки
        rect.left = schemaRect.left + 20;
        rect.top = centerY;
        rect.right = rect.left + elementWidth;
        rect.bottom = rect.top + elementHeight;
    }
    
    return rect;
}

void SchematicModule::CalculateSchematicLayout(const RECT& schemaRect) {
    InitializeSchematicElements(schemaRect);
    
    // Рассчитываем координаты рамки для основных элементов (T1-T4, R1-R2)
    // Рамка должна охватывать элементы с позициями 0-5 (T3, T1, R1, R2, T2, T4)
    
    if (schematicElements.size() >= 6) {
        // Находим минимальные и максимальные координаты для первых 6 элементов
        int minX = INT_MAX, minY = INT_MAX;
        int maxX = INT_MIN, maxY = INT_MIN;
        
        for (size_t i = 0; i < 6; i++) {
            const RECT& rect = schematicElements[i].rect;
            minX = (minX < (int)rect.left) ? minX : (int)rect.left;
            minY = (minY < (int)rect.top) ? minY : (int)rect.top;
            maxX = (maxX > (int)rect.right) ? maxX : (int)rect.right;
            maxY = (maxY > (int)rect.bottom) ? maxY : (int)rect.bottom;
        }
        
        // Добавляем отступы для рамки
        deviceFrameRect.left = minX - FRAME_PADDING;
        deviceFrameRect.top = minY - FRAME_PADDING;
        deviceFrameRect.right = maxX + FRAME_PADDING;
        deviceFrameRect.bottom = maxY + FRAME_PADDING;
    } else {
        // Если элементов меньше 6, рамка не отрисовывается
        deviceFrameRect = {0, 0, 0, 0};
    }
}

// Методы для работы с диалоговым окном
void SchematicModule::InitializeDialogControls(HWND hDlg) {
    // Пытаемся восстановить текущую конфигурацию графика
    bool hasExistingConfig = false;
    if (curveSelectionModule && curveSelectionModule->HasConfiguration(targetChartNumber)) {
        // Получаем текущую конфигурацию графика
        ChartConfiguration existingConfig = curveSelectionModule->GetChartConfiguration(targetChartNumber);
        
        if (!existingConfig.IsEmpty()) {
            hasExistingConfig = true;
            
            // Очищаем текущий выбор
            currentSelection.Clear();
            
            // Восстанавливаем частоту из первой кривой
            if (!existingConfig.curves.empty()) {
                currentSelection.frequency = existingConfig.curves[0].frequency;
            }
            
            // Определяем фильтр типов данных по существующим кривым
            currentSelection.dataFilter = DetermineDataFilterFromCurves(existingConfig.curves);
            
            // ВАЖНО: Восстанавливаем элементы схемы ТОЛЬКО для режима ручного выбора (dataFilter == ALL)
            // Для пресетов (dataFilter != ALL) элементы схемы должны оставаться неактивными
            if (currentSelection.dataFilter == DataTypeFilter::ALL) {
                // Режим ручного выбора - восстанавливаем элементы схемы из кривых
                
                // ОТЛАДКА: Выводим информацию о восстанавливаемых кривых
                OutputDebugStringA("=== RESTORING CONFIGURATION ===\n");
                char debugMsg[256];
                sprintf_s(debugMsg, "Total curves to restore: %zu\n", existingConfig.curves.size());
                OutputDebugStringA(debugMsg);
                
                for (const auto& curve : existingConfig.curves) {
                    // ОТЛАДКА: Информация о каждой кривой
                    sprintf_s(debugMsg, "Curve: %s, SourceElement: %d, ElementIndex: %d, Enabled: %s\n", 
                             curve.id.c_str(), 
                             static_cast<int>(curve.sourceElement), 
                             curve.elementIndex,
                             curve.enabled ? "true" : "false");
                    OutputDebugStringA(debugMsg);
                    
                    switch (curve.sourceElement) {
                        case ElementType::TRANSMITTER:
                            if (curve.elementIndex >= 0 && curve.elementIndex < 4) {
                                currentSelection.transmitters[curve.elementIndex] = true;
                                sprintf_s(debugMsg, "  -> Restored transmitter T%d\n", curve.elementIndex + 1);
                                OutputDebugStringA(debugMsg);
                            }
                            break;
                        case ElementType::RECEIVER:
                            if (curve.elementIndex >= 0 && curve.elementIndex < 2) {
                                currentSelection.receivers[curve.elementIndex] = true;
                                sprintf_s(debugMsg, "  -> Restored receiver R%d\n", curve.elementIndex + 1);
                                OutputDebugStringA(debugMsg);
                            }
                            break;
                        case ElementType::SILENT_TX:
                            if (curve.elementIndex >= 0 && curve.elementIndex < 2) {
                                currentSelection.silentTx[curve.elementIndex] = true;
                                sprintf_s(debugMsg, "  -> Restored silent R%d\n", curve.elementIndex + 1);
                                OutputDebugStringA(debugMsg);
                            }
                            break;
                    }
                }
                
                // ОТЛАДКА: Итоговое состояние выбора
                OutputDebugStringA("Final selection state:\n");
                for (int i = 0; i < 4; i++) {
                    sprintf_s(debugMsg, "  T%d: %s\n", i + 1, currentSelection.transmitters[i] ? "selected" : "not selected");
                    OutputDebugStringA(debugMsg);
                }
                for (int i = 0; i < 2; i++) {
                    sprintf_s(debugMsg, "  R%d: %s\n", i + 1, currentSelection.receivers[i] ? "selected" : "not selected");
                    OutputDebugStringA(debugMsg);
                    sprintf_s(debugMsg, "  Silent R%d: %s\n", i + 1, currentSelection.silentTx[i] ? "selected" : "not selected");
                    OutputDebugStringA(debugMsg);
                }
                OutputDebugStringA("=== END RESTORE ===\n");
            }
            else {
                // Для пресетов (dataFilter != ALL) элементы схемы НЕ подсвечиваются,
                // НО нужно восстановить информацию о приемниках для генерации кривых
                // Определяем приемники по типу данных в кривых
                for (const auto& curve : existingConfig.curves) {
                    switch (curve.dataType) {
                        case DataType::AMPLITUDE_RX1:
                        case DataType::PHASE_RX1:
                            currentSelection.receivers[0] = true; // R1
                            break;
                        case DataType::AMPLITUDE_RX2:
                        case DataType::PHASE_RX2:
                            currentSelection.receivers[1] = true; // R2
                            break;
                        case DataType::AMPLITUDE_RATIO:
                        case DataType::PHASE_DIFFERENCE:
                            // Для отношений и разностей нужны оба приемника
                            currentSelection.receivers[0] = true; // R1
                            currentSelection.receivers[1] = true; // R2
                            break;
                        default:
                            break;
                    }
                }
            }
            
            // Сохраняем кривые с их состоянием enabled
            currentAvailableCurves = existingConfig.curves;
        }
    }
    
    // Инициализируем комбобокс частоты
    HWND hFreqCombo = GetDlgItem(hDlg, IDC_FREQUENCY_COMBO);
    if (hFreqCombo) {
        SendMessage(hFreqCombo, CB_ADDSTRING, 0, (LPARAM)L"400 kHz");
        SendMessage(hFreqCombo, CB_ADDSTRING, 0, (LPARAM)L"2000 kHz");
        
        // Устанавливаем частоту из восстановленной конфигурации или по умолчанию
        int freqIndex = (currentSelection.frequency == FrequencyBand::FREQ_400_KHZ) ? 0 : 1;
        SendMessage(hFreqCombo, CB_SETCURSEL, freqIndex, 0);
    }
    
    // Инициализируем область схемы
    HWND hSchematicArea = GetDlgItem(hDlg, IDC_SCHEMATIC_AREA);
    if (hSchematicArea) {
        RECT schematicRect;
        GetClientRect(hSchematicArea, &schematicRect);
        InitializeSchematicElements(schematicRect);
        
        // Синхронизируем визуальное состояние элементов с восстановленным выбором
        if (hasExistingConfig) {
            // Для пресетов (dataFilter != ALL) элементы схемы НЕ подсвечиваются
            if (currentSelection.dataFilter == DataTypeFilter::ALL) {
                // Режим ручного выбора - синхронизируем элементы схемы
                SetCurrentSelection(currentSelection);
            }
            else {
                // Режим пресета - все элементы схемы неактивны
                for (auto& element : schematicElements) {
                    element.selected = false;
                }
            }
        }
    }
    
    // Обновляем список доступных кривых
    UpdateAvailableCurves(hDlg);
}

void SchematicModule::UpdateAvailableCurves(HWND hDlg) {
    HWND hCurveList = GetDlgItem(hDlg, IDC_CURVE_LIST);
    if (!hCurveList) return;
    
    // Очищаем ListView (удаляем все элементы и колонки)
    ListView_DeleteAllItems(hCurveList);
    
    // Удаляем все существующие колонки
    int columnCount = Header_GetItemCount(ListView_GetHeader(hCurveList));
    for (int i = columnCount - 1; i >= 0; i--) {
        ListView_DeleteColumn(hCurveList, i);
    }
    
    if (!currentSelection.IsValid()) {
        // Показываем сообщение об ошибке выбора
        std::string error = currentSelection.GetValidationError();
        std::wstring werror(error.begin(), error.end());
        
        // Создаём одну колонку для сообщения об ошибке
        LVCOLUMN column = {};
        column.mask = LVCF_TEXT | LVCF_WIDTH;
        column.cx = 350;
        column.pszText = const_cast<LPWSTR>(L"Message");
        ListView_InsertColumn(hCurveList, 0, &column);
        
        LVITEM item = {};
        item.mask = LVIF_TEXT;
        item.iItem = 0;
        item.iSubItem = 0;
        item.pszText = const_cast<LPWSTR>(werror.c_str());
        ListView_InsertItem(hCurveList, &item);
        return;
    }
    
    if (!curveSelectionModule) {
        // Создаём одну колонку для сообщения об ошибке
        LVCOLUMN column = {};
        column.mask = LVCF_TEXT | LVCF_WIDTH;
        column.cx = 350;
        column.pszText = const_cast<LPWSTR>(L"Message");
        ListView_InsertColumn(hCurveList, 0, &column);
        
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
    
    // Если у нас уже есть сохранённые кривые (из существующей конфигурации),
    // синхронизируем состояние enabled с новыми доступными кривыми
    if (!currentAvailableCurves.empty()) {
        // Создаём карту ID -> enabled для быстрого поиска
        std::map<std::string, bool> savedEnabledStates;
        for (const auto& savedCurve : currentAvailableCurves) {
            savedEnabledStates[savedCurve.id] = savedCurve.enabled;
        }
        
        // Применяем сохранённые состояния к новым кривым
        for (auto& curve : availableCurves) {
            auto it = savedEnabledStates.find(curve.id);
            if (it != savedEnabledStates.end()) {
                curve.enabled = it->second;
            }
        }
    }
    
    if (availableCurves.empty()) {
        // Создаём одну колонку для сообщения
        LVCOLUMN column = {};
        column.mask = LVCF_TEXT | LVCF_WIDTH;
        column.cx = 350;
        column.pszText = const_cast<LPWSTR>(L"Message");
        ListView_InsertColumn(hCurveList, 0, &column);
        
        LVITEM item = {};
        item.mask = LVIF_TEXT;
        item.iItem = 0;
        item.iSubItem = 0;
        item.pszText = const_cast<LPWSTR>(L"Нет доступных кривых для данного выбора");
        ListView_InsertItem(hCurveList, &item);
        return;
    }
    
    // Настраиваем ListView для чекбоксов и группировки
    ListView_SetExtendedListViewStyle(hCurveList, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
    
    // Включаем группировку в ListView
    ListView_EnableGroupView(hCurveList, TRUE);
    
    // Группируем кривые по элементам (T1-T4, R1-R2)
    std::map<int, std::vector<size_t>> curvesByElement; // Ключ: elementIndex, Значение: индексы кривых
    std::vector<int> elementOrder; // Порядок элементов
    
    for (size_t i = 0; i < availableCurves.size(); i++) {
        const auto& curve = availableCurves[i];
        int elementKey = curve.elementIndex;
        
        if (curvesByElement.find(elementKey) == curvesByElement.end()) {
            elementOrder.push_back(elementKey);
        }
        
        curvesByElement[elementKey].push_back(i);
    }
    
    int numElements = static_cast<int>(elementOrder.size());
    int selectedReceivers = currentSelection.GetSelectedReceiversCount();
    
    // Создаём одну колонку для всех кривых
    LVCOLUMN column = {};
    column.mask = LVCF_TEXT | LVCF_WIDTH;
    column.cx = 400;
    column.pszText = const_cast<LPWSTR>(L"Available Curves");
    ListView_InsertColumn(hCurveList, 0, &column);
    
    // Специальный случай: R1 + R2 (Ratios & Differences) - без группировки
    if (selectedReceivers == 2 && numElements > 1) {
        // Отключаем группировку для этого случая
        ListView_EnableGroupView(hCurveList, FALSE);
        
        // Добавляем все кривые без группировки
        for (size_t i = 0; i < availableCurves.size(); i++) {
            const auto& curve = availableCurves[i];
            
            std::wstring displayText = std::wstring(curve.displayName.begin(), curve.displayName.end());
            displayText += L" (";
            displayText += std::wstring(curve.units.begin(), curve.units.end());
            displayText += L")";
            
            if (curve.yAxisGroup == 0) {
                displayText += L" [Left Axis]";
            } else {
                displayText += L" [Right Axis]";
            }
            
            LVITEM item = {};
            item.mask = LVIF_TEXT | LVIF_PARAM;
            item.iItem = static_cast<int>(i);
            item.iSubItem = 0;
            item.pszText = const_cast<LPWSTR>(displayText.c_str());
            item.lParam = static_cast<LPARAM>(i);
            
            int itemIndex = ListView_InsertItem(hCurveList, &item);
            ListView_SetCheckState(hCurveList, itemIndex, curve.enabled ? TRUE : FALSE);
        }
    }
    // Один элемент - без группировки
    else if (numElements == 1) {
        // Отключаем группировку
        ListView_EnableGroupView(hCurveList, FALSE);
        
        int elementIndex = elementOrder[0];
        
        for (size_t curveIdx : curvesByElement[elementIndex]) {
            const auto& curve = availableCurves[curveIdx];
            
            std::wstring displayText = std::wstring(curve.displayName.begin(), curve.displayName.end());
            displayText += L" (";
            displayText += std::wstring(curve.units.begin(), curve.units.end());
            displayText += L")";
            
            if (curve.yAxisGroup == 0) {
                displayText += L" [Left Axis]";
            } else {
                displayText += L" [Right Axis]";
            }
            
            LVITEM item = {};
            item.mask = LVIF_TEXT | LVIF_PARAM;
            item.iItem = static_cast<int>(curveIdx);
            item.iSubItem = 0;
            item.pszText = const_cast<LPWSTR>(displayText.c_str());
            item.lParam = static_cast<LPARAM>(curveIdx);
            
            int itemIndex = ListView_InsertItem(hCurveList, &item);
            ListView_SetCheckState(hCurveList, itemIndex, curve.enabled ? TRUE : FALSE);
        }
    }
    // Несколько элементов - используем группировку
    else {
        // Создаём группы для каждого элемента
        for (int groupIndex = 0; groupIndex < numElements; groupIndex++) {
            int elementIndex = elementOrder[groupIndex];
            size_t firstCurveIdx = curvesByElement[elementIndex][0];
            ElementType elementType = availableCurves[firstCurveIdx].sourceElement;
            
            // Определяем заголовок группы
            std::wstring groupHeader;
            switch (elementType) {
                case ElementType::TRANSMITTER:
                    groupHeader = L"T" + std::to_wstring(elementIndex + 1);
                    break;
                case ElementType::RECEIVER:
                    groupHeader = L"R" + std::to_wstring(elementIndex + 1);
                    break;
                case ElementType::SILENT_TX:
                    groupHeader = L"Silent R" + std::to_wstring(elementIndex + 1);
                    break;
                default:
                    groupHeader = L"Group " + std::to_wstring(groupIndex + 1);
            }
            
            // Создаём группу
            LVGROUP group = {};
            group.cbSize = sizeof(LVGROUP);
            group.mask = LVGF_HEADER | LVGF_GROUPID | LVGF_STATE;
            group.pszHeader = const_cast<LPWSTR>(groupHeader.c_str());
            group.iGroupId = groupIndex;
            group.state = LVGS_NORMAL | LVGS_COLLAPSIBLE;
            group.stateMask = LVGS_NORMAL | LVGS_COLLAPSIBLE;
            
            ListView_InsertGroup(hCurveList, groupIndex, &group);
        }
        
        // Добавляем кривые в соответствующие группы
        int globalItemIndex = 0;
        for (int groupIndex = 0; groupIndex < numElements; groupIndex++) {
            int elementIndex = elementOrder[groupIndex];
            const auto& curvesInGroup = curvesByElement[elementIndex];
            
            for (size_t curveIdx : curvesInGroup) {
                const auto& curve = availableCurves[curveIdx];
                
                std::wstring displayText = std::wstring(curve.displayName.begin(), curve.displayName.end());
                displayText += L" (";
                displayText += std::wstring(curve.units.begin(), curve.units.end());
                displayText += L")";
                
                if (curve.yAxisGroup == 0) {
                    displayText += L" [Left Axis]";
                } else {
                    displayText += L" [Right Axis]";
                }
                
                LVITEM item = {};
                item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_GROUPID;
                item.iItem = globalItemIndex;
                item.iSubItem = 0;
                item.pszText = const_cast<LPWSTR>(displayText.c_str());
                item.lParam = static_cast<LPARAM>(curveIdx);
                item.iGroupId = groupIndex;
                
                int itemIndex = ListView_InsertItem(hCurveList, &item);
                ListView_SetCheckState(hCurveList, itemIndex, curve.enabled ? TRUE : FALSE);
                
                globalItemIndex++;
            }
        }
    }
    
    // Сохраняем доступные кривые для последующего использования
    currentAvailableCurves = availableCurves;
}

void SchematicModule::HandleFrequencyChange(HWND hDlg) {
    HWND hFreqCombo = GetDlgItem(hDlg, IDC_FREQUENCY_COMBO);
    if (hFreqCombo) {
        int selection = (int)SendMessage(hFreqCombo, CB_GETCURSEL, 0, 0);
        currentSelection.frequency = (selection == 0) ? FrequencyBand::FREQ_400_KHZ : FrequencyBand::FREQ_2000_KHZ;
        
        // При изменении частоты сохраняем текущий фильтр (не сбрасываем)
        // Это позволяет пользователю менять частоту без потери выбранного пресета
        
        // Обновляем список доступных кривых
        UpdateAvailableCurves(hDlg);
    }
}

void SchematicModule::HandlePresetButton(HWND hDlg, int presetId) {
    // Очищаем текущий выбор (включая старые кривые)
    currentSelection.Clear();
    currentAvailableCurves.clear();
    
    // Применяем пресет с соответствующим фильтром типов данных
    switch (presetId) {
        case IDC_PRESET_ALL_AMP_R1:
            currentSelection.receivers[0] = true; // R1
            currentSelection.dataFilter = DataTypeFilter::AMPLITUDE_ONLY;
            break;
        case IDC_PRESET_ALL_AMP_R2:
            currentSelection.receivers[1] = true; // R2
            currentSelection.dataFilter = DataTypeFilter::AMPLITUDE_ONLY;
            break;
        case IDC_PRESET_ALL_PHASE_R1:
            currentSelection.receivers[0] = true; // R1
            currentSelection.dataFilter = DataTypeFilter::PHASE_ONLY;
            break;
        case IDC_PRESET_ALL_PHASE_R2:
            currentSelection.receivers[1] = true; // R2
            currentSelection.dataFilter = DataTypeFilter::PHASE_ONLY;
            break;
        case IDC_PRESET_RATIOS:
            currentSelection.receivers[0] = true; // R1
            currentSelection.receivers[1] = true; // R2
            currentSelection.dataFilter = DataTypeFilter::RATIO_ONLY;
            break;
        case IDC_PRESET_DIFFERENCES:
            currentSelection.receivers[0] = true; // R1
            currentSelection.receivers[1] = true; // R2
            currentSelection.dataFilter = DataTypeFilter::DIFFERENCE_ONLY;
            break;
    }
    
    // Для пресетов НЕ синхронизируем элементы схемы (они должны оставаться неактивными)
    // Просто очищаем выбор всех элементов схемы
    for (auto& element : schematicElements) {
        element.selected = false;
    }
    
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
    // Обновляем hover состояние элементов
    UpdateHoverState(x, y);
    
    ElementRect* element = FindElementAt(x, y);
    
    if (element) {
        // Устанавливаем курсор "рука" для интерактивных элементов
        SetCursor(LoadCursor(NULL, IDC_HAND));
        
        // Можно добавить показ подсказки с информацией об элементе
        std::wstring tooltip = GetElementTooltip(element->type, element->index);
        // TODO: Показать tooltip (можно использовать SetWindowText для статусной строки)
        
        // Перерисовываем область схемы для отображения hover эффекта
        HWND hSchematicArea = GetDlgItem(hDlg, IDC_SCHEMATIC_AREA);
        if (hSchematicArea) {
            RECT schematicRect;
            GetClientRect(hSchematicArea, &schematicRect);
            MapWindowPoints(hSchematicArea, hDlg, (POINT*)&schematicRect, 2);
            InvalidateRect(hDlg, &schematicRect, FALSE);
        }
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

DataTypeFilter SchematicModule::DetermineDataFilterFromCurves(const std::vector<CurveDefinition>& curves) const {
    if (curves.empty()) {
        return DataTypeFilter::ALL;
    }
    
    // ОТЛАДКА: Выводим информацию о кривых
    char debugMsg[256];
    sprintf_s(debugMsg, "=== DetermineDataFilterFromCurves ===\n");
    OutputDebugStringA(debugMsg);
    sprintf_s(debugMsg, "Total curves: %zu\n", curves.size());
    OutputDebugStringA(debugMsg);
    
    // Подсчитываем типы данных в кривых
    bool hasAmplitudes = false;
    bool hasPhases = false;
    bool hasRatios = false;
    bool hasDifferences = false;
    bool hasSilentAmplitudes = false;
    
    for (const auto& curve : curves) {
        sprintf_s(debugMsg, "Curve: %s, DataType: %d, SourceElement: %d\n", 
                 curve.id.c_str(), static_cast<int>(curve.dataType), static_cast<int>(curve.sourceElement));
        OutputDebugStringA(debugMsg);
        
        switch (curve.dataType) {
            case DataType::AMPLITUDE_RX1:
            case DataType::AMPLITUDE_RX2:
                hasAmplitudes = true;
                break;
            case DataType::SILENT_AMPLITUDE_RX1:
            case DataType::SILENT_AMPLITUDE_RX2:
                hasSilentAmplitudes = true;
                break;
            case DataType::PHASE_RX1:
            case DataType::PHASE_RX2:
                hasPhases = true;
                break;
            case DataType::AMPLITUDE_RATIO:
                hasRatios = true;
                break;
            case DataType::PHASE_DIFFERENCE:
                hasDifferences = true;
                break;
            default:
                break;
        }
    }
    
    sprintf_s(debugMsg, "Data types found: Ampl=%s, Silent=%s, Phase=%s, Ratio=%s, Diff=%s\n",
             hasAmplitudes ? "yes" : "no",
             hasSilentAmplitudes ? "yes" : "no", 
             hasPhases ? "yes" : "no",
             hasRatios ? "yes" : "no",
             hasDifferences ? "yes" : "no");
    OutputDebugStringA(debugMsg);
    
    // Определяем фильтр на основе найденных типов
    // ИСПРАВЛЕНО: Учитываем молчащие передатчики
    
    DataTypeFilter result = DataTypeFilter::ALL;
    
    // Если есть только отношения амплитуд
    if (hasRatios && !hasDifferences && !hasAmplitudes && !hasPhases && !hasSilentAmplitudes) {
        result = DataTypeFilter::RATIO_ONLY;
    }
    // Если есть только разности фаз
    else if (hasDifferences && !hasRatios && !hasAmplitudes && !hasPhases && !hasSilentAmplitudes) {
        result = DataTypeFilter::DIFFERENCE_ONLY;
    }
    // Если есть только обычные амплитуды (без отношений и молчащих)
    else if (hasAmplitudes && !hasPhases && !hasRatios && !hasDifferences && !hasSilentAmplitudes) {
        result = DataTypeFilter::AMPLITUDE_ONLY;
    }
    // Если есть только фазы (без разностей)
    else if (hasPhases && !hasAmplitudes && !hasRatios && !hasDifferences && !hasSilentAmplitudes) {
        result = DataTypeFilter::PHASE_ONLY;
    }
    // ИСПРАВЛЕНО: Если есть только молчащие передатчики или смешанные типы - ALL
    else {
        result = DataTypeFilter::ALL;
    }
    
    sprintf_s(debugMsg, "Determined filter: %d\n", static_cast<int>(result));
    OutputDebugStringA(debugMsg);
    OutputDebugStringA("=== END DetermineDataFilterFromCurves ===\n");
    
    return result;
}

// Дополнительные методы отрисовки схемы
void SchematicModule::DrawDeviceFrame(HDC hdc, const RECT& frameRect) {
    // Рисуем фон рамки
    HBRUSH bgBrush = CreateSolidBrush(FRAME_BACKGROUND);
    HPEN borderPen = CreatePen(PS_SOLID, FRAME_BORDER_WIDTH, FRAME_BORDER);
    
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, bgBrush);
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    
    // Рисуем скруглённый прямоугольник для рамки
    RoundRect(hdc, frameRect.left, frameRect.top, 
              frameRect.right, frameRect.bottom,
              FRAME_CORNER_RADIUS, FRAME_CORNER_RADIUS);
    
    // Восстанавливаем объекты GDI
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(bgBrush);
    DeleteObject(borderPen);
}

void SchematicModule::DrawSchematicConnections(HDC hdc, const RECT& rect) {
    // В горизонтальной схеме соединительные линии не нужны
    // Все элементы расположены в логическом порядке слева направо
    // Разделители убраны для чистого современного вида
}


void SchematicModule::DrawGroupLabels(HDC hdc, const RECT& rect) {
    // Устанавливаем шрифт для подписей
    HFONT labelFont = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
    HFONT oldFont = (HFONT)SelectObject(hdc, labelFont);
    
    SetTextColor(hdc, RGB(80, 80, 80));
    SetBkMode(hdc, TRANSPARENT);
    
    // Поскольку все элементы теперь в одной линии, добавляем подписи под каждым элементом
    // Получаем позиции элементов для подписей
    int elementWidth = 70, spacing = 10;
    int totalElements = 8;
    int totalElementsWidth = totalElements * elementWidth + (totalElements - 1) * spacing;
    int startX = rect.left + (rect.right - rect.left - totalElementsWidth) / 2;
    int centerY = rect.top + (rect.bottom - rect.top) / 2;
    int labelY = centerY + 45 + 5; // Под элементами
    
    // Подписи типов элементов убраны для более чистого вида
    // Пользователи ИАТК понимают обозначения T1-T4, R1-R2, Silent R1-R2 без дополнительных пояснений
    
    SelectObject(hdc, oldFont);
    DeleteObject(labelFont);
}

void SchematicModule::DrawConnectionArrows(HDC hdc, const std::vector<RECT>& transmitterRects, 
                                          const RECT& r1Rect, const RECT& r2Rect) {
    // В горизонтальной схеме стрелки не нужны, так как логика понятна из расположения
    // Можно добавить небольшие индикаторы направления сигнала, если необходимо
    
    // Создаем перо для индикаторов
    HPEN indicatorPen = CreatePen(PS_SOLID, 1, RGB(120, 120, 120));
    HPEN oldPen = (HPEN)SelectObject(hdc, indicatorPen);
    
    // Добавляем небольшие стрелки над приемниками для указания входящего сигнала
    int arrowSize = 4;
    
    // Стрелка над R1
    int r1CenterX = r1Rect.left + (r1Rect.right - r1Rect.left) / 2;
    int r1TopY = r1Rect.top - 8;
    
    MoveToEx(hdc, r1CenterX - arrowSize, r1TopY - arrowSize, NULL);
    LineTo(hdc, r1CenterX, r1TopY);
    LineTo(hdc, r1CenterX + arrowSize, r1TopY - arrowSize);
    
    // Стрелка над R2
    int r2CenterX = r2Rect.left + (r2Rect.right - r2Rect.left) / 2;
    int r2TopY = r2Rect.top - 8;
    
    MoveToEx(hdc, r2CenterX - arrowSize, r2TopY - arrowSize, NULL);
    LineTo(hdc, r2CenterX, r2TopY);
    LineTo(hdc, r2CenterX + arrowSize, r2TopY - arrowSize);
    
    SelectObject(hdc, oldPen);
    DeleteObject(indicatorPen);
}

// Методы для интеллектуальной обработки выбора элементов
bool SchematicModule::IsSelectionCompatible(ElementType type, int index) const {
    // ИСПРАВЛЕНО: Обновленные правила совместимости выбора элементов
    
    // Правило 1: Нельзя одновременно выбирать передатчики и приемники
    if (type == ElementType::TRANSMITTER) {
        return currentSelection.GetSelectedReceiversCount() == 0;
    }
    
    if (type == ElementType::RECEIVER) {
        return currentSelection.GetSelectedTransmittersCount() == 0;
    }
    
    // ИСПРАВЛЕНО: Молчащие передатчики МОЖНО комбинировать с активными элементами
    // Запрещена только комбинация: оба приемника (R1+R2) + молчащие
    if (type == ElementType::SILENT_TX) {
        // Разрешаем комбинации с передатчиками и одним приемником
        // Запрещаем только комбинацию с двумя приемниками
        return currentSelection.GetSelectedReceiversCount() < 2;
    }
    
    return true;
}

void SchematicModule::ClearIncompatibleSelections(ElementType newType) {
    // ИСПРАВЛЕНО: Минимальная очистка - только реально несовместимые элементы
    
    char debugMsg[256];
    sprintf_s(debugMsg, "ClearIncompatibleSelections for type: %d\n", static_cast<int>(newType));
    OutputDebugStringA(debugMsg);
    
    if (newType == ElementType::TRANSMITTER) {
        // Передатчики несовместимы только с приемниками
        if (currentSelection.GetSelectedReceiversCount() > 0) {
            OutputDebugStringA("  Clearing receivers (incompatible with transmitters)\n");
            std::fill(currentSelection.receivers.begin(), currentSelection.receivers.end(), false);
        }
        // Молчащие передатчики СОВМЕСТИМЫ с активными передатчиками - НЕ очищаем
    }
    else if (newType == ElementType::RECEIVER) {
        // Приемники несовместимы только с передатчиками
        if (currentSelection.GetSelectedTransmittersCount() > 0) {
            OutputDebugStringA("  Clearing transmitters (incompatible with receivers)\n");
            std::fill(currentSelection.transmitters.begin(), currentSelection.transmitters.end(), false);
        }
        
        // Если выбираем второй приемник, очищаем молчащие (R1+R2+Silent запрещено)
        int selectedReceivers = currentSelection.GetSelectedReceiversCount();
        if (selectedReceivers >= 1 && currentSelection.GetSelectedSilentCount() > 0) {
            OutputDebugStringA("  Clearing silent transmitters (R1+R2+Silent not allowed)\n");
            std::fill(currentSelection.silentTx.begin(), currentSelection.silentTx.end(), false);
        }
    }
    else if (newType == ElementType::SILENT_TX) {
        // Молчащие передатчики несовместимы только с R1+R2 одновременно
        if (currentSelection.GetSelectedReceiversCount() == 2) {
            OutputDebugStringA("  Clearing receivers (R1+R2+Silent not allowed)\n");
            std::fill(currentSelection.receivers.begin(), currentSelection.receivers.end(), false);
        }
        // С передатчиками и одним приемником молчащие СОВМЕСТИМЫ - НЕ очищаем
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
                std::wstring title = L"Device Schematic - Chart " + std::to_wstring(pSchematic->targetChartNumber);
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

