#include "AutoUpdateModule.h"
#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>

// ===================================================================
// Конфигурация сервера
// ===================================================================
static const wchar_t* kUpdateHost = L"ivanbohg.beget.tech";
static const wchar_t* kVersionPath = L"/FilesForProgramVisualisation/version.txt";
static const wchar_t* kBaseUrl = L"http://ivanbohg.beget.tech/FilesForProgramVisualisation/";
static const wchar_t* kUserAgent = L"AmkHorizonUpdater/1.0";

// ===================================================================
// Вспомогательная функция: загрузка URL в std::string (маленькие файлы)
// ===================================================================
static bool HttpDownloadToString(const wchar_t* url, std::string& outData, std::wstring& outError) {
    outData.clear();
    outError.clear();

    HINTERNET hInet = InternetOpenW(kUserAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInet) {
        outError = L"InternetOpen failed";
        return false;
    }

    // Устанавливает таймаут подключения/приёма 5 секунд, чтобы избежать длительной блокировки UI
    DWORD timeout = 5000;
    InternetSetOptionW(hInet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionW(hInet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionW(hInet, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));

    HINTERNET hUrl = InternetOpenUrlW(hInet, url, NULL, 0,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_PRAGMA_NOCACHE, 0);
    if (!hUrl) {
        DWORD err = GetLastError();
        outError = L"Cannot connect to update server (error " + std::to_wstring(err) + L")";
        InternetCloseHandle(hInet);
        return false;
    }

    char buf[4096];
    DWORD bytesRead = 0;
    while (InternetReadFile(hUrl, buf, sizeof(buf), &bytesRead) && bytesRead > 0) {
        outData.append(buf, bytesRead);
    }

    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInet);
    return true;
}

// ===================================================================
// Вспомогательная функция: загрузка URL в файл (большие файлы, с простым прогрессом)
// ===================================================================
static bool HttpDownloadToFile(const wchar_t* url, const std::wstring& localPath, std::wstring& outError) {
    outError.clear();

    HINTERNET hInet = InternetOpenW(kUserAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInet) {
        outError = L"InternetOpen failed";
        return false;
    }

    // Более длинный таймаут для загрузки exe (60 секунд)
    DWORD timeout = 60000;
    InternetSetOptionW(hInet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
    InternetSetOptionW(hInet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

    HINTERNET hUrl = InternetOpenUrlW(hInet, url, NULL, 0,
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_PRAGMA_NOCACHE, 0);
    if (!hUrl) {
        DWORD err = GetLastError();
        outError = L"Cannot download update (error " + std::to_wstring(err) + L")";
        InternetCloseHandle(hInet);
        return false;
    }

    std::ofstream ofs(localPath, std::ios::binary);
    if (!ofs.is_open()) {
        outError = L"Cannot create temp file";
        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInet);
        return false;
    }

    char buf[8192];
    DWORD bytesRead = 0;
    while (InternetReadFile(hUrl, buf, sizeof(buf), &bytesRead) && bytesRead > 0) {
        ofs.write(buf, bytesRead);
    }

    ofs.close();
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInet);
    return true;
}

// ===================================================================
// Удаляет пробелы и символы возврата каретки из строки
// ===================================================================
static std::string TrimString(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// ===================================================================
// AutoUpdate::ParseVersion — парсит строку версии
// ===================================================================
AutoUpdate::Version AutoUpdate::ParseVersion(const std::string& s) {
    Version v;
    if (sscanf_s(s.c_str(), "%d.%d.%d", &v.major, &v.minor, &v.patch) < 2) {
        v = {};
    }
    return v;
}

// ===================================================================
// AutoUpdate::CheckForUpdate — проверяет наличие обновлений
// ===================================================================
AutoUpdate::UpdateInfo AutoUpdate::CheckForUpdate() {
    UpdateInfo info;

    // Формирует URL для version.txt
    std::wstring versionUrl = std::wstring(L"http://") + kUpdateHost + kVersionPath;

    // Загружает version.txt
    std::string body;
    if (!HttpDownloadToString(versionUrl.c_str(), body, info.errorMessage)) {
        return info;
    }

    // Парсит: строка 1 = версия, строка 2 = имя exe-файла
    std::istringstream iss(body);
    std::string versionLine, filenameLine;
    if (!std::getline(iss, versionLine)) {
        info.errorMessage = L"Invalid version.txt format";
        return info;
    }
    if (!std::getline(iss, filenameLine)) {
        info.errorMessage = L"Invalid version.txt format (missing filename)";
        return info;
    }

    versionLine = TrimString(versionLine);
    filenameLine = TrimString(filenameLine);

    if (versionLine.empty() || filenameLine.empty()) {
        info.errorMessage = L"Empty version.txt fields";
        return info;
    }

    info.remoteVersion = ParseVersion(versionLine);
    info.exeFileName = filenameLine;

    // Формирует URL для скачивания
    std::wstring wFilename(filenameLine.begin(), filenameLine.end());
    info.downloadUrl = std::wstring(kBaseUrl) + wFilename;

    // Сравнивает версии
    Version current = GetCurrentVersion();
    info.available = info.remoteVersion > current;

    return info;
}

// ===================================================================
// AutoUpdate::DownloadUpdate — загружает обновление
// ===================================================================
std::wstring AutoUpdate::DownloadUpdate(const UpdateInfo& info, HWND hWndParent) {
    if (info.downloadUrl.empty()) return L"";

    // Получает временную папку
    wchar_t tempDir[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, tempDir);

    // Формирует локальный путь
    std::wstring wFilename(info.exeFileName.begin(), info.exeFileName.end());
    std::wstring localPath = std::wstring(tempDir) + L"amk_update_" + wFilename;

    // Загружает файл
    std::wstring error;
    if (!HttpDownloadToFile(info.downloadUrl.c_str(), localPath, error)) {
        if (hWndParent) {
            MessageBoxW(hWndParent, error.c_str(), L"Ошибка обновления", MB_ICONERROR | MB_OK);
        }
        return L"";
    }

    // Проверяет, что скачанный файл не пустой и является корректным PE-файлом
    HANDLE hFile = CreateFileW(localPath.c_str(), GENERIC_READ, FILE_SHARE_READ,
        NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        if (hWndParent) {
            MessageBoxW(hWndParent, L"Не удалось открыть скачанный файл.", L"Ошибка обновления", MB_ICONERROR | MB_OK);
        }
        return L"";
    }
    DWORD fileSize = GetFileSize(hFile, NULL);
    bool validPE = false;
    if (fileSize > 1024) { // минимальная проверка на валидность
        BYTE header[2] = {};
        DWORD read = 0;
        ReadFile(hFile, header, 2, &read, NULL);
        validPE = (header[0] == 'M' && header[1] == 'Z');
    }
    CloseHandle(hFile);

    if (!validPE) {
        DeleteFileW(localPath.c_str());
        if (hWndParent) {
            MessageBoxW(hWndParent,
                L"Скачанный файл повреждён или не является исполняемым файлом.",
                L"Ошибка обновления", MB_ICONERROR | MB_OK);
        }
        return L"";
    }

    return localPath;
}

// ===================================================================
// AutoUpdate::ApplyUpdate — применяет обновление
// ===================================================================
bool AutoUpdate::ApplyUpdate(const std::wstring& downloadedExePath) {
    // Получает путь к текущему исполняемому файлу
    wchar_t currentExe[MAX_PATH] = {};
    GetModuleFileNameW(NULL, currentExe, MAX_PATH);

    // Создаёт bat-скрипт в TEMP, который:
    // 1. Ждёт завершения текущего процесса
    // 2. Копирует новый exe поверх старого
    // 3. Запускает новый exe
    // 4. Удаляет сам себя
    wchar_t tempDir[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, tempDir);
    std::wstring batPath = std::wstring(tempDir) + L"amk_updater.bat";

    // Получает текущий PID для проверки через tasklist
    DWORD pid = GetCurrentProcessId();

    std::wofstream bat(batPath);
    if (!bat.is_open()) return false;

    bat << L"@echo off\r\n";
    bat << L"echo Updating Amk-Horizon-New-Chart...\r\n";
    bat << L"echo Waiting for the application to close...\r\n";
    // Цикл ожидания: проверяет каждую секунду, запущен ли процесс
    bat << L":wait_loop\r\n";
    bat << L"tasklist /FI \"PID eq " << pid << L"\" 2>NUL | find \"" << pid << L"\" >NUL\r\n";
    bat << L"if not errorlevel 1 (\r\n";
    bat << L"    timeout /t 1 /nobreak >NUL\r\n";
    bat << L"    goto wait_loop\r\n";
    bat << L")\r\n";
    bat << L"timeout /t 1 /nobreak >NUL\r\n";
    bat << L"copy /Y \"" << downloadedExePath << L"\" \"" << currentExe << L"\"\r\n";
    bat << L"if errorlevel 1 (\r\n";
    bat << L"    echo Update failed! Could not replace the executable.\r\n";
    bat << L"    pause\r\n";
    bat << L"    goto end\r\n";
    bat << L")\r\n";
    bat << L"echo Update complete. Starting application...\r\n";
    bat << L"start \"\" \"" << currentExe << L"\"\r\n";
    bat << L":end\r\n";
    bat << L"del \"" << downloadedExePath << L"\" >NUL 2>&1\r\n";
    bat << L"del \"%~f0\" >NUL 2>&1\r\n";

    bat.close();

    // Запускает bat-скрипт в скрытом режиме
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    std::wstring cmdLine = L"cmd.exe /C \"" + batPath + L"\"";
    std::vector<wchar_t> cmdBuf(cmdLine.begin(), cmdLine.end());
    cmdBuf.push_back(0);

    BOOL ok = CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, FALSE,
        CREATE_NO_WINDOW, NULL, tempDir, &si, &pi);

    if (ok) {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    return ok != FALSE;
}

// ===================================================================
// AutoUpdate::RunUpdateCheck — высокоуровневая функция-координатор
// ===================================================================
void AutoUpdate::RunUpdateCheck(HWND hWndParent, bool silent) {
    UpdateInfo info = CheckForUpdate();

    if (!info.errorMessage.empty()) {
        if (!silent) {
            MessageBoxW(hWndParent, info.errorMessage.c_str(),
                L"Проверка обновлений", MB_ICONWARNING | MB_OK);
        }
        return;
    }

    if (!info.available) {
        if (!silent) {
            std::wstring msg = L"У вас установлена последняя версия (";
            msg += APP_VERSION_WSTR;
            msg += L").";
            MessageBoxW(hWndParent, msg.c_str(),
                L"Проверка обновлений", MB_ICONINFORMATION | MB_OK);
        }
        return;
    }

    // Формирует строку версии для отображения
    std::wstring remoteStr =
        std::to_wstring(info.remoteVersion.major) + L"." +
        std::to_wstring(info.remoteVersion.minor) + L"." +
        std::to_wstring(info.remoteVersion.patch);

    std::wstring prompt =
        L"Доступна новая версия: " + remoteStr + L"\n"
        L"Текущая версия: " + APP_VERSION_WSTR + L"\n\n"
        L"Скачать и установить обновление?";

    int result = MessageBoxW(hWndParent, prompt.c_str(),
        L"Обновление доступно", MB_ICONQUESTION | MB_YESNO);

    if (result != IDYES) return;

    // Показывает курсор ожидания
    HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    std::wstring downloadedPath = DownloadUpdate(info, hWndParent);

    SetCursor(hOldCursor);

    if (downloadedPath.empty()) return; // ошибка уже была показана

    if (ApplyUpdate(downloadedPath)) {
        MessageBoxW(hWndParent,
            L"Обновление скачано. Приложение будет перезапущено.",
            L"Обновление", MB_ICONINFORMATION | MB_OK);
        // Закрывает приложение — bat-скрипт выполнит остальное
        PostMessage(hWndParent, WM_CLOSE, 0, 0);
    } else {
        MessageBoxW(hWndParent,
            L"Не удалось запустить процесс обновления.",
            L"Ошибка обновления", MB_ICONERROR | MB_OK);
    }
}
