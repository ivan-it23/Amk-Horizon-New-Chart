# Новая функция: Независимые элементы управления X Range для каждого графика

## Дата
24 октября 2025

## Описание
Добавлены независимые элементы управления для точного задания диапазона отображения по оси X для каждого графика отдельно. В Single Mode управление осуществляется через панель инструментов, в Dual Mode каждый график имеет свои элементы управления.

## Интерфейс

### Single Mode
```
┌─────────────────────────────────────────────────────────────────┐
│ [Dual Mode ☐] [Device Schematic] │ X Range: [____] to [____] [Apply] [Reset] │
├─────────────────────────────────────────────────────────────────┤
│  Chart 1: [Custom Curves]                                      │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                График 1                                 │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

### Dual Mode
```
┌─────────────────────────────────────────────────────────────────┐
│ [Dual Mode ☑] [Device Schematic] │ X Range: [____] to [____] [Apply] [Reset] │
├─────────────────────────────────────────────────────────────────┤
│  Chart 1: [Custom Curves]                                      │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                График 1                                 │   │
│  └─────────────────────────────────────────────────────────┘   │
│                                                                 │
│  Chart 2: [Custom Curves]                                      │
│  [Device Schematic] X Range: [____] to [____] [Apply] [Reset]  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                График 2                                 │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
```

## Функциональность

### Независимое управление
- **Chart 1**: Элементы в панели инструментов (всегда видны)
- **Chart 2**: Элементы рядом с кнопкой Device Schematic (показываются только в Dual Mode)
- **Независимые диапазоны**: Каждый график может иметь свой диапазон X
- **Отдельная валидация**: Проверка данных для каждого графика

### Применение диапазона (Apply)
- Валидация входных данных для конкретного графика
- Применение диапазона только к выбранному графику
- Сообщения об ошибках с указанием номера графика
- Автоматическое обновление скроллбаров

### Сброс диапазона (Reset)
- Сброс только для конкретного графика
- Другой график сохраняет свой диапазон
- Независимое управление скроллбарами

## Технические детали

### ID элементов управления (Resource.h)
```cpp
// Chart 1 (панель инструментов)
#define IDC_XRANGE_FROM_EDIT     1024
#define IDC_XRANGE_TO_EDIT       1025
#define IDC_XRANGE_APPLY_BTN     1026
#define IDC_XRANGE_RESET_BTN     1027
#define IDC_XRANGE_FROM_LABEL    1028
#define IDC_XRANGE_TO_LABEL      1029

// Chart 2 (рядом с Device Schematic)
#define IDC_XRANGE2_FROM_EDIT    1030
#define IDC_XRANGE2_TO_EDIT      1031
#define IDC_XRANGE2_APPLY_BTN    1032
#define IDC_XRANGE2_RESET_BTN    1033
#define IDC_XRANGE2_FROM_LABEL   1034
#define IDC_XRANGE2_TO_LABEL     1035
```

### Методы UIModule
```cpp
// Все методы теперь принимают chartNumber (1 или 2)
void UpdateXRangeControls(int chartNumber, double minX, double maxX, bool isCustomRange);
void GetXRangeValues(int chartNumber, double& fromX, double& toX) const;
void SetXRangeValues(int chartNumber, double fromX, double toX);
void ResetXRangeControls(int chartNumber);
```

### Методы ChartManager
```cpp
void HandleXRangeApply(HWND hWnd, int chartNumber);
void HandleXRangeReset(HWND hWnd, int chartNumber);
void UpdateXRangeUI(HWND hWnd); // Обновляет оба графика
```

### Создание элементов (UIModule::CreateControls)
```cpp
// Chart 1 - в панели инструментов (позиция X=280)
hXRangeFromEdit = CreateWindow(..., xRangeStartX + 65, yPos - 2, ...);

// Chart 2 - рядом с Device Schematic (позиция X=160, изначально скрыты)
hXRange2FromEdit = CreateWindow(..., WS_CHILD, xRange2StartX + 65, 298, ...);
```

### Управление видимостью (UIModule::UpdateControlsVisibility)
```cpp
void UpdateControlsVisibility(bool showChart2Controls) {
    int showFlag = showChart2Controls ? SW_SHOW : SW_HIDE;
    // Показываем/скрываем все 6 элементов Chart 2
    ShowWindow(hXRange2FromLabel, showFlag);
    ShowWindow(hXRange2FromEdit, showFlag);
    // ... остальные элементы
}
```

### Позиционирование (UIModule::PositionChart2Controls)
```cpp
void PositionChart2Controls(int panel2Y) {
    // Позиционируем элементы Chart 2 относительно panel2Y
    int yPos = panel2Y + 2; // Выравниваем с Device Schematic
    SetWindowPos(hXRange2FromEdit, HWND_TOP, xRange2StartX + 65, yPos - 2, ...);
    // ... остальные элементы
}
```

## Валидация данных

### Проверки для каждого графика
1. **Логическая валидация**: From < To
2. **Диапазон данных**: 0 ≤ From < To ≤ (размер_данных - 1)
3. **Наличие данных**: Проверка загруженности данных

### Сообщения об ошибках
- "No data loaded for Chart [N]" - если данные не загружены
- "Chart [N]: 'From' value must be less than 'To' value" - логическая ошибка
- "Chart [N]: X Range must be between 0 and [max]" - выход за границы

## Интеграция

### Совместимость с zoom мышью
- X Range и zoom мышью работают независимо для каждого графика
- Каждый график имеет свою ZoomRange структуру
- Последний применённый метод имеет приоритет для конкретного графика

### Синхронизация UI
- При переключении в Dual Mode показываются элементы Chart 2
- При загрузке данных обновляются элементы обоих графиков
- Кнопки Reset активны только при установленном пользовательском диапазоне

## Изменённые файлы

### Resource.h
- Добавлены 6 новых ID для Chart 2

### UIModule.h/.cpp
- Добавлены 6 новых HWND для Chart 2
- Обновлён конструктор
- Обновлены методы для поддержки chartNumber параметра
- Обновлены CreateControls, UpdateControlsVisibility, PositionChart2Controls

### ChartManager.h/.cpp
- Обновлены методы для поддержки chartNumber параметра
- Добавлена обработка команд Chart 2
- Обновлён UpdateXRangeUI для работы с обоими графиками

## Тестирование

### Основные сценарии
1. **Single Mode**
   - Применить диапазон через панель инструментов
   - Проверить отображение на единственном графике

2. **Dual Mode - независимое управление**
   - Применить разные диапазоны к Chart 1 и Chart 2
   - Проверить независимость zoom состояний
   - Сбросить диапазон только одного графика

3. **Переключение режимов**
   - Установить диапазон в Single Mode
   - Переключиться в Dual Mode
   - Проверить сохранение диапазона Chart 1

4. **Валидация**
   - Проверить ошибки для каждого графика отдельно
   - Проверить корректность сообщений с номером графика

## Статус
✅ Реализовано
✅ Поддержка Dual Mode
⏳ Требуется тестирование

## Связанные документы
- FEATURE_ZOOM_SELECTION_COLOR.md (zoom мышью)
- REFACTORING_UI_SIMPLIFICATION.md (упрощение UI)