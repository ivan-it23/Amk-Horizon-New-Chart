# 🔗 Интеграция функции Save Range

## Что уже сделано ✅

1. ✅ Добавлен метод `DataModule::SaveDataRange()` - сохранение данных
2. ✅ Добавлен метод `ChartManager::HandleSaveRange()` - обработка команды
3. ✅ Добавлены кнопки "Save Range" в UIModule
4. ✅ Добавлены ID кнопок в Resource.h
5. ✅ Код компилируется без ошибок

## Что нужно добавить вручную ⚠️

### Обработка команд кнопок в WndProc

Откройте файл `Amk-Horizon-New-Chart.cpp` и найдите функцию `WndProc`.

В обработчике `case WM_COMMAND:` добавьте следующий код:

```cpp
case WM_COMMAND:
    {
        int wmId = LOWORD(wParam);
        switch (wmId)
        {
            // ... существующие case (IDM_ABOUT, IDM_EXIT, и т.д.) ...
            
            // ДОБАВИТЬ ЭТИ ДВА CASE:
            case IDC_SAVE_RANGE1_BTN:
                if (g_chartManager) {
                    g_chartManager->HandleSaveRange(hWnd, 1);
                }
                break;
                
            case IDC_SAVE_RANGE2_BTN:
                if (g_chartManager) {
                    g_chartManager->HandleSaveRange(hWnd, 2);
                }
                break;
            
            // ... остальные case ...
            
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
        }
    }
    break;
```

### Где именно добавить?

Найдите в WndProc секцию, где обрабатываются другие кнопки, например:
- `IDC_DUAL_MODE_CHECK`
- `IDC_CHART1_SCHEMATIC_BTN`
- `IDC_XRANGE_APPLY_BTN`

Добавьте новые case рядом с ними.

## Проверка работы

После добавления обработчиков:

1. Пересоберите проект (Build → Rebuild Solution)
2. Запустите приложение
3. Загрузите файл данных
4. Задайте X Range (например, From 100 To 200)
5. Нажмите Apply
6. Нажмите "Save Range"
7. Должен открыться диалог сохранения файла
8. Сохраните файл
9. Должно появиться сообщение об успехе

## Если кнопки не работают

Проверьте:
1. Добавлены ли case в WndProc
2. Правильно ли указаны ID (`IDC_SAVE_RANGE1_BTN`, `IDC_SAVE_RANGE2_BTN`)
3. Существует ли `g_chartManager` (не NULL)
4. Загружены ли данные перед нажатием кнопки

## Альтернативный способ (если WndProc сложно найти)

Если не можете найти WndProc, попробуйте:

1. Откройте `Amk-Horizon-New-Chart.cpp`
2. Нажмите Ctrl+F (поиск)
3. Найдите текст: `case WM_COMMAND`
4. Добавьте код в найденную секцию

Или:

1. Найдите где обрабатывается `IDM_ABOUT` или `IDM_EXIT`
2. Добавьте новые case рядом

---

**После добавления обработчиков функция Save Range будет полностью работоспособна!** 🚀
