#pragma once

#include "framework.h"
#include "AppVersion.h"
#include <string>
#include <wininet.h>
#pragma comment(lib, "wininet.lib")

// ===================================================================
// AutoUpdateModule — автоматическое обновление через HTTP по адресу ivanbohg.beget.tech
//
// Структура файлов на сервере:
//   http://ivanbohg.beget.tech/FilesForProgramVisualisation/version.txt
//   http://ivanbohg.beget.tech/FilesForProgramVisualisation/<exe_name>
//
// Формат version.txt (UTF-8, две строки):
//   2.20.0
//   Amk-Horizon-New-Chart.exe
// ===================================================================

namespace AutoUpdate {

    // Распарсенная версия в формате major.minor.patch
    struct Version {
        int major = 0;
        int minor = 0;
        int patch = 0;

        bool operator>(const Version& o) const {
            if (major != o.major) return major > o.major;
            if (minor != o.minor) return minor > o.minor;
            return patch > o.patch;
        }
        bool operator==(const Version& o) const {
            return major == o.major && minor == o.minor && patch == o.patch;
        }
    };

    // Результат проверки обновлений
    struct UpdateInfo {
        bool available = false;
        Version remoteVersion;
        std::string exeFileName;       // например: "Amk-Horizon-New-Chart.exe"
        std::wstring downloadUrl;      // полный URL к exe-файлу
        std::wstring errorMessage;     // не пустая строка, если проверка не удалась
    };

    // Проверяет наличие обновлений (блокирующая операция, вызывать из фонового потока или при запуске).
    // Возвращает UpdateInfo с available=true, если существует более новая версия.
    UpdateInfo CheckForUpdate();

    // Загружает новый exe-файл во временную папку. Возвращает локальный путь к
    // загруженному файлу или пустую строку при ошибке.
    // hWndParent используется для сообщений о прогрессе (может быть NULL).
    std::wstring DownloadUpdate(const UpdateInfo& info, HWND hWndParent);

    // Применяет обновление: создаёт вспомогательный bat-скрипт, который ждёт завершения
    // текущего процесса, заменяет exe-файл и перезапускает приложение. Возвращает true,
    // если скрипт был успешно запущен. Вызывающий код должен завершить приложение
    // после возврата true.
    bool ApplyUpdate(const std::wstring& downloadedExePath);

    // Высокоуровневая функция: проверка + запрос + загрузка + применение.
    // Если silent=true, диалоговое окно не показывается, если приложение уже обновлено.
    // hWndParent — родительское окно для диалоговых окон.
    void RunUpdateCheck(HWND hWndParent, bool silent);

    // Парсит строку "major.minor.patch" в структуру Version.
    Version ParseVersion(const std::string& s);

    // Возвращает текущую скомпилированную версию приложения.
    inline Version GetCurrentVersion() {
        return { APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH };
    }

} // namespace AutoUpdate
