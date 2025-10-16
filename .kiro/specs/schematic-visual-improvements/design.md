# 🎨 Design Document - Schematic Visual Improvements

## Overview

Улучшение визуального представления схемы прибора с современным корпоративным дизайном, горизонтальной компоновкой и рамкой для основных элементов.

---

## 🎯 Финальный дизайн

### Визуальный эскиз

```
┌────────────────────────────────────────────────────────────────────────────┐
│  Device Schematic                                                          │
├────────────────────────────────────────────────────────────────────────────┤
│                                                                            │
│  ┌──────────────────────────────────────────────────────────┐             │
│  │  Device Components                                       │             │
│  │                                                          │             │
│  │  ╭──────╮ ╭──────╮ ╭──────╮ ╭──────╮ ╭──────╮ ╭──────╮ │  ╭──────────╮│
│  │  │  T1  │ │  T2  │ │  T3  │ │  T4  │ │  R1  │ │  R2  │ │  │ Silent R1││
│  │  ╰──────╯ ╰──────╯ ╰──────╯ ╰──────╯ ╰──────╯ ╰──────╯ │  ╰──────────╯│
│  │                                                          │             │
│  └──────────────────────────────────────────────────────────┘  ╭──────────╮│
│                                                                 │ Silent R2││
│                                                                 ╰──────────╯│
└────────────────────────────────────────────────────────────────────────────┘
```

### Ключевые особенности:

1. **Рамка для основных элементов**
   - T1-T4 и R1-R2 внутри рамки
   - Нейтральный фон (светло-серый)
   - Тонкая граница

2. **Горизонтальное расположение**
   - Все элементы в одну линию
   - Логический порядок: TX → RX → Silent

3. **Молчащие передатчики отдельно**
   - Вне рамки
   - Справа от основных элементов

4. **Современный дизайн**
   - Скруглённые углы
   - Градиенты
   - Тени
   - Hover эффекты

---

## 🏗️ Architecture

### Компоненты отрисовки

```cpp
class SchematicModule {
    // Основной метод отрисовки
    void RenderSchematic(HDC hdc, const RECT& rect);
    
    // Новые методы
    void DrawDeviceFrame(HDC hdc, const RECT& frameRect);
    void DrawElementWithGradient(HDC hdc, const RECT& elementRect, 
                                 ElementType type, bool selected, bool hovered);
    void DrawElementShadow(HDC hdc, const RECT& elementRect);
    
    // Обработка hover состояния
    void UpdateHoverState(int x, int y);
    ElementRect* hoveredElement;
};
```

### Структура данных

```cpp
struct ElementRect {
    RECT rect;
    ElementType type;
    int index;
    bool selected;
    bool hovered;  // Новое поле для hover состояния
};
```

---

## 🎨 Components and Interfaces

### 1. Device Frame (Рамка прибора)

#### Параметры:
```cpp
// Цвета рамки
COLORREF FRAME_BACKGROUND = RGB(245, 245, 245);  // Светло-серый фон
COLORREF FRAME_BORDER = RGB(180, 180, 180);      // Тёмно-серая граница

// Размеры
int FRAME_PADDING = 15;        // Отступ внутри рамки
int FRAME_BORDER_WIDTH = 1;    // Толщина границы
int FRAME_CORNER_RADIUS = 8;   // Радиус скругления углов
```

#### Метод отрисовки:
```cpp
void SchematicModule::DrawDeviceFrame(HDC hdc, const RECT& frameRect) {
    // 1. Рисуем фон рамки
    HBRUSH bgBrush = CreateSolidBrush(FRAME_BACKGROUND);
    HPEN borderPen = CreatePen(PS_SOLID, FRAME_BORDER_WIDTH, FRAME_BORDER);
    
    SelectObject(hdc, bgBrush);
    SelectObject(hdc, borderPen);
    
    // 2. Рисуем скруглённый прямоугольник
    RoundRect(hdc, frameRect.left, frameRect.top, 
              frameRect.right, frameRect.bottom,
              FRAME_CORNER_RADIUS, FRAME_CORNER_RADIUS);
    
    // 3. Очистка ресурсов
    DeleteObject(bgBrush);
    DeleteObject(borderPen);
}
```

---

### 2. Element Rendering (Отрисовка элементов)

#### Цветовая схема:

##### Передатчики (T1-T4):
```cpp
// Неактивное состояние
COLORREF TX_GRADIENT_START = RGB(200, 120, 120);
COLORREF TX_GRADIENT_END = RGB(170, 90, 90);
COLORREF TX_BORDER = RGB(140, 70, 70);

// Активное состояние (выбран)
COLORREF TX_ACTIVE_GRADIENT_START = RGB(230, 130, 130);
COLORREF TX_ACTIVE_GRADIENT_END = RGB(200, 100, 100);
COLORREF TX_ACTIVE_BORDER = RGB(180, 60, 60);
int TX_ACTIVE_BORDER_WIDTH = 3;

// Hover состояние
COLORREF TX_HOVER_GRADIENT_START = RGB(215, 125, 125);
COLORREF TX_HOVER_GRADIENT_END = RGB(185, 95, 95);
```

##### Приемники (R1-R2):
```cpp
// Неактивное состояние
COLORREF RX_GRADIENT_START = RGB(120, 200, 120);
COLORREF RX_GRADIENT_END = RGB(90, 170, 90);
COLORREF RX_BORDER = RGB(70, 140, 70);

// Активное состояние (выбран)
COLORREF RX_ACTIVE_GRADIENT_START = RGB(130, 230, 130);
COLORREF RX_ACTIVE_GRADIENT_END = RGB(100, 200, 100);
COLORREF RX_ACTIVE_BORDER = RGB(60, 180, 60);
int RX_ACTIVE_BORDER_WIDTH = 3;

// Hover состояние
COLORREF RX_HOVER_GRADIENT_START = RGB(125, 215, 125);
COLORREF RX_HOVER_GRADIENT_END = RGB(95, 185, 95);
```

##### Молчащие передатчики (Silent R1-R2):
```cpp
// Неактивное состояние
COLORREF SILENT_GRADIENT_START = RGB(160, 160, 160);
COLORREF SILENT_GRADIENT_END = RGB(130, 130, 130);
COLORREF SILENT_BORDER = RGB(100, 100, 100);

// Активное состояние (выбран)
COLORREF SILENT_ACTIVE_GRADIENT_START = RGB(180, 180, 180);
COLORREF SILENT_ACTIVE_GRADIENT_END = RGB(150, 150, 150);
COLORREF SILENT_ACTIVE_BORDER = RGB(80, 80, 80);
int SILENT_ACTIVE_BORDER_WIDTH = 3;

// Hover состояние
COLORREF SILENT_HOVER_GRADIENT_START = RGB(170, 170, 170);
COLORREF SILENT_HOVER_GRADIENT_END = RGB(140, 140, 140);
```

#### Метод отрисовки с градиентом:
```cpp
void SchematicModule::DrawElementWithGradient(HDC hdc, const RECT& elementRect,
                                              ElementType type, bool selected, bool hovered) {
    // 1. Определяем цвета в зависимости от состояния
    COLORREF gradientStart, gradientEnd, borderColor;
    int borderWidth = 2;
    
    if (selected) {
        // Активное состояние
        GetActiveColors(type, gradientStart, gradientEnd, borderColor);
        borderWidth = 3;
    } else if (hovered) {
        // Hover состояние
        GetHoverColors(type, gradientStart, gradientEnd, borderColor);
    } else {
        // Неактивное состояние
        GetNormalColors(type, gradientStart, gradientEnd, borderColor);
    }
    
    // 2. Рисуем тень (если не GDI+)
    DrawElementShadow(hdc, elementRect);
    
    // 3. Рисуем градиент (используем GDI+ для лучшего качества)
    DrawGradientRect(hdc, elementRect, gradientStart, gradientEnd);
    
    // 4. Рисуем рамку
    HPEN borderPen = CreatePen(PS_SOLID, borderWidth, borderColor);
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    RoundRect(hdc, elementRect.left, elementRect.top,
              elementRect.right, elementRect.bottom, 8, 8);
    
    SelectObject(hdc, oldPen);
    DeleteObject(borderPen);
}
```

---

### 3. Shadow Effect (Эффект тени)

#### Параметры тени:
```cpp
int SHADOW_OFFSET_X = 2;
int SHADOW_OFFSET_Y = 2;
int SHADOW_BLUR = 3;
COLORREF SHADOW_COLOR = RGB(0, 0, 0);  // С альфа-каналом 30%
```

#### Метод отрисовки тени:
```cpp
void SchematicModule::DrawElementShadow(HDC hdc, const RECT& elementRect) {
    // Создаём смещённый прямоугольник для тени
    RECT shadowRect = {
        elementRect.left + SHADOW_OFFSET_X,
        elementRect.top + SHADOW_OFFSET_Y,
        elementRect.right + SHADOW_OFFSET_X,
        elementRect.bottom + SHADOW_OFFSET_Y
    };
    
    // Рисуем полупрозрачную тень
    // Примечание: Для полной реализации нужен GDI+ с альфа-каналом
    HBRUSH shadowBrush = CreateSolidBrush(RGB(200, 200, 200));
    HPEN shadowPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
    
    SelectObject(hdc, shadowBrush);
    SelectObject(hdc, shadowPen);
    
    RoundRect(hdc, shadowRect.left, shadowRect.top,
              shadowRect.right, shadowRect.bottom, 8, 8);
    
    DeleteObject(shadowBrush);
    DeleteObject(shadowPen);
}
```

---

### 4. Hover State Management (Управление hover состоянием)

#### Обработка движения мыши:
```cpp
void SchematicModule::UpdateHoverState(int x, int y) {
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
        
        // Перерисовываем схему
        InvalidateRect(dialogHwnd, NULL, FALSE);
    }
}
```

#### Обработка в WM_MOUSEMOVE:
```cpp
case WM_MOUSEMOVE: {
    int x = LOWORD(lParam);
    int y = HIWORD(lParam);
    
    pSchematic->UpdateHoverState(x, y);
    
    // Устанавливаем курсор "hand" при наведении на элемент
    if (pSchematic->hoveredElement) {
        SetCursor(LoadCursor(NULL, IDC_HAND));
    } else {
        SetCursor(LoadCursor(NULL, IDC_ARROW));
    }
    
    return 0;
}
```

---

## 📐 Data Models

### Layout Calculation (Расчёт компоновки)

```cpp
void SchematicModule::CalculateSchematicLayout(const RECT& rect) {
    schematicElements.clear();
    
    // Параметры элементов
    const int elementWidth = 70;
    const int elementHeight = 45;
    const int spacing = 12;
    const int framePadding = 15;
    const int frameToSilentSpacing = 25;
    
    // Рассчитываем размер рамки для основных элементов (T1-T4, R1-R2)
    int frameElementsCount = 6;  // 4 TX + 2 RX
    int frameWidth = frameElementsCount * elementWidth + 
                     (frameElementsCount - 1) * spacing + 
                     2 * framePadding;
    int frameHeight = elementHeight + 2 * framePadding;
    
    // Позиция рамки (центрируем)
    int totalWidth = frameWidth + frameToSilentSpacing + 
                     2 * elementWidth + spacing;
    int startX = rect.left + (rect.right - rect.left - totalWidth) / 2;
    int centerY = rect.top + (rect.bottom - rect.top) / 2;
    
    // Сохраняем координаты рамки
    deviceFrameRect = {
        startX,
        centerY - frameHeight / 2,
        startX + frameWidth,
        centerY + frameHeight / 2
    };
    
    // Позиция первого элемента внутри рамки
    int currentX = startX + framePadding;
    int elementY = centerY - elementHeight / 2;
    
    // Добавляем передатчики T1-T4
    for (int i = 0; i < 4; i++) {
        ElementRect element;
        element.rect = {currentX, elementY, 
                       currentX + elementWidth, elementY + elementHeight};
        element.type = ElementType::TRANSMITTER;
        element.index = i;
        element.selected = currentSelection.transmitters[i];
        element.hovered = false;
        schematicElements.push_back(element);
        
        currentX += elementWidth + spacing;
    }
    
    // Добавляем приемники R1-R2
    for (int i = 0; i < 2; i++) {
        ElementRect element;
        element.rect = {currentX, elementY,
                       currentX + elementWidth, elementY + elementHeight};
        element.type = ElementType::RECEIVER;
        element.index = i;
        element.selected = currentSelection.receivers[i];
        element.hovered = false;
        schematicElements.push_back(element);
        
        currentX += elementWidth + spacing;
    }
    
    // Добавляем молчащие передатчики (вне рамки)
    currentX = deviceFrameRect.right + frameToSilentSpacing;
    
    for (int i = 0; i < 2; i++) {
        ElementRect element;
        element.rect = {currentX, elementY,
                       currentX + elementWidth, elementY + elementHeight};
        element.type = ElementType::SILENT_TX;
        element.index = i;
        element.selected = currentSelection.silentTx[i];
        element.hovered = false;
        schematicElements.push_back(element);
        
        currentX += elementWidth + spacing;
    }
}
```

---

## 🎯 Error Handling

### Обработка ошибок GDI

```cpp
// Проверка создания объектов GDI
HBRUSH brush = CreateSolidBrush(color);
if (!brush) {
    // Логирование ошибки
    OutputDebugString(L"Failed to create brush");
    return;
}

// Всегда удаляем объекты GDI
DeleteObject(brush);
```

### Обработка ошибок GDI+

```cpp
// Инициализация GDI+
Gdiplus::GdiplusStartupInput gdiplusStartupInput;
ULONG_PTR gdiplusToken;
Gdiplus::Status status = Gdiplus::GdiplusStartup(&gdiplusToken, 
                                                  &gdiplusStartupInput, NULL);
if (status != Gdiplus::Ok) {
    // Fallback на обычный GDI
    useGdiPlus = false;
}
```

---

## 🧪 Testing Strategy

### Визуальное тестирование

1. **Проверка рамки**
   - Рамка отображается корректно
   - Фон нейтрального цвета
   - Граница видна и аккуратна

2. **Проверка элементов**
   - Все элементы внутри/вне рамки согласно дизайну
   - Градиенты отображаются корректно
   - Тени видны и не перекрывают элементы

3. **Проверка hover эффектов**
   - Элемент подсвечивается при наведении
   - Курсор меняется на "hand"
   - Переход плавный

4. **Проверка выделения**
   - Выбранный элемент имеет яркую рамку
   - Цвет более насыщенный
   - Невыбранный элемент приглушён

### Функциональное тестирование

```cpp
// Unit тесты
TEST(SchematicModule, LayoutCalculation) {
    SchematicModule module;
    RECT rect = {0, 0, 1000, 400};
    
    module.CalculateSchematicLayout(rect);
    
    // Проверяем количество элементов
    ASSERT_EQ(module.schematicElements.size(), 8);
    
    // Проверяем, что T1-T4 и R1-R2 внутри рамки
    for (int i = 0; i < 6; i++) {
        ASSERT_TRUE(IsRectInside(module.schematicElements[i].rect, 
                                 module.deviceFrameRect));
    }
    
    // Проверяем, что Silent вне рамки
    for (int i = 6; i < 8; i++) {
        ASSERT_FALSE(IsRectInside(module.schematicElements[i].rect,
                                  module.deviceFrameRect));
    }
}
```

---

## 📊 Performance Considerations

### Оптимизация отрисовки

1. **Кэширование GDI объектов**
   ```cpp
   // Создаём кисти и перья один раз
   static HBRUSH cachedBrushes[10];
   static HPEN cachedPens[10];
   ```

2. **Частичная перерисовка**
   ```cpp
   // Перерисовываем только изменённые элементы
   InvalidateRect(hwnd, &element.rect, FALSE);
   ```

3. **Двойная буферизация**
   ```cpp
   // Рисуем в память, затем копируем на экран
   HDC memDC = CreateCompatibleDC(hdc);
   HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
   SelectObject(memDC, memBitmap);
   
   // Рисуем в memDC
   RenderSchematic(memDC, rect);
   
   // Копируем на экран
   BitBlt(hdc, 0, 0, width, height, memDC, 0, 0, SRCCOPY);
   
   DeleteObject(memBitmap);
   DeleteDC(memDC);
   ```

---

**Design Document - Schematic Visual Improvements** - версия 1.0
