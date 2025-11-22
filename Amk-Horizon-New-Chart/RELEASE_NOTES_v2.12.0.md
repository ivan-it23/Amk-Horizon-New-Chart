# 🎉 Release Notes - AMK-Horizon-New-Chart v2.12.0

**Дата релиза:** 21 ноября 2025  
**Тип релиза:** Feature Release

## 📝 Обзор

Версия 2.12.0 добавляет полную поддержку zoom и скроллинга по оси Y, что позволяет детально анализировать данные как по горизонтали, так и по вертикали.

## ✨ Новые возможности

### 1. Zoom по оси Y
**Изменение:** При выделении области мышью теперь происходит приближение как по оси X, так и по оси Y

**Как работает:**
- Выделите область на графике мышью (левая кнопка + перетаскивание)
- График автоматически увеличит выбранный участок по обеим осям
- Минимальный диапазон Y: 10 единиц (аналогично оси X)

**Преимущества:**
- Детальный анализ конкретных участков данных
- Возможность рассмотреть мелкие изменения амплитуд и фаз
- Одновременное приближение по X и Y для максимальной детализации

### 2. Вертикальный scrollbar
**Новое:** Добавлен вертикальный scrollbar для прокрутки графика по оси Y

**Расположение:**
- Для одной оси: справа от графика
- Для двойных осей: применяется к обеим осям одновременно
- Появляется только при активном zoom

**Управление:**
- Перетаскивание ползунка для плавной прокрутки
- Клик по стрелкам для точной настройки
- Page Up/Page Down для быстрой прокрутки

### 3. Ограничение минимального диапазона Y
**Новое правило:** Нельзя приближать график больше, чем до диапазона в 10 единиц по оси Y

**Логика:**
- При попытке zoom с диапазоном Y < 10 единиц отображается сообщение
- Минимальный диапазон обеспечивает читаемость подписей осей
- Применяется как при выделении области мышью, так и при скроллинге

**Сообщение пользователю:**
```
Cannot zoom further: Y-axis range must be at least 10 units.
Minimum range is 10 units.
```

### 4. Двойные оси
**Поведение:** Zoom и scroll применяются к обеим осям Y одновременно

**Особенности:**
- Левая ось (фазы) и правая ось (амплитуды) масштабируются пропорционально
- Вертикальный scrollbar управляет обеими осями
- Сохраняется соотношение между осями

### 6. Поддержка режима кроссплота (Temperature)
**Новое:** Вертикальный скроллинг теперь работает и в режиме кроссплота

**Особенности:**
- Ось X: температура (вместо номера кадра)
- Ось Y: zoom и scroll работают так же, как в обычном режиме
- Кривые корректно обрезаются по границам графика
- Clipping region предотвращает наложение на легенду

### 5. Сброс zoom
**Обновлено:** Правая кнопка мыши сбрасывает zoom по X и Y одновременно

**Поведение:**
- Клик правой кнопкой на графике
- Мгновенный возврат к полному диапазону данных
- Скроллбары автоматически скрываются

## 🐛 Исправленные ошибки

### Проблема: График не обновлялся при вертикальном скроллинге
**Симптомы:**
- Значения minY и maxY обновлялись в консоли
- Вертикальный scrollbar двигался
- График не перерисовывался

**Причина:**
В функции `DrawChartWithCurves` при вызове `CalculateCurveData` передавались параметры `zoom->minY` и `zoom->maxY`, которые использовались для **фильтрации данных**, а не только для **отображения диапазона**. Это приводило к тому, что при вертикальном скроллинге данные пересчитывались с новыми границами фильтрации, что было неправильно.

**Решение:**
Изменена логика в `DrawChartWithCurves`:
```cpp
// БЫЛО (неправильно):
curveSelectionModule->CalculateCurveData(config, dataModule.get(),
                                       leftAxisData, rightAxisData,
                                       leftAxisCurves, rightAxisCurves,
                                       zoom->minX, zoom->maxX, zoom->minY, zoom->maxY);

// СТАЛО (правильно):
curveSelectionModule->CalculateCurveData(config, dataModule.get(),
                                       leftAxisData, rightAxisData,
                                       leftAxisCurves, rightAxisCurves,
                                       zoom->minX, zoom->maxX, -1e10, 1e10);
```

**Результат:**
- ✅ Вертикальный скроллинг теперь работает корректно
- ✅ График обновляется при движении scrollbar
- ✅ Данные не фильтруются по Y, только диапазон отображения меняется

### Проблема: Кривые выходили за границы графика при скроллинге
**Симптомы:**
- При вертикальном скроллинге кривые выходили за границы области графика
- Кривые налезали на легенду и заголовок графика
- Визуальное наложение элементов интерфейса

**Причина:**
В функциях `DrawDualAxisData` и `DrawDualAxisDataWithZoom` не использовался clipping region для ограничения области рисования кривых.

**Решение:**
Добавлен clipping region в обе функции отрисовки:
```cpp
// Создаем clipping region, чтобы кривые не выходили за границы графика
HRGN hClipRegion = CreateRectRgn(plotRect.left, plotRect.top, plotRect.right, plotRect.bottom);
HRGN hOldClipRegion = CreateRectRgn(0, 0, 0, 0);
int regionType = GetClipRgn(hdc, hOldClipRegion);
SelectClipRgn(hdc, hClipRegion);

// ... отрисовка кривых ...

// Восстанавливаем предыдущий clipping region
if (regionType == 1) {
    SelectClipRgn(hdc, hOldClipRegion);
} else {
    SelectClipRgn(hdc, NULL);
}
DeleteObject(hClipRegion);
DeleteObject(hOldClipRegion);
```

**Результат:**
- ✅ Кривые корректно обрезаются по границам области графика
- ✅ Нет наложения на легенду и заголовок
- ✅ Чистый и профессиональный вид графика

### Проблема: Подписи оси Y не обновлялись при скроллинге
**Симптомы:**
- При вертикальном скроллинге подписи оси Y показывали старые значения
- Значения не соответствовали текущему zoom диапазону
- Подписи оставались из полного диапазона данных

**Причина:**
Функции `DrawLeftAxis` и `DrawRightAxis` вычисляли диапазон из данных, а не использовали переданные параметры zoom (`minYData`, `maxYData`).

**Решение:**
Реализованы функции `DrawLeftAxisWithZoom` и `DrawRightAxisWithZoom`, которые используют переданные параметры zoom:
```cpp
void ChartModule::DrawLeftAxisWithZoom(HDC hdc, const RECT& plotRect,
                                      const std::vector<std::vector<float>>& leftAxisData,
                                      const std::string& units,
                                      double minYData, double maxYData) const {
    // Используем переданные параметры zoom вместо вычисления из данных
    float minVal = static_cast<float>(minYData);
    float maxVal = static_cast<float>(maxYData);
    
    // ... отрисовка оси с правильными подписями ...
}
```

**Решение (дополнение):**
Также обновлена функция `DrawDualAxisGrid` для поддержки zoom по Y:
```cpp
void ChartModule::DrawDualAxisGrid(HDC hdc, const RECT& plotRect,
                                  const std::vector<std::vector<float>>& leftAxisData,
                                  const std::vector<std::vector<float>>& rightAxisData,
                                  double minXData, double maxXData,
                                  double minYData, double maxYData) const {
    // Используем переданные параметры zoom для Y вместо вычисления из данных
    float leftMin = static_cast<float>(minYData);
    float leftMax = static_cast<float>(maxYData);
    
    // ... отрисовка сетки с правильными значениями ...
}
```

**Результат:**
- ✅ Подписи оси Y корректно обновляются при скроллинге
- ✅ Значения соответствуют текущему zoom диапазону
- ✅ Сетка графика отрисовывается с правильными значениями Y
- ✅ Работает как в обычном режиме, так и в режиме кроссплота

## 🔧 Технические изменения

### UIModule.h
**Добавлены поля:**
```cpp
HWND hChart1VScrollBar;  // Вертикальный скроллбар для первого графика
HWND hChart2VScrollBar;  // Вертикальный скроллбар для второго графика
```

**Добавлены методы:**
```cpp
void SetVScrollBarRange(int chartNumber, int min, int max, int page, int pos);
HWND GetVScrollBar(int chartNumber) const;
```

### UIModule.cpp
**Создание вертикальных scrollbar:**
```cpp
hChart1VScrollBar = CreateWindow(L"SCROLLBAR", NULL,
    WS_CHILD | SBS_VERT,
    0, 0, 20, 100, hWnd, (HMENU)IDC_CHART1_VSCROLL, hInst, NULL);

hChart2VScrollBar = CreateWindow(L"SCROLLBAR", NULL,
    WS_CHILD | SBS_VERT,
    0, 0, 20, 100, hWnd, (HMENU)IDC_CHART2_VSCROLL, hInst, NULL);
```

**Обновлено позиционирование:**
```cpp
void UIModule::UpdateScrollBarPositions(int chartNumber, const RECT& chartRect) {
    // Вертикальный скроллбар располагается справа от графика
    SetWindowPos(hChart1VScrollBar, HWND_TOP,
                 chartRect.right - SCROLLBAR_SIZE - MARGIN_RIGHT,
                 chartRect.top + MARGIN_TOP,
                 SCROLLBAR_SIZE,
                 chartRect.bottom - chartRect.top - MARGIN_TOP - MARGIN_BOTTOM - SCROLLBAR_SIZE,
                 SWP_NOZORDER);
}
```

### ChartManager.cpp
**Обновлен HandleMouseUp:**
```cpp
// Проверяем минимальный диапазон для Y (аналогично X)
const double MIN_Y_RANGE = 10.0;
double yRange = maxDataY - minDataY;

if (yRange < MIN_Y_RANGE) {
    MessageBox(hWnd, 
        L"Cannot zoom further: Y-axis range must be at least 10 units.\nMinimum range is 10 units.",
        L"Zoom Limit Reached", 
        MB_OK | MB_ICONINFORMATION);
}
```

**Обновлен HandleScrollEvent:**
```cpp
if (isVertical) {
    // Вертикальный скроллинг (ось Y)
    double totalYRange = maxYFull - minYFull;
    double viewYRange = zoom->maxY - zoom->minY;
    
    // Вертикальный скроллбар инвертирован (верх = 0, низ = 100)
    double newMinY = minYFull + ((100.0 - newPos) / 100.0) * (totalYRange - viewYRange);
    zoom->minY = newMinY;
    zoom->maxY = newMinY + viewYRange;
}
```

**Обновлен UpdateScrollBars:**
```cpp
// Настраиваем диапазон вертикального скроллбара (ось Y)
double totalYRange = maxYFull - minYFull;
double viewRangeY = chart1Zoom.maxY - chart1Zoom.minY;

// Вертикальный скроллбар инвертирован (верх = 0, низ = 100)
int vScrollPos = static_cast<int>(100.0 - ((chart1Zoom.minY - minYFull) / (totalYRange - viewRangeY)) * 100.0);
uiModule->SetVScrollBarRange(1, 0, 100, static_cast<int>((viewRangeY / totalYRange) * 100), vScrollPos);
```

**Обновлен DrawChartWithCurves:**
```cpp
if (zoom->isZoomed) {
    // Используем zoom параметры только для X (не фильтруем по Y!)
    // Y zoom используется только для отображения, не для фильтрации данных
    curveSelectionModule->CalculateCurveData(config, dataModule.get(),
                                           leftAxisData, rightAxisData,
                                           leftAxisCurves, rightAxisCurves,
                                           zoom->minX, zoom->maxX, -1e10, 1e10);
}
```

**Обновлен HandleScrollEvent (вертикальный скроллинг):**
```cpp
if (isVertical) {
    // Вертикальный скроллинг (ось Y)
    if (!zoom->isZoomed) {
        // Если график не в режиме zoom, вертикальный скроллинг не работает
        return;
    }
    
    // Получаем полный диапазон Y из данных (без Y zoom, но с X zoom если есть)
    curveSelectionModule->CalculateCurveData(config, dataModule.get(), 
                                            leftAxisDataFull, rightAxisDataFull, 
                                            leftAxisCurves, rightAxisCurves,
                                            zoom->minX, zoom->maxX, -1e10, 1e10);
    
    double totalYRange = maxYFull - minYFull;
    double viewYRange = zoom->maxY - zoom->minY;
    
    // Вертикальный скроллбар инвертирован (верх = 0, низ = 100)
    double scrollRatio = (100.0 - newPos) / 100.0;
    double newMinY = minYFull + scrollRatio * (totalYRange - viewYRange);
    zoom->minY = newMinY;
    zoom->maxY = newMinY + viewYRange;
}
```

### Resource.h
**Добавлены константы:**
```cpp
#define IDC_CHART1_VSCROLL		1041
#define IDC_CHART2_VSCROLL		1042
```

## 🎯 Результаты

### До изменений (v2.11.0)
- ❌ Zoom только по оси X
- ❌ Скроллинг только по горизонтали
- ❌ Невозможно детально рассмотреть вертикальные изменения

### После изменений (v2.12.0)
- ✅ Zoom по обеим осям (X и Y)
- ✅ Скроллинг по горизонтали и вертикали
- ✅ Детальный анализ любых участков графика
- ✅ Минимальные ограничения для обеих осей
- ✅ Интуитивное управление

## 📖 Использование

### Zoom по обеим осям
1. Выделите область на графике мышью (левая кнопка + перетаскивание)
2. График автоматически увеличит выбранный участок по X и Y
3. Если диапазон X или Y < 10, появится сообщение о достижении предела

### Вертикальный скроллинг
1. После zoom появятся горизонтальный и вертикальный scrollbar
2. Используйте вертикальный scrollbar для прокрутки по оси Y
3. Перетаскивайте ползунок или кликайте по стрелкам

### Сброс zoom
1. Кликните правой кнопкой мыши на графике
2. График вернётся к полному диапазону по X и Y
3. Scrollbar автоматически скроются

### Примеры использования
- **Анализ шумов**: Приблизьте участок с малыми амплитудами для детального изучения
- **Фазовые переходы**: Увеличьте область с резкими изменениями фаз
- **Сравнение каналов**: Приблизьте участок для точного сравнения разных передатчиков
- **Температурные зависимости**: В режиме кроссплота увеличьте интересующий диапазон температур и значений

## 🔍 Обратная связь

Эти изменения были внесены для улучшения возможностей анализа данных:
- **Требование:** Возможность детально рассматривать данные по вертикали
- **Требование:** Скроллинг по обеим осям для навигации
- **Реализация:** Полная поддержка zoom и scroll по X и Y

## 🚀 Совместимость

- ✅ Полная обратная совместимость с файлами данных
- ✅ Все существующие функции работают без изменений
- ✅ Сохраненные конфигурации графиков совместимы
- ✅ Работает в обоих режимах: Frame Number и Temperature

## 📊 Статистика изменений

- **Изменённых файлов:** 5
- **Добавлено строк:** ~200
- **Изменено строк:** ~50
- **Новых функций:** 2 (SetVScrollBarRange, GetVScrollBar)
- **Новых констант:** 2 (IDC_CHART1_VSCROLL, IDC_CHART2_VSCROLL)

---

**AMK-Horizon-New-Chart v2.12.0** - Полный контроль над zoom и скроллингом по обеим осям! 🎉
