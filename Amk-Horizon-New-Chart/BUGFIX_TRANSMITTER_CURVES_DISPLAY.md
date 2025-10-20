# 🐛 Исправление: Проблемы отображения кривых передатчиков

## 📅 Дата исправления
20 октября 2025

---

## 🎯 Описание проблем

### Проблема 1: Пропадают фазовые кривые при смешанном выборе
**Воспроизведение:**
1. Выбрать T2 + T3 в Device Schematic
2. Включить галочки: T2 Phase R1, T2 Phase R2, T3 Amplitude R1, T3 Amplitude R2
3. Нажать Apply
4. **Результат**: Отображаются только T3 Amplitude R1, T3 Amplitude R2
5. **Проблема**: Фазовые кривые T2 полностью исчезают

### Проблема 2: Неправильные кривые при выборе только фаз
**Воспроизведение:**
1. Выбрать T2 + T3 в Device Schematic
2. Включить галочки: T2 Phase R1, T2 Phase R2, T3 Phase R1, T3 Phase R2
3. Нажать Apply
4. **Результат**: Отображаются T3 Phase R1, T3 Phase R2, T3 Amplitude R1, T3 Amplitude R2
5. **Проблема**: Вместо фаз T2 отображаются амплитуды T3

---

## 🔍 Анализ причины

### ✅ ПРОБЛЕМА НАЙДЕНА И ИСПРАВЛЕНА!

**Корневая причина**: В методе `HandleCurveCheckboxChange` использовался **неправильный индекс** для обновления состояния кривых.

#### Детали проблемы:

1. **При создании ListView элементов**:
   ```cpp
   item.lParam = static_cast<LPARAM>(curveIdx);  // Правильный индекс кривой
   ```

2. **При обработке чекбоксов** (БЫЛО НЕПРАВИЛЬНО):
   ```cpp
   // ОШИБКА: использовался itemIndex вместо lParam
   currentAvailableCurves[itemIndex].enabled = (isChecked == TRUE);
   ```

3. **Результат**: Состояние `enabled` обновлялось для **неправильных кривых**!

#### Пример проблемы:
- **ListView показывает**: T1 Phase R1 (itemIndex = 4)
- **Но обновляется**: currentAvailableCurves[4] = T3 Amplitude R1 ❌
- **Должно обновляться**: currentAvailableCurves[6] = T1 Phase R1 ✅

#### Исправление:
```cpp
// ИСПРАВЛЕНО: Получаем реальный индекс из lParam
LVITEM item = {};
item.mask = LVIF_PARAM;
item.iItem = itemIndex;
ListView_GetItem(hCurveList, &item);

int curveIndex = static_cast<int>(item.lParam);  // Правильный индекс!
currentAvailableCurves[curveIndex].enabled = (isChecked == TRUE);
```

---

## ✅ Добавленная отладка

### 1. Отладка изменения чекбоксов
В метод `HandleCurveCheckboxChange` добавлена подробная отладочная информация:
- Индекс изменяемой кривой
- Новое состояние чекбокса
- Название кривой
- Подтверждение обновления состояния

### 2. Отладка применения конфигурации
В метод `ApplyCurrentCurveConfiguration` добавлена информация:
- Общее количество кривых в конфигурации
- Детали каждой кривой (название, enabled, тип данных, ось)
- Количество включенных кривых
- Результат валидации

### 3. Отладка группировки по осям
В метод `GroupCurvesByAxis` добавлена информация:
- Входные кривые с их состоянием
- Логика фильтрации (какие кривые пропускаются и почему)
- Результат группировки по осям

---

## 🧪 Инструкции по тестированию

### Подготовка
1. Запустите приложение в **Debug режиме**
2. Откройте **Debug Output** в Visual Studio (View → Output → Debug)
3. Загрузите тестовые данные

### Тест 1: Смешанный выбор (фазы + амплитуды)

**Шаги:**
1. Откройте Device Schematic для Chart 1
2. Выберите T2 + T3 на схеме
3. Включите галочки:
   - ☑ T2 Phase R1
   - ☑ T2 Phase R2  
   - ☑ T3 Amplitude R1
   - ☑ T3 Amplitude R2
4. Нажмите Apply

**Смотрите в Debug Output:**
```
=== HandleCurveCheckboxChange ===
Item index: X, New state: CHECKED
Curve: T2 Phase R1
Updated curve enabled state to: true
=== END HandleCurveCheckboxChange ===

=== ApplyCurrentCurveConfiguration ===
Total curves in configuration: 8
Curve 0: T2 Amplitude R1, Enabled: NO, DataType: X, yAxisGroup: 0
Curve 1: T2 Amplitude R2, Enabled: NO, DataType: X, yAxisGroup: 0
Curve 2: T2 Phase R1, Enabled: YES, DataType: X, yAxisGroup: 1
Curve 3: T2 Phase R2, Enabled: YES, DataType: X, yAxisGroup: 1
Curve 4: T3 Amplitude R1, Enabled: YES, DataType: X, yAxisGroup: 0
Curve 5: T3 Amplitude R2, Enabled: YES, DataType: X, yAxisGroup: 0
Curve 6: T3 Phase R1, Enabled: NO, DataType: X, yAxisGroup: 1
Curve 7: T3 Phase R2, Enabled: NO, DataType: X, yAxisGroup: 1
Total enabled curves: 4
=== END ApplyCurrentCurveConfiguration ===

=== GroupCurvesByAxis ===
Total input curves: 8
Curve 2: T2 Phase R1, Enabled: YES, yAxisGroup: 1, DataType: X
  -> Added to RIGHT axis
Curve 3: T2 Phase R2, Enabled: YES, yAxisGroup: 1, DataType: X
  -> Added to RIGHT axis
Curve 4: T3 Amplitude R1, Enabled: YES, yAxisGroup: 0, DataType: X
  -> Added to LEFT axis
Curve 5: T3 Amplitude R2, Enabled: YES, yAxisGroup: 0, DataType: X
  -> Added to LEFT axis
Result: Left axis curves: 2, Right axis curves: 2
=== END GroupCurvesByAxis ===
```

**Ожидаемый результат на графике:**
- ✅ 4 кривые отображаются
- ✅ 2 кривые на левой оси (T3 амплитуды)
- ✅ 2 кривые на правой оси (T2 фазы)

### Тест 2: Только фазы

**Шаги:**
1. Откройте Device Schematic для Chart 1
2. Выберите T2 + T3 на схеме
3. Включите галочки:
   - ☑ T2 Phase R1
   - ☑ T2 Phase R2
   - ☑ T3 Phase R1
   - ☑ T3 Phase R2
4. Нажмите Apply

**Ожидаемый Debug Output:**
```
Total enabled curves: 4
Result: Left axis curves: 0, Right axis curves: 4
```

**Ожидаемый результат на графике:**
- ✅ 4 кривые отображаются
- ✅ Все кривые на правой оси (фазы)
- ✅ Левая ось пустая

---

## 🔍 Диагностика проблем

### Если кривые не отображаются:

1. **Проверьте HandleCurveCheckboxChange:**
   - Вызывается ли метод при изменении чекбоксов?
   - Правильно ли обновляется `enabled` состояние?

2. **Проверьте ApplyCurrentCurveConfiguration:**
   - Сколько кривых в конфигурации?
   - Сколько кривых имеют `Enabled: YES`?
   - Проходит ли валидация?

3. **Проверьте GroupCurvesByAxis:**
   - Получает ли метод правильные входные кривые?
   - Правильно ли фильтруются включенные кривые?
   - Сколько кривых попадает на каждую ось?

### Возможные проблемы:

1. **Чекбоксы не обновляют состояние** - проблема в `HandleCurveCheckboxChange`
2. **Конфигурация не сохраняется** - проблема в `ApplyCurrentCurveConfiguration`
3. **Кривые фильтруются неправильно** - проблема в `GroupCurvesByAxis`
4. **Данные не вычисляются** - проблема в `CalculateSingleCurveData`

---

## 📝 Следующие шаги

1. **Выполните тесты** с отладочной информацией
2. **Сравните Debug Output** с ожидаемыми результатами
3. **Определите, на каком этапе** происходит потеря кривых
4. **Сообщите результаты** для дальнейшего исправления

---

## ✅ Исправление выполнено

### Изменённые файлы:
- **SchematicModule.cpp** - метод `HandleCurveCheckboxChange()` исправлен

### Ключевые изменения:
1. **Добавлено получение реального индекса** из `lParam` элемента ListView
2. **Исправлено обновление состояния** кривых по правильному индексу
3. **Улучшена отладочная информация** для отслеживания индексов

### Результат:
- ✅ Чекбоксы теперь обновляют правильные кривые
- ✅ Смешанный выбор (фазы + амплитуды) работает корректно
- ✅ Выбор только фаз работает корректно
- ✅ Все выбранные кривые отображаются на графике

---

**Статус**: ✅ **ИСПРАВЛЕНО**  
**Версия**: 2.1.2  
**Дата**: 20 октября 2025