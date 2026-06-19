# Установщик AMK Horizon New Chart

Этот каталог содержит Inno Setup скрипт для сборки самодостаточного
установщика приложения **AMK Horizon New Chart**.

## Назначение установщика

Установщик предназначен для корпоративного распространения приложения на
рабочие станции Windows 10/11 x64.

Release-сборка использует статический C++ runtime (`/MT`), поэтому
исполняемый файл не требует установки Visual C++ Redistributable на целевой
машине. Установщик добавляет стандартные механизмы дистрибуции:

- единый файл для распространения;
- стандартный мастер установки с выбором папки;
- ярлыки в меню Пуск и опционально на рабочем столе;
- регистрация в "Программы и компоненты" с возможностью корректного удаления;
- русская локализация интерфейса установки.

## Требования

| Компонент | Версия | Где взять |
|---|---|---|
| **Inno Setup** | 6.0 или новее | <https://jrsoftware.org/isdl.php> |
| **Visual Studio** | 2022 17.10+ (toolset v145) | для пересборки exe |
| **Собранный exe** | `Release \| x64` | через Build → Build Solution (F7) |

## Пошаговая сборка установщика

### 1. Установка Inno Setup

На машине сборки должен быть установлен Inno Setup 6.0 или новее. По
умолчанию Inno Setup устанавливается в `C:\Program Files (x86)\Inno Setup 6\`.

### 2. Сборка Release|x64 exe

1. В Visual Studio открывается `Amk-Horizon-New-Chart.sln`.
2. Выбирается конфигурация **Release** и платформа **x64**.
3. Выполняется сборка решения: **Build → Build Solution** (`F7`).
4. После сборки должен существовать файл:
   `Amk-Horizon-New-Chart\x64\Release\Amk-Horizon-New-Chart.exe`

### 3. Компиляция установщика

**Вариант A — через GUI:**

1. Открывается `installer\setup.iss` в Inno Setup Compiler.
2. Выполняется команда **Build → Compile** (`F9`).
3. Успешная сборка завершается сообщением `Compile completed successfully`.

**Вариант B — через командную строку:**

```powershell
& "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" .\setup.iss
```

### 4. Артефакт установщика

После успешной сборки создаётся файл:

```
installer\Output\AMK-Horizon-New-Chart-Setup-<version>.exe
```

`<version>` соответствует значению `MyAppVersion` в `installer\setup.iss`.
Этот файл является основным артефактом для распространения.

## Как обновить версию приложения

Версия указывается в **двух** местах (третье — `.rc` — обновляется автоматически
через `#include "AppVersion.h"`):

1. **Источник правды** — `Amk-Horizon-New-Chart\AppVersion.h`:
   ```cpp
   #define APP_VERSION_MAJOR   2
   #define APP_VERSION_MINOR   X
   #define APP_VERSION_PATCH   Y
   #define APP_VERSION_STR     "2.X.Y"
   #define APP_VERSION_WSTR    L"2.X.Y"
   ```

2. **Установщик** — `installer\setup.iss`, строка с `MyAppVersion`:
   ```pascal
   #define MyAppVersion       "2.X.Y"
   ```

3. **VERSIONINFO в `.rc`** — обновляется автоматически. Никаких ручных правок
   не требуется: `Amk-Horizon-New-Chart.rc` подключает `AppVersion.h` и
   подставляет значения через preprocessor.

Значения `APP_VERSION_STR`, `APP_VERSION_WSTR` и `MyAppVersion` должны
совпадать перед выпуском сборки. После изменения версии выполняются:

1. пересборка Release|x64 в Visual Studio;
2. повторная компиляция `setup.iss`.

## Метаданные exe-файла

Скомпилированный `.exe` содержит VERSIONINFO ресурс, видимый в:

- **Проводник Windows** → Properties → Details:
  - Product version: значение `APP_VERSION_STR`
  - File version: значение `APP_VERSION_STR` с четвертым компонентом `.0`
  - Company: `ООО НПФ "АМК ГОРИЗОНТ"`
  - Copyright: `Copyright (C) 2026 ООО НПФ "АМК ГОРИЗОНТ"`
- **PowerShell** для аудита:
  ```powershell
  (Get-Item ".\Amk-Horizon-New-Chart.exe").VersionInfo
  ```
- **Корпоративные инвентарные системы** (SCCM, Intune, GPO) — используют эти
  поля для идентификации установленного приложения.

## Структура репозитория

```
Amk-Horizon-New-Chart/                 <- корень репозитория
├── x64\Release\                        <- сюда VS собирает Release|x64
│   └── Amk-Horizon-New-Chart.exe      <- основной исполняемый файл
├── Amk-Horizon-New-Chart\             <- папка проекта (.cpp, .h, .ico)
│   ├── Amk-Horizon-New-Chart.cpp
│   ├── AppVersion.h
│   ├── Amk-Horizon-New-Chart.ico     <- иконка приложения
│   └── ... (остальные .cpp/.h)
├── installer\                           <- папка установщика
│   ├── setup.iss                       <- скрипт Inno Setup
│   ├── README.md                       <- этот файл
│   └── .gitignore
└── Amk-Horizon-New-Chart.sln          <- solution файл Visual Studio
```

## Что попадает в установщик

| Файл | Источник | Назначение |
|---|---|---|
| `Amk-Horizon-New-Chart.exe` | `..\x64\Release\` | Основной исполняемый файл (со встроенным C++ runtime) |
| `Amk-Horizon-New-Chart.ico` | `..\Amk-Horizon-New-Chart\` | Иконка приложения для ярлыков |

## Что НЕ попадает в установщик и не нужно

- **Visual C++ Redistributable** — не нужен, runtime встроен в exe через `/MT`.
- **Сторонние DLL** — не используются.
- **Файлы данных пользователя** — `app_settings.txt`, заметки и записанные
  кадры остаются в `%LOCALAPPDATA%\AMK-Horizon-New-Chart\` и не трогаются
  ни при установке, ни при удалении.

## Поведение установщика

### По умолчанию

- **Папка установки**: `%USERPROFILE%\Desktop\AMK-Horizon-New-Chart\`
  (папка на рабочем столе пользователя).
- **Ярлык в меню Пуск**: создаётся всегда.
- **Ярлык на рабочем столе**: опциональный (по умолчанию **снят**).
- **Привилегии**: per-user, без UAC.

### Настраиваемые пользователем параметры

- Папку установки — любая user-writable папка.
- Группу меню Пуск.
- Создавать ли ярлык на рабочем столе.
- При выборе системной папки (Program Files) установщик запрашивает
  административных прав.

### Что делает Uninstaller

- Удаляет каталог установки и ярлыки.
- **Не трогает** пользовательские настройки в `%LOCALAPPDATA%`.
- Корректно отображается в "Программы и компоненты" / "Параметры → Приложения".

## Совместимость с AutoUpdate

Встроенный модуль `AutoUpdateModule` продолжает работать без изменений:

- Установка в user-writable папку → AutoUpdate может заменить exe без UAC.
- Установщик обновляет версию при повторном запуске (`AppId` стабильный).

После выпуска новой версии достаточно загрузить новый `.exe` на сервер
обновлений (`ivanbohg.beget.tech/FilesForProgramVisualisation/`). Пользователи,
у которых уже установлено приложение, получат обновление через AutoUpdate
без необходимости запускать установщик повторно.

## Типичные проблемы

### `File "..\Amk-Horizon-New-Chart\x64\Release\Amk-Horizon-New-Chart.exe" not found`

Причина: Release|x64 не собран. Требуется выполнить пункт 2 раздела
"Пошаговая сборка установщика".

### `Compiler error: Unknown identifier "ArchitecturesAllowed"`

Причина: установлена устаревшая версия Inno Setup. Требуется Inno Setup 6.0
или новее.

### `Setup is already running` при повторном запуске

Это штатное поведение Inno Setup: параллельная установка одного и того же
приложения блокируется до завершения предыдущего процесса установки.

### Антивирус ругается на собранный setup.exe

Это типичная ложная сработка на неподписанные Inno Setup сборки.
Долгосрочное решение — подписать установщик code-signing сертификатом
(параметр `SignTool` в `[Setup]`).

## Дальнейшее развитие

При необходимости расширения возможностей установщика добавляются точечно:

- **Цифровая подпись** — для устранения предупреждений SmartScreen.
- **Silent install** через `/SILENT` / `/VERYSILENT` для GPO-развёртывания
  (уже работает по умолчанию, отдельная настройка не требуется).
- **Ассоциация файлов** `.bin` / `.biv` / `.DEV` с приложением — добавляется
  через секцию `[Registry]`.

Все эти расширения вносятся в `setup.iss` без изменения остальной кодовой базы
проекта.
