# 🎉 Release Notes - AMK-Horizon-New-Chart v2.11.0

**Дата релиза:** 20 ноября 2025  
**Тип релиза:** Feature Release

## 📝 Обзор

Версия 2.11.0 изменяет формат отображения оси X на целые числа и добавляет ограничение на минимальный диапазон zoom для обеспечения читаемости подписей осей.

## ✨ Новые возможности

### 1. Целочисленные подписи оси X
**Изменение:** Все подписи оси X теперь отображаются как целые числа без дробной части

**Применяется к:**
- **Frame Number режим** - номера кадров (0, 1, 2, 3, ... вместо 0.0, 12.5, 25.0, ...)
- **Temperature режим** - температура в градусах Цельсия (12, 13, 14, ... вместо 12.5, 13.0, 13.5, ...)
- **X Range Controls** - поля ввода диапазона (целые числа)
- **Статусная строка** - координата X (целое число)

**Устранение дубликатов:**
- Добавлено отслеживание уже отрисованных значений с помощью `std::set<int>`
- Каждое целое значение отображается только один раз
- Пример: вместо "415 416 416 417 417 418" теперь "415 416 417 418"
- Применяется как для Frame Number, так и для Temperature режима

**Преимущества:**
- Более чистый и профессиональный вид графиков
- Легче читать и интерпретировать значения
- Нет повторяющихся подписей на оси X
- Соответствует требованиям пользователей

### 2. Ограничение минимального диапазона zoom
**Новое ограничение:** Нельзя приближать график больше, чем до диапазона в 10 точек

**Логика:**
- При попытке zoom с диапазоном < 10 точек отображается сообщение
- Минимальный диапазон обеспечивает, что подписи осей остаются целыми числами с шагом минимум 1
- Применяется как при выделении области мышью, так и при ручном вводе X Range

**Сообщение пользователю:**
```
Cannot zoom further: X-axis labels must be integers with minimum spacing of 1.
Minimum range is 10 points.
```

**Для X Range Controls:**
```
Chart 1: X Range must be at least 10 points.
X-axis labels must be integers with minimum spacing of 1.
```

## 🔧 Технические изменения

### ChartModule.cpp
**Добавлен include:**
```cpp
#include <set>     // Для std::set (отслеживание уникальных значений)
```

**Строка ~827 (Frame Number режим):**
```cpp
// Было:
ssX << std::fixed << std::setprecision(1) << currentXVal;
std::wstring xLabel = ssX.str();
TextOut(hdc, screenX - 10, plotRect.bottom + 5, xLabel.c_str(), (int)xLabel.length());

// Стало:
std::set<int> drawnLabels;
// ...
int roundedValue = static_cast<int>(std::round(currentXVal));
if (drawnLabels.find(roundedValue) == drawnLabels.end()) {
    drawnLabels.insert(roundedValue);
    std::wstringstream ssX;
    ssX << roundedValue;
    std::wstring xLabel = ssX.str();
    TextOut(hdc, screenX - 10, plotRect.bottom + 5, xLabel.c_str(), (int)xLabel.length());
}
```

**Строка ~1437 (Temperature режим):**
```cpp
// Было:
float temp = minTemp + (maxTemp - minTemp) * i / numVerticalLines;
swprintf_s(label, L"%.0f", temp);
TextOut(hdc, x - textSize.cx / 2, plotRect.bottom + 5, label, (int)wcslen(label));

// Стало:
std::set<int> drawnTempLabels;
// ...
float temp = minTemp + (maxTemp - minTemp) * i / numVerticalLines;
int roundedTemp = static_cast<int>(std::round(temp));
if (drawnTempLabels.find(roundedTemp) == drawnTempLabels.end()) {
    drawnTempLabels.insert(roundedTemp);
    swprintf_s(label, L"%d", roundedTemp);
    TextOut(hdc, x - textSize.cx / 2, plotRect.bottom + 5, label, (int)wcslen(label));
}
```

### UIModule.cpp
**Строки 331, 332, 384, 385:**
```cpp
// Было:
swprintf_s(fromText, L"%.1f", minX);
swprintf_s(toText, L"%.1f", maxX);

// Стало:
swprintf_s(fromText, L"%.0f", minX);
swprintf_s(toText, L"%.0f", maxX);
```

**Строка 431:**
```cpp
// Было:
swprintf_s(xText, L"X: %.1f", x);

// Стало:
swprintf_s(xText, L"X: %.0f", x);
```

### ChartManager.cpp
**Строка ~994:**
```cpp
// Было:
swprintf_s(message, L"Chart %d: X Range must be between 0 and %.1f", chartNumber, dataSize);

// Стало:
swprintf_s(message, L"Chart %d: X Range must be between 0 and %.0f", chartNumber, dataSize);
```

**HandleMouseUp (строка ~340):**
```cpp
// Добавлена проверка минимального диапазона
const double MIN_X_RANGE = 10.0;
double xRange = maxDataX - minDataX;

if (xRange < MIN_X_RANGE) {
    MessageBox(hWnd, 
        L"Cannot zoom further: X-axis labels must be integers with minimum spacing of 1.\nMinimum range is 10 points.",
        L"Zoom Limit Reached", 
        MB_OK | MB_ICONINFORMATION);
} else {
    // Применяем zoom
}
```

**ApplyXRange (строка ~1005):**
```cpp
// Добавлена проверка минимального диапазона
const double MIN_X_RANGE = 10.0;
double xRange = toX - fromX;

if (xRange < MIN_X_RANGE) {
    wchar_t message[256];
    swprintf_s(message, L"Chart %d: X Range must be at least %.0f points.\nX-axis labels must be integers with minimum spacing of 1.", 
               chartNumber, MIN_X_RANGE);
    MessageBox(hWnd, message, L"X Range Error", MB_OK | MB_ICONERROR);
    return;
}
```

## 🎯 Результаты

### До изменений (v2.10.1)
- ❌ Подписи оси X с дробной частью (0.0, 12.5, 25.0, 37.5, ...)
- ❌ Можно было приблизить до очень малых диапазонов (< 1 точки)
- ❌ Подписи могли перекрываться при сильном zoom

### После изменений (v2.11.0)
- ✅ Подписи оси X целые числа (0, 13, 25, 38, ...)
- ✅ Минимальный диапазон 10 точек гарантирует читаемость
- ✅ Подписи всегда различимы и не перекрываются
- ✅ Профессиональный вид графиков

## 📖 Использование

### Zoom с помощью мыши
1. Выделите область на графике (левая кнопка + перетаскивание)
2. Если диапазон >= 10 точек, zoom применится
3. Если диапазон < 10 точек, появится сообщение о достижении предела

### X Range Controls
1. Введите значения From и To (целые числа)
2. Убедитесь, что To - From >= 10
3. Нажмите Apply
4. Если диапазон < 10, появится сообщение об ошибке

### Примеры допустимых диапазонов
- ✅ From: 0, To: 10 (диапазон = 10)
- ✅ From: 100, To: 150 (диапазон = 50)
- ✅ From: 500, To: 1000 (диапазон = 500)
- ❌ From: 0, To: 5 (диапазон = 5, слишком мал)
- ❌ From: 100, To: 108 (диапазон = 8, слишком мал)

## 🔍 Обратная связь

Эти изменения были внесены на основе обратной связи от сотрудников компании:
- **Требование:** Ось X должна показывать только целые значения
- **Требование:** Нельзя приближать больше, чем до уровня где подписи отличаются на 1
- **Реализация:** Минимальный диапазон 10 точек обеспечивает оба требования

## 🚀 Совместимость

- ✅ Полная обратная совместимость с файлами данных
- ✅ Все существующие функции работают без изменений
- ✅ Сохраненные конфигурации графиков совместимы

---

**AMK-Horizon-New-Chart v2.11.0** - Целочисленные подписи осей и контроль минимального zoom! 🎉
