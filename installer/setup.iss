; ============================================================================
; AMK Horizon New Chart - Inno Setup installer script
;
; ОПИСАНИЕ
;   Скрипт сборки установщика для AMK Horizon New Chart.
;   Создаёт самодостаточный setup.exe, который устанавливает приложение
;   в выбранную пользователем папку (по умолчанию - папка на рабочем столе).
;
; ВАЖНО
;   При изменении версии в Amk-Horizon-New-Chart\AppVersion.h
;   обновите значение MyAppVersion ниже (раздел "Define").
;
; ТРЕБОВАНИЯ
;   - Inno Setup 6.x (https://jrsoftware.org/isinfo.php)
;   - Предварительно собранный Release|x64 exe-файл:
;     ..\Amk-Horizon-New-Chart\x64\Release\Amk-Horizon-New-Chart.exe
;
; СБОРКА
;   1. Соберите Release|x64 в Visual Studio (F7).
;   2. Откройте этот файл в Inno Setup Compiler и нажмите Build (F9),
;      либо запустите из командной строки:
;        "C:\Program Files (x86)\Inno Setup 6\ISCC.exe" setup.iss
;   3. Готовый установщик появится в installer\Output\.
;
; ВЫХОДНОЙ ФАЙЛ
;   installer\Output\AMK-Horizon-New-Chart-Setup-<version>.exe
;
; СОВМЕСТИМОСТЬ
;   Релизная сборка использует статический C++ runtime (/MT),
;   поэтому установщик НЕ требует Visual C++ Redistributable
;   на целевой машине. Работает на любой чистой Windows 10/11 x64.
; ============================================================================

; ---- Define: основные параметры приложения ----
; AppId - стабильный GUID v4, по которому Inno Setup опознаёт предыдущие
; установки и корректно их обновляет. Сгенерирован через PowerShell.
; НЕ ИЗМЕНЯТЬ после первого релиза - смена AppId сломает upgrade-путь
; для пользователей, у которых уже установлена эта версия приложения.
; Если потребуется новый AppId (например, для совершенно нового продукта),
; используйте: powershell -Command "[guid]::NewGuid().ToString().ToUpper()"
#define MyAppId            "{{8AA8E905-A0B5-4FD1-9D3A-2DA2A35A4819}"

#define MyAppName          "AMK Horizon New Chart"
#define MyAppNameShort     "AMK-Horizon-New-Chart"
#define MyAppVersion       "2.22.0"
; Полное юридическое название компании-разработчика. Отображается в свойствах
; setup.exe, в окне "Программы и компоненты" и в About-диалогах системы.
; Одинарные кавычки используются как ISPP-делимитер, потому что значение
; содержит двойные кавычки внутри (часть юридического обозначения).
#define MyAppPublisher     'ООО НПФ "АМК ГОРИЗОНТ"'
#define MyAppExeName       "Amk-Horizon-New-Chart.exe"
#define MyAppIconName      "Amk-Horizon-New-Chart.ico"
#define MyAppDescription   "Программа визуализации данных геофизических приборов"
#define MyAppCopyright     'Copyright (C) 2026 ООО НПФ "АМК ГОРИЗОНТ"'

; ---- Относительные пути от расположения этого .iss файла ----
; Структура репозитория:
;   Amk-Horizon-New-Chart/          <- корень (здесь x64\Release с exe)
;   ├── x64\Release\Amk-Horizon-New-Chart.exe  <- собирает VS
;   ├── Amk-Horizon-New-Chart\      <- папка проекта (.cpp, .h, .ico)
;   └── installer\setup.iss           <- этот файл
; Поэтому SourceDir = ".." (выходим в корень), exe берём оттуда, иконку из папки проекта.
#define SourceDir          ".."
#define BuiltExePath       SourceDir + "\x64\Release\" + MyAppExeName
#define BuiltIconPath      SourceDir + "\Amk-Horizon-New-Chart\" + MyAppIconName

[Setup]
; ---- Идентификация приложения ----
AppId={#MyAppId}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppCopyright={#MyAppCopyright}

; ---- Привилегии: per-user install без UAC ----
; lowest = установка в user-writable папку без запроса админ-прав.
; OverridesAllowed=dialog позволяет пользователю выбрать elevation,
; если он хочет установить в Program Files (потребует UAC).
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog

; ---- Папка установки ----
; По умолчанию - папка на рабочем столе пользователя.
; Пользователь может изменить путь через стандартный диалог выбора.
DefaultDirName={userdesktop}\{#MyAppNameShort}
DefaultGroupName={#MyAppName}
DisableProgramGroupPage=auto
DisableDirPage=no
AllowNoIcons=yes

; ---- Архитектура и минимальная версия Windows ----
ArchitecturesAllowed=x64
ArchitecturesInstallIn64BitMode=x64
MinVersion=10.0

; ---- Поведение при существующей установке ----
; Предлагает закрыть запущенное приложение перед обновлением.
CloseApplications=force
CloseApplicationsFilter=*.exe
RestartApplications=no

; ---- Выходной файл ----
OutputDir=Output
OutputBaseFilename=AMK-Horizon-New-Chart-Setup-{#MyAppVersion}
SetupIconFile={#BuiltIconPath}

; ---- Сжатие ----
; lzma2/max даёт хороший баланс между размером и временем установки.
Compression=lzma2/max
SolidCompression=yes
LZMAUseSeparateProcess=yes

; ---- UI и информация о версии ----
WizardStyle=modern
ShowLanguageDialog=auto
UninstallDisplayName={#MyAppName} {#MyAppVersion}
UninstallDisplayIcon={app}\{#MyAppExeName}

; Метаданные setup.exe (видны в Properties → Details).
VersionInfoVersion={#MyAppVersion}
VersionInfoProductName={#MyAppName}
VersionInfoProductVersion={#MyAppVersion}
VersionInfoCompany={#MyAppPublisher}
VersionInfoCopyright={#MyAppCopyright}
VersionInfoDescription={#MyAppName} Setup

[Languages]
; Русский - основной язык. Английский - резервный для нерусских локалей.
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
; Дополнительные опции, доступные пользователю на странице выбора задач.
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked

[Files]
; Основной исполняемый файл (со встроенным C++ runtime через /MT).
Source: "{#BuiltExePath}"; DestDir: "{app}"; Flags: ignoreversion

; Иконка приложения (для ярлыков и uninstaller).
Source: "{#BuiltIconPath}"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
; Ярлык в меню Пуск (создаётся всегда).
Name: "{group}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\{#MyAppIconName}"; Comment: "{#MyAppDescription}"
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"

; Ярлык на рабочем столе (опционально, через checkbox в Tasks).
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\{#MyAppExeName}"; IconFilename: "{app}\{#MyAppIconName}"; Comment: "{#MyAppDescription}"; Tasks: desktopicon

[Run]
; Предложение запустить приложение после успешной установки.
Filename: "{app}\{#MyAppExeName}"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent

[UninstallDelete]
; Кэш каталога приложения; пользовательские настройки в %LOCALAPPDATA%
; сохраняются, чтобы их не терять при переустановке.
Type: filesandordirs; Name: "{app}\*.log"
Type: filesandordirs; Name: "{app}\*.tmp"

; ============================================================================
; Конец скрипта.
;
; Примечания по обслуживанию:
;   - При выходе новой версии: обновите #define MyAppVersion в начале файла.
;   - При смене публишера/имени продукта: правьте только #define секцию,
;     остальной скрипт построен через подстановки и менять не требуется.
;   - НЕ меняйте AppId после первого релиза - это сломает обновления.
; ============================================================================
