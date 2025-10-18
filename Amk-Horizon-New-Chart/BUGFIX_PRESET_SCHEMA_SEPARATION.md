# 🐛 Исправление: Разделение логики пресетов и схемы прибора

## 📋 Описание проблемы

При повторном открытии Device Schematic после использования быстрого пресета (например, "Ampl Ratios"):
- ❌ Все передатчики T1-T4 подсвечивались как активные (светло-красный цвет)
- ❌ Отображались лишние кривые (амплитуды и фазы для T1-T4)
- ❌ Вместо 4 кривых отношений амплитуд показывалось 8+ кривых

### Корневая причина:
Система восстанавливала элементы схемы из кривых, не различая два режима работы:
1. **Режим пресета** - кривые генерируются на основе фильтра, элементы схемы не используются
2. **Режим ручного выбора** - кривые генерируются на основе выбранных элементов схемы

---

## ✅ Решение

### Концепция: Две независимые логики

**1. Логика быстрых пресетов (`dataFilter != ALL`):**
- Кривые генерируются на основе **фильтра типов данных**
- Элементы схемы **НЕ подсвечиваются** (все неактивны)
- Информация о приемниках хранится в `selection.receivers[]` только для генерации кривых
- Визуально схема остаётся нейтральной

**2. Логика ручного выбора (`dataFilter == ALL`):**
- Кривые генерируются на основе **выбранных элементов схемы**
- Выбранные элементы **подсвечиваются** (активны)
- Пользователь видит, какие элементы выбраны

---

## 🔧 Технические изменения

### 1. Обновлена валидация `SchematicSelection`

```cpp
std::string SchematicSelection::GetValidationError() const {
    // Для пресетов (dataFilter != ALL) валидация всегда проходит
    if (dataFilter != DataTypeFilter::ALL) {
        return ""; // Пресет всегда валиден
    }
    
    // Для ручного выбора проверяем выбранные элементы
    if (receivers == 0 && transmitters == 0 && silent == 0) {
        return "Выберите элементы для построения графика";
    }
    // ...
}
```

### 2. Обновлена генерация кривых `GetAvailableCurves()`

```cpp
std::vector<CurveDefinition> CurveSelectionModule::GetAvailableCurves(
    const SchematicSelection& selection) const {
    
    // РЕЖИМ ПРЕСЕТА: генерируем кривые на основе фильтра
    if (selection.dataFilter != DataTypeFilter::ALL) {
        if (selection.dataFilter == DataTypeFilter::RATIO_ONLY || 
            selection.dataFilter == DataTypeFilter::DIFFERENCE_ONLY) {
            AddTwoReceiversCurves(curves, selection);
        }
        else if (selection.dataFilter == DataTypeFilter::AMPLITUDE_ONLY || 
                 selection.dataFilter == DataTypeFilter::PHASE_ONLY) {
            AddSingleReceiverCurves(curves, selection);
        }
    }
    // РЕЖИМ РУЧНОГО ВЫБОРА: генерируем кривые на основе элементов
    else {
        if (selectedReceivers == 2) {
            AddTwoReceiversCurves(curves, selection);
        }
        // ...
    }
}
```

### 3. Обновлена логика восстановления `InitializeDialogControls()`

```cpp
// Определяем фильтр
currentSelection.dataFilter = DetermineDataFilterFromCurves(existingConfig.curves);

// Восстанавливаем элементы ТОЛЬКО для ручного выбора
if (currentSelection.dataFilter == DataTypeFilter::ALL) {
    // Режим ручного выбора - восстанавливаем элементы схемы
    for (const auto& curve : existingConfig.curves) {
        // Восстанавливаем transmitters, receivers, silentTx
    }
}
else {
    // Режим пресета - восстанавливаем только информацию о приемниках
    // для генерации кривых (НЕ для подсветки схемы)
    for (const auto& curve : existingConfig.curves) {
        switch (curve.dataType) {
            case DataType::AMPLITUDE_RX1:
            case DataType::PHASE_RX1:
                currentSelection.receivers[0] = true; // R1 для генерации
                break;
            case DataType::AMPLITUDE_RX2:
            case DataType::PHASE_RX2:
                currentSelection.receivers[1] = true; // R2 для генерации
                break;
        }
    }
}

// Синхронизация визуального состояния
if (currentSelection.dataFilter == DataTypeFilter::ALL) {
    SetCurrentSelection(currentSelection); // Подсвечиваем элементы
}
else {
    // Для пресетов все элементы неактивны
    for (auto& element : schematicElements) {
        element.selected = false;
    }
}
```

### 4. Обновлён `HandlePresetButton()`

```cpp
void SchematicModule::HandlePresetButton(HWND hDlg, int presetId) {
    currentSelection.Clear();
    currentAvailableCurves.clear();
    
    // Устанавливаем фильтр и receivers для генерации кривых
    switch (presetId) {
        case IDC_PRESET_ALL_AMP_R1:
            currentSelection.receivers[0] = true; // Для генерации кривых
            currentSelection.dataFilter = DataTypeFilter::AMPLITUDE_ONLY;
            break;
        // ...
    }
    
    // Для пресетов НЕ синхронизируем элементы схемы
    for (auto& element : schematicElements) {
        element.selected = false; // Все неактивны
    }
    
    // Обновляем UI
    InvalidateRect(hSchematicArea, NULL, TRUE);
    UpdateAvailableCurves(hDlg);
}
```

---

## 🧪 Тестирование

### Тест 1: Пресет "Ampl Ratios"
**Шаги:**
1. Открыть Device Schematic
2. Нажать "Ampl Ratios" → Apply
3. Повторно открыть Device Schematic

**Ожидаемый результат:**
- ✅ Все элементы схемы неактивны (обычный цвет)
- ✅ Отображается ровно 4 чекбокса с кривыми отношений амплитуд
- ✅ Фильтр `RATIO_ONLY` активен

**Фактический результат:** ✅ PASS

---

### Тест 2: Пресет "All Phase R1"
**Шаги:**
1. Открыть Device Schematic
2. Нажать "All Phase R1" → Apply
3. Повторно открыть Device Schematic

**Ожидаемый результат:**
- ✅ Все элементы схемы неактивны
- ✅ Отображается ровно 4 чекбокса с фазами R1
- ✅ Фильтр `PHASE_ONLY` активен

**Фактический результат:** ✅ PASS

---

### Тест 3: Ручной выбор T3
**Шаги:**
1. Открыть Device Schematic
2. Кликнуть на T3 на схеме
3. Нажать Apply
4. Повторно открыть Device Schematic

**Ожидаемый результат:**
- ✅ T3 подсвечен (активен)
- ✅ Отображаются все доступные кривые для T3
- ✅ Фильтр `ALL` активен

**Фактический результат:** ✅ PASS

---

### Тест 4: Переключение между режимами
**Шаги:**
1. Открыть Device Schematic
2. Нажать "Ampl Ratios" → Apply → Закрыть
3. Повторно открыть → видим 4 кривые, схема неактивна
4. Кликнуть на T2 → схема активируется, показываются все кривые T2
5. Apply → Закрыть
6. Повторно открыть

**Ожидаемый результат:**
- ✅ T2 подсвечен (активен)
- ✅ Отображаются кривые для T2
- ✅ Фильтр `ALL` активен

**Фактический результат:** ✅ PASS

---

## 📊 Результаты

### До исправления:
- ❌ Пресеты подсвечивали элементы схемы
- ❌ Отображались лишние кривые
- ❌ Логика пресетов и ручного выбора смешивалась

### После исправления:
- ✅ Пресеты работают независимо от схемы
- ✅ Элементы схемы не подсвечиваются для пресетов
- ✅ Отображаются только нужные кривые
- ✅ Две логики полностью разделены

---

## 🎯 Ключевые достижения

1. **Чистое разделение логики** - пресеты и ручной выбор работают независимо
2. **Правильная визуализация** - схема не подсвечивается для пресетов
3. **Корректная генерация кривых** - только нужные кривые отображаются
4. **Сохранение состояния** - при повторном открытии всё восстанавливается правильно

---

**Дата исправления:** 18 октября 2025  
**Версия:** 2.0.2  
**Статус:** ✅ Исправлено и протестировано
