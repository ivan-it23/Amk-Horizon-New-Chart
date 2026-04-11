#include "AutoUpdateModule.h"
#include <sstream>
#include <fstream>
#include <vector>
#include <algorithm>

// ===================================================================
// Server configuration
// ===================================================================
static const wchar_t* kUpdateHost = L"ivanbohg.beget.tech";
static const wchar_t* kVersionPath = L"/FilesForProgramVisualisation/version.txt";
static const wchar_t* kBaseUrl = L"http://ivanbohg.beget.tech/FilesForProgramVisualisation/";
static const wchar_t* kUserAgent = L"AmkHorizonUpdater/1.0";

// ===================================================================
// Helper: download a URL to a std::string (small files)
// ===================================================================
static bool HttpDownloadToString(const wchar_t* url, std::string& outData, std::wstring& outError) {
    outData.clear();
    outError.clear();

    HINTERNET hInet = InternetOpenW(kUserAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInet) {
        outError = L"InternetOpen failed";
        return false;
    }

    // Set connect/receive timeout to 5 seconds to avoid long UI freeze
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
// Helper: download a URL to a file (large files, with simple progress)
// ===================================================================
static bool HttpDownloadToFile(const wchar_t* url, const std::wstring& localPath, std::wstring& outError) {
    outError.clear();

    HINTERNET hInet = InternetOpenW(kUserAgent, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
    if (!hInet) {
        outError = L"InternetOpen failed";
        return false;
    }

    // Longer timeout for exe download (60 seconds)
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
// Trim whitespace / carriage returns from a string
// ===================================================================
static std::string TrimString(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// ===================================================================
// AutoUpdate::ParseVersion
// ===================================================================
AutoUpdate::Version AutoUpdate::ParseVersion(const std::string& s) {
    Version v;
    if (sscanf_s(s.c_str(), "%d.%d.%d", &v.major, &v.minor, &v.patch) < 2) {
        v = {};
    }
    return v;
}

// ===================================================================
// AutoUpdate::CheckForUpdate
// ===================================================================
AutoUpdate::UpdateInfo AutoUpdate::CheckForUpdate() {
    UpdateInfo info;

    // Build URL for version.txt
    std::wstring versionUrl = std::wstring(L"http://") + kUpdateHost + kVersionPath;

    // Download version.txt
    std::string body;
    if (!HttpDownloadToString(versionUrl.c_str(), body, info.errorMessage)) {
        return info;
    }

    // Parse: line 1 = version, line 2 = exe filename
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

    // Build download URL
    std::wstring wFilename(filenameLine.begin(), filenameLine.end());
    info.downloadUrl = std::wstring(kBaseUrl) + wFilename;

    // Compare versions
    Version current = GetCurrentVersion();
    info.available = info.remoteVersion > current;

    return info;
}

// ===================================================================
// AutoUpdate::DownloadUpdate
// ===================================================================
std::wstring AutoUpdate::DownloadUpdate(const UpdateInfo& info, HWND hWndParent) {
    if (info.downloadUrl.empty()) return L"";

    // Get temp folder
    wchar_t tempDir[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, tempDir);

    // Build local path
    std::wstring wFilename(info.exeFileName.begin(), info.exeFileName.end());
    std::wstring localPath = std::wstring(tempDir) + L"amk_update_" + wFilename;

    // Download
    std::wstring error;
    if (!HttpDownloadToFile(info.downloadUrl.c_str(), localPath, error)) {
        if (hWndParent) {
            MessageBoxW(hWndParent, error.c_str(), L"Ошибка обновления", MB_ICONERROR | MB_OK);
        }
        return L"";
    }

    // Verify the downloaded file is not empty and is a valid PE
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
    if (fileSize > 1024) { // minimal sanity check
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
// AutoUpdate::ApplyUpdate
// ===================================================================
bool AutoUpdate::ApplyUpdate(const std::wstring& downloadedExePath) {
    // Get path to the currently running exe
    wchar_t currentExe[MAX_PATH] = {};
    GetModuleFileNameW(NULL, currentExe, MAX_PATH);

    // Create a batch script in TEMP that:
    // 1. Waits for the current process to exit
    // 2. Copies the new exe over the old one
    // 3. Starts the new exe
    // 4. Deletes itself
    wchar_t tempDir[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, tempDir);
    std::wstring batPath = std::wstring(tempDir) + L"amk_updater.bat";

    // Get current PID for tasklist check
    DWORD pid = GetCurrentProcessId();

    std::wofstream bat(batPath);
    if (!bat.is_open()) return false;

    bat << L"@echo off\r\n";
    bat << L"echo Updating Amk-Horizon-New-Chart...\r\n";
    bat << L"echo Waiting for the application to close...\r\n";
    // Wait loop: check every second if the process is still running
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

    // Launch the batch script hidden
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
// AutoUpdate::RunUpdateCheck — high-level orchestrator
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

    // Build version string for display
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

    // Show wait cursor
    HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    std::wstring downloadedPath = DownloadUpdate(info, hWndParent);

    SetCursor(hOldCursor);

    if (downloadedPath.empty()) return; // error was already shown

    if (ApplyUpdate(downloadedPath)) {
        MessageBoxW(hWndParent,
            L"Обновление скачано. Приложение будет перезапущено.",
            L"Обновление", MB_ICONINFORMATION | MB_OK);
        // Close the application — the batch script will handle the rest
        PostMessage(hWndParent, WM_CLOSE, 0, 0);
    } else {
        MessageBoxW(hWndParent,
            L"Не удалось запустить процесс обновления.",
            L"Ошибка обновления", MB_ICONERROR | MB_OK);
    }
}
