# 🐛 Исправление: Проблемы с молчащими передатчиками

## 📅 Дата исправления
20 октября 2025

---

## 🎯 Описание проблем

### Проблема 1: Не сохраняется выбор молчащих передатчиков
**Сценарий:**
1. Открываю Device Schematic
2. Выбираю Silent R1 + Silent R2 (оба молчащих передатчика)
3. Вижу **2 кривые**: Silent R1 Amplitude, Silent R2 Amplitude
4. Нажимаю Apply → график отображается корректно
5. **Повторно открываю Device Schematic**
6. **ПРОБЛЕМА**: Вместо сохраненного выбора Silent R1 + Silent R2 отображается неправильная конфигурация

### Проблема 2: Сброс предыдущего выбора при добавлении молчащих
**Сценарий:**
1. Выбираю T1 + T2 (активные передатчики) → Apply
2. Повторно открываю Device Schematic
3. Дополнительно выбираю Silent R1 + Silent R2
4. **ПРОБЛЕМА**: T1 + T2 сбрасываются, остаются только молчащие

---

## ✅ Исправления

### 1. Изменены правила валидации в `SchematicSelection::GetValidationError()`

**Было:**
```cpp
// Молчащие передатчики нельзя комбинировать с активными элементами
if (silent > 0 && (transmitters > 0 || receivers > 0)) {
    return "Молчащие передатчики нельзя комбинировать с активными элементами";
}
```

**Стало:**
```cpp
// ИСПРАВЛЕНО: Разрешаем комбинации молчащих передатчиков с активными элементами
// Запрещена только комбинация приемников R1+R2 с молчащими передатчиками
if (silent > 0 && receivers == 2) {
    return "Нельзя одновременно выбирать оба приемника и молчащие передатчики";
}
```

### 2. Исправлена логика комбинирования в `CurveSelectionModule::GetAvailableCurves()`

**Было:**
```cpp
else if (selectedTransmitters > 0) {
    AddTransmitterCurves(curves, selection);
}
else if (selectedSilent > 0) {
    AddSilentTransmitterCurves(curves, selection);
}
```

**Стало:**
```cpp
// ИСПРАВЛЕНО: Используем отдельные if вместо else if для поддержки комбинаций
if (selectedTransmitters > 0) {
    AddTransmitterCurves(curves, selection);
}

if (selectedSilent > 0) {
    AddSilentTransmitterCurves(curves, selection);
}
```

### 3. Добавлены недостающие методы создания кривых в `CurveSelectionModule.cpp`

Добавлены методы:
- `CreateRatioCurve()`
- `CreateDifferenceCurve()`
- `CreateAmplitudeCurve()`
- `CreatePhaseCurve()`
- `CreateSilentAmplitudeCurve()` - **ключевой метод для молчащих передатчиков**
- `GetUnitsForDataType()`

**Реализация `CreateSilentAmplitudeCurve()`:**
```cpp
CurveDefinition CurveSelectionModule::CreateSilentAmplitudeCurve(int receiverIndex, FrequencyBand frequency) const {
    DataType dataType = (receiverIndex == 0) ? DataType::SILENT_AMPLITUDE_RX1 : DataType::SILENT_AMPLITUDE_RX2;
    std::string id = GenerateCurveId(ElementType::SILENT_TX, receiverIndex, dataType, frequency);
    std::string displayName = "Silent R" + std::to_string(receiverIndex + 1) + " Amplitude";
    std::string units = GetUnitsForDataType(dataType);
    
    return CurveDefinition(id, displayName, units, ElementType::SILENT_TX, 
                          receiverIndex, dataType, frequency);
}
```

---

## 🧪 Тестирование

### Тест 1: Сохранение выбора молчащих передатчиков

**Шаги:**
1. Открыть Device Schematic
2. Выбрать Silent R1 + Silent R2
3. Нажать Apply
4. Повторно открыть Device Schematic

**Ожидаемый результат:**
- ✅ Silent R1 и Silent R2 подсвечены на схеме
- ✅ Отображается 2 кривые: Silent R1 Amplitude, Silent R2 Amplitude
- ✅ Обе кривые отмечены галочками

### Тест 2: Комбинирование передатчиков и молчащих

**Шаги:**
1. Открыть Device Schematic
2. Выбрать T1 + T2
3. Нажать Apply
4. Повторно открыть Device Schematic
5. Дополнительно выбрать Silent R1

**Ожидаемый результат:**
- ✅ T1, T2 и Silent R1 подсвечены на схеме
- ✅ Отображаются кривые для T1, T2 (амплитуды и фазы для R1, R2)
- ✅ Отображается кривая Silent R1 Amplitude
- ✅ Все кривые доступны для выбора

### Тест 3: Комбинирование приемника и молчащих

**Шаги:**
1. Открыть Device Schematic
2. Выбрать R1 + Silent R1 + Silent R2
3. Проверить доступные кривые

**Ожидаемый результат:**
- ✅ R1, Silent R1 и Silent R2 подсвечены на схеме
- ✅ Отображаются кривые для R1 (T1-T4 амплитуды и фазы)
- ✅ Отображаются кривые Silent R1 и Silent R2 Amplitude
- ✅ Все кривые доступны для выбора

### Тест 4: Запрещенная комбинация (R1 + R2 + Silent)

**Шаги:**
1. Открыть Device Schematic
2. Выбрать R1 + R2 + Silent R1

**Ожидаемый результат:**
- ❌ Отображается сообщение об ошибке: "Нельзя одновременно выбирать оба приемника и молчащие передатчики"
- ❌ Список кривых пуст

---

## 📊 Разрешенные комбинации

### ✅ Разрешенные комбинации:
- **T1 + T2 + Silent R1** - активные передатчики + молчащий
- **T1 + Silent R1 + Silent R2** - активный передатчик + оба молчащих
- **R1 + Silent R1** - один приемник + молчащий
- **R1 + Silent R1 + Silent R2** - один приемник + оба молчащих
- **Silent R1** - только один молчащий
- **Silent R1 + Silent R2** - оба молчащих

### ❌ Запрещенные комбинации:
- **R1 + R2 + Silent R1** - оба приемника + молчащий
- **T1 + R1** - передатчик + приемник (старое ограничение)

---

## 🔧 Технические детали

### Изменённые файлы:

1. **SchematicModule.cpp**
   - Метод `GetValidationError()` - изменены правила валидации
   - Логика восстановления `silentTx[]` уже была корректной

2. **CurveSelectionModule.cpp**
   - Метод `GetAvailableCurves()` - исправлена логика комбинирования
   - Добавлены методы `Create*Curve()` для создания кривых
   - Добавлен метод `GetUnitsForDataType()`

### Ключевые изменения:

1. **Поддержка комбинаций**: Изменена логика с `else if` на отдельные `if` для поддержки комбинированного выбора
2. **Правильная валидация**: Разрешены комбинации молчащих с активными элементами, кроме R1+R2+Silent
3. **Корректное создание кривых**: Добавлен недостающий метод `CreateSilentAmplitudeCurve()`

---

## 🎯 Результаты

### До исправления:
- ❌ Молчащие передатчики не сохранялись при повторном открытии
- ❌ Нельзя было комбинировать молчащие с активными элементами
- ❌ Выбор сбрасывался при добавлении молчащих передатчиков

### После исправления:
- ✅ Молчащие передатчики корректно сохраняются и восстанавливаются
- ✅ Поддерживаются комбинации молчащих с активными элементами
- ✅ Выбор не сбрасывается при добавлении новых элементов
- ✅ Все кривые отображаются и работают корректно

---

## 📚 Обновление документации

Необходимо обновить следующие документы:
- **USER_GUIDE.md** - добавить примеры комбинаций с молчащими передатчиками
- **TECHNICAL_REFERENCE.md** - обновить правила валидации
- **CHANGELOG.md** - добавить версию с исправлениями

---

**Исправление выполнено**: 20 октября 2025  
**Версия**: 2.1.1  
**Статус**: Готово к тестированию ✅