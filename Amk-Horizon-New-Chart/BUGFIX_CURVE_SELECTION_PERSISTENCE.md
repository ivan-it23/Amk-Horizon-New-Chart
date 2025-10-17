# 🐛 Исправление: Сохранение выбора кривых в Device Schematic

## 📋 Описание проблемы

При повторном открытии диалога Device Schematic для графика:
- ✅ Выбор элементов на схеме (T1-T4, R1-R2, Silent) сохранялся корректно
- ❌ Состояние чекбоксов кривых сбрасывалось (все включались по умолчанию)
- ❌ Выбранная частота (400 kHz / 2000 kHz) не восстанавливалась

### Воспроизведение проблемы

1. Открыть Device Schematic для Chart 1
2. Выбрать передатчик T3 на схеме
3. Отметить галочки только для "T3 Amplitude R2" и "T3 Phase R2"
4. Нажать "Apply" - график корректно отображает выбранные кривые
5. Повторно открыть Device Schematic для Chart 1
6. **Проблема**: T3 выбран, но все 4 галочки кривых включены (вместо 2)

---

## ✅ Решение

### Изменения в `SchematicModule.cpp`

#### 1. Метод `InitializeDialogControls` - восстановление конфигурации

**Было:**
```cpp
void SchematicModule::InitializeDialogControls(HWND hDlg) {
    // Инициализация с дефолтными значениями
    SendMessage(hFreqCombo, CB_SETCURSEL, 0, 0); // Всегда 400 kHz
    currentSelection.frequency = FrequencyBand::FREQ_400_KHZ;
    
    // Список кривых генерировался заново без учёта текущей конфигурации
    UpdateAvailableCurves(hDlg);
}
```

**Стало:**
```cpp
void SchematicModule::InitializeDialogControls(HWND hDlg) {
    // 1. Проверяем, есть ли существующая конфигурация графика
    bool hasExistingConfig = false;
    if (curveSelectionModule && curveSelectionModule->HasConfiguration(targetChartNumber)) {
        ChartConfiguration existingConfig = curveSelectionModule->GetChartConfiguration(targetChartNumber);
        
        if (!existingConfig.IsEmpty()) {
            hasExistingConfig = true;
            
            // 2. Восстанавливаем выбор элементов схемы из кривых
            currentSelection.Clear();
            
            for (const auto& curve : existingConfig.curves) {
                // Восстанавливаем частоту
                currentSelection.frequency = curve.frequency;
                
                // Восстанавливаем выбор элементов
                switch (curve.sourceElement) {
                    case ElementType::TRANSMITTER:
                        currentSelection.transmitters[curve.elementIndex] = true;
                        break;
                    case ElementType::RECEIVER:
                        currentSelection.receivers[curve.elementIndex] = true;
                        break;
                    case ElementType::SILENT_TX:
                        currentSelection.silentTx[curve.elementIndex] = true;
                        break;
                }
            }
            
            // 3. Сохраняем кривые с их состоянием enabled
            currentAvailableCurves = existingConfig.curves;
        }
    }
    
    // 4. Устанавливаем частоту в комбобоксе
    int freqIndex = (currentSelection.frequency == FrequencyBand::FREQ_400_KHZ) ? 0 : 1;
    SendMessage(hFreqCombo, CB_SETCURSEL, freqIndex, 0);
    
    // 5. Синхронизируем визуальное состояние элементов схемы
    if (hasExistingConfig) {
        SetCurrentSelection(currentSelection);
    }
    
    // 6. Обновляем список кривых (с восстановленными состояниями)
    UpdateAvailableCurves(hDlg);
}
```

#### 2. Метод `UpdateAvailableCurves` - синхронизация состояния чекбоксов

**Было:**
```cpp
void SchematicModule::UpdateAvailableCurves(HWND hDlg) {
    // Получаем доступные кривые
    std::vector<CurveDefinition> availableCurves = 
        curveSelectionModule->GetAvailableCurves(currentSelection);
    
    // Все кривые enabled по умолчанию
    for (const auto& curve : availableCurves) {
        ListView_SetCheckState(hCurveList, itemIndex, curve.enabled ? TRUE : FALSE);
    }
}
```

**Стало:**
```cpp
void SchematicModule::UpdateAvailableCurves(HWND hDlg) {
    // Получаем доступные кривые
    std::vector<CurveDefinition> availableCurves = 
        curveSelectionModule->GetAvailableCurves(currentSelection);
    
    // Если у нас есть сохранённые кривые, синхронизируем состояние enabled
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
    
    // Отображаем кривые с правильным состоянием чекбоксов
    for (const auto& curve : availableCurves) {
        ListView_SetCheckState(hCurveList, itemIndex, curve.enabled ? TRUE : FALSE);
    }
}
```

---

## 🧪 Тестирование

### Сценарий 1: Базовое сохранение выбора

**Шаги:**
1. Открыть Device Schematic для Chart 1
2. Выбрать T3 на схеме
3. Отметить только "T3 Amplitude R2" и "T3 Phase R2"
4. Нажать "Apply"
5. Повторно открыть Device Schematic для Chart 1

**Ожидаемый результат:**
- ✅ T3 выбран на схеме
- ✅ Частота 400 kHz выбрана
- ✅ Только 2 галочки: "T3 Amplitude R2" и "T3 Phase R2"
- ✅ Остальные галочки сняты

### Сценарий 2: Сохранение частоты

**Шаги:**
1. Открыть Device Schematic для Chart 1
2. Выбрать R1 + R2 на схеме
3. Изменить частоту на 2000 kHz
4. Отметить "T1 Ratio" и "T2 Phase Diff"
5. Нажать "Apply"
6. Повторно открыть Device Schematic для Chart 1

**Ожидаемый результат:**
- ✅ R1 + R2 выбраны на схеме
- ✅ Частота 2000 kHz выбрана
- ✅ Только 2 галочки: "T1 Ratio" и "T2 Phase Diff"

### Сценарий 3: Изменение выбора элементов

**Шаги:**
1. Открыть Device Schematic для Chart 1 (с существующей конфигурацией T3)
2. Снять T3, выбрать T1
3. Проверить список кривых

**Ожидаемый результат:**
- ✅ T1 выбран на схеме
- ✅ Список кривых обновился для T1
- ✅ Все галочки включены по умолчанию (новый выбор)

### Сценарий 4: Кнопка Cancel

**Шаги:**
1. Открыть Device Schematic для Chart 1 (с существующей конфигурацией)
2. Изменить выбор элементов и галочки
3. Нажать "Cancel"
4. Повторно открыть Device Schematic для Chart 1

**Ожидаемый результат:**
- ✅ Конфигурация не изменилась
- ✅ Отображается исходная конфигурация

### Сценарий 5: Первое открытие (нет конфигурации)

**Шаги:**
1. Открыть Device Schematic для нового графика (без конфигурации)

**Ожидаемый результат:**
- ✅ Ничего не выбрано на схеме
- ✅ Частота 400 kHz по умолчанию
- ✅ Список кривых пуст (сообщение "Выберите элементы")

---

## 📊 Технические детали

### Алгоритм восстановления конфигурации

```
1. При открытии диалога:
   ├─ Проверить, есть ли конфигурация для targetChartNumber
   ├─ Если есть:
   │  ├─ Получить ChartConfiguration из CurveSelectionModule
   │  ├─ Извлечь выбор элементов из кривых (sourceElement, elementIndex)
   │  ├─ Извлечь частоту из первой кривой
   │  ├─ Сохранить кривые с их состоянием enabled в currentAvailableCurves
   │  └─ Установить currentSelection
   └─ Если нет:
      └─ Использовать дефолтные значения

2. При обновлении списка кривых:
   ├─ Получить доступные кривые для текущего выбора
   ├─ Если есть сохранённые кривые (currentAvailableCurves):
   │  ├─ Создать карту ID -> enabled
   │  └─ Применить сохранённые состояния к новым кривым
   └─ Отобразить кривые с правильными чекбоксами
```

### Структуры данных

```cpp
// Сохранённые кривые с состоянием enabled
std::vector<CurveDefinition> currentAvailableCurves;

// Карта для быстрого поиска состояния enabled по ID кривой
std::map<std::string, bool> savedEnabledStates;
```

### Идентификация кривых

Кривые идентифицируются по уникальному ID:
```cpp
// Примеры ID кривых:
"T3_R2_AMP_400"   // T3 Amplitude R2 @ 400 kHz
"T3_R2_PHASE_400" // T3 Phase R2 @ 400 kHz
"T1_RATIO_2000"   // T1 Ratio @ 2000 kHz
```

---

## 🔄 Обратная совместимость

Изменения полностью обратно совместимы:
- ✅ Если конфигурации нет - работает как раньше (дефолтные значения)
- ✅ Существующие конфигурации корректно восстанавливаются
- ✅ Не требуется миграция данных
- ✅ API модулей не изменился

---

## 📝 Изменённые файлы

1. **SchematicModule.cpp**
   - Метод `InitializeDialogControls()` - добавлено восстановление конфигурации
   - Метод `UpdateAvailableCurves()` - добавлена синхронизация состояния чекбоксов
   - Добавлен `#include <map>` для использования `std::map`

---

## ✅ Результат

После исправления:
- ✅ Выбор элементов схемы сохраняется
- ✅ Частота сохраняется
- ✅ Состояние чекбоксов кривых сохраняется
- ✅ При изменении выбора элементов список кривых обновляется корректно
- ✅ Кнопка Cancel корректно отменяет изменения

---

**Исправление выполнено**: 16 октября 2025  
**Версия**: 2.0.1  
**Статус**: Протестировано и готово к использованию ✅
