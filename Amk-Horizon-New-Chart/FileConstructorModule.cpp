#include "framework.h"
#include "FileConstructorModule.h"
#include "DataModule.h"
#include "resource.h"

#include <algorithm>
#include <cstddef>
#include <commdlg.h>
#include <cwctype>
#include <fstream>
#include <shlobj.h>
#include <sstream>
#include <utility>

namespace FileConstructor {
namespace {

constexpr char kSidecarMagic[] = "AMK_HORIZON_SEGMENTS_V1";

struct ConstructorSegment {
    std::wstring sourcePath;
    std::wstring label;
    size_t sourceStartIndex = 0;
    uint32_t signature = 0;
    bool homogeneous = false;
    bool compatible = false;
    std::vector<GP_TOOL_DATA> records;
};

struct ConstructorDialogContext {
    std::vector<ConstructorSegment> segments;
    std::wstring savedPath;
    HWND owner = nullptr;
    int dialogResult = IDCANCEL;
};

// Один экземпляр диалога конструктора файлов. Повторный вызов меню активирует
// уже открытое окно вместо создания второго.
HWND g_constructorDialog = nullptr;

std::wstring GetFileNameOnly(const std::wstring& path) {
    const size_t slashPos = path.find_last_of(L"\\/");
    return (slashPos == std::wstring::npos) ? path : path.substr(slashPos + 1);
}

std::wstring FormatSignature(uint32_t signature) {
    wchar_t buffer[32] = {};
    swprintf_s(buffer, L"0x%08X", signature);
    return buffer;
}

std::string WideToUtf8(const std::wstring& value) {
    if (value.empty()) {
        return std::string();
    }

    const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (size <= 1) {
        return std::string();
    }

    std::string result(static_cast<size_t>(size), '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), -1, result.data(), size, nullptr, nullptr);
    result.resize(static_cast<size_t>(size - 1));
    return result;
}

std::wstring Utf8ToWide(const std::string& value) {
    if (value.empty()) {
        return std::wstring();
    }

    const int size = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, nullptr, 0);
    if (size <= 1) {
        return std::wstring();
    }

    std::wstring result(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), -1, result.data(), size);
    result.resize(static_cast<size_t>(size - 1));
    return result;
}

std::wstring EscapeField(const std::wstring& value) {
    std::wstring result;
    result.reserve(value.size());
    for (wchar_t ch : value) {
        switch (ch) {
            case L'\\':
                result += L"\\\\";
                break;
            case L'\t':
                result += L"\\t";
                break;
            case L'\r':
                result += L"\\r";
                break;
            case L'\n':
                result += L"\\n";
                break;
            default:
                result += ch;
                break;
        }
    }
    return result;
}

std::wstring UnescapeField(const std::wstring& value) {
    std::wstring result;
    result.reserve(value.size());
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] != L'\\' || i + 1 >= value.size()) {
            result += value[i];
            continue;
        }

        const wchar_t next = value[++i];
        switch (next) {
            case L't':
                result += L'\t';
                break;
            case L'r':
                result += L'\r';
                break;
            case L'n':
                result += L'\n';
                break;
            case L'\\':
                result += L'\\';
                break;
            default:
                result += next;
                break;
        }
    }
    return result;
}

std::vector<std::string> SplitTabLine(const std::string& line) {
    std::vector<std::string> parts;
    size_t start = 0;
    while (start <= line.size()) {
        const size_t tab = line.find('\t', start);
        if (tab == std::string::npos) {
            parts.push_back(line.substr(start));
            break;
        }
        parts.push_back(line.substr(start, tab - start));
        start = tab + 1;
    }
    return parts;
}

bool WriteCanonicalRecords(const std::wstring& path,
                           const std::vector<GP_TOOL_DATA>& records,
                           std::wstring& outError) {
    outError.clear();
    if (records.empty()) {
        outError = L"Нет данных для сохранения.";
        return false;
    }

    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        outError = L"Не удалось открыть файл для записи.";
        return false;
    }

    file.write(reinterpret_cast<const char*>(records.data()),
               static_cast<std::streamsize>(records.size() * sizeof(GP_TOOL_DATA)));
    if (!file) {
        outError = L"Не удалось записать данные в файл.";
        return false;
    }

    return true;
}

bool SaveSegmentLayoutInternal(const std::wstring& dataFilePath,
                               uint32_t signature,
                               const std::vector<SegmentBoundary>& segments,
                               std::wstring& outError) {
    outError.clear();

    std::ofstream file(BuildSegmentSidecarPath(dataFilePath), std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        outError = L"Не удалось сохранить описание сегментов файла.";
        return false;
    }

    file << kSidecarMagic << "\n";
    file << "signature\t" << signature << "\n";
    for (const auto& segment : segments) {
        file << "segment\t"
             << segment.startIndex << "\t"
             << segment.recordCount << "\t"
             << segment.sourceStartIndex << "\t"
             << WideToUtf8(EscapeField(segment.sourcePath)) << "\t"
             << WideToUtf8(EscapeField(segment.label)) << "\n";
    }

    if (!file) {
        outError = L"Не удалось записать описание сегментов файла.";
        return false;
    }

    return true;
}

bool IsCanonicalBinFile(const std::wstring& path) {
    return DataModule::DetectFileFormat(path) == FileFormat::FORMAT_BIN;
}

bool LoadDataFileSegment(const std::wstring& path,
                         size_t sourceStartIndex,
                         size_t recordCount,
                         ConstructorSegment& outSegment,
                         std::wstring& outError) {
    outSegment = {};
    outError.clear();

    DataLoadResult loadedData;
    if (!DataModule::LoadDataFromFileResult(path, loadedData, outError)) {
        if (outError.empty()) {
            outError = L"Не удалось открыть файл данных.";
        }
        return false;
    }

    if (recordCount == 0) {
        recordCount = loadedData.data.size();
    }

    if (sourceStartIndex > loadedData.data.size() ||
        recordCount > loadedData.data.size() - sourceStartIndex) {
        outError = L"Описание сборки ссылается на диапазон, которого нет в исходном файле.";
        return false;
    }

    outSegment.sourcePath = path;
    outSegment.label = GetFileNameOnly(path);
    outSegment.sourceStartIndex = sourceStartIndex;
    outSegment.records.assign(loadedData.data.begin() + static_cast<std::ptrdiff_t>(sourceStartIndex),
                              loadedData.data.begin() + static_cast<std::ptrdiff_t>(sourceStartIndex + recordCount));
    outSegment.homogeneous = TryGetHomogeneousSignature(outSegment.records, outSegment.signature);
    return true;
}

void UpdateCompatibility(ConstructorDialogContext& ctx) {
    uint32_t expectedSignature = 0;
    bool hasExpected = false;

    for (const auto& segment : ctx.segments) {
        if (segment.homogeneous) {
            expectedSignature = segment.signature;
            hasExpected = true;
            break;
        }
    }

    for (auto& segment : ctx.segments) {
        segment.compatible = segment.homogeneous &&
                             (!hasExpected || segment.signature == expectedSignature);
    }
}

bool CanSave(const ConstructorDialogContext& ctx) {
    if (ctx.segments.empty()) {
        return false;
    }

    for (const auto& segment : ctx.segments) {
        if (!segment.compatible) {
            return false;
        }
    }

    return true;
}

void CenterDialogOnOwner(HWND hDlg, HWND owner) {
    RECT dialogRect = {};
    if (!GetWindowRect(hDlg, &dialogRect)) {
        return;
    }

    RECT targetRect = {};
    if (owner && IsWindow(owner)) {
        GetWindowRect(owner, &targetRect);
    } else {
        HMONITOR monitor = MonitorFromWindow(hDlg, MONITOR_DEFAULTTONEAREST);
        MONITORINFO monitorInfo = {};
        monitorInfo.cbSize = sizeof(monitorInfo);
        if (!GetMonitorInfoW(monitor, &monitorInfo)) {
            return;
        }
        targetRect = monitorInfo.rcWork;
    }

    const int dialogWidth = dialogRect.right - dialogRect.left;
    const int dialogHeight = dialogRect.bottom - dialogRect.top;
    const int x = targetRect.left + ((targetRect.right - targetRect.left) - dialogWidth) / 2;
    const int y = targetRect.top + ((targetRect.bottom - targetRect.top) - dialogHeight) / 2;
    SetWindowPos(hDlg, nullptr, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
}

void SetConstructorDialogIcons(HWND hDlg) {
    HINSTANCE hInst = GetModuleHandleW(nullptr);
    HICON bigIcon = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_AMKHORIZONNEWCHART));
    HICON smallIcon = LoadIconW(hInst, MAKEINTRESOURCEW(IDI_SMALL));
    if (bigIcon) {
        SendMessageW(hDlg, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(bigIcon));
    }
    if (smallIcon) {
        SendMessageW(hDlg, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(smallIcon));
    }
}

void CloseConstructorDialog(HWND hDlg, ConstructorDialogContext* ctx, int result) {
    if (ctx) {
        ctx->dialogResult = result;
    }
    DestroyWindow(hDlg);
}

void RefreshConstructorDialog(HWND hDlg, ConstructorDialogContext& ctx, int selectedIndex = -1) {
    UpdateCompatibility(ctx);

    HWND list = GetDlgItem(hDlg, IDC_FILE_CONSTRUCTOR_LIST);
    SendMessageW(list, LB_RESETCONTENT, 0, 0);

    HDC hdc = GetDC(list);
    HFONT listFont = reinterpret_cast<HFONT>(SendMessageW(list, WM_GETFONT, 0, 0));
    HGDIOBJ oldFont = nullptr;
    if (hdc && listFont) {
        oldFont = SelectObject(hdc, listFont);
    }

    int maxItemWidth = 0;
    size_t totalRecords = 0;
    for (size_t i = 0; i < ctx.segments.size(); ++i) {
        const ConstructorSegment& segment = ctx.segments[i];
        totalRecords += segment.records.size();

        const wchar_t* status = segment.compatible ? L"совместимо" : L"не совместимо";
        std::wstring item = std::to_wstring(i + 1) + L". " + status +
            L" | " + std::to_wstring(segment.records.size()) + L" кадров" +
            L" | sig=" + FormatSignature(segment.signature) +
            L" | " + segment.label;

        if (hdc && listFont) {
            SIZE textSize = {};
            if (GetTextExtentPoint32W(hdc, item.c_str(), static_cast<int>(item.size()), &textSize)) {
                if (textSize.cx > maxItemWidth) {
                    maxItemWidth = textSize.cx;
                }
            }
        }

        SendMessageW(list, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item.c_str()));
    }

    if (hdc && listFont && oldFont) {
        SelectObject(hdc, oldFont);
    }
    if (hdc) {
        ReleaseDC(list, hdc);
    }

    SendMessageW(list, LB_SETHORIZONTALEXTENT, (maxItemWidth > 0) ? maxItemWidth + 4 : 0, 0);

    if (!ctx.segments.empty()) {
        if (selectedIndex < 0) {
            selectedIndex = 0;
        }
        if (selectedIndex >= static_cast<int>(ctx.segments.size())) {
            selectedIndex = static_cast<int>(ctx.segments.size()) - 1;
        }
        SendMessageW(list, LB_SETCURSEL, static_cast<WPARAM>(selectedIndex), 0);
    }

    std::wstring statusText;
    if (ctx.segments.empty()) {
        statusText = L"";
    } else if (CanSave(ctx)) {
        statusText = L"Совместимо: все файлы относятся к одному прибору. Всего кадров: " + std::to_wstring(totalRecords);
    } else {
        statusText = L"Не совместимо: сигнатуры файлов отличаются или один из файлов содержит смешанные сигнатуры.";
    }

    SetDlgItemTextW(hDlg, IDC_FILE_CONSTRUCTOR_STATUS, statusText.c_str());
    EnableWindow(GetDlgItem(hDlg, IDC_FILE_CONSTRUCTOR_SAVE), CanSave(ctx));
    EnableWindow(GetDlgItem(hDlg, IDC_FILE_CONSTRUCTOR_RESAVE),
                  CanSave(ctx) && !ctx.savedPath.empty());
    EnableWindow(GetDlgItem(hDlg, IDC_FILE_CONSTRUCTOR_REMOVE), !ctx.segments.empty());
    EnableWindow(GetDlgItem(hDlg, IDC_FILE_CONSTRUCTOR_UP), ctx.segments.size() > 1);
    EnableWindow(GetDlgItem(hDlg, IDC_FILE_CONSTRUCTOR_DOWN), ctx.segments.size() > 1);
}

std::vector<std::wstring> BrowseInputDataFiles(HWND owner) {
    wchar_t buffer[32768] = {};

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = _countof(buffer);
    ofn.lpstrFilter = L"Файлы данных\0*.bin;*.biv;*.DEV\0BIN (*.bin)\0*.bin\0BIV (*.biv)\0*.biv\0DEV (*.DEV)\0*.DEV\0Все файлы\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = L"Добавить файлы данных";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;

    std::vector<std::wstring> result;
    if (GetOpenFileNameW(&ofn) != TRUE) {
        return result;
    }

    const std::wstring first = buffer;
    wchar_t* next = buffer + first.size() + 1;
    if (*next == L'\0') {
        result.push_back(first);
        return result;
    }

    const std::wstring directory = first;
    while (*next != L'\0') {
        std::wstring fileName = next;
        std::wstring fullPath = directory;
        if (!fullPath.empty() && fullPath.back() != L'\\' && fullPath.back() != L'/') {
            fullPath += L"\\";
        }
        fullPath += fileName;
        result.push_back(fullPath);
        next += fileName.size() + 1;
    }

    return result;
}

bool BrowseExistingAssembly(HWND owner, std::wstring& outPath) {
    outPath.clear();
    wchar_t buffer[1024] = {};

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = _countof(buffer);
    ofn.lpstrFilter = L"BIN (*.bin)\0*.bin\0Все файлы\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrTitle = L"Открыть существующую сборку";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameW(&ofn) != TRUE) {
        return false;
    }

    outPath = buffer;
    return true;
}

bool BrowseSaveOutput(HWND owner, std::wstring& outPath) {
    outPath.clear();
    wchar_t buffer[1024] = L"constructed_file.bin";

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = owner;
    ofn.lpstrFile = buffer;
    ofn.nMaxFile = _countof(buffer);
    ofn.lpstrFilter = L"BIN (*.bin)\0*.bin\0Все файлы\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = L"bin";
    ofn.lpstrTitle = L"Сохранить собранный файл";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    if (GetSaveFileNameW(&ofn) != TRUE) {
        return false;
    }

    outPath = buffer;
    return true;
}

void AddInputFiles(HWND hDlg, ConstructorDialogContext& ctx) {
    const std::vector<std::wstring> files = BrowseInputDataFiles(hDlg);
    if (files.empty()) {
        return;
    }

    for (const auto& path : files) {
        ConstructorSegment segment;
        std::wstring error;
        if (!LoadDataFileSegment(path, 0, 0, segment, error)) {
            std::wstring message = L"Не удалось добавить файл:\n" + path + L"\n\n" + error;
            MessageBoxW(hDlg, message.c_str(), L"Конструктор файлов", MB_OK | MB_ICONERROR);
            continue;
        }

        ctx.segments.push_back(std::move(segment));
    }

    RefreshConstructorDialog(hDlg, ctx, static_cast<int>(ctx.segments.size()) - 1);
}

void LoadExistingAssembly(HWND hDlg, ConstructorDialogContext& ctx) {
    std::wstring assemblyPath;
    if (!BrowseExistingAssembly(hDlg, assemblyPath)) {
        return;
    }

    std::vector<SegmentBoundary> boundaries;
    if (!LoadSegmentLayout(assemblyPath, boundaries) || boundaries.empty()) {
        MessageBoxW(hDlg,
                    L"Для выбранного файла не найдено описание сборки. Изменение порядка доступно только для файлов, созданных конструктором или через добавление отрезков.",
                    L"Конструктор файлов",
                    MB_OK | MB_ICONINFORMATION);
        return;
    }

    std::vector<ConstructorSegment> loadedSegments;
    loadedSegments.reserve(boundaries.size());

    for (const auto& boundary : boundaries) {
        if (boundary.sourcePath.empty() || GetFileAttributesW(boundary.sourcePath.c_str()) == INVALID_FILE_ATTRIBUTES) {
            std::wstring message = L"Не найден исходный файл сегмента:\n" + boundary.sourcePath;
            MessageBoxW(hDlg, message.c_str(), L"Конструктор файлов", MB_OK | MB_ICONERROR);
            return;
        }

        ConstructorSegment segment;
        std::wstring error;
        if (!LoadDataFileSegment(boundary.sourcePath, boundary.sourceStartIndex, boundary.recordCount, segment, error)) {
            std::wstring message = L"Не удалось прочитать исходный сегмент:\n" + boundary.sourcePath + L"\n\n" + error;
            MessageBoxW(hDlg, message.c_str(), L"Конструктор файлов", MB_OK | MB_ICONERROR);
            return;
        }

        if (!boundary.label.empty()) {
            segment.label = boundary.label;
        }
        loadedSegments.push_back(std::move(segment));
    }

    ctx.segments = std::move(loadedSegments);
    ctx.savedPath = assemblyPath;
    RefreshConstructorDialog(hDlg, ctx, 0);
}

void RemoveSelectedSegment(HWND hDlg, ConstructorDialogContext& ctx) {
    const int selected = static_cast<int>(SendDlgItemMessageW(hDlg, IDC_FILE_CONSTRUCTOR_LIST, LB_GETCURSEL, 0, 0));
    if (selected == LB_ERR || selected < 0 || selected >= static_cast<int>(ctx.segments.size())) {
        return;
    }

    ctx.segments.erase(ctx.segments.begin() + selected);
    RefreshConstructorDialog(hDlg, ctx, selected);
}

void MoveSelectedSegment(HWND hDlg, ConstructorDialogContext& ctx, int delta) {
    const int selected = static_cast<int>(SendDlgItemMessageW(hDlg, IDC_FILE_CONSTRUCTOR_LIST, LB_GETCURSEL, 0, 0));
    const int target = selected + delta;
    if (selected == LB_ERR ||
        selected < 0 ||
        target < 0 ||
        selected >= static_cast<int>(ctx.segments.size()) ||
        target >= static_cast<int>(ctx.segments.size())) {
        return;
    }

    std::swap(ctx.segments[static_cast<size_t>(selected)], ctx.segments[static_cast<size_t>(target)]);
    RefreshConstructorDialog(hDlg, ctx, target);
}

void SaveConstructedFile(HWND hDlg, ConstructorDialogContext& ctx, bool isResave) {
    if (!CanSave(ctx)) {
        MessageBoxW(hDlg,
                    L"Нельзя сохранить единый файл: все сегменты должны иметь одинаковую сигнатуру одного прибора.",
                    L"Конструктор файлов",
                    MB_OK | MB_ICONWARNING);
        return;
    }

    std::wstring outputPath = ctx.savedPath;
    if (!isResave && !BrowseSaveOutput(hDlg, outputPath)) {
        return;
    }

    if (outputPath.empty()) {
        MessageBoxW(hDlg,
                    L"Не выбран путь для сохранения.",
                    L"Конструктор файлов",
                    MB_OK | MB_ICONWARNING);
        return;
    }

    std::vector<GP_TOOL_DATA> combinedRecords;
    size_t totalRecords = 0;
    for (const auto& segment : ctx.segments) {
        totalRecords += segment.records.size();
    }
    combinedRecords.reserve(totalRecords);

    std::vector<SegmentBoundary> boundaries;
    boundaries.reserve(ctx.segments.size());

    size_t start = 0;
    for (const auto& segment : ctx.segments) {
        SegmentBoundary boundary;
        boundary.startIndex = start;
        boundary.recordCount = segment.records.size();
        boundary.sourceStartIndex = segment.sourceStartIndex;
        boundary.sourcePath = segment.sourcePath;
        boundary.label = segment.label;
        boundaries.push_back(std::move(boundary));

        combinedRecords.insert(combinedRecords.end(), segment.records.begin(), segment.records.end());
        start += segment.records.size();
    }

    std::wstring error;
    if (!WriteCanonicalRecords(outputPath, combinedRecords, error)) {
        MessageBoxW(hDlg, error.c_str(), L"Конструктор файлов", MB_OK | MB_ICONERROR);
        return;
    }

    if (!SaveSegmentLayoutInternal(outputPath, ctx.segments.front().signature, boundaries, error)) {
        std::wstring message = L"Файл сохранён, но не удалось сохранить описание границ сегментов:\n" + error;
        MessageBoxW(hDlg, message.c_str(), L"Конструктор файлов", MB_OK | MB_ICONWARNING);
    } else {
        std::wstring message = isResave
            ? L"Файл успешно обновлён:\n" + outputPath
            : L"Файл успешно создан:\n" + outputPath;
        MessageBoxW(hDlg, message.c_str(), L"Конструктор файлов", MB_OK | MB_ICONINFORMATION);
    }

    ctx.savedPath = outputPath;

    if (!isResave) {
        CloseConstructorDialog(hDlg, &ctx, IDOK);
    }
}

INT_PTR CALLBACK FileConstructorDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            SetWindowLongPtrW(hDlg, GWLP_USERDATA, static_cast<LONG_PTR>(lParam));
            ConstructorDialogContext* ctx = reinterpret_cast<ConstructorDialogContext*>(lParam);
            g_constructorDialog = hDlg;
            LONG_PTR exStyle = GetWindowLongPtrW(hDlg, GWL_EXSTYLE);
            SetWindowLongPtrW(hDlg, GWL_EXSTYLE, (exStyle & ~WS_EX_TOOLWINDOW) | WS_EX_APPWINDOW);
            SetWindowPos(hDlg, nullptr, 0, 0, 0, 0,
                         SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
            SetConstructorDialogIcons(hDlg);
            if (ctx) {
                CenterDialogOnOwner(hDlg, ctx->owner);
                RefreshConstructorDialog(hDlg, *ctx);
            }
            return TRUE;
        }

        case WM_DESTROY: {
            if (g_constructorDialog == hDlg) {
                g_constructorDialog = nullptr;
            }
            return TRUE;
        }

        case WM_CLOSE: {
            ConstructorDialogContext* ctx = reinterpret_cast<ConstructorDialogContext*>(GetWindowLongPtrW(hDlg, GWLP_USERDATA));
            CloseConstructorDialog(hDlg, ctx, IDCANCEL);
            return TRUE;
        }

        case WM_COMMAND: {
            ConstructorDialogContext* ctx = reinterpret_cast<ConstructorDialogContext*>(GetWindowLongPtrW(hDlg, GWLP_USERDATA));
            const int id = LOWORD(wParam);
            if (!ctx) {
                if (id == IDCANCEL) {
                    DestroyWindow(hDlg);
                    return TRUE;
                }
                return FALSE;
            }

            switch (id) {
                case IDC_FILE_CONSTRUCTOR_ADD:
                    AddInputFiles(hDlg, *ctx);
                    return TRUE;

                case IDC_FILE_CONSTRUCTOR_OPEN_ASSEMBLY:
                    LoadExistingAssembly(hDlg, *ctx);
                    return TRUE;

                case IDC_FILE_CONSTRUCTOR_REMOVE:
                    RemoveSelectedSegment(hDlg, *ctx);
                    return TRUE;

                case IDC_FILE_CONSTRUCTOR_UP:
                    MoveSelectedSegment(hDlg, *ctx, -1);
                    return TRUE;

                case IDC_FILE_CONSTRUCTOR_DOWN:
                    MoveSelectedSegment(hDlg, *ctx, 1);
                    return TRUE;

                case IDC_FILE_CONSTRUCTOR_SAVE:
                    SaveConstructedFile(hDlg, *ctx, false);
                    return TRUE;

                case IDC_FILE_CONSTRUCTOR_RESAVE:
                    SaveConstructedFile(hDlg, *ctx, true);
                    return TRUE;

                case IDCANCEL:
                    CloseConstructorDialog(hDlg, ctx, IDCANCEL);
                    return TRUE;

                default:
                    break;
            }
            break;
        }
    }

    return FALSE;
}

// Каталог хранения описаний сегментов: %LOCALAPPDATA%\AMK-Horizon-New-Chart\segments.
// Хранение вне папки с данными защищает конфигурацию сборки от случайного
// удаления пользователем и соответствует остальным настройкам приложения.
std::wstring GetSegmentsStorageDirectory() {
    wchar_t localAppData[MAX_PATH] = {};
    if (SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, localAppData) != S_OK) {
        const DWORD len = GetEnvironmentVariableW(L"LOCALAPPDATA", localAppData, MAX_PATH);
        if (len == 0 || len >= MAX_PATH) {
            return std::wstring();
        }
    }

    std::wstring dir = std::wstring(localAppData) + L"\\AMK-Horizon-New-Chart";
    CreateDirectoryW(dir.c_str(), nullptr);
    dir += L"\\segments";
    CreateDirectoryW(dir.c_str(), nullptr);
    return dir;
}

// FNV-1a 64 по нормализованному (полному, в нижнем регистре) пути файла данных.
// Хэш в имени sidecar устраняет коллизии между одноимёнными файлами из разных папок.
uint64_t HashNormalizedDataFilePath(const std::wstring& dataFilePath) {
    std::wstring normalized = dataFilePath;

    wchar_t fullPath[4096] = {};
    const DWORD len = GetFullPathNameW(dataFilePath.c_str(), _countof(fullPath), fullPath, nullptr);
    if (len > 0 && len < _countof(fullPath)) {
        normalized.assign(fullPath, len);
    }

    for (wchar_t& ch : normalized) {
        ch = static_cast<wchar_t>(std::towlower(ch));
    }

    uint64_t hash = 14695981039346656037ULL;
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(normalized.data());
    const size_t byteCount = normalized.size() * sizeof(wchar_t);
    for (size_t i = 0; i < byteCount; ++i) {
        hash ^= bytes[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

// Прежнее расположение sidecar рядом с файлом данных. Используется только
// для чтения и миграции сборок, созданных предыдущими версиями приложения.
std::wstring BuildLegacySegmentSidecarPath(const std::wstring& dataFilePath) {
    return dataFilePath + L".segments.tsv";
}

bool TryLoadSegmentLayoutFromSidecar(const std::wstring& sidecarPath,
                                     std::vector<SegmentBoundary>& outSegments) {
    outSegments.clear();

    std::ifstream file(sidecarPath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    if (!std::getline(file, line) || line != kSidecarMagic) {
        return false;
    }

    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        const std::vector<std::string> parts = SplitTabLine(line);
        if (parts.empty() || parts[0] != "segment") {
            continue;
        }

        if (parts.size() < 6) {
            return false;
        }

        try {
            SegmentBoundary segment;
            segment.startIndex = static_cast<size_t>(std::stoull(parts[1]));
            segment.recordCount = static_cast<size_t>(std::stoull(parts[2]));
            segment.sourceStartIndex = static_cast<size_t>(std::stoull(parts[3]));
            segment.sourcePath = UnescapeField(Utf8ToWide(parts[4]));
            segment.label = UnescapeField(Utf8ToWide(parts[5]));
            if (segment.recordCount > 0) {
                outSegments.push_back(std::move(segment));
            }
        } catch (...) {
            outSegments.clear();
            return false;
        }
    }

    return !outSegments.empty();
}

} // namespace

std::wstring BuildSegmentSidecarPath(const std::wstring& dataFilePath) {
    const std::wstring storageDir = GetSegmentsStorageDirectory();
    if (storageDir.empty()) {
        // Резервный путь на случай недоступности LOCALAPPDATA: прежнее расположение
        // рядом с файлом данных сохраняет работоспособность функции.
        return BuildLegacySegmentSidecarPath(dataFilePath);
    }

    wchar_t hashText[24] = {};
    swprintf_s(hashText, L"%016llX",
               static_cast<unsigned long long>(HashNormalizedDataFilePath(dataFilePath)));

    return storageDir + L"\\" + GetFileNameOnly(dataFilePath) + L"." + hashText + L".segments.tsv";
}

bool LoadSegmentLayout(const std::wstring& dataFilePath, std::vector<SegmentBoundary>& outSegments) {
    outSegments.clear();

    const std::wstring sidecarPath = BuildSegmentSidecarPath(dataFilePath);
    if (TryLoadSegmentLayoutFromSidecar(sidecarPath, outSegments)) {
        return true;
    }

    // Миграция сборок предыдущих версий: sidecar мог остаться рядом с файлом данных.
    const std::wstring legacyPath = BuildLegacySegmentSidecarPath(dataFilePath);
    if (legacyPath == sidecarPath || !TryLoadSegmentLayoutFromSidecar(legacyPath, outSegments)) {
        return false;
    }

    if (CopyFileW(legacyPath.c_str(), sidecarPath.c_str(), FALSE)) {
        DeleteFileW(legacyPath.c_str());
    }

    return true;
}

bool DeleteSegmentLayout(const std::wstring& dataFilePath) {
    bool success = true;

    const std::wstring sidecarPath = BuildSegmentSidecarPath(dataFilePath);
    if (GetFileAttributesW(sidecarPath.c_str()) != INVALID_FILE_ATTRIBUTES &&
        DeleteFileW(sidecarPath.c_str()) != TRUE) {
        success = false;
    }

    // Legacy-расположение зачищается вместе с основным, чтобы устаревшее описание
    // сегментов не «воскресало» через миграцию при следующей загрузке файла.
    const std::wstring legacyPath = BuildLegacySegmentSidecarPath(dataFilePath);
    if (legacyPath != sidecarPath &&
        GetFileAttributesW(legacyPath.c_str()) != INVALID_FILE_ATTRIBUTES &&
        DeleteFileW(legacyPath.c_str()) != TRUE) {
        success = false;
    }

    return success;
}

bool TryGetHomogeneousSignature(const std::vector<GP_TOOL_DATA>& records, uint32_t& outSignature) {
    outSignature = 0;
    if (records.empty()) {
        return false;
    }

    outSignature = records.front().signature;
    for (const auto& record : records) {
        if (record.signature != outSignature) {
            return false;
        }
    }

    return true;
}

bool AppendRecordsToExistingFile(const std::wstring& targetFilePath,
                                 const std::vector<GP_TOOL_DATA>& records,
                                 const std::wstring& segmentSourcePath,
                                 size_t segmentSourceStartIndex,
                                 const std::wstring& segmentLabel,
                                 std::wstring& outError) {
    outError.clear();

    if (!IsCanonicalBinFile(targetFilePath)) {
        outError = L"Добавление поддерживается только для канонических .bin файлов.";
        return false;
    }

    uint32_t appendedSignature = 0;
    if (!TryGetHomogeneousSignature(records, appendedSignature)) {
        outError = L"Выбранный отрезок содержит разные сигнатуры прибора.";
        return false;
    }

    DataLoadResult targetData;
    if (!DataModule::LoadDataFromFileResult(targetFilePath, targetData, outError)) {
        if (outError.empty()) {
            outError = L"Не удалось открыть файл для добавления.";
        }
        return false;
    }

    uint32_t targetSignature = 0;
    if (!TryGetHomogeneousSignature(targetData.data, targetSignature)) {
        outError = L"Целевой файл содержит разные сигнатуры прибора.";
        return false;
    }

    if (targetSignature != appendedSignature) {
        outError = L"Сигнатура выбранного отрезка не совпадает с сигнатурой целевого файла.";
        return false;
    }

    std::vector<GP_TOOL_DATA> combinedRecords = targetData.data;
    combinedRecords.insert(combinedRecords.end(), records.begin(), records.end());

    const std::wstring tempPath = targetFilePath + L".append_tmp";
    if (!WriteCanonicalRecords(tempPath, combinedRecords, outError)) {
        DeleteFileW(tempPath.c_str());
        return false;
    }

    if (!MoveFileExW(tempPath.c_str(), targetFilePath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
        DeleteFileW(tempPath.c_str());
        outError = L"Не удалось заменить целевой файл после добавления отрезка.";
        return false;
    }

    std::vector<SegmentBoundary> boundaries;
    const bool hasLayout = LoadSegmentLayout(targetFilePath, boundaries);
    size_t knownRecords = 0;
    for (const auto& segment : boundaries) {
        knownRecords += segment.recordCount;
    }

    if (!hasLayout || knownRecords != targetData.data.size()) {
        boundaries.clear();
        SegmentBoundary baseSegment;
        baseSegment.startIndex = 0;
        baseSegment.recordCount = targetData.data.size();
        baseSegment.sourceStartIndex = 0;
        baseSegment.sourcePath = targetFilePath;
        baseSegment.label = GetFileNameOnly(targetFilePath);
        boundaries.push_back(std::move(baseSegment));
    }

    SegmentBoundary appendedSegment;
    appendedSegment.startIndex = targetData.data.size();
    appendedSegment.recordCount = records.size();
    appendedSegment.sourceStartIndex = segmentSourceStartIndex;
    appendedSegment.sourcePath = segmentSourcePath;
    appendedSegment.label = segmentLabel.empty() ? L"Добавленный отрезок" : segmentLabel;
    boundaries.push_back(std::move(appendedSegment));

    if (!SaveSegmentLayoutInternal(targetFilePath, targetSignature, boundaries, outError)) {
        return false;
    }

    return true;
}

bool SaveLayoutExcludingRange(const std::wstring& sourceDataFilePath,
                              const std::wstring& targetDataFilePath,
                              size_t startIndex,
                              size_t endIndex,
                              size_t originalRecordCount,
                              std::wstring& outError) {
    outError.clear();

    std::vector<SegmentBoundary> sourceSegments;
    if (!LoadSegmentLayout(sourceDataFilePath, sourceSegments)) {
        DeleteSegmentLayout(targetDataFilePath);
        return true;
    }

    if (startIndex > endIndex || endIndex >= originalRecordCount) {
        outError = L"Некорректный диапазон для обновления границ сегментов.";
        return false;
    }

    const size_t cutStart = startIndex;
    const size_t cutEndExclusive = endIndex + 1;

    std::vector<SegmentBoundary> updatedSegments;
    size_t newStart = 0;

    auto appendKeptPiece = [&](const SegmentBoundary& source, size_t pieceStart, size_t pieceEnd) {
        if (pieceEnd <= pieceStart) {
            return;
        }

        SegmentBoundary piece = source;
        piece.startIndex = newStart;
        piece.recordCount = pieceEnd - pieceStart;
        piece.sourceStartIndex = source.sourceStartIndex + (pieceStart - source.startIndex);
        updatedSegments.push_back(std::move(piece));
        newStart += pieceEnd - pieceStart;
    };

    for (const auto& segment : sourceSegments) {
        const size_t segmentStart = segment.startIndex;
        const size_t segmentEnd = segment.startIndex + segment.recordCount;

        if (segmentEnd <= cutStart || segmentStart >= cutEndExclusive) {
            appendKeptPiece(segment, segmentStart, segmentEnd);
            continue;
        }

        appendKeptPiece(segment, segmentStart, (std::min)(segmentEnd, cutStart));
        appendKeptPiece(segment, (std::max)(segmentStart, cutEndExclusive), segmentEnd);
    }

    if (updatedSegments.empty()) {
        DeleteSegmentLayout(targetDataFilePath);
        return true;
    }

    DataLoadResult targetData;
    if (!DataModule::LoadDataFromFileResult(targetDataFilePath, targetData, outError)) {
        return false;
    }

    uint32_t signature = 0;
    if (!TryGetHomogeneousSignature(targetData.data, signature)) {
        outError = L"Файл после вырезания содержит разные сигнатуры прибора.";
        return false;
    }

    return SaveSegmentLayoutInternal(targetDataFilePath, signature, updatedSegments, outError);
}

bool ShowFileConstructorDialog(HWND parent, std::wstring& outSavedFilePath) {
    outSavedFilePath.clear();

    if (g_constructorDialog && IsWindow(g_constructorDialog)) {
        SetForegroundWindow(g_constructorDialog);
        return false;
    }

    ConstructorDialogContext ctx;
    ctx.owner = parent;

    HWND dialog = CreateDialogParamW(GetModuleHandleW(nullptr),
                                     MAKEINTRESOURCEW(IDD_FILE_CONSTRUCTOR_DIALOG),
                                     nullptr,
                                     FileConstructorDialogProc,
                                     reinterpret_cast<LPARAM>(&ctx));
    if (!dialog) {
        return false;
    }

    ShowWindow(dialog, SW_SHOW);
    SetForegroundWindow(dialog);

    MSG msg = {};
    BOOL getMessageResult = 0;
    while (IsWindow(dialog) && (getMessageResult = GetMessageW(&msg, nullptr, 0, 0)) > 0) {
        if (!IsDialogMessageW(dialog, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (getMessageResult == 0) {
        PostQuitMessage(static_cast<int>(msg.wParam));
    }

    if (ctx.dialogResult == IDOK && !ctx.savedPath.empty()) {
        outSavedFilePath = ctx.savedPath;
        return true;
    }

    return false;
}

} // namespace FileConstructor
