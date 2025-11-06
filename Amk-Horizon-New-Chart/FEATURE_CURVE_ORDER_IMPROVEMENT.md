# 📊 Улучшение порядка отображения кривых - Версия 2.6.2

**Дата**: 6 ноября 2025  
**Версия**: 2.6.2

## 📋 Описание проблемы

### Симптомы
При выборе приёмников (R1, R2 или R1+R2) в диалоге Device Schematic кривые отображались вперемешку:
- **Было**: T1 Amplitude, T1 Phase, T2 Amplitude, T2 Phase, T3 Amplitude, T3 Phase, T4 Amplitude, T4 Phase
- **Проблема**: Неудобно для анализа - фазы и амплитуды перемешаны
- **Причина**: Фазы более важны для анализа и должны быть в начале списка

### Требование
Изменить порядок отображения кривых так, чтобы:
1. **Фазы отображались первыми** (более важны для анализа)
2. **Амплитуды отображались вторыми**
3. Порядок применялся для всех случаев выбора приёмников

## ✅ Решение

### Изменённый порядок

#### Случай 1: Выбран только R1 или только R2
**Новый порядок** - сначала все фазы, потом все амплитуды:
1. ☑ T1 Phase R1 (mdeg) [Right Axis]
2. ☑ T2 Phase R1 (mdeg) [Right Axis]
3. ☑ T3 Phase R1 (mdeg) [Right Axis]
4. ☑ T4 Phase R1 (mdeg) [Right Axis]
5. ☑ T1 Amplitude R1 (mV) [Left Axis]
6. ☑ T2 Amplitude R1 (mV) [Left Axis]
7. ☑ T3 Amplitude R1 (mV) [Left Axis]
8. ☑ T4 Amplitude R1 (mV) [Left Axis]

#### Случай 2: Выбраны оба приёмника R1+R2
**Новый порядок** - сначала все Phase Difference, потом все Amplitude Ratio:
1. ☑ T1 Phase Difference (mdeg) [Right Axis]
2. ☑ T2 Phase Difference (mdeg) [Right Axis]
3. ☑ T3 Phase Difference (mdeg) [Right Axis]
4. ☑ T4 Phase Difference (mdeg) [Right Axis]
5. ☑ T1 Amplitude Ratio (V/V) [Left Axis]
6. ☑ T2 Amplitude Ratio (V/V) [Left Axis]
7. ☑ T3 Amplitude Ratio (V/V) [Left Axis]
8. ☑ T4 Amplitude Ratio (V/V) [Left Axis]

## 🔧 Техническая реализация

### Изменённые методы

#### 1. AddSingleReceiverCurves()
**Было**: Сначала амплитуды, потом фазы (в одном цикле)
**Стало**: Два отдельных цикла - сначала все фазы, потом все амплитуды

```cpp
// ИЗМЕНЕНО: Сначала добавляем фазы (более важны для анализа)
for (int i = 0; i < 4; i++) {
    if (addPhases) {
        CurveDefinition phaseCurve = CreatePhaseCurve(i, receiverIndex, selection.frequency);
        // ... настройка кривой
        curves.push_back(phaseCurve);
    }
}

// ИЗМЕНЕНО: Потом добавляем амплитуды
for (int i = 0; i < 4; i++) {
    if (addAmplitudes) {
        CurveDefinition amplitudeCurve = CreateAmplitudeCurve(i, receiverIndex, selection.frequency);
        // ... настройка кривой
        curves.push_back(amplitudeCurve);
    }
}
```

#### 2. AddTwoReceiversCurves()
**Было**: Сначала отношения амплитуд, потом разности фаз (в одном цикле)
**Стало**: Два отдельных цикла - сначала все разности фаз, потом все отношения амплитуд

```cpp
// ИЗМЕНЕНО: Сначала добавляем разности фаз (более важны для анализа)
for (int i = 0; i < 4; i++) {
    if (addDifferences) {
        CurveDefinition differenceCurve = CreateDifferenceCurve(i, selection.frequency);
        // ... настройка кривой
        curves.push_back(differenceCurve);
    }
}

// ИЗМЕНЕНО: Потом добавляем отношения амплитуд
for (int i = 0; i < 4; i++) {
    if (addRatios) {
        CurveDefinition ratioCurve = CreateRatioCurve(i, selection.frequency);
        // ... настройка кривой
        curves.push_back(ratioCurve);
    }
}
```

#### 3. GetAvailableCurves()
**Было**: Вызывалась сортировка `SortCurvesByType()`, которая меняла порядок
**Стало**: Сортировка отключена, сохраняется порядок добавления

```cpp
// ИЗМЕНЕНО v2.6.2: Отключена сортировка, чтобы сохранить порядок добавления кривых
// Теперь фазы добавляются первыми (более важны), затем амплитуды
// SortCurvesByType(curves);
```

## ✅ Преимущества

### 1. Улучшенная эргономика
- Фазы в начале списка - легче найти и выбрать
- Логичная группировка - все фазы вместе, все амплитуды вместе
- Соответствует приоритетам анализа данных

### 2. Удобство для пользователей
- Сотрудники чаще работают с фазами - они теперь сверху
- Не нужно прокручивать список для поиска нужного типа данных
- Визуально понятная структура списка

### 3. Согласованность
- Одинаковый порядок для всех случаев (R1, R2, R1+R2)
- Предсказуемое поведение интерфейса
- Легко запомнить расположение элементов

## 📊 Визуальное сравнение

### До изменений (v2.6.1)
```
Available Curves
☑ T1 Amplitude R1 (mV) [Left Axis]
☑ T1 Phase R1 (mdeg) [Right Axis]
☑ T2 Amplitude R1 (mV) [Left Axis]
☑ T2 Phase R1 (mdeg) [Right Axis]
☑ T3 Amplitude R1 (mV) [Left Axis]
☑ T3 Phase R1 (mdeg) [Right Axis]
☑ T4 Amplitude R1 (mV) [Left Axis]
☑ T4 Phase R1 (mdeg) [Right Axis]
```

### После изменений (v2.6.2)
```
Available Curves
☑ T1 Phase R1 (mdeg) [Right Axis]
☑ T2 Phase R1 (mdeg) [Right Axis]
☑ T3 Phase R1 (mdeg) [Right Axis]
☑ T4 Phase R1 (mdeg) [Right Axis]
☑ T1 Amplitude R1 (mV) [Left Axis]
☑ T2 Amplitude R1 (mV) [Left Axis]
☑ T3 Amplitude R1 (mV) [Left Axis]
☑ T4 Amplitude R1 (mV) [Left Axis]
```

## 🧪 Тестирование

### Тестовые сценарии

#### Сценарий 1: Выбор R1
```
Действие:
1. Открыть Device Schematic
2. Выбрать только R1
3. Проверить порядок кривых в Available Curves

Ожидаемый результат:
- Сначала 4 кривые фаз (T1-T4 Phase R1)
- Затем 4 кривые амплитуд (T1-T4 Amplitude R1)
```

#### Сценарий 2: Выбор R2
```
Действие:
1. Открыть Device Schematic
2. Выбрать только R2
3. Проверить порядок кривых в Available Curves

Ожидаемый результат:
- Сначала 4 кривые фаз (T1-T4 Phase R2)
- Затем 4 кривые амплитуд (T1-T4 Amplitude R2)
```

#### Сценарий 3: Выбор R1+R2
```
Действие:
1. Открыть Device Schematic
2. Выбрать оба приёмника R1 и R2
3. Проверить порядок кривых в Available Curves

Ожидаемый результат:
- Сначала 4 кривые разностей фаз (T1-T4 Phase Difference)
- Затем 4 кривые отношений амплитуд (T1-T4 Amplitude Ratio)
```

#### Сценарий 4: Пресеты
```
Действие:
1. Открыть Device Schematic
2. Нажать "All Phase R1"
3. Проверить порядок кривых

Ожидаемый результат:
- Только 4 кривые фаз (T1-T4 Phase R1)
- Порядок: T1, T2, T3, T4
```

## 📁 Измененные файлы

### CurveSelectionModule.cpp
- **Метод**: `AddSingleReceiverCurves()` - изменён порядок добавления кривых
- **Метод**: `AddTwoReceiversCurves()` - изменён порядок добавления кривых
- **Метод**: `GetAvailableCurves()` - отключена сортировка

## 🔗 Связанные компоненты

### Не требуют изменений
- `SchematicModule.cpp` - отображение списка кривых работает корректно
- `ChartModule.cpp` - отрисовка графиков не зависит от порядка
- `DataModule.cpp` - вычисление данных не зависит от порядка

## ✅ Результат

После изменений:
- ✅ Фазы отображаются первыми во всех случаях
- ✅ Амплитуды отображаются вторыми
- ✅ Порядок логичный и удобный для анализа
- ✅ Согласованность между разными режимами выбора

---

**Версия документа**: 1.0  
**Автор изменения**: AI Assistant  
**Дата создания**: 6 ноября 2025


## 📁 Дополнительные изменения (v2.6.2 - исправление)

### SchematicModule.cpp
- **Проблема**: Кривые группировались по передатчикам (T1, T2, T3, T4), что нарушало желаемый порядок
- **Решение**: Для одного приёмника (R1 или R2) отключена группировка
- **Результат**: Кривые отображаются в плоском списке в порядке из `GetAvailableCurves()`
- **Порядок**: Сначала все фазы (T1-T4 Phase), затем все амплитуды (T1-T4 Amplitude)

### Логика изменений
**Было**: Кривые группировались по `elementIndex` (передатчику), что приводило к порядку:
- Группа T1: T1 Phase, T1 Amplitude
- Группа T2: T2 Phase, T2 Amplitude
- Группа T3: T3 Phase, T3 Amplitude
- Группа T4: T4 Phase, T4 Amplitude

**Стало**: Для `selectedReceivers == 1` группировка отключена, кривые в плоском списке:
- T1 Phase
- T2 Phase
- T3 Phase
- T4 Phase
- T1 Amplitude
- T2 Amplitude
- T3 Amplitude
- T4 Amplitude

---

**Версия документа**: 1.1  
**Дата обновления**: 6 ноября 2025
