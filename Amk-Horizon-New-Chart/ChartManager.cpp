#include "framework.h"
#include "ChartManager.h"
#include "SchematicModule.h"
#include "CurveSelectionModule.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cfloat>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <functional>

// === Р—Р°РјРµС‚РєРё: РґРёР°Р»РѕРі РїСЂРѕСЃРјРѕС‚СЂР°/СѓРґР°Р»РµРЅРёСЏ (v2.16.x) ===
namespace {
    struct NoteViewDialogContext {
        std::wstring text;
        std::string noteId;
        bool deleteRequested = false;
        bool saveRequested = false;
    };

    struct NoteCreateDialogContext {
        std::wstring text;
        bool ok = false;
    };

    static INT_PTR CALLBACK NoteCreateDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
        switch (message) {
            case WM_INITDIALOG: {
                SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam);
                NoteCreateDialogContext* ctx = reinterpret_cast<NoteCreateDialogContext*>(lParam);
                if (ctx) {
                    SetDlgItemText(hDlg, IDC_NOTE_TEXT_EDIT, ctx->text.c_str());
                    SendMessage(GetDlgItem(hDlg, IDC_NOTE_TEXT_EDIT), EM_SETSEL, 0, -1);
                    SetFocus(GetDlgItem(hDlg, IDC_NOTE_TEXT_EDIT));
                    return (INT_PTR)FALSE;
                }
                return (INT_PTR)TRUE;
            }
            case WM_COMMAND: {
                int id = LOWORD(wParam);
                if (id == IDOK) {
                    NoteCreateDialogContext* ctx = reinterpret_cast<NoteCreateDialogContext*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
                    if (ctx) {
                        wchar_t buf[4096];
                        GetDlgItemText(hDlg, IDC_NOTE_TEXT_EDIT, buf, _countof(buf));
                        ctx->text = buf;
                        ctx->ok = true;
                    }
                    EndDialog(hDlg, IDOK);
                    return (INT_PTR)TRUE;
                }
                if (id == IDCANCEL) {
                    EndDialog(hDlg, IDCANCEL);
                    return (INT_PTR)TRUE;
                }
                break;
            }
        }
        return (INT_PTR)FALSE;
    }

    static INT_PTR CALLBACK NoteViewDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
        switch (message) {
            case WM_INITDIALOG: {
                SetWindowLongPtr(hDlg, GWLP_USERDATA, (LONG_PTR)lParam);
                NoteViewDialogContext* ctx = reinterpret_cast<NoteViewDialogContext*>(lParam);
                if (ctx) {
                    SetDlgItemText(hDlg, IDC_NOTE_VIEW_TEXT_EDIT, ctx->text.c_str());
                    return (INT_PTR)TRUE;
                }
                return (INT_PTR)TRUE;
            }
            case WM_COMMAND: {
                int id = LOWORD(wParam);
                if (id == IDC_NOTE_DELETE_BTN) {
                    int res = MessageBox(hDlg,
                                         L"Удалить эту заметку?\nДействие нельзя отменить.",
                                         L"Удаление заметки",
                                         MB_YESNO | MB_ICONWARNING);
                    if (res == IDYES) {
                        NoteViewDialogContext* ctx = reinterpret_cast<NoteViewDialogContext*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
                        if (ctx) ctx->deleteRequested = true;
                        EndDialog(hDlg, IDC_NOTE_DELETE_BTN);
                        return (INT_PTR)TRUE;
                    }
                    return (INT_PTR)TRUE;
                }

                if (id == IDC_NOTE_SAVE_BTN) {
                    NoteViewDialogContext* ctx = reinterpret_cast<NoteViewDialogContext*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
                    if (ctx) {
                        wchar_t buf[4096];
                        GetDlgItemText(hDlg, IDC_NOTE_VIEW_TEXT_EDIT, buf, _countof(buf));
                        ctx->text = buf;
                        ctx->saveRequested = true;
                    }
                    EndDialog(hDlg, IDC_NOTE_SAVE_BTN);
                    return (INT_PTR)TRUE;
                }

                if (id == IDOK || id == IDCANCEL) {
                    EndDialog(hDlg, id);
                    return (INT_PTR)TRUE;
                }
                break;
            }
        }
        return (INT_PTR)FALSE;
    }

    std::wstring BuildRangeExportSuggestedFileName(const std::wstring& baseFileName,
                                                   size_t startIndex,
                                                   size_t endIndex,
                                                   const SYSTEMTIME& st) {
        wchar_t fileName[260] = {0};
        if (!baseFileName.empty()) {
            swprintf_s(fileName, L"%s_range_%zu_%zu_%02d_%02d_%04d.bin",
                      baseFileName.c_str(), startIndex, endIndex,
                      st.wDay, st.wMonth, st.wYear);
        } else {
            swprintf_s(fileName, L"data_range_%zu_%zu_%02d_%02d_%04d.bin",
                      startIndex, endIndex,
                      st.wDay, st.wMonth, st.wYear);
        }

        return fileName;
    }

    std::wstring BuildCutRangeSuggestedFileName(const std::wstring& baseFileName,
                                                size_t startIndex,
                                                size_t endIndex,
                                                const SYSTEMTIME& st,
                                                const std::wstring& extension) {
        wchar_t fileName[260] = {0};
        if (!baseFileName.empty()) {
            swprintf_s(fileName, L"%s_cut_%zu_%zu_%02d_%02d_%04d%s",
                      baseFileName.c_str(), startIndex, endIndex,
                      st.wDay, st.wMonth, st.wYear, extension.c_str());
        } else {
            swprintf_s(fileName, L"data_cut_%zu_%zu_%02d_%02d_%04d%s",
                      startIndex, endIndex,
                      st.wDay, st.wMonth, st.wYear, extension.c_str());
        }

        return fileName;
    }

    std::wstring BuildRangeExportSuccessMessage(size_t recordCount,
                                                size_t startIndex,
                                                size_t endIndex,
                                                const wchar_t* filePath,
                                                bool hasExportAnalysis,
                                                const DataRangeExportAnalysis& exportAnalysis) {
        wchar_t message[768] = {0};
        if (hasExportAnalysis && exportAnalysis.requiresAdapterAwarePolicy) {
            swprintf_s(message, L"Successfully saved %zu records (indices %zu to %zu) to file:\n%s\n\nNote: the selected range contains mixed record metadata and was exported using the current canonical compatibility .bin format.",
                      recordCount, startIndex, endIndex, filePath);
        } else {
            swprintf_s(message, L"Successfully saved %zu records (indices %zu to %zu) to file:\n%s",
                      recordCount, startIndex, endIndex, filePath);
        }

        return message;
    }

    bool TryFindClosestTemperatureIndex(const std::vector<float>& temperatureData,
                                        size_t baseIndex,
                                        size_t startIdx,
                                        size_t endIdx,
                                        double xRatio,
                                        size_t& outClosestIdx) {
        outClosestIdx = 0;
        if (startIdx >= endIdx || endIdx > temperatureData.size()) {
            return false;
        }

        auto minMaxTemperature = std::minmax_element(temperatureData.begin() + startIdx, temperatureData.begin() + endIdx);
        float minTemp = *minMaxTemperature.first;
        float maxTemp = *minMaxTemperature.second;
        if (maxTemp <= minTemp) {
            maxTemp = minTemp + 1.0f;
        }

        const float clickedTemp = minTemp + static_cast<float>(xRatio) * (maxTemp - minTemp);
        float bestDiff = FLT_MAX;
        size_t closestIdx = startIdx;
        for (size_t i = startIdx; i < endIdx; ++i) {
            const float diff = fabs(temperatureData[i] - clickedTemp);
            if (diff < bestDiff) {
                bestDiff = diff;
                closestIdx = i;
            }
        }

        outClosestIdx = baseIndex + closestIdx;
        return true;
    }

    bool TryResolveTemperatureAxisIndex(const std::vector<float>& temperatureData,
                                        size_t baseIndex,
                                        double xRatio,
                                        size_t& outClosestIdx) {
        outClosestIdx = 0;
        if (temperatureData.empty()) {
            return false;
        }

        size_t endIdx = temperatureData.size();
        return TryFindClosestTemperatureIndex(temperatureData, baseIndex, 0, endIdx, xRatio, outClosestIdx);
    }

    struct TemperatureAxisRenderData {
        size_t startIndex = 0;
        size_t endIndex = 0;
        std::vector<float> temperatureData;
        double minXData = 0.0;
        double maxXData = 0.0;
    };

    struct DualAxisChartGeometry {
        RECT plotRect = {};
        RECT dataRect = {};
        int plotWidth = 0;
        int plotHeight = 0;
        int dataHeight = 0;
    };

    struct CurveDataSnapshot {
        std::vector<std::vector<float>> leftAxisData;
        std::vector<std::vector<float>> rightAxisData;
        std::vector<CurveDefinition> leftAxisCurves;
        std::vector<CurveDefinition> rightAxisCurves;
        double minX = 0.0;
        double maxX = 0.0;
        double minY1 = 0.0;
        double maxY1 = 0.0;
        double minY2 = 0.0;
        double maxY2 = 0.0;
    };

    struct VisibleFrameAxisRange {
        double minX = 0.0;
        double maxX = 0.0;
    };

    bool TryBuildTemperatureAxisRenderData(const DataModule& dataModule,
                                           bool isZoomed,
                                           double zoomMinX,
                                           double zoomMaxX,
                                           TemperatureAxisRenderData& outData) {
        outData = {};

        if (isZoomed) {
            const size_t dataSize = dataModule.GetDataSize();
            outData.startIndex = static_cast<size_t>((std::max)(0.0, zoomMinX));
            outData.endIndex = static_cast<size_t>((std::min)(static_cast<double>(dataSize), zoomMaxX + 1.0));
            outData.startIndex = (std::min)(outData.startIndex, dataSize);
            outData.endIndex = (std::min)(outData.endIndex, dataSize);
            if (outData.startIndex >= outData.endIndex) {
                return false;
            }
            dataModule.GetTemperatureData(outData.temperatureData, zoomMinX, zoomMaxX);
            outData.minXData = 0.0;
            outData.maxXData = static_cast<double>(outData.temperatureData.empty() ? 0 : (outData.temperatureData.size() - 1));
        } else {
            outData.startIndex = 0;
            outData.endIndex = dataModule.GetDataSize();
            dataModule.GetTemperatureData(outData.temperatureData);
            outData.minXData = 0.0;
            outData.maxXData = static_cast<double>(outData.temperatureData.empty() ? 0 : (outData.temperatureData.size() - 1));
        }

        return !outData.temperatureData.empty();
    }

    void BuildVisibleFrameAxisRange(bool isZoomed,
                                    double zoomMinX,
                                    double zoomMaxX,
                                    size_t dataSize,
                                    VisibleFrameAxisRange& outRange) {
        if (!isZoomed) {
            outRange.minX = 0.0;
            outRange.maxX = static_cast<double>(dataSize > 0 ? dataSize - 1 : 0);
            return;
        }

        const size_t startIndex = static_cast<size_t>((std::max)(0.0, zoomMinX));
        const size_t endIndex = static_cast<size_t>((std::min)(static_cast<double>(dataSize), zoomMaxX + 1.0));
        if (startIndex >= endIndex) {
            outRange.minX = 0.0;
            outRange.maxX = static_cast<double>(dataSize > 0 ? dataSize - 1 : 0);
            return;
        }

        outRange.minX = static_cast<double>(startIndex);
        outRange.maxX = static_cast<double>(endIndex - 1);
    }

    void BuildCurveDataSnapshot(const CurveSelectionModule& curveSelectionModule,
                                const DataModule& dataModule,
                                const ChartModule& chartModule,
                                const ChartConfiguration& config,
                                bool isZoomed,
                                double zoomMinX,
                                double zoomMaxX,
                                CurveDataSnapshot& outSnapshot) {
        outSnapshot = {};

        if (isZoomed) {
            curveSelectionModule.CalculateCurveData(config, &dataModule,
                                                    outSnapshot.leftAxisData, outSnapshot.rightAxisData,
                                                    outSnapshot.leftAxisCurves, outSnapshot.rightAxisCurves,
                                                    zoomMinX, zoomMaxX, -1e10, 1e10);
        } else {
            curveSelectionModule.CalculateCurveData(config, &dataModule,
                                                    outSnapshot.leftAxisData, outSnapshot.rightAxisData,
                                                    outSnapshot.leftAxisCurves, outSnapshot.rightAxisCurves);
        }

        chartModule.GetDataRanges(outSnapshot.leftAxisData, outSnapshot.rightAxisData,
                                  outSnapshot.minX, outSnapshot.maxX,
                                  outSnapshot.minY1, outSnapshot.maxY1,
                                  outSnapshot.minY2, outSnapshot.maxY2);

        VisibleFrameAxisRange visibleRange = {};
        BuildVisibleFrameAxisRange(isZoomed, zoomMinX, zoomMaxX, dataModule.GetDataSize(), visibleRange);
        outSnapshot.minX = visibleRange.minX;
        outSnapshot.maxX = visibleRange.maxX;
    }

    void HashCombine(uint64_t& seed, uint64_t value) {
        seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
    }

    uint64_t HashDouble(double value) {
        return std::hash<double>()(value);
    }

    uint64_t BuildChartConfigurationHash(const ChartConfiguration& config) {
        uint64_t hashValue = 0;
        HashCombine(hashValue, static_cast<uint64_t>(config.hasDualAxis ? 1 : 0));
        HashCombine(hashValue, static_cast<uint64_t>(config.xAxisType));
        HashCombine(hashValue, static_cast<uint64_t>(config.activePreset));
        HashCombine(hashValue, std::hash<std::string>()(config.leftAxisUnits));
        HashCombine(hashValue, std::hash<std::string>()(config.rightAxisUnits));
        HashCombine(hashValue, std::hash<std::wstring>()(config.presetDisplayName));

        for (const auto& curve : config.curves) {
            HashCombine(hashValue, std::hash<std::string>()(curve.id));
            HashCombine(hashValue, std::hash<std::string>()(curve.displayName));
            HashCombine(hashValue, std::hash<std::string>()(curve.units));
            HashCombine(hashValue, static_cast<uint64_t>(curve.yAxisGroup));
            HashCombine(hashValue, static_cast<uint64_t>(curve.enabled ? 1 : 0));
            HashCombine(hashValue, static_cast<uint64_t>(curve.color));
            HashCombine(hashValue, static_cast<uint64_t>(curve.lineStyle));
            HashCombine(hashValue, static_cast<uint64_t>(curve.sourceElement));
            HashCombine(hashValue, static_cast<uint64_t>(curve.elementIndex));
            HashCombine(hashValue, static_cast<uint64_t>(curve.dataType));
            HashCombine(hashValue, static_cast<uint64_t>(curve.frequency));
            HashCombine(hashValue, static_cast<uint64_t>(curve.smtMode));
        }

        return hashValue;
    }

    void DrawFrameAxisChart(ChartModule& chartModule,
                            HDC hdc,
                            const RECT& chartRect,
                            const std::vector<CurveDefinition>& curves,
                            const std::vector<std::vector<float>>& leftAxisData,
                            const std::vector<std::vector<float>>& rightAxisData,
                            const std::wstring& title,
                            bool hasData,
                            const VisibleFrameAxisRange& visibleXRange,
                            double displayMinY,
                            double displayMaxY,
                            const ChartConfiguration& config,
                            bool showAxes,
                            bool useExplicitRightAxisRange,
                            double explicitRightMinY,
                            double explicitRightMaxY) {
        chartModule.DrawDualAxisChart(hdc, chartRect, curves,
                                      leftAxisData, rightAxisData,
                                      title, hasData,
                                      visibleXRange.minX,
                                      visibleXRange.maxX,
                                      displayMinY,
                                      displayMaxY,
                                      config,
                                      showAxes,
                                      useExplicitRightAxisRange,
                                      explicitRightMinY,
                                      explicitRightMaxY);
    }

    bool TryBuildDualAxisChartGeometry(const RECT& chartRect, DualAxisChartGeometry& outGeometry) {
        outGeometry = {};

        outGeometry.plotRect.left = chartRect.left + 100;
        outGeometry.plotRect.top = chartRect.top + 45;
        outGeometry.plotRect.right = chartRect.right - 140;
        outGeometry.plotRect.bottom = chartRect.bottom - 50;

        outGeometry.plotWidth = outGeometry.plotRect.right - outGeometry.plotRect.left;
        outGeometry.plotHeight = outGeometry.plotRect.bottom - outGeometry.plotRect.top;
        if (outGeometry.plotWidth <= 0 || outGeometry.plotHeight <= 0) {
            return false;
        }

        outGeometry.dataRect = outGeometry.plotRect;
        int yMargin = outGeometry.plotHeight * 3 / 100;
        yMargin = (std::max)(8, (std::min)(24, yMargin));
        outGeometry.dataRect.top += yMargin;
        outGeometry.dataRect.bottom -= yMargin;
        outGeometry.dataHeight = outGeometry.dataRect.bottom - outGeometry.dataRect.top;

        return true;
    }

    bool TryClampSelectionToVisibleDataArea(const RECT& chartRect,
                                            int minScreenX,
                                            int minScreenY,
                                            int maxScreenX,
                                            int maxScreenY,
                                            int& outMinScreenX,
                                            int& outMinScreenY,
                                            int& outMaxScreenX,
                                            int& outMaxScreenY) {
        outMinScreenX = minScreenX;
        outMinScreenY = minScreenY;
        outMaxScreenX = maxScreenX;
        outMaxScreenY = maxScreenY;

        DualAxisChartGeometry geometry = {};
        if (!TryBuildDualAxisChartGeometry(chartRect, geometry)) {
            return false;
        }

        const LONG minX = static_cast<LONG>(minScreenX);
        const LONG maxX = static_cast<LONG>(maxScreenX);
        const LONG minY = static_cast<LONG>(minScreenY);
        const LONG maxY = static_cast<LONG>(maxScreenY);

        outMinScreenX = static_cast<int>((std::max)(geometry.plotRect.left, (std::min)(geometry.plotRect.right, minX)));
        outMaxScreenX = static_cast<int>((std::max)(geometry.plotRect.left, (std::min)(geometry.plotRect.right, maxX)));
        outMinScreenY = static_cast<int>((std::max)(geometry.dataRect.top, (std::min)(geometry.dataRect.bottom, minY)));
        outMaxScreenY = static_cast<int>((std::max)(geometry.dataRect.top, (std::min)(geometry.dataRect.bottom, maxY)));
        return true;
    }

    double ResolveLinearAxisDataX(bool isZoomed,
                                  double zoomMinX,
                                  double zoomMaxX,
                                  size_t dataSize,
                                  double xRatio) {
        if (isZoomed) {
            return zoomMinX + xRatio * (zoomMaxX - zoomMinX);
        }

        return xRatio * (dataSize > 0 ? static_cast<double>(dataSize - 1) : 0.0);
    }

    bool TryResolveNoteMarkerScreenX(const DataModule& dataModule,
                                     XAxisType xAxisType,
                                     int noteXIndex,
                                     double minXData,
                                     double maxXData,
                                     double xRange,
                                     float minTemp,
                                     float maxTemp,
                                     bool hasTempRange,
                                     size_t dataSize,
                                     const RECT& plotRect,
                                     int plotWidth,
                                     int& outScreenX) {
        outScreenX = 0;

        if (noteXIndex < 0) {
            return false;
        }

        double xRatio = 0.0;
        if (xAxisType == XAxisType::TEMPERATURE) {
            if (!hasTempRange) {
                return false;
            }
            const size_t noteIndex = static_cast<size_t>(noteXIndex);
            if (noteIndex >= dataSize) {
                return false;
            }

            const MeasurementView measurement = dataModule.GetMeasurementView(noteIndex);
            const float temperature = measurement.GetTemperature();
            xRatio = static_cast<double>((temperature - minTemp) / (maxTemp - minTemp));
        } else {
            xRatio = (static_cast<double>(noteXIndex) - minXData) / xRange;
        }

        outScreenX = plotRect.left + static_cast<int>(lround(xRatio * plotWidth));
        return true;
    }

    void ApplyDisplayYPadding(double& minY, double& maxY) {
        double range = maxY - minY;
        if (!(range > 0.0)) {
            range = 1.0;
        }
        const double padding = range * 0.05;
        minY -= padding;
        maxY += padding;
    }

    void BuildDisplayedLeftAxisRange(bool isZoomed,
                                     double zoomMinY,
                                     double zoomMaxY,
                                     double fullMinY,
                                     double fullMaxY,
                                     double& outDisplayMinY,
                                     double& outDisplayMaxY) {
        outDisplayMinY = isZoomed ? zoomMinY : fullMinY;
        outDisplayMaxY = isZoomed ? zoomMaxY : fullMaxY;
        if (isZoomed) {
            ApplyDisplayYPadding(outDisplayMinY, outDisplayMaxY);
        }
    }

    void ClearExplicitRightAxisRange(double& minYRight,
                                     double& maxYRight,
                                     bool& hasRightAxisCustomRange) {
        minYRight = 0.0;
        maxYRight = 0.0;
        hasRightAxisCustomRange = false;
    }

    void SetExplicitRightAxisRange(double& storedMinYRight,
                                   double& storedMaxYRight,
                                   bool& hasRightAxisCustomRange,
                                   bool hasRightAxisRange,
                                   double minYRight,
                                   double maxYRight) {
        if (!hasRightAxisRange || !(maxYRight > minYRight)) {
            ClearExplicitRightAxisRange(storedMinYRight, storedMaxYRight, hasRightAxisCustomRange);
            return;
        }

        storedMinYRight = minYRight;
        storedMaxYRight = maxYRight;
        hasRightAxisCustomRange = true;
    }

    void BuildDisplayedRightAxisRange(bool zoomIsYCustom,
                                      bool hasRightAxisCustomRange,
                                      double zoomMinYRight,
                                      double zoomMaxYRight,
                                      double fullMinYLeft,
                                      double fullMaxYLeft,
                                      double fullMinYRight,
                                      double fullMaxYRight,
                                      double displayMinYLeft,
                                      double displayMaxYLeft,
                                      double& outDisplayMinYRight,
                                      double& outDisplayMaxYRight) {
        outDisplayMinYRight = fullMinYRight;
        outDisplayMaxYRight = fullMaxYRight;

        if (zoomIsYCustom && hasRightAxisCustomRange && zoomMaxYRight > zoomMinYRight) {
            outDisplayMinYRight = zoomMinYRight;
            outDisplayMaxYRight = zoomMaxYRight;
            ApplyDisplayYPadding(outDisplayMinYRight, outDisplayMaxYRight);
            return;
        }

        const double leftFullRange = fullMaxYLeft - fullMinYLeft;
        const double rightFullRange = fullMaxYRight - fullMinYRight;
        if (leftFullRange > 0.0001 && rightFullRange > 0.0001) {
            const double minRatio = (displayMinYLeft - fullMinYLeft) / leftFullRange;
            const double maxRatio = (displayMaxYLeft - fullMinYLeft) / leftFullRange;
            outDisplayMinYRight = fullMinYRight + minRatio * rightFullRange;
            outDisplayMaxYRight = fullMinYRight + maxRatio * rightFullRange;
        }
    }

    bool TryBuildDrawnAxisRange(double displayMinY,
                                double displayMaxY,
                                double& outDrawMinY,
                                double& outDrawMaxY) {
        outDrawMinY = displayMinY;
        outDrawMaxY = displayMaxY;

        const double displayRangeY = outDrawMaxY - outDrawMinY;
        if (!(displayRangeY > 0.0)) {
            return false;
        }

        outDrawMinY -= displayRangeY * 0.02;
        return (outDrawMaxY - outDrawMinY) > 0.0;
    }

}

 static void AppendDebugLog(const wchar_t* fmt, ...) {
     wchar_t buffer[2048];
     va_list args;
     va_start(args, fmt);
     _vsnwprintf_s(buffer, _countof(buffer), _TRUNCATE, fmt, args);
     va_end(args);

     OutputDebugStringW(buffer);
 }

// Р РµР°Р»РёР·Р°С†РёСЏ РєР»Р°СЃСЃР° ChartManager СЃ РјРѕРґСѓР»СЊРЅРѕР№ Р°СЂС…РёС‚РµРєС‚СѓСЂРѕР№

ChartManager::ChartManager() 
    : dataModule(std::make_unique<DataModule>())
    , chartModule(std::make_unique<ChartModule>())
    , uiModule(std::make_unique<UIModule>())
    , layoutModule(std::make_unique<LayoutModule>())
    , schematicModule(std::make_unique<SchematicModule>())
    , curveSelectionModule(std::make_unique<CurveSelectionModule>())
    , serialModule(std::make_unique<SerialModule>())  // v2.13.0
    , metrologyModule(std::make_unique<MetrologyModule>())
    , notesModule(std::make_unique<NotesModule>())    // v2.16.x
    , asyncTaskQueue(std::make_unique<AsyncTaskQueue>())
    , isSelecting(false)
    , activeChart(0)
{
    selectionStart.x = selectionStart.y = 0;
    selectionEnd.x = selectionEnd.y = 0;
    
    // РЈСЃС‚Р°РЅР°РІР»РёРІР°РµРј СЃРІСЏР·СЊ РјРµР¶РґСѓ РјРѕРґСѓР»СЏРјРё
    schematicModule->SetCurveSelectionModule(curveSelectionModule.get());
    curveSelectionModule->SetMetrologyModule(metrologyModule.get());
    
    // РЈСЃС‚Р°РЅР°РІР»РёРІР°РµРј callback РґР»СЏ РѕР±РЅРѕРІР»РµРЅРёСЏ РіСЂР°С„РёРєР° РїРѕСЃР»Рµ РїСЂРёРјРµРЅРµРЅРёСЏ РЅР°СЃС‚СЂРѕРµРє (v2.9.4)
    schematicModule->SetOnConfigurationAppliedCallback([this](int chartNumber) {
        // РџРѕР»СѓС‡Р°РµРј handle РіР»Р°РІРЅРѕРіРѕ РѕРєРЅР° РёР· СЂРѕРґРёС‚РµР»СЊСЃРєРѕРіРѕ РѕРєРЅР° РґРёР°Р»РѕРіР°
        HWND hSchematicDlg = schematicModule->GetSchematicDialogHandle();
        if (hSchematicDlg) {
            HWND hMainWnd = GetParent(hSchematicDlg);
            if (hMainWnd) {
                // РЎР±СЂР°СЃС‹РІР°РµРј X Range РїСЂРё СЃРјРµРЅРµ С„СѓРЅРєС†РёРё/РїСЂРµСЃРµС‚Р°
                if (dataModule && dataModule->HasData()) {
                    uiModule->ResetXRangeControls(chartNumber);
                    
                    // РЎР±СЂР°СЃС‹РІР°РµРј zoom РґР»СЏ СЃРѕРѕС‚РІРµС‚СЃС‚РІСѓСЋС‰РµРіРѕ РіСЂР°С„РёРєР°
                    if (chartNumber == 1) {
                        chart1Zoom.isZoomed = false;
                        chart1Zoom.isXCustom = false;
                        chart1Zoom.hasXRangeLock = false;
                        chart1Zoom.isXToLatest = false;
                        chart1Zoom.isYCustom = false;
                        chart1ZoomHistory.clear(); // v2.12.5: РћС‡РёС‰Р°РµРј РёСЃС‚РѕСЂРёСЋ zoom
                        ClearAxisHistory(1);
                    } else if (chartNumber == 2) {
                        chart2Zoom.isZoomed = false;
                        chart2Zoom.isXCustom = false;
                        chart2Zoom.hasXRangeLock = false;
                        chart2Zoom.isXToLatest = false;
                        chart2Zoom.isYCustom = false;
                        chart2ZoomHistory.clear(); // v2.12.5: РћС‡РёС‰Р°РµРј РёСЃС‚РѕСЂРёСЋ zoom
                        ClearAxisHistory(2);
                    }
                }
                
                // РџРµСЂРµСЂРёСЃРѕРІС‹РІР°РµРј СЃРѕРѕС‚РІРµС‚СЃС‚РІСѓСЋС‰РёР№ РіСЂР°С„РёРє
                InvalidateChart(hMainWnd, chartNumber);
            }
        }
    });
}

ChartManager::~ChartManager() {
    EndSelectionOverlay();
}

void ChartManager::InvalidateCachedSnapshots() {
    chart1SnapshotCache = {};
    chart2SnapshotCache = {};
    chart1TemperatureCache = {};
    chart2TemperatureCache = {};
    chart1SnapshotWarmup = {};
    chart2SnapshotWarmup = {};
}

void ChartManager::ResetCoordinateTracking() {
    lastChart1CoordinateTick = 0;
    lastChart2CoordinateTick = 0;
    lastChart1CoordinatePoint = { -1, -1 };
    lastChart2CoordinatePoint = { -1, -1 };
}

bool ChartManager::ShouldUseAsyncCurveWarmup() const {
    return asyncTaskQueue && dataModule && dataModule->GetDataSize() >= 50000;
}

void ChartManager::ScheduleAsyncCurveSnapshotWarmup(HWND hWnd,
                                                    int chartNumber,
                                                    const ChartConfiguration& config,
                                                    const ZoomRange& zoom) {
    if (!ShouldUseAsyncCurveWarmup() || !hWnd || !curveSelectionModule || !dataModule || !chartModule || config.curves.empty()) {
        return;
    }

    AsyncSnapshotWarmupState* warmupState = (chartNumber == 1) ? &chart1SnapshotWarmup : &chart2SnapshotWarmup;
    const CachedCurveSnapshot* cache = (chartNumber == 1) ? &chart1SnapshotCache : &chart2SnapshotCache;
    const CachedTemperatureAxisData* temperatureCache = (chartNumber == 1) ? &chart1TemperatureCache : &chart2TemperatureCache;
    const uint64_t dataVersion = dataModule->GetDataVersion();
    const uint64_t configHash = BuildChartConfigurationHash(config);
    const bool needsTemperatureAxis = (config.xAxisType == XAxisType::TEMPERATURE);
    const bool hasMatchingTemperatureCache = !needsTemperatureAxis ||
        (temperatureCache->valid &&
         temperatureCache->dataVersion == dataVersion &&
         temperatureCache->isZoomed == zoom.isZoomed &&
         temperatureCache->zoomMinX == zoom.minX &&
         temperatureCache->zoomMaxX == zoom.maxX);
    if (cache->valid &&
        cache->dataVersion == dataVersion &&
        cache->configHash == configHash &&
        cache->isZoomed == zoom.isZoomed &&
        cache->zoomMinX == zoom.minX &&
        cache->zoomMaxX == zoom.maxX &&
        hasMatchingTemperatureCache) {
        return;
    }
    if (warmupState->inFlight &&
        warmupState->dataVersion == dataVersion &&
        warmupState->configHash == configHash &&
        warmupState->isZoomed == zoom.isZoomed &&
        warmupState->zoomMinX == zoom.minX &&
        warmupState->zoomMaxX == zoom.maxX) {
        return;
    }

    const uint64_t requestId = nextSnapshotWarmupRequestId++;
    warmupState->requestId = requestId;
    warmupState->inFlight = true;
    warmupState->dataVersion = dataVersion;
    warmupState->configHash = configHash;
    warmupState->isZoomed = zoom.isZoomed;
    warmupState->zoomMinX = zoom.minX;
    warmupState->zoomMaxX = zoom.maxX;

    asyncTaskQueue->PostWorkerTask([this, hWnd, chartNumber, requestId, dataVersion, configHash, config, zoom]() {
        CurveDataSnapshot builtSnapshot = {};
        TemperatureAxisRenderData builtTemperatureAxis = {};
        BuildCurveDataSnapshot(*curveSelectionModule, *dataModule, *chartModule, config,
                               zoom.isZoomed, zoom.minX, zoom.maxX, builtSnapshot);
        if (config.xAxisType == XAxisType::TEMPERATURE) {
            TryBuildTemperatureAxisRenderData(*dataModule, zoom.isZoomed, zoom.minX, zoom.maxX, builtTemperatureAxis);
        }

        AsyncTaskQueue::PostUiTask(hWnd, [this, hWnd, chartNumber, requestId, dataVersion, configHash, zoom,
                                         builtSnapshot = std::move(builtSnapshot),
                                         builtTemperatureAxis = std::move(builtTemperatureAxis)]() mutable {
            AsyncSnapshotWarmupState* warmup = (chartNumber == 1) ? &chart1SnapshotWarmup : &chart2SnapshotWarmup;
            if (warmup->requestId != requestId) {
                return;
            }

            warmup->inFlight = false;

            if (!dataModule || !curveSelectionModule) {
                return;
            }

            const ZoomRange currentZoom = (chartNumber == 1) ? chart1Zoom : chart2Zoom;
            const ChartConfiguration currentConfig = curveSelectionModule->GetChartConfiguration(chartNumber);
            if (dataModule->GetDataVersion() != dataVersion ||
                BuildChartConfigurationHash(currentConfig) != configHash ||
                currentZoom.isZoomed != zoom.isZoomed ||
                currentZoom.minX != zoom.minX ||
                currentZoom.maxX != zoom.maxX) {
                return;
            }

            CachedCurveSnapshot* cache = (chartNumber == 1) ? &chart1SnapshotCache : &chart2SnapshotCache;
            cache->valid = true;
            cache->dataVersion = dataVersion;
            cache->configHash = configHash;
            cache->isZoomed = zoom.isZoomed;
            cache->zoomMinX = zoom.minX;
            cache->zoomMaxX = zoom.maxX;
            cache->leftAxisData = std::move(builtSnapshot.leftAxisData);
            cache->rightAxisData = std::move(builtSnapshot.rightAxisData);
            cache->leftAxisCurves = std::move(builtSnapshot.leftAxisCurves);
            cache->rightAxisCurves = std::move(builtSnapshot.rightAxisCurves);
            cache->minX = builtSnapshot.minX;
            cache->maxX = builtSnapshot.maxX;
            cache->minY1 = builtSnapshot.minY1;
            cache->maxY1 = builtSnapshot.maxY1;
            cache->minY2 = builtSnapshot.minY2;
            cache->maxY2 = builtSnapshot.maxY2;

            if (!builtTemperatureAxis.temperatureData.empty()) {
                CachedTemperatureAxisData* temperatureCache = (chartNumber == 1) ? &chart1TemperatureCache : &chart2TemperatureCache;
                temperatureCache->valid = true;
                temperatureCache->dataVersion = dataVersion;
                temperatureCache->isZoomed = zoom.isZoomed;
                temperatureCache->zoomMinX = zoom.minX;
                temperatureCache->zoomMaxX = zoom.maxX;
                temperatureCache->startIndex = builtTemperatureAxis.startIndex;
                temperatureCache->endIndex = builtTemperatureAxis.endIndex;
                temperatureCache->temperatureData = std::move(builtTemperatureAxis.temperatureData);
                temperatureCache->minXData = builtTemperatureAxis.minXData;
                temperatureCache->maxXData = builtTemperatureAxis.maxXData;
            }

            UpdateScrollBars(hWnd);
            InvalidateChart(hWnd, chartNumber);
        });
    });
}

void ChartManager::ScheduleVisibleChartsWarmup(HWND hWnd) {
    if (!ShouldUseAsyncCurveWarmup() || !curveSelectionModule || !dataModule || !dataModule->HasData()) {
        return;
    }

    if (IsUsingNewCurveSystem(1)) {
        ScheduleAsyncCurveSnapshotWarmup(hWnd, 1, curveSelectionModule->GetChartConfiguration(1), chart1Zoom);
    }
    if (uiModule && uiModule->IsDualMode() && IsUsingNewCurveSystem(2)) {
        ScheduleAsyncCurveSnapshotWarmup(hWnd, 2, curveSelectionModule->GetChartConfiguration(2), chart2Zoom);
    }
}

bool ChartManager::TryGetCurveSnapshot(int chartNumber,
                                       const ChartConfiguration& config,
                                       const ZoomRange& zoom,
                                       bool allowSynchronousBuild,
                                       const CachedCurveSnapshot*& outSnapshot) const {
    if (!curveSelectionModule || !dataModule || !chartModule || config.curves.empty()) {
        return false;
    }

    CachedCurveSnapshot* cache = (chartNumber == 1) ? &chart1SnapshotCache : &chart2SnapshotCache;
    const uint64_t dataVersion = dataModule->GetDataVersion();
    const uint64_t configHash = BuildChartConfigurationHash(config);
    if (cache->valid &&
        cache->dataVersion == dataVersion &&
        cache->configHash == configHash &&
        cache->isZoomed == zoom.isZoomed &&
        cache->zoomMinX == zoom.minX &&
        cache->zoomMaxX == zoom.maxX) {
        outSnapshot = cache;
        return true;
    }

    if (!allowSynchronousBuild) {
        return false;
    }

    CurveDataSnapshot builtSnapshot = {};
    BuildCurveDataSnapshot(*curveSelectionModule, *dataModule, *chartModule, config,
                           zoom.isZoomed, zoom.minX, zoom.maxX, builtSnapshot);

    cache->valid = true;
    cache->dataVersion = dataVersion;
    cache->configHash = configHash;
    cache->isZoomed = zoom.isZoomed;
    cache->zoomMinX = zoom.minX;
    cache->zoomMaxX = zoom.maxX;
    cache->leftAxisData = std::move(builtSnapshot.leftAxisData);
    cache->rightAxisData = std::move(builtSnapshot.rightAxisData);
    cache->leftAxisCurves = std::move(builtSnapshot.leftAxisCurves);
    cache->rightAxisCurves = std::move(builtSnapshot.rightAxisCurves);
    cache->minX = builtSnapshot.minX;
    cache->maxX = builtSnapshot.maxX;
    cache->minY1 = builtSnapshot.minY1;
    cache->maxY1 = builtSnapshot.maxY1;
    cache->minY2 = builtSnapshot.minY2;
    cache->maxY2 = builtSnapshot.maxY2;

    outSnapshot = cache;
    return true;
}

bool ChartManager::TryGetTemperatureAxisData(int chartNumber,
                                             const ZoomRange& zoom,
                                             bool allowSynchronousBuild,
                                             const CachedTemperatureAxisData*& outData) const {
    if (!dataModule) {
        return false;
    }

    CachedTemperatureAxisData* cache = (chartNumber == 1) ? &chart1TemperatureCache : &chart2TemperatureCache;
    const uint64_t dataVersion = dataModule->GetDataVersion();
    if (cache->valid &&
        cache->dataVersion == dataVersion &&
        cache->isZoomed == zoom.isZoomed &&
        cache->zoomMinX == zoom.minX &&
        cache->zoomMaxX == zoom.maxX) {
        outData = cache;
        return !outData->temperatureData.empty();
    }

    if (!allowSynchronousBuild) {
        return false;
    }

    TemperatureAxisRenderData renderData = {};
    if (!TryBuildTemperatureAxisRenderData(*dataModule, zoom.isZoomed, zoom.minX, zoom.maxX, renderData)) {
        *cache = {};
        return false;
    }

    cache->valid = true;
    cache->dataVersion = dataVersion;
    cache->isZoomed = zoom.isZoomed;
    cache->zoomMinX = zoom.minX;
    cache->zoomMaxX = zoom.maxX;
    cache->startIndex = renderData.startIndex;
    cache->endIndex = renderData.endIndex;
    cache->temperatureData = std::move(renderData.temperatureData);
    cache->minXData = renderData.minXData;
    cache->maxXData = renderData.maxXData;

    outData = cache;
    return true;
}

double ChartManager::ResolveTemperatureAxisDataX(int chartNumber,
                                                 const ZoomRange& zoom,
                                                 double xRatio,
                                                 double fallbackDataX,
                                                 bool allowSynchronousBuild) const {
    const CachedTemperatureAxisData* temperatureAxis = nullptr;
    if (!TryGetTemperatureAxisData(chartNumber, zoom, allowSynchronousBuild, temperatureAxis)) {
        return fallbackDataX;
    }

    size_t closestIdx = 0;
    if (TryResolveTemperatureAxisIndex(temperatureAxis->temperatureData, temperatureAxis->startIndex, xRatio, closestIdx)) {
        return static_cast<double>(closestIdx);
    }

    return fallbackDataX;
}

void ChartManager::PushAxisHistory(int chartNumber) {
    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
    std::vector<ZoomRange>* zoomHistory = (chartNumber == 1) ? &chart1ZoomHistory : &chart2ZoomHistory;

    const double fullMinX = 0.0;
    const double fullMaxX = static_cast<double>(dataModule->GetDataSize() - 1);
    double xLockMin = fullMinX;
    double xLockMax = fullMaxX;

    // Р•СЃР»Рё РїРѕР»СЊР·РѕРІР°С‚РµР»СЊ Р·Р°РґР°Р» X Range С‡РµСЂРµР· РїРѕР»СЏ, РЅРµ РїРѕР·РІРѕР»СЏРµРј X-scroll РІС‹С…РѕРґРёС‚СЊ Р·Р° СЌС‚Рё РіСЂР°РЅРёС†С‹.
    if (zoom->hasXRangeLock) {
        xLockMin = zoom->minX;
        xLockMax = zoom->maxX;
    } else {
        for (auto it = zoomHistory->rbegin(); it != zoomHistory->rend(); ++it) {
            if (it->hasXRangeLock) {
                xLockMin = it->minX;
                xLockMax = it->maxX;
                break;
            }
        }
    }

    if (xLockMin < fullMinX) xLockMin = fullMinX;
    if (xLockMax > fullMaxX) xLockMax = fullMaxX;
    if (xLockMax < xLockMin) {
        xLockMin = fullMinX;
        xLockMax = fullMaxX;
    }
    std::vector<AxisZoomRange>* xHistory = (chartNumber == 1) ? &chart1XZoomHistory : &chart2XZoomHistory;
    std::vector<AxisZoomRange>* yHistory = (chartNumber == 1) ? &chart1YZoomHistory : &chart2YZoomHistory;

    xHistory->emplace_back(zoom->minX, zoom->maxX, zoom->isXCustom);
    xHistory->back().hasXRangeLock = zoom->hasXRangeLock;
    yHistory->emplace_back(zoom->minY,
                           zoom->maxY,
                           zoom->isYCustom,
                           zoom->minYRight,
                           zoom->maxYRight,
                           zoom->hasRightAxisCustomRange);
}

bool ChartManager::PopAxisHistoryX(int chartNumber, AxisZoomRange& outState) {
    std::vector<AxisZoomRange>* xHistory = (chartNumber == 1) ? &chart1XZoomHistory : &chart2XZoomHistory;
    if (xHistory->empty()) {
        return false;
    }

    outState = xHistory->back();
    xHistory->pop_back();
    return true;
}

bool ChartManager::PopAxisHistoryY(int chartNumber, AxisZoomRange& outState) {
    std::vector<AxisZoomRange>* yHistory = (chartNumber == 1) ? &chart1YZoomHistory : &chart2YZoomHistory;
    if (yHistory->empty()) {
        return false;
    }

    outState = yHistory->back();
    yHistory->pop_back();
    return true;
}

void ChartManager::ClearAxisHistory(int chartNumber) {
    if (chartNumber == 1) {
        chart1XZoomHistory.clear();
        chart1YZoomHistory.clear();
    } else if (chartNumber == 2) {
        chart2XZoomHistory.clear();
        chart2YZoomHistory.clear();
    }
}

void ChartManager::UpdateZoomActiveState(int chartNumber) {
    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
    const bool surfEnabled = uiModule && uiModule->IsSurfEnabled(chartNumber);
    zoom->isZoomed = surfEnabled || zoom->isXCustom || zoom->isYCustom;
}

void ChartManager::AutoFitYForCurrentX(int chartNumber) {
    if (!dataModule || !curveSelectionModule || !chartModule || !dataModule->HasData()) {
        return;
    }

    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;

    ChartConfiguration config = curveSelectionModule->GetChartConfiguration(chartNumber);
    std::vector<std::vector<float>> leftAxisData, rightAxisData;
    std::vector<CurveDefinition> leftAxisCurves, rightAxisCurves;

    curveSelectionModule->CalculateCurveData(config, dataModule.get(),
                                             leftAxisData, rightAxisData,
                                             leftAxisCurves, rightAxisCurves,
                                             zoom->minX, zoom->maxX, -1e10, 1e10);

    double minYData, maxYData, minXData, maxXData, minY2, maxY2;
    chartModule->GetDataRanges(leftAxisData, rightAxisData, minXData, maxXData, minYData, maxYData, minY2, maxY2);

    double yDataRange = maxYData - minYData;
    double padding = yDataRange * 0.05;
    zoom->minY = minYData - padding;
    zoom->maxY = maxYData + padding;
    zoom->isYCustom = false;
    ClearExplicitRightAxisRange(zoom->minYRight, zoom->maxYRight, zoom->hasRightAxisCustomRange);
}

void ChartManager::EnsureNotesLoadedForPath(const std::wstring& notesPath) {
    if (!notesModule) return;
    if (currentNotesPath == notesPath && notesModule->IsLoaded()) return;
    currentNotesPath = notesPath;
    notesModule->SetNotesPath(notesPath);
    notesModule->LoadFromDisk();
}

std::wstring ChartManager::ResolveCurrentNotesPath() const {
    if (!currentDataFilePath.empty()) {
        return currentDataFilePath + L".notes.jsonl";
    }

    if (serialModule && serialModule->IsRecordingEnabled() && serialModule->IsRecordingActive()) {
        std::wstring recordPath = serialModule->GetRecordFilePath();
        if (!recordPath.empty()) {
            return recordPath + L".notes.jsonl";
        }
    }

    return L"";
}

bool ChartManager::GetNotesForCurrentSource(std::vector<ChartNote>& outNotes, std::wstring& outNotesPath) {
    outNotes.clear();
    outNotesPath.clear();

    if (!notesModule) return false;

    const std::wstring notesPath = ResolveCurrentNotesPath();
    if (notesPath.empty()) {
        return false;
    }

    EnsureNotesLoadedForPath(notesPath);
    outNotesPath = notesPath;
    outNotes = notesModule->GetAllNotes();
    return true;
}

int ChartManager::GetActiveChartNumber() const {
    if (!uiModule || !uiModule->IsDualMode()) {
        return 1;
    }
    return (activeChart == 2) ? 2 : 1;
}

bool ChartManager::ApplyNoteConfiguration(HWND hWnd, const ChartNote& note, int chartNumber) {
    if (!curveSelectionModule || !hWnd) return false;
    if (chartNumber != 1 && chartNumber != 2) return false;

    XAxisType parsedXAxis = XAxisType::FRAME_NUMBER;
    PresetType parsedPreset = PresetType::NONE;
    std::vector<NotesModule::SnapshotCurveItem> parsedCurves;
    if (!NotesModule::ParseConfigSnapshot(note.configSnapshot, parsedXAxis, parsedPreset, parsedCurves)) {
        MessageBox(hWnd,
                   L"Не удалось прочитать конфигурацию кривых из заметки.",
                   L"Журнал заметок",
                   MB_OK | MB_ICONWARNING);
        return false;
    }

    ChartConfiguration config = GetChartConfiguration(chartNumber);

    if (parsedPreset != PresetType::NONE && parsedPreset != PresetType::CUSTOM) {
        FrequencyBand targetFrequency = FrequencyBand::FREQ_400_KHZ;
        if (!config.curves.empty()) {
            targetFrequency = config.curves[0].frequency;
        }
        for (const auto& c : parsedCurves) {
            if (c.curveId.find("_2000") != std::string::npos) {
                targetFrequency = FrequencyBand::FREQ_2000_KHZ;
                break;
            }
            if (c.curveId.find("_400") != std::string::npos) {
                targetFrequency = FrequencyBand::FREQ_400_KHZ;
                break;
            }
        }
        config = curveSelectionModule->CreatePresetConfiguration(parsedPreset, targetFrequency);
    }

    if (config.curves.empty()) {
        MessageBox(hWnd,
                   L"В текущем состоянии графика нельзя восстановить конфигурацию заметки.",
                   L"Журнал заметок",
                   MB_OK | MB_ICONWARNING);
        return false;
    }

    for (auto& c : config.curves) {
        c.enabled = false;
    }

    int enabledCount = 0;
    for (const auto& parsed : parsedCurves) {
        auto it = std::find_if(config.curves.begin(), config.curves.end(), [&](const CurveDefinition& c) {
            return c.id == parsed.curveId;
        });
        if (it != config.curves.end()) {
            it->enabled = true;
            it->yAxisGroup = parsed.yAxisGroup;
            enabledCount++;
        }
    }

    if (enabledCount <= 0) {
        MessageBox(hWnd,
                   L"Не найдено ни одной совместимой кривой для применения заметки.",
                   L"Журнал заметок",
                   MB_OK | MB_ICONWARNING);
        return false;
    }

    config.xAxisType = parsedXAxis;
    config.activePreset = parsedPreset;
    if (!note.presetName.empty()) {
        config.presetDisplayName = note.presetName;
    }
    config.UpdateAxisConfiguration();

    if (!ValidateChartConfiguration(config)) {
        MessageBox(hWnd,
                   L"Конфигурация из заметки не прошла проверку валидности.",
                   L"Журнал заметок",
                   MB_OK | MB_ICONWARNING);
        return false;
    }

    ApplyCurveConfiguration(chartNumber, config);
    UpdateChartAfterConfigurationChange(hWnd, chartNumber);
    return true;
}

bool ChartManager::UpdateNoteTextById(HWND hWnd, const std::string& noteId, const std::wstring& newText) {
    if (!notesModule) return false;
    if (noteId.empty()) return false;

    const std::wstring notesPath = ResolveCurrentNotesPath();
    if (notesPath.empty()) {
        MessageBox(hWnd,
                   L"Нет активного источника заметок для сохранения изменений.",
                   L"Заметки",
                   MB_OK | MB_ICONWARNING);
        return false;
    }

    EnsureNotesLoadedForPath(notesPath);
    if (!notesModule->UpdateNoteTextById(noteId, newText)) {
        MessageBox(hWnd,
                   L"Не удалось сохранить изменения заметки.",
                   L"Заметки",
                   MB_OK | MB_ICONERROR);
        return false;
    }

    return true;
}

bool ChartManager::AddNoteAtCrosshair(HWND hWnd, int chartNumber, const std::wstring& text) {
    if (!notesModule || !dataModule || !dataModule->HasData()) {
        MessageBox(hWnd, L"Нет данных для создания заметки.", L"Заметки", MB_OK | MB_ICONWARNING);
        return false;
    }

    CrosshairPosition* crosshair = nullptr;
    if (chartNumber == 1) crosshair = &chart1Crosshair;
    else if (chartNumber == 2) crosshair = &chart2Crosshair;

    if (!crosshair || !crosshair->isVisible || crosshair->chartNumber != chartNumber) {
        MessageBox(hWnd, L"Сначала выберите точку на графике (клик по области графика).", L"Заметки", MB_OK | MB_ICONINFORMATION);
        return false;
    }

    // РћРїСЂРµРґРµР»СЏРµРј РїСѓС‚СЊ РґР»СЏ Р·Р°РјРµС‚РѕРє: Р»РёР±Рѕ РѕС‚РєСЂС‹С‚ С„Р°Р№Р», Р»РёР±Рѕ Р°РєС‚РёРІРЅР° Р·Р°РїРёСЃСЊ COM
    std::wstring notesPath = ResolveCurrentNotesPath();

    if (notesPath.empty()) {
        MessageBox(hWnd,
                   L"Не найден файл данных для привязки заметок.\n"
                   L"Откройте файл данных или включите запись при работе через COM.",
                   L"Заметки",
                   MB_OK | MB_ICONWARNING);
        return false;
    }

    EnsureNotesLoadedForPath(notesPath);

    ChartConfiguration config = GetChartConfiguration(chartNumber);
    uint64_t cfgKey = NotesModule::ComputeConfigKey(config);
    std::string cfgSnapshot = NotesModule::BuildConfigSnapshot(config);

    double resolvedDataX = 0.0;
    double resolvedDataY1 = 0.0;
    double resolvedDataY2 = 0.0;
    std::wstring y1Unit, y2Unit;
    bool hasDualAxis = false;
    if (!ConvertScreenToChartCoords(chartNumber,
                                    crosshair->screenX,
                                    crosshair->screenY,
                                    resolvedDataX,
                                    resolvedDataY1,
                                    resolvedDataY2,
                                    y1Unit,
                                    y2Unit,
                                    hasDualAxis)) {
        MessageBox(hWnd,
                   L"Не удалось определить координаты заметки по текущему перекрестию.\n"
                   L"Выберите точку на графике повторно.",
                   L"Заметки",
                   MB_OK | MB_ICONWARNING);
        return false;
    }

    AppendDebugLog(L"[AddNote] chart=%d screen=(%d,%d) resolved=(x %.3f y1 %.3f y2 %.3f) dual=%d\r\n",
                   chartNumber,
                   crosshair->screenX,
                   crosshair->screenY,
                   resolvedDataX,
                   resolvedDataY1,
                   resolvedDataY2,
                   hasDualAxis ? 1 : 0);

    int xIndex = static_cast<int>(llround(resolvedDataX));
    if (xIndex < 0) xIndex = 0;
    if (dataModule->HasData()) {
        int maxIdx = static_cast<int>(dataModule->GetDataSize() > 0 ? (dataModule->GetDataSize() - 1) : 0);
        if (xIndex > maxIdx) xIndex = maxIdx;
    }

    const double y1 = resolvedDataY1;
    const double y2 = resolvedDataY2;
    int anchorAxisGroup = 0;

    if (hasDualAxis) {
        ZoomRange zoom = (chartNumber == 1) ? chart1Zoom : chart2Zoom;
        const CachedCurveSnapshot* visibleSnapshot = nullptr;
        if (TryGetCurveSnapshot(chartNumber, config, zoom, !ShouldUseAsyncCurveWarmup(), visibleSnapshot)) {
            const bool hasRightAxisData = !visibleSnapshot->rightAxisData.empty();
            if (!hasRightAxisData) {
                hasDualAxis = false;
            }
        }
    }

    AppendDebugLog(L"[AddNoteFinal] chart=%d preset=%d stored=(xIndex %d y1 %.3f y2 %.3f dual=%d anchor=%d)\r\n",
                   chartNumber,
                   static_cast<int>(config.activePreset),
                   xIndex,
                   y1,
                   y2,
                   hasDualAxis ? 1 : 0,
                   anchorAxisGroup);

    ChartNote n;
    n.noteId = NotesModule::GenerateNoteId();
    n.createdUtcMs = NotesModule::NowUtcMs();
    n.configKey = cfgKey;
    n.configSnapshot = cfgSnapshot;
    n.presetName = config.presetDisplayName;
    n.xAxisType = config.xAxisType;
    n.xIndex = xIndex;
    n.yLeft = y1;
    n.yRight = y2;
    n.hasYRight = hasDualAxis;
    n.anchorAxisGroup = anchorAxisGroup;
    n.text = text;

    if (!notesModule->AppendNote(n)) {
        MessageBox(hWnd, L"Не удалось сохранить заметку на диск.", L"Заметки", MB_OK | MB_ICONERROR);
        return false;
    }

    InvalidateRect(hWnd, NULL, FALSE);
    return true;
}

void ChartManager::BeginSelectionOverlay(HWND hWnd, const RECT& chartRect) {
    EndSelectionOverlay();

    if (!hWnd)
        return;

    HDC hdc = GetDC(hWnd);
    if (!hdc)
        return;

    const int w = chartRect.right - chartRect.left;
    const int h = chartRect.bottom - chartRect.top;
    if (w <= 0 || h <= 0) {
        ReleaseDC(hWnd, hdc);
        return;
    }

    selectionBaseDC = CreateCompatibleDC(hdc);
    if (!selectionBaseDC) {
        ReleaseDC(hWnd, hdc);
        return;
    }

    selectionBaseBmp = CreateCompatibleBitmap(hdc, w, h);
    if (!selectionBaseBmp) {
        DeleteDC(selectionBaseDC);
        selectionBaseDC = nullptr;
        ReleaseDC(hWnd, hdc);
        return;
    }

    SelectObject(selectionBaseDC, selectionBaseBmp);
    BitBlt(selectionBaseDC, 0, 0, w, h, hdc, chartRect.left, chartRect.top, SRCCOPY);

    selectionBaseRect = chartRect;
    hasSelectionBase = true;
    hasLastSelectionDrawRect = false;

    ReleaseDC(hWnd, hdc);
}

void ChartManager::EndSelectionOverlay() {
    if (selectionBaseBmp) {
        DeleteObject(selectionBaseBmp);
        selectionBaseBmp = nullptr;
    }
    if (selectionBaseDC) {
        DeleteDC(selectionBaseDC);
        selectionBaseDC = nullptr;
    }
    hasSelectionBase = false;
    hasLastSelectionDrawRect = false;
    selectionBaseRect = { 0, 0, 0, 0 };
    lastSelectionDrawRect = { 0, 0, 0, 0 };
}

void ChartManager::RestoreSelectionBackground(HDC hdc, const RECT& screenRect) {
    if (!hasSelectionBase || !selectionBaseDC || !selectionBaseBmp || !hdc)
        return;

    RECT r = screenRect;
    IntersectRect(&r, &r, &selectionBaseRect);
    if (r.right <= r.left || r.bottom <= r.top)
        return;

    const int w = r.right - r.left;
    const int h = r.bottom - r.top;
    const int srcX = r.left - selectionBaseRect.left;
    const int srcY = r.top - selectionBaseRect.top;

    BitBlt(hdc, r.left, r.top, w, h, selectionBaseDC, srcX, srcY, SRCCOPY);
}

void ChartManager::DrawSelectionOverlay(HDC hdc, const RECT& screenRect) {
    if (!hdc)
        return;

    RECT r = screenRect;
    RECT chartRect = (activeChart == 2) ? layoutModule->GetChart2Rect() : layoutModule->GetChart1Rect();
    IntersectRect(&r, &r, &chartRect);
    if (r.right <= r.left || r.bottom <= r.top)
        return;

    chartModule->DrawZoomRectangle(hdc, r);
    lastSelectionDrawRect = r;
    hasLastSelectionDrawRect = true;
}

void ChartManager::CreateControls(HWND hWnd, HINSTANCE hInst)
{
    uiModule->CreateControls(hWnd, hInst);
    
    // v2.13.0: РЈСЃС‚Р°РЅР°РІР»РёРІР°РµРј callback РґР»СЏ РїРѕР»СѓС‡РµРЅРёСЏ РґР°РЅРЅС‹С… СЃ РїСЂРёР±РѕСЂР°
    // Р­Р»РµРјРµРЅС‚С‹ СѓРїСЂР°РІР»РµРЅРёСЏ COM-РїРѕСЂС‚РѕРј С‚РµРїРµСЂСЊ РІ РјРµРЅСЋ "РџСЂРёР±РѕСЂ" Рё РґРёР°Р»РѕРіРµ IDD_SERIAL_STATUS
    serialModule->SetParsedDataCallback([this, hWnd](const ParsedToolFrame& parsedData) {
        // v2.16.x: РµСЃР»Рё РёРґС‘С‚ Р·Р°РїРёСЃСЊ РІ С„Р°Р№Р», РїСЂРёРІСЏР·С‹РІР°РµРј Р·Р°РјРµС‚РєРё Рє С‚РµРєСѓС‰РµРјСѓ recordFilePath
        if (notesModule && serialModule && serialModule->IsRecordingEnabled() && serialModule->IsRecordingActive()) {
            std::wstring recordPath = serialModule->GetRecordFilePath();
            if (!recordPath.empty()) {
                EnsureNotesLoadedForPath(recordPath + L".notes.jsonl");
            }
        }

        // Р”РѕР±Р°РІР»СЏРµРј РґР°РЅРЅС‹Рµ РІ DataModule
        dataModule->AddParsedData(parsedData);
        InvalidateCachedSnapshots();
        
        // v2.14.0: РћР±РЅРѕРІР»СЏРµРј РґРёР°РїР°Р·РѕРЅ РґР»СЏ СЂРµР¶РёРјР° Surf
        if (dataModule->HasData()) {
            // Р’РђР–РќРћ: РїСЂРё РїРѕСЃС‚СѓРїР»РµРЅРёРё РЅРѕРІС‹С… С‚РѕС‡РµРє РІ СЂРµР¶РёРјРµ СЂРµР°Р»СЊРЅРѕРіРѕ РІСЂРµРјРµРЅРё
            // РќР• СЃР±СЂР°СЃС‹РІР°РµРј РїРѕР»СЊР·РѕРІР°С‚РµР»СЊСЃРєРёР№ zoom/РєР°СЃС‚РѕРјРЅС‹Р№ X-range.
            //
            // Surf (auto-follow) РїРѕ-РїСЂРµР¶РЅРµРјСѓ РѕР±РЅРѕРІР»СЏРµС‚ РґРёР°РїР°Р·РѕРЅ РЅР° РїРѕСЃР»РµРґРЅРёРµ N С‚РѕС‡РµРє.
            if (uiModule->IsSurfEnabled(1)) {
                UpdateSurfRange(1);
            }

            if (uiModule->IsDualMode() && uiModule->IsSurfEnabled(2)) {
                UpdateSurfRange(2);
            }

            // РћР±РЅРѕРІР»СЏРµРј X Range controls РёР· С‚РµРєСѓС‰РµРіРѕ СЃРѕСЃС‚РѕСЏРЅРёСЏ zoom
            // (РёР»Рё РїРѕР»РЅРѕРіРѕ РґРёР°РїР°Р·РѕРЅР°, РµСЃР»Рё zoom РЅРµ Р°РєС‚РёРІРµРЅ).
            UpdateXRangeUI(hWnd);

            // РћР±РЅРѕРІР»СЏРµРј СЃС‚Р°С‚СѓСЃРЅСѓСЋ СЃС‚СЂРѕРєСѓ СЃ РёРЅС„РѕСЂРјР°С†РёРµР№ Рѕ Surf
            UpdateSurfStatusBar();

            // РћР±РЅРѕРІР»СЏРµРј СЃРєСЂРѕР»Р»Р±Р°СЂС‹ (total range РјРµРЅСЏРµС‚СЃСЏ РїСЂРё СЂРѕСЃС‚Рµ РґР°РЅРЅС‹С…)
            UpdateScrollBars(hWnd);
        }
        
        // РџРµСЂРµСЂРёСЃРѕРІС‹РІР°РµРј РіСЂР°С„РёРєРё
        InvalidateChart(hWnd, 1);
        if (uiModule->IsDualMode()) {
            InvalidateChart(hWnd, 2);
        }
    });
    
    // РЈСЃС‚Р°РЅР°РІР»РёРІР°РµРј callback РґР»СЏ РёР·РјРµРЅРµРЅРёСЏ СЃРѕСЃС‚РѕСЏРЅРёСЏ РїРѕРґРєР»СЋС‡РµРЅРёСЏ
    serialModule->SetStateCallback([this, hWnd](ConnectionState state, const std::wstring& error) {
        if (state == ConnectionState::CONNECTION_ERROR && !error.empty()) {
            MessageBox(hWnd, error.c_str(), L"Ошибка COM-порта", MB_OK | MB_ICONERROR);
        }
    });
}

void ChartManager::UpdateLayout(HWND hWnd)
{
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    
    int width = clientRect.right - clientRect.left;
    int height = clientRect.bottom - clientRect.top;
    
    bool dualMode = uiModule->IsDualMode();
    
    // Р’Р°Р»РёРґР°С†РёСЏ РѕРіСЂР°РЅРёС‡РµРЅРёР№ РєРѕРјРїРѕРЅРѕРІРєРё
    if (!layoutModule->ValidateLayoutConstraints(width, height, dualMode))
        return;
    
    // Р Р°СЃС‡РµС‚ РєРѕРјРїРѕРЅРѕРІРєРё
    layoutModule->CalculateLayout(width, height, dualMode);
    
    // РЈРїСЂР°РІР»РµРЅРёРµ РІРёРґРёРјРѕСЃС‚СЊСЋ Рё РїРѕР·РёС†РёРѕРЅРёСЂРѕРІР°РЅРёРµРј СЌР»РµРјРµРЅС‚РѕРІ СѓРїСЂР°РІР»РµРЅРёСЏ
    uiModule->UpdateControlsVisibility(dualMode);
    
    if (dualMode) {
        int panel2Y = layoutModule->GetChart2PanelY();
        uiModule->PositionChart2Controls(panel2Y);
        // РџРѕР·РёС†РёРѕРЅРёСЂСѓРµРј СЌР»РµРјРµРЅС‚С‹ РєРѕРѕСЂРґРёРЅР°С‚ Рє РїСЂР°РІРѕРјСѓ РєСЂР°СЋ РґР»СЏ РѕР±РѕРёС… РіСЂР°С„РёРєРѕРІ
        uiModule->PositionCursorControls(width, panel2Y);
    } else {
        // РџРѕР·РёС†РёРѕРЅРёСЂСѓРµРј СЌР»РµРјРµРЅС‚С‹ РєРѕРѕСЂРґРёРЅР°С‚ Рє РїСЂР°РІРѕРјСѓ РєСЂР°СЋ С‚РѕР»СЊРєРѕ РґР»СЏ Chart 1
        uiModule->PositionCursorControls(width, 0);
    }

    // РџРѕР·РёС†РёРѕРЅРёСЂСѓРµРј РєРѕРјРїР°РєС‚РЅС‹Рµ С‡РµРєР±РѕРєСЃС‹ РѕС‚РѕР±СЂР°Р¶РµРЅРёСЏ Р·Р°РјРµС‚РѕРє РІ РїСЂР°РІРѕРј РІРµСЂС…РЅРµРј СѓРіР»Сѓ РіСЂР°С„РёРєРѕРІ
    uiModule->PositionNotesVisibilityControls(layoutModule->GetChart1Rect(), layoutModule->GetChart2Rect(), dualMode);
    
    // РћР±РЅРѕРІР»СЏРµРј СЃРєСЂРѕР»Р»Р±Р°СЂС‹ РїРѕСЃР»Рµ РёР·РјРµРЅРµРЅРёСЏ РєРѕРјРїРѕРЅРѕРІРєРё
    UpdateScrollBars(hWnd);
    
    // РћР±РЅРѕРІР»СЏРµРј СЌР»РµРјРµРЅС‚С‹ СѓРїСЂР°РІР»РµРЅРёСЏ X Range
    UpdateXRangeUI(hWnd);
    
    // РћР±РЅРѕРІР»СЏРµРј СЂР°Р·РјРµСЂ СЃС‚Р°С‚СѓСЃРЅРѕР№ СЃС‚СЂРѕРєРё
    uiModule->ResizeStatusBar();
}

void ChartManager::ToggleDualMode()
{
    uiModule->ToggleDualMode();
}

void ChartManager::RefreshChart1()
{
    // РњРµС‚РѕРґ РѕСЃС‚Р°РІР»РµРЅ РґР»СЏ СЃРѕРІРјРµСЃС‚РёРјРѕСЃС‚Рё API, РЅРѕ Р±РѕР»СЊС€Рµ РЅРёС‡РµРіРѕ РЅРµ РґРµР»Р°РµС‚
    // РќР°СЃС‚СЂРѕР№РєРё СѓРїСЂР°РІР»СЏСЋС‚СЃСЏ С‡РµСЂРµР· Device Schematic
}

void ChartManager::RefreshChart2()
{
    // РњРµС‚РѕРґ РѕСЃС‚Р°РІР»РµРЅ РґР»СЏ СЃРѕРІРјРµСЃС‚РёРјРѕСЃС‚Рё API, РЅРѕ Р±РѕР»СЊС€Рµ РЅРёС‡РµРіРѕ РЅРµ РґРµР»Р°РµС‚
    // РќР°СЃС‚СЂРѕР№РєРё СѓРїСЂР°РІР»СЏСЋС‚СЃСЏ С‡РµСЂРµР· Device Schematic
}

void ChartManager::DrawChart1(HDC hdc)
{
    // v2.16.x: РЅР°С‡РёРЅР°РµРј РЅРѕРІС‹Р№ РЅР°Р±РѕСЂ РјР°СЂРєРµСЂРѕРІ Р·Р°РјРµС‚РѕРє РЅР° РєР°РґСЂ РѕС‚СЂРёСЃРѕРІРєРё
    noteFrameMarkers.clear();

    const RECT& chart1Rect = layoutModule->GetChart1Rect();
    
    // РџСЂРѕРІРµСЂСЏРµРј, РµСЃС‚СЊ Р»Рё РєРѕРЅС„РёРіСѓСЂР°С†РёСЏ РєСЂРёРІС‹С… РґР»СЏ РїРµСЂРІРѕРіРѕ РіСЂР°С„РёРєР°
    if (IsUsingNewCurveSystem(1)) {
        // РСЃРїРѕР»СЊР·СѓРµРј РЅРѕРІСѓСЋ СЃРёСЃС‚РµРјСѓ РєСЂРёРІС‹С…
        DrawChartWithCurves(hdc, chart1Rect, 1);
    } else {
        // РСЃРїРѕР»СЊР·СѓРµРј СЃС‚Р°СЂСѓСЋ СЃРёСЃС‚РµРјСѓ (РґР»СЏ РѕР±СЂР°С‚РЅРѕР№ СЃРѕРІРјРµСЃС‚РёРјРѕСЃС‚Рё)
        DrawChartLegacy(hdc, chart1Rect, 1);
    }

    // v2.16.x: РћС‚СЂРёСЃРѕРІС‹РІР°РµРј С‚РѕС‡РєРё Р·Р°РјРµС‚РѕРє РїРѕРІРµСЂС… РіСЂР°С„РёРєР° (РµСЃР»Рё РІРєР»СЋС‡РµРЅРѕ)
    if (uiModule->IsNotesVisible(1)) {
        DrawNoteMarkers(hdc, chart1Rect, 1);
    }

    // РћС‚СЂРёСЃРѕРІС‹РІР°РµРј РїРµСЂРµРєСЂРµСЃС‚РёРµ РїРѕРІРµСЂС… РіСЂР°С„РёРєР°
    DrawCrosshair(hdc, chart1Rect, 1);

    // Р•СЃР»Рё РІС‚РѕСЂРѕР№ РіСЂР°С„РёРє РЅРµ СЂРёСЃСѓРµС‚СЃСЏ - С„РёРєСЃРёСЂСѓРµРј РјР°СЂРєРµСЂС‹ РґР»СЏ hit-test
    if (!uiModule->IsDualMode() && notesModule) {
        notesModule->SetRenderedMarkers(std::move(noteFrameMarkers));
    }
}

void ChartManager::DrawChart2(HDC hdc)
{
    if (!uiModule->IsDualMode())
        return;
        
    const RECT& chart2Rect = layoutModule->GetChart2Rect();
    
    // РџСЂРѕРІРµСЂСЏРµРј, РµСЃС‚СЊ Р»Рё РєРѕРЅС„РёРіСѓСЂР°С†РёСЏ РєСЂРёРІС‹С… РґР»СЏ РІС‚РѕСЂРѕРіРѕ РіСЂР°С„РёРєР°
    if (IsUsingNewCurveSystem(2)) {
        // РСЃРїРѕР»СЊР·СѓРµРј РЅРѕРІСѓСЋ СЃРёСЃС‚РµРјСѓ РєСЂРёРІС‹С…
        DrawChartWithCurves(hdc, chart2Rect, 2);
    } else {
        // РСЃРїРѕР»СЊР·СѓРµРј СЃС‚Р°СЂСѓСЋ СЃРёСЃС‚РµРјСѓ (РґР»СЏ РѕР±СЂР°С‚РЅРѕР№ СЃРѕРІРјРµСЃС‚РёРјРѕСЃС‚Рё)
        DrawChartLegacy(hdc, chart2Rect, 2);
    }

    // v2.16.x: РћС‚СЂРёСЃРѕРІС‹РІР°РµРј С‚РѕС‡РєРё Р·Р°РјРµС‚РѕРє РїРѕРІРµСЂС… РіСЂР°С„РёРєР° (РµСЃР»Рё РІРєР»СЋС‡РµРЅРѕ)
    if (uiModule->IsNotesVisible(2)) {
        DrawNoteMarkers(hdc, chart2Rect, 2);
    }

    // РћС‚СЂРёСЃРѕРІС‹РІР°РµРј РїРµСЂРµРєСЂРµСЃС‚РёРµ РїРѕРІРµСЂС… РіСЂР°С„РёРєР°
    DrawCrosshair(hdc, chart2Rect, 2);

    // Р’ dual-mode РїРѕСЃР»Рµ РѕС‚СЂРёСЃРѕРІРєРё РѕР±РѕРёС… РіСЂР°С„РёРєРѕРІ С„РёРєСЃРёСЂСѓРµРј РјР°СЂРєРµСЂС‹ РґР»СЏ hit-test
    if (notesModule) {
        notesModule->SetRenderedMarkers(std::move(noteFrameMarkers));
    }
}

void ChartManager::LoadDataFromFile(HWND hWnd, const std::wstring& filename)
{
    // v2.14.1: РџСЂРё РѕС‚РєСЂС‹С‚РёРё С„Р°Р№Р»Р° РѕСЃС‚Р°РЅР°РІР»РёРІР°РµРј РѕРїСЂРѕСЃ COM-РїРѕСЂС‚Р°
    // Р РµР¶РёРјС‹ СЂР°Р±РѕС‚С‹ СЃ С„Р°Р№Р»РѕРј Рё COM-РїРѕСЂС‚РѕРј РІР·Р°РёРјРѕРёСЃРєР»СЋС‡Р°СЋС‰РёРµ
    if (serialModule && serialModule->IsPolling()) {
        serialModule->StopPolling();
    }
    if (serialModule && serialModule->IsConnected()) {
        serialModule->Disconnect();
    }

    if (!dataModule || !asyncTaskQueue) {
        return;
    }

    if (uiModule) {
        uiModule->UpdateStatusBar(L"Loading data file...");
    }

    asyncTaskQueue->PostWorkerTask([this, hWnd, filename]() {
        DataLoadResult loadedData;
        std::wstring loadError;
        const bool loaded = DataModule::LoadDataFromFileResult(filename, loadedData, loadError);

        AsyncTaskQueue::PostUiTask(hWnd, [this, hWnd, filename, loaded, loadedData = std::move(loadedData), loadError = std::move(loadError)]() mutable {
            if (!loaded) {
                const wchar_t* message = loadError.empty()
                    ? L"Не удалось открыть файл данных"
                    : loadError.c_str();
                MessageBox(hWnd, message, L"Ошибка открытия файла", MB_OK | MB_ICONERROR);
                if (uiModule) {
                    uiModule->UpdateStatusBar(L"Ready");
                }
                return;
            }

            dataModule->ApplyLoadedData(std::move(loadedData));
            InvalidateCachedSnapshots();
            ResetCoordinateTracking();

            currentDataFilePath = filename;
            if (notesModule) {
                EnsureNotesLoadedForPath(filename + L".notes.jsonl");
            }

            UpdateStatusBarFileInfo();

            chart1Zoom.isZoomed = false;
            chart2Zoom.isZoomed = false;
            chart1Zoom.isXCustom = false;
            chart2Zoom.isXCustom = false;
            chart1Zoom.hasXRangeLock = false;
            chart2Zoom.hasXRangeLock = false;
            chart1Zoom.isXToLatest = false;
            chart2Zoom.isXToLatest = false;
            chart1Zoom.isYCustom = false;
            chart2Zoom.isYCustom = false;

            chart1ZoomHistory.clear();
            chart2ZoomHistory.clear();
            ClearAxisHistory(1);
            ClearAxisHistory(2);

            if (dataModule->HasData()) {
                if (uiModule->IsSurfEnabled(1)) {
                    UpdateSurfRange(1);
                } else {
                    uiModule->ResetXRangeControls(1);
                }

                if (uiModule->IsSurfEnabled(2)) {
                    UpdateSurfRange(2);
                } else {
                    uiModule->ResetXRangeControls(2);
                }

                UpdateSurfStatusBar();
            }

            ScheduleVisibleChartsWarmup(hWnd);
            UpdateScrollBars(hWnd);
            InvalidateRect(hWnd, nullptr, TRUE);
        });
    });

    if (hWnd) {
        InvalidateRect(hWnd, nullptr, FALSE);
    }
}

bool ChartManager::PrepareSerialDisplayData(HWND hWnd, bool showFullFile, const std::wstring& filePathForPreload) {
    if (!dataModule) {
        return false;
    }

    if (!showFullFile) {
        HandleSerialClear(hWnd);
        return true;
    }

    if (filePathForPreload.empty()) {
        return false;
    }

    std::wstring loadError;
    DataLoadResult loadedData;
    if (!DataModule::LoadDataFromFileResult(filePathForPreload, loadedData, loadError)) {
        const wchar_t* message = loadError.empty()
            ? L"Не удалось открыть файл данных"
            : loadError.c_str();
        MessageBox(hWnd, message, L"Ошибка открытия файла", MB_OK | MB_ICONERROR);
        return false;
    }

    return ApplyPreparedSerialDisplayData(hWnd, filePathForPreload, std::move(loadedData));
}

bool ChartManager::ApplyPreparedSerialDisplayData(HWND hWnd,
                                                  const std::wstring& filePathForPreload,
                                                  DataLoadResult&& loadedData) {
    if (!dataModule || !loadedData.HasData()) {
        return false;
    }

    dataModule->ApplyLoadedData(std::move(loadedData));
    InvalidateCachedSnapshots();
    ResetCoordinateTracking();

    currentDataFilePath = filePathForPreload;
    if (notesModule) {
        EnsureNotesLoadedForPath(filePathForPreload + L".notes.jsonl");
    }

    if (serialModule) {
        serialModule->ResetStatistics();
    }

    chart1Zoom.isZoomed = false;
    chart2Zoom.isZoomed = false;
    chart1Zoom.isXCustom = false;
    chart2Zoom.isXCustom = false;
    chart1Zoom.isXToLatest = false;
    chart2Zoom.isXToLatest = false;
    chart1Zoom.isYCustom = false;
    chart2Zoom.isYCustom = false;
    chart1ZoomHistory.clear();
    chart2ZoomHistory.clear();
    ClearAxisHistory(1);
    ClearAxisHistory(2);

    if (dataModule->HasData() && uiModule) {
        if (uiModule->IsSurfEnabled(1)) {
            UpdateSurfRange(1);
        } else {
            uiModule->ResetXRangeControls(1);
        }

        if (uiModule->IsSurfEnabled(2)) {
            UpdateSurfRange(2);
        } else {
            uiModule->ResetXRangeControls(2);
        }

        UpdateSurfStatusBar();
    }

    UpdateStatusBarFileInfo();
    ScheduleVisibleChartsWarmup(hWnd);
    UpdateScrollBars(hWnd);
    InvalidateRect(hWnd, nullptr, TRUE);
    return true;
}

bool ChartManager::HandleCommand(WPARAM wParam, LPARAM lParam)
{
    HWND hWnd = GetParent((HWND)lParam);
    int wmId = LOWORD(wParam);
    bool needsRedraw = false;
    
    // РћР±СЂР°Р±РѕС‚РєР° РєРѕРјР°РЅРґ РѕС‚ РґРёР°Р»РѕРіРѕРІРѕРіРѕ РѕРєРЅР° СЃС…РµРјС‹ РїСЂРёР±РѕСЂР°
    switch (wmId) {
        case IDC_CHART1_SCHEMATIC_BTN:
            ShowSchematicDialog(hWnd, 1);
            return true;
            
        case IDC_CHART2_SCHEMATIC_BTN:
            ShowSchematicDialog(hWnd, 2);
            return true;
            
        case IDC_APPLY_SETTINGS:
            // РљРѕРјР°РЅРґР° РїСЂРёРјРµРЅРµРЅРёСЏ РЅР°СЃС‚СЂРѕРµРє РёР· РґРёР°Р»РѕРіРѕРІРѕРіРѕ РѕРєРЅР° СЃС…РµРјС‹
            // РћР±СЂР°Р±РѕС‚РєР° РїСЂРѕРёСЃС…РѕРґРёС‚ РІ СЃР°РјРѕРј РґРёР°Р»РѕРіРѕРІРѕРј РѕРєРЅРµ
            return true;
            
        case IDC_PRESET_ALL_AMP_R1:
        case IDC_PRESET_ALL_AMP_R2:
        case IDC_PRESET_RATIOS:
        case IDC_PRESET_DIFFERENCES:
            // РљРѕРјР°РЅРґС‹ РїСЂРµСЃРµС‚РѕРІ РѕР±СЂР°Р±Р°С‚С‹РІР°СЋС‚СЃСЏ РІ РґРёР°Р»РѕРіРѕРІРѕРј РѕРєРЅРµ СЃС…РµРјС‹
            return true;
            
        case IDC_DUAL_MODE_CHECK:
            // РџРµСЂРµРєР»СЋС‡РµРЅРёРµ СЂРµР¶РёРјР° РґРІСѓС… РіСЂР°С„РёРєРѕРІ
            needsRedraw = uiModule->HandleCommand(wParam, lParam, hWnd);
            UpdateLayout(hWnd);
            return needsRedraw;
            
        case IDC_XRANGE_APPLY_BTN:
            // РџСЂРёРјРµРЅРµРЅРёРµ РїРѕР»СЊР·РѕРІР°С‚РµР»СЊСЃРєРѕРіРѕ РґРёР°РїР°Р·РѕРЅР° X РґР»СЏ Chart 1
            HandleXRangeApply(hWnd, 1);
            return true;
            
        case IDC_XRANGE_RESET_BTN:
            // РЎР±СЂРѕСЃ РґРёР°РїР°Р·РѕРЅР° X Рє РїРѕР»РЅРѕРјСѓ РґР»СЏ Chart 1
            HandleXRangeReset(hWnd, 1);
            return true;
            
        case IDC_XRANGE2_APPLY_BTN:
            // РџСЂРёРјРµРЅРµРЅРёРµ РїРѕР»СЊР·РѕРІР°С‚РµР»СЊСЃРєРѕРіРѕ РґРёР°РїР°Р·РѕРЅР° X РґР»СЏ Chart 2
            HandleXRangeApply(hWnd, 2);
            return true;
            
        case IDC_XRANGE2_RESET_BTN:
            // РЎР±СЂРѕСЃ РґРёР°РїР°Р·РѕРЅР° X Рє РїРѕР»РЅРѕРјСѓ РґР»СЏ Chart 2
            HandleXRangeReset(hWnd, 2);
            return true;
            
        case IDC_RESET_ZOOM1_BTN:
            // РЎР±СЂРѕСЃ zoom РґР»СЏ Chart 1 (Р°РЅР°Р»РѕРіРёС‡РЅРѕ РїСЂР°РІРѕРјСѓ РєР»РёРєСѓ РјС‹С€Рё)
            HandleResetZoom(hWnd, 1);
            return true;
            
        case IDC_RESET_ZOOM2_BTN:
            // РЎР±СЂРѕСЃ zoom РґР»СЏ Chart 2 (Р°РЅР°Р»РѕРіРёС‡РЅРѕ РїСЂР°РІРѕРјСѓ РєР»РёРєСѓ РјС‹С€Рё)
            HandleResetZoom(hWnd, 2);
            return true;

        case IDC_CHART1_UNDO_ZOOM_BTN:
            HandleUndoZoomStep(hWnd, 1);
            return true;

        case IDC_CHART2_UNDO_ZOOM_BTN:
            HandleUndoZoomStep(hWnd, 2);
            return true;

        case IDC_CHART1_HUNDO_BTN:
            HandleScrollEvent(hWnd, MAKEWPARAM(SB_LINELEFT, 0), (LPARAM)uiModule->GetHScrollBar(1));
            return true;

        case IDC_CHART1_VUNDO_BTN:
            HandleScrollEvent(hWnd, MAKEWPARAM(SB_LINEUP, 0), (LPARAM)uiModule->GetVScrollBar(1));
            return true;

        case IDC_CHART2_HUNDO_BTN:
            HandleScrollEvent(hWnd, MAKEWPARAM(SB_LINELEFT, 0), (LPARAM)uiModule->GetHScrollBar(2));
            return true;

        case IDC_CHART2_VUNDO_BTN:
            HandleScrollEvent(hWnd, MAKEWPARAM(SB_LINEUP, 0), (LPARAM)uiModule->GetVScrollBar(2));
            return true;
            
        // === РљРѕРјР°РЅРґС‹ COM-РїРѕСЂС‚Р° (v2.13.0) ===
        case IDC_SERIAL_CONNECT_BTN:
            if (serialModule->IsConnected()) {
                HandleSerialDisconnect(hWnd);
            } else {
                HandleSerialConnect(hWnd);
            }
            return true;
            
        case IDC_SERIAL_START_BTN:
            HandleSerialStart(hWnd);
            return true;
            
        case IDC_SERIAL_STOP_BTN:
            HandleSerialStop(hWnd);
            return true;
            
        case IDC_SERIAL_CLEAR_BTN:
            HandleSerialClear(hWnd);
            return true;
            
        // === РљРѕРјР°РЅРґС‹ СЂРµР¶РёРјР° Surf (v2.14.0) ===
        case IDC_CHART1_SURF_CHECK:
            HandleSurfToggle(hWnd, 1);
            return true;
            
        case IDC_CHART2_SURF_CHECK:
            HandleSurfToggle(hWnd, 2);
            return true;

        case IDC_CHART1_SHOW_NOTES_CHECK:
        case IDC_CHART2_SHOW_NOTES_CHECK:
            // РџРµСЂРµРєР»СЋС‡РµРЅРёРµ РѕС‚РѕР±СЂР°Р¶РµРЅРёСЏ РјР°СЂРєРµСЂРѕРІ Р·Р°РјРµС‚РѕРє
            return true;
            
        case IDC_CHART1_SURF_EDIT:
            // РџСЂРё РёР·РјРµРЅРµРЅРёРё СЂР°Р·РјРµСЂР° РѕРєРЅР° Surf РґР»СЏ Chart 1
            if (HIWORD(wParam) == EN_CHANGE && uiModule->IsSurfEnabled(1)) {
                UpdateSurfRange(1);
                UpdateSurfStatusBar();
                InvalidateChart(hWnd, 1);
            }
            return true;
            
        case IDC_CHART2_SURF_EDIT:
            // РџСЂРё РёР·РјРµРЅРµРЅРёРё СЂР°Р·РјРµСЂР° РѕРєРЅР° Surf РґР»СЏ Chart 2
            if (HIWORD(wParam) == EN_CHANGE && uiModule->IsSurfEnabled(2)) {
                UpdateSurfRange(2);
                UpdateSurfStatusBar();
                InvalidateChart(hWnd, 2);
            }
            return true;
            
        default:
            // РџРµСЂРµРґР°РµРј РѕСЃС‚Р°Р»СЊРЅС‹Рµ РєРѕРјР°РЅРґС‹ РІ UIModule
            needsRedraw = uiModule->HandleCommand(wParam, lParam, hWnd);
            break;
    }
    
    return needsRedraw;
}

bool ChartManager::IsDualMode() const
{
    return uiModule->IsDualMode();
}

std::wstring ChartManager::GetChartTitle(int chartNumber, FrequencyBand frequency, const std::wstring& customTitle) const {
    if (!customTitle.empty()) {
        return customTitle;
    }
    std::wstring title = L"\x0413\x0440\x0430\x0444\x0438\x043A ";
    title += std::to_wstring(chartNumber);
    title += L" (";
    // Р›РѕРєР°Р»РёР·РѕРІР°РЅРЅРѕРµ РѕС‚РѕР±СЂР°Р¶РµРЅРёРµ С‡Р°СЃС‚РѕС‚С‹ РІ Р·Р°РіРѕР»РѕРІРєРµ (РєР“С†)
    title += (frequency == FrequencyBand::FREQ_400_KHZ) ? L"400 РєР“С†" : L"2000 РєР“С†";
    title += L")";
    return title;
}

void ChartManager::HandleLegendCheckboxClick(HWND hWnd, int x, int y, int chartId) {
    // РњРµС‚РѕРґ РѕСЃС‚Р°РІР»РµРЅ РґР»СЏ СЃРѕРІРјРµСЃС‚РёРјРѕСЃС‚Рё API, РЅРѕ Р±РѕР»СЊС€Рµ РЅРёС‡РµРіРѕ РЅРµ РґРµР»Р°РµС‚
    // РЈРїСЂР°РІР»РµРЅРёРµ РєСЂРёРІС‹РјРё РїСЂРѕРёСЃС…РѕРґРёС‚ С‚РѕР»СЊРєРѕ С‡РµСЂРµР· Device Schematic
}

// РћР±СЂР°Р±РѕС‚С‡РёРєРё СЃРѕР±С‹С‚РёР№ РјС‹С€Рё
void ChartManager::HandleMouseDown(HWND hWnd, int x, int y) {
    if (!dataModule->HasData())
        return;

    RECT chart1Rect = layoutModule->GetChart1Rect();
    RECT chart2Rect = layoutModule->GetChart2Rect();

    int clickedChart = GetChartAtPoint(x, y);

    // РќР°С‡РёРЅР°РµРј РІС‹РґРµР»РµРЅРёРµ С‚РѕР»СЊРєРѕ РµСЃР»Рё РєР»РёРє Р±С‹Р» РїРѕ РіСЂР°С„РёРєСѓ
    if (clickedChart == 0) {
        return;
    }

    // v2.16.x: РµСЃР»Рё РєР»РёРєРЅСѓР»Рё РїРѕ РјР°СЂРєРµСЂСѓ Р·Р°РјРµС‚РєРё вЂ” РѕС‚РєСЂС‹РІР°РµРј Р·Р°РјРµС‚РєСѓ (СЃ РІРѕР·РјРѕР¶РЅРѕСЃС‚СЊСЋ СѓРґР°Р»РµРЅРёСЏ)
    // Рё РЅРµ РЅР°С‡РёРЅР°РµРј zoom/selection
    if (notesModule) {
        const ChartNote* note = notesModule->HitTestRenderedMarkers(clickedChart, x, y, 6);
        if (note) {
            NoteViewDialogContext ctx;
            ctx.text = note->text;
            ctx.noteId = note->noteId;
            ctx.deleteRequested = false;
            ctx.saveRequested = false;

            HINSTANCE hInst = GetModuleHandle(nullptr);
            DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_NOTE_VIEW_DIALOG), hWnd, NoteViewDialogProc, (LPARAM)&ctx);

            if (ctx.deleteRequested) {
                (void)notesModule->DeleteNoteById(ctx.noteId);
                InvalidateRect(hWnd, NULL, FALSE);
            } else if (ctx.saveRequested) {
                if (UpdateNoteTextById(hWnd, ctx.noteId, ctx.text)) {
                    InvalidateRect(hWnd, NULL, FALSE);
                }
            }
            return;
        }
    }

    activeChart = clickedChart;
    // v2.14.2: РљРѕРѕСЂРґРёРЅР°С‚С‹ Рё РїРµСЂРµРєСЂРµСЃС‚РёРµ РѕР±РЅРѕРІР»СЏСЋС‚СЃСЏ РІ HandleMouseUp,
    // С‚РѕР»СЊРєРѕ РµСЃР»Рё СЌС‚Рѕ Р±С‹Р» РєР»РёРє, Р° РЅРµ drag (РІС‹РґРµР»РµРЅРёРµ РѕР±Р»Р°СЃС‚Рё РґР»СЏ zoom)

    isSelecting = true;
    selectionStart.x = x;
    selectionStart.y = y;
    selectionEnd.x = x;
    selectionEnd.y = y;

    // Р“РѕС‚РѕРІРёРј Р±С‹СЃС‚СЂС‹Р№ РѕРІРµСЂР»РµР№ РІС‹РґРµР»РµРЅРёСЏ (СЃРЅРёРјРѕРє РѕР±Р»Р°СЃС‚Рё РіСЂР°С„РёРєР°)
    RECT chartRect = (activeChart == 2) ? layoutModule->GetChart2Rect() : layoutModule->GetChart1Rect();
    BeginSelectionOverlay(hWnd, chartRect);

    SetCapture(hWnd);
}

void ChartManager::HandleMouseMove(HWND hWnd, int x, int y)
{
    if (!isSelecting)
        return;

    // Р‘С‹СЃС‚СЂР°СЏ РѕС‚СЂРёСЃРѕРІРєР° РІС‹РґРµР»РµРЅРёСЏ РїРѕРІРµСЂС… СѓР¶Рµ РЅР°СЂРёСЃРѕРІР°РЅРЅРѕРіРѕ РіСЂР°С„РёРєР° (Р±РµР· РїРѕР»РЅРѕРіРѕ WM_PAINT)
    HDC hdc = GetDC(hWnd);
    if (!hdc)
        return;

    // РћР±РЅРѕРІР»СЏРµРј РєРѕРЅРµС‡РЅСѓСЋ С‚РѕС‡РєСѓ РІС‹РґРµР»РµРЅРёСЏ
    selectionEnd.x = x;
    selectionEnd.y = y;

    RECT newRect;
    newRect.left = (std::min)(selectionStart.x, selectionEnd.x);
    newRect.top = (std::min)(selectionStart.y, selectionEnd.y);
    newRect.right = (std::max)(selectionStart.x, selectionEnd.x);
    newRect.bottom = (std::max)(selectionStart.y, selectionEnd.y);

    // РћР±Р»Р°СЃС‚СЊ РѕР±РЅРѕРІР»РµРЅРёСЏ = union(РїСЂРµРґС‹РґСѓС‰РµРіРѕ РЅР°СЂРёСЃРѕРІР°РЅРЅРѕРіРѕ rect, РЅРѕРІРѕРіРѕ rect)
    RECT updateRect = newRect;
    if (hasLastSelectionDrawRect) {
        UnionRect(&updateRect, &lastSelectionDrawRect, &newRect);
    }
    InflateRect(&updateRect, 2, 2);

    // Р’РѕСЃСЃС‚Р°РЅР°РІР»РёРІР°РµРј С„РѕРЅ РїРѕ "СЃРЅРёРјРєСѓ" Рё СЂРёСЃСѓРµРј РЅРѕРІС‹Р№ РѕРІРµСЂР»РµР№
    RestoreSelectionBackground(hdc, updateRect);
    DrawSelectionOverlay(hdc, newRect);

    ReleaseDC(hWnd, hdc);
}

void ChartManager::HandleMouseUp(HWND hWnd, int x, int y)
{
    if (!isSelecting)
        return;

    isSelecting = false;
    ReleaseCapture();
    
    selectionEnd.x = x;
    selectionEnd.y = y;

    // РЈР±РёСЂР°РµРј РѕРІРµСЂР»РµР№: РІРѕСЃСЃС‚Р°РЅР°РІР»РёРІР°РµРј С„РѕРЅ Рё РѕСЃРІРѕР±РѕР¶РґР°РµРј СЂРµСЃСѓСЂСЃС‹
    if (hasSelectionBase) {
        HDC hdc = GetDC(hWnd);
        if (hdc) {
            RECT restoreRect = selectionBaseRect;
            RestoreSelectionBackground(hdc, restoreRect);
            ReleaseDC(hWnd, hdc);
        }
    }
    EndSelectionOverlay();
    
    // РџСЂРѕРІРµСЂСЏРµРј, С‡С‚Рѕ РІС‹РґРµР»РµРЅР° РґРѕСЃС‚Р°С‚РѕС‡РЅРѕ Р±РѕР»СЊС€Р°СЏ РѕР±Р»Р°СЃС‚СЊ
    int deltaX = abs(selectionEnd.x - selectionStart.x);
    int deltaY = abs(selectionEnd.y - selectionStart.y);
    
    const bool surfEnabled = (activeChart != 0) ? uiModule->IsSurfEnabled(activeChart) : false;
    const bool isZoomGesture = surfEnabled ? (deltaY > 10) : (deltaX > 10 && deltaY > 10);
    
    if (isZoomGesture) {
        // Р­С‚Рѕ zoom вЂ” СЃРєСЂС‹РІР°РµРј РїРµСЂРµРєСЂРµСЃС‚РёРµ
        if (activeChart == 1) {
            chart1Crosshair.isVisible = false;
        } else if (activeChart == 2) {
            chart2Crosshair.isVisible = false;
        }
        
        // РџСЂРµРѕР±СЂР°Р·СѓРµРј СЌРєСЂР°РЅРЅС‹Рµ РєРѕРѕСЂРґРёРЅР°С‚С‹ РІ РєРѕРѕСЂРґРёРЅР°С‚С‹ РґР°РЅРЅС‹С…
        double minDataX, minDataY, maxDataX, maxDataY;
        double minDataYRight = 0.0;
        double maxDataYRight = 0.0;
        std::wstring minY1Unit, minY2Unit, maxY1Unit, maxY2Unit;
        bool minCornerHasDualAxis = false;
        bool maxCornerHasDualAxis = false;

        const int rawMinScreenX = (std::min)(selectionStart.x, selectionEnd.x);
        const int rawMaxScreenX = (std::max)(selectionStart.x, selectionEnd.x);
        const int rawMinScreenY = (std::min)(selectionStart.y, selectionEnd.y);
        const int rawMaxScreenY = (std::max)(selectionStart.y, selectionEnd.y);

        int minScreenX = rawMinScreenX;
        int maxScreenX = rawMaxScreenX;
        int minScreenY = rawMinScreenY;
        int maxScreenY = rawMaxScreenY;

        const RECT chartRect = (activeChart == 2) ? layoutModule->GetChart2Rect() : layoutModule->GetChart1Rect();
        TryClampSelectionToVisibleDataArea(chartRect,
                                           rawMinScreenX,
                                           rawMinScreenY,
                                           rawMaxScreenX,
                                           rawMaxScreenY,
                                           minScreenX,
                                           minScreenY,
                                           maxScreenX,
                                           maxScreenY);

        if (maxScreenX - minScreenX <= 10 || maxScreenY - minScreenY <= 10) {
            AppendDebugLog(L"[ZoomApply] chart=%d ignored rawRect=[(%d,%d)-(%d,%d)] clampedRect=[(%d,%d)-(%d,%d)]\r\n",
                           activeChart,
                           rawMinScreenX,
                           rawMinScreenY,
                           rawMaxScreenX,
                           rawMaxScreenY,
                           minScreenX,
                           minScreenY,
                           maxScreenX,
                           maxScreenY);
            UpdateScrollBars(hWnd);
            if (activeChart > 0) {
                InvalidateChart(hWnd, activeChart);
            } else {
                InvalidateRect(hWnd, NULL, FALSE);
            }
            return;
        }

        bool resolvedMinCorner = ConvertScreenToChartCoords(activeChart,
                                                            minScreenX,
                                                            maxScreenY,
                                                            minDataX,
                                                            minDataY,
                                                            minDataYRight,
                                                            minY1Unit,
                                                            minY2Unit,
                                                            minCornerHasDualAxis);
        bool resolvedMaxCorner = ConvertScreenToChartCoords(activeChart,
                                                            maxScreenX,
                                                            minScreenY,
                                                            maxDataX,
                                                            maxDataY,
                                                            maxDataYRight,
                                                            maxY1Unit,
                                                            maxY2Unit,
                                                            maxCornerHasDualAxis);

        if (!resolvedMinCorner) {
            ConvertScreenToDataCoords(activeChart, minScreenX, maxScreenY, minDataX, minDataY);
        }
        if (!resolvedMaxCorner) {
            ConvertScreenToDataCoords(activeChart, maxScreenX, minScreenY, maxDataX, maxDataY);
        }

        AppendDebugLog(L"[ZoomApply] chart=%d rawRect=[(%d,%d)-(%d,%d)] clampedRect=[(%d,%d)-(%d,%d)] dataRect=[x %.3f..%.3f y1 %.3f..%.3f y2 %.3f..%.3f dual=%d]\r\n",
                       activeChart,
                       rawMinScreenX,
                       rawMinScreenY,
                       rawMaxScreenX,
                       rawMaxScreenY,
                       minScreenX,
                       minScreenY,
                       maxScreenX,
                       maxScreenY,
                       minDataX,
                       maxDataX,
                       minDataY,
                       maxDataY,
                       minDataYRight,
                       maxDataYRight,
                       minCornerHasDualAxis && maxCornerHasDualAxis ? 1 : 0);
        
        const double MIN_X_RANGE = 10.0;
        const double MIN_Y_RANGE = 10.0;
        
        // Surf ON: zoom С‚РѕР»СЊРєРѕ РїРѕ РѕСЃРё Y, X РІСЃРµРіРґР° С„РёРєСЃРёСЂРѕРІР°РЅ РЅР° РѕРєРЅРµ Surf (РїРѕСЃР»РµРґРЅРёРµ N С‚РѕС‡РµРє).
        if (surfEnabled) {
            // Р¤РёРєСЃРёСЂСѓРµРј X РґРёР°РїР°Р·РѕРЅ РЅР° РѕРєРЅРѕ Surf (РїРѕСЃР»РµРґРЅРёРµ N С‚РѕС‡РµРє)
            size_t dataSize = dataModule->GetDataSize();
            int surfWindow = uiModule->GetSurfWindowSize(activeChart);
            double surfMinX = (dataSize > static_cast<size_t>(surfWindow))
                ? static_cast<double>(dataSize - surfWindow) : 0.0;
            double surfMaxX = (dataSize > 0) ? static_cast<double>(dataSize - 1) : 0.0;
            
            // РџСЂРѕРІРµСЂСЏРµРј РјРёРЅРёРјР°Р»СЊРЅС‹Р№ РґРёР°РїР°Р·РѕРЅ РґР»СЏ Y (С‡С‚РѕР±С‹ РЅРµ СѓС…РѕРґРёС‚СЊ РІ "Р±РµСЃРєРѕРЅРµС‡РЅС‹Р№" zoom)
            double yRange = maxDataY - minDataY;
            if (yRange < MIN_Y_RANGE) {
                MessageBox(hWnd,
                    L"Невозможно увеличить масштаб дальше: диапазон оси Y должен быть не менее 10 единиц.\nМинимальный диапазон — 10 единиц.",
                    L"Достигнут предел масштаба",
                    MB_OK | MB_ICONINFORMATION);
                return;
            }
            
            if (activeChart == 1) {
                chart1ZoomHistory.push_back(chart1Zoom);
                PushAxisHistory(1);
                chart1Zoom.minX = surfMinX;
                chart1Zoom.maxX = surfMaxX;
                chart1Zoom.minY = minDataY;
                chart1Zoom.maxY = maxDataY;
                SetExplicitRightAxisRange(chart1Zoom.minYRight,
                                          chart1Zoom.maxYRight,
                                          chart1Zoom.hasRightAxisCustomRange,
                                          minCornerHasDualAxis && maxCornerHasDualAxis,
                                          minDataYRight,
                                          maxDataYRight);
                chart1Zoom.isZoomed = true;
                chart1Zoom.isXCustom = false;
                chart1Zoom.hasXRangeLock = false;
                chart1Zoom.isXToLatest = false;
                chart1Zoom.isYCustom = true;
            } else if (activeChart == 2) {
                chart2ZoomHistory.push_back(chart2Zoom);
                PushAxisHistory(2);
                chart2Zoom.minX = surfMinX;
                chart2Zoom.maxX = surfMaxX;
                chart2Zoom.minY = minDataY;
                chart2Zoom.maxY = maxDataY;
                SetExplicitRightAxisRange(chart2Zoom.minYRight,
                                          chart2Zoom.maxYRight,
                                          chart2Zoom.hasRightAxisCustomRange,
                                          minCornerHasDualAxis && maxCornerHasDualAxis,
                                          minDataYRight,
                                          maxDataYRight);
                chart2Zoom.isZoomed = true;
                chart2Zoom.isXCustom = false;
                chart2Zoom.hasXRangeLock = false;
                chart2Zoom.isXToLatest = false;
                chart2Zoom.isYCustom = true;
            }
        } else {
            // РћР±С‹С‡РЅС‹Р№ СЂРµР¶РёРј: zoom РїРѕ X Рё Y
            double xRange = maxDataX - minDataX;
            if (xRange < MIN_X_RANGE) {
                MessageBox(hWnd,
                    L"Невозможно увеличить масштаб дальше: подписи оси X должны оставаться целыми числами с минимальным шагом 1.\nМинимальный диапазон — 10 точек.",
                    L"Достигнут предел масштаба",
                    MB_OK | MB_ICONINFORMATION);
                return;
            }
            
            // v2.14.0: РћРіСЂР°РЅРёС‡РёРІР°РµРј zoom РѕР±Р»Р°СЃС‚СЊСЋ Surf, РµСЃР»Рё СЂРµР¶РёРј Surf Р°РєС‚РёРІРµРЅ (РЅР° РІСЃСЏРєРёР№ СЃР»СѓС‡Р°Р№)
            // (РІ СЌС‚РѕР№ РІРµС‚РєРµ surfEnabled == false, РЅРѕ РѕСЃС‚Р°РІР»СЏРµРј Р»РѕРіРёРєСѓ Р±СѓРґСѓС‰РµР№ СЃРѕРІРјРµСЃС‚РёРјРѕСЃС‚Рё)
            
            if (activeChart == 1) {
                chart1ZoomHistory.push_back(chart1Zoom);
                PushAxisHistory(1);
                chart1Zoom.minX = minDataX;
                chart1Zoom.maxX = maxDataX;
                chart1Zoom.minY = minDataY;
                chart1Zoom.maxY = maxDataY;
                SetExplicitRightAxisRange(chart1Zoom.minYRight,
                                          chart1Zoom.maxYRight,
                                          chart1Zoom.hasRightAxisCustomRange,
                                          minCornerHasDualAxis && maxCornerHasDualAxis,
                                          minDataYRight,
                                          maxDataYRight);
                chart1Zoom.isZoomed = true;
                chart1Zoom.isXCustom = true;
                chart1Zoom.hasXRangeLock = false;
                chart1Zoom.isYCustom = true;
            } else if (activeChart == 2) {
                chart2ZoomHistory.push_back(chart2Zoom);
                PushAxisHistory(2);
                chart2Zoom.minX = minDataX;
                chart2Zoom.maxX = maxDataX;
                chart2Zoom.minY = minDataY;
                chart2Zoom.maxY = maxDataY;
                SetExplicitRightAxisRange(chart2Zoom.minYRight,
                                          chart2Zoom.maxYRight,
                                          chart2Zoom.hasRightAxisCustomRange,
                                          minCornerHasDualAxis && maxCornerHasDualAxis,
                                          minDataYRight,
                                          maxDataYRight);
                chart2Zoom.isZoomed = true;
                chart2Zoom.isXCustom = true;
                chart2Zoom.hasXRangeLock = false;
                chart2Zoom.isYCustom = true;
            }
        }
    } else {
        // v2.14.2: Р­С‚Рѕ РєР»РёРє (РЅРµ drag) вЂ” РѕР±РЅРѕРІР»СЏРµРј РєРѕРѕСЂРґРёРЅР°С‚С‹ Рё РїРµСЂРµРєСЂРµСЃС‚РёРµ
        if (activeChart != 0) {
            double dataX, dataY1, dataY2;
            std::wstring y1Unit, y2Unit;
            bool hasDualAxis;
            
            if (ConvertScreenToChartCoords(activeChart, selectionStart.x, selectionStart.y, 
                                           dataX, dataY1, dataY2, y1Unit, y2Unit, hasDualAxis)) {
                uiModule->UpdateClickCoordinates(activeChart, dataX, dataY1, y1Unit, dataY2, y2Unit, hasDualAxis);
                
                // РЎРѕС…СЂР°РЅСЏРµРј РїРѕР·РёС†РёСЋ РїРµСЂРµРєСЂРµСЃС‚РёСЏ С‚РѕР»СЊРєРѕ РµСЃР»Рё screen->chart РєРѕРЅРІРµСЂС‚Р°С†РёСЏ СѓСЃРїРµС€РЅР°.
                CrosshairPosition* crosshair = (activeChart == 1) ? &chart1Crosshair : &chart2Crosshair;
                crosshair->screenX = selectionStart.x;
                crosshair->screenY = selectionStart.y;
                crosshair->dataX = dataX;
                crosshair->dataY = dataY1;
                crosshair->isVisible = true;
                crosshair->chartNumber = activeChart;
            } else {
                CrosshairPosition* crosshair = (activeChart == 1) ? &chart1Crosshair : &chart2Crosshair;
                crosshair->isVisible = false;
            }
        }
    }
    
    // РћР±РЅРѕРІР»СЏРµРј СЃРєСЂРѕР»Р»Р±Р°СЂС‹ РїРѕСЃР»Рµ РёР·РјРµРЅРµРЅРёСЏ Р·СѓРјР°
    UpdateScrollBars(hWnd);
    
    // РџРµСЂРµСЂРёСЃРѕРІС‹РІР°РµРј С‚РѕР»СЊРєРѕ РёР·РјРµРЅРµРЅРЅС‹Р№ РіСЂР°С„РёРє (РёР·Р±РµРіР°РµРј РјРёРіР°РЅРёСЏ)
    if (activeChart > 0) {
        InvalidateChart(hWnd, activeChart);
    } else {
        InvalidateRect(hWnd, NULL, FALSE); // Р•СЃР»Рё РЅРµ РѕРїСЂРµРґРµР»РµРЅ РіСЂР°С„РёРє, РїРµСЂРµСЂРёСЃРѕРІС‹РІР°РµРј РІСЃС‘ (FALSE РґР»СЏ double buffering)
    }
}

void ChartManager::HandleRightClick(HWND hWnd, int x, int y)
{
    const int chartNumber = GetChartAtPoint(x, y);
    if (chartNumber == 0) {
        return;
    }

    CrosshairPosition* crosshair = (chartNumber == 1) ? &chart1Crosshair : &chart2Crosshair;
    const bool hasCrosshair = crosshair && crosshair->isVisible && crosshair->chartNumber == chartNumber;

    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) {
        return;
    }

    AppendMenuW(hMenu, MF_STRING | (hasCrosshair ? MF_ENABLED : MF_GRAYED), IDM_CHART_SET_X_FROM, L"Установить как X начальное");
    AppendMenuW(hMenu, MF_STRING | (hasCrosshair ? MF_ENABLED : MF_GRAYED), IDM_CHART_SET_X_TO, L"Установить как X конечное");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING | (hasCrosshair ? MF_ENABLED : MF_GRAYED), IDM_CHART_ADD_NOTE, L"Создать заметку");

    POINT screenPoint = { x, y };
    ClientToScreen(hWnd, &screenPoint);

    UINT selected = TrackPopupMenu(
        hMenu,
        TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN | TPM_TOPALIGN,
        screenPoint.x,
        screenPoint.y,
        0,
        hWnd,
        NULL);

    DestroyMenu(hMenu);

    if (!hasCrosshair) {
        return;
    }

    if (selected == IDM_CHART_SET_X_FROM || selected == IDM_CHART_SET_X_TO) {
        double selectedX = crosshair->dataX;
        if (dataModule && dataModule->HasData()) {
            const double maxX = static_cast<double>(dataModule->GetDataSize() > 0 ? (dataModule->GetDataSize() - 1) : 0);
            if (selectedX < 0.0) selectedX = 0.0;
            if (selectedX > maxX) selectedX = maxX;
        }

        const double roundedX = static_cast<double>(llround(selectedX));
        if (selected == IDM_CHART_SET_X_FROM) {
            uiModule->SetXRangeFromValue(chartNumber, roundedX);
        } else {
            uiModule->SetXRangeToValue(chartNumber, roundedX);
        }
        uiModule->SetXRangeResetEnabled(chartNumber, true);
        return;
    }

    if (selected != IDM_CHART_ADD_NOTE) {
        return;
    }

    NoteCreateDialogContext ctx;
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
    DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_NOTE_DIALOG), hWnd, NoteCreateDialogProc, (LPARAM)&ctx);
    if (!ctx.ok) {
        return;
    }

    AddNoteAtCrosshair(hWnd, chartNumber, ctx.text);
}

int ChartManager::GetChartAtPoint(int x, int y) const {
    RECT chart1Rect = layoutModule->GetChart1Rect();
    RECT chart2Rect = layoutModule->GetChart2Rect();

    if (PtInRect(&chart1Rect, {x, y}))
        return 1;
    if (uiModule->IsDualMode() && PtInRect(&chart2Rect, {x, y}))
        return 2;
    return 0;
}

void ChartManager::ConvertScreenToDataCoords(int chartNum, int screenX, int screenY, double& dataX, double& dataY) const {
    const RECT* chartRect = nullptr;
    const ZoomRange* zoom = nullptr;

    if (chartNum == 1) {
        chartRect = &layoutModule->GetChart1Rect();
        zoom = &chart1Zoom;
    } else if (chartNum == 2) {
        chartRect = &layoutModule->GetChart2Rect();
        zoom = &chart2Zoom;
    }

    if (!chartRect || !zoom || (chartRect->right - chartRect->left == 0) || (chartRect->bottom - chartRect->top == 0)) {
        dataX = 0.0;
        dataY = 0.0;
        return;
    }

    // РџРѕР»СѓС‡Р°РµРј РєРѕРЅС„РёРіСѓСЂР°С†РёСЋ РіСЂР°С„РёРєР° РґР»СЏ РѕРїСЂРµРґРµР»РµРЅРёСЏ С‚РёРїР° РѕСЃРё X
    ChartConfiguration config = curveSelectionModule->GetChartConfiguration(chartNum);
    
    // Р’С‹С‡РёСЃР»СЏРµРј РѕР±Р»Р°СЃС‚СЊ РіСЂР°С„РёРєР° (plotRect) СЃ СѓС‡С‘С‚РѕРј РѕС‚СЃС‚СѓРїРѕРІ
    // Р’РђР–РќРћ: Р’СЃРµРіРґР° РёСЃРїРѕР»СЊР·СѓРµРј РѕС‚СЃС‚СѓРїС‹ РґР»СЏ РґРІРѕР№РЅС‹С… РѕСЃРµР№, РїРѕС‚РѕРјСѓ С‡С‚Рѕ DrawDualAxisChart
    // РІС‹Р·С‹РІР°РµС‚СЃСЏ РґР»СЏ РІСЃРµС… РіСЂР°С„РёРєРѕРІ (РґР°Р¶Рµ СЃ РѕРґРЅРѕР№ РѕСЃСЊСЋ) Рё РёСЃРїРѕР»СЊР·СѓРµС‚ CalculateDualAxisPlotRect
    DualAxisChartGeometry geometry = {};
    if (!TryBuildDualAxisChartGeometry(*chartRect, geometry)) {
        dataX = 0.0;
        dataY = 0.0;
        return;
    }

    const RECT& plotRect = geometry.plotRect;
    const RECT& dataRect = geometry.dataRect;
    double plotWidth = static_cast<double>(geometry.plotWidth);

    // Р’РђР–РќРћ: СЃРёРЅС…СЂРѕРЅРёР·РёСЂСѓРµРј Y-РіРµРѕРјРµС‚СЂРёСЋ СЃ СЂРµР°Р»СЊРЅРѕР№ РѕС‚СЂРёСЃРѕРІРєРѕР№ РІ DrawDualAxisDataWithZoom,
    // РіРґРµ РґР°РЅРЅС‹Рµ СЂРёСЃСѓСЋС‚СЃСЏ РЅРµ РїРѕ РІСЃРµРјСѓ plotRect, Р° РїРѕ dataRect СЃ РІРµСЂС‚РёРєР°Р»СЊРЅС‹РјРё margin.
    double dataHeight = static_cast<double>(geometry.dataHeight);
    if (!(dataHeight > 0.0)) {
        dataX = 0.0;
        dataY = 0.0;
        return;
    }

    double xRatio = static_cast<double>(screenX - plotRect.left) / plotWidth;
    if (xRatio < 0.0) xRatio = 0.0;
    if (xRatio > 1.0) xRatio = 1.0;

    // v2.10.0: Р”Р»СЏ РєСЂРѕСЃСЃРїР»РѕС‚Р° (С‚РµРјРїРµСЂР°С‚СѓСЂР° РЅР° РѕСЃРё X) РЅСѓР¶РЅРѕ РїСЂРµРѕР±СЂР°Р·РѕРІР°С‚СЊ РєРѕРѕСЂРґРёРЅР°С‚С‹ РїРѕ-РґСЂСѓРіРѕРјСѓ
    VisibleFrameAxisRange visibleXRange = {};
    BuildVisibleFrameAxisRange(zoom->isZoomed, zoom->minX, zoom->maxX, dataModule->GetDataSize(), visibleXRange);
    const double linearDataX = visibleXRange.minX + xRatio * (visibleXRange.maxX - visibleXRange.minX);
    if (config.xAxisType == XAxisType::TEMPERATURE) {
        // Р’ СЂРµР¶РёРјРµ РєСЂРѕСЃСЃРїР»РѕС‚Р° СЌРєСЂР°РЅРЅР°СЏ X РєРѕРѕСЂРґРёРЅР°С‚Р° СЃРѕРѕС‚РІРµС‚СЃС‚РІСѓРµС‚ С‚РµРјРїРµСЂР°С‚СѓСЂРµ
        // РќРѕ zoom РІСЃС‘ СЂР°РІРЅРѕ СЂР°Р±РѕС‚Р°РµС‚ СЃ РёРЅРґРµРєСЃР°РјРё РєР°РґСЂРѕРІ
        dataX = ResolveTemperatureAxisDataX(chartNum, *zoom, xRatio, linearDataX, !ShouldUseAsyncCurveWarmup());
    } else {
        // РћР±С‹С‡РЅС‹Р№ СЂРµР¶РёРј: РЅРѕРјРµСЂ РєР°РґСЂР° РЅР° РѕСЃРё X
        dataX = linearDataX;
    }
    
    // Y РєРѕРѕСЂРґРёРЅР°С‚Р° РѕР±СЂР°Р±Р°С‚С‹РІР°РµС‚СЃСЏ РѕРґРёРЅР°РєРѕРІРѕ РґР»СЏ РѕР±РѕРёС… СЂРµР¶РёРјРѕРІ
    // РџРѕР»СѓС‡Р°РµРј СЂРµР°Р»СЊРЅС‹Р№ РґРёР°РїР°Р·РѕРЅ Y РёР· РґР°РЅРЅС‹С…
    ChartConfiguration config2 = curveSelectionModule->GetChartConfiguration(chartNum);
    const CachedCurveSnapshot* curveSnapshot = nullptr;
    ZoomRange fullRangeSnapshot = {};
    if (!TryGetCurveSnapshot(chartNum, config2, fullRangeSnapshot, !ShouldUseAsyncCurveWarmup(), curveSnapshot)) {
        dataY = 0.0;
        return;
    }

    double displayMinY = 0.0;
    double displayMaxY = 0.0;
    BuildDisplayedLeftAxisRange(zoom->isYCustom, zoom->minY, zoom->maxY, curveSnapshot->minY1, curveSnapshot->maxY1, displayMinY, displayMaxY);

    // Р’ DrawDualAxisDataWithZoom РґРѕРїРѕР»РЅРёС‚РµР»СЊРЅРѕ РїСЂРёРјРµРЅСЏРµС‚СЃСЏ РЅРёР¶РЅРёР№ padding 2%.
    double drawMinY = 0.0;
    double drawMaxY = 0.0;
    if (!TryBuildDrawnAxisRange(displayMinY, displayMaxY, drawMinY, drawMaxY)) {
        dataY = displayMinY;
        return;
    }
    double drawRangeY = drawMaxY - drawMinY;
    if (!(drawRangeY > 0.0)) {
        dataY = drawMinY;
        return;
    }

    double clampedScreenY = static_cast<double>((std::max)(dataRect.top, (std::min)(dataRect.bottom, static_cast<LONG>(screenY))));
    double yRatio = (static_cast<double>(dataRect.bottom) - clampedScreenY) / dataHeight;
    dataY = drawMinY + yRatio * drawRangeY;
}


// РњРµС‚РѕРґС‹ РґР»СЏ РѕР±СЂР°Р±РѕС‚РєРё СЃРєСЂРѕР»Р»РёРЅРіР°
void ChartManager::HandleScrollEvent(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    HWND hScrollBar = (HWND)lParam;
    int scrollCode = LOWORD(wParam);
    
    // РћРїСЂРµРґРµР»СЏРµРј, РєР°РєРѕР№ СЃРєСЂРѕР»Р»Р±Р°СЂ Р±С‹Р» РёСЃРїРѕР»СЊР·РѕРІР°РЅ
    int chartNumber = 0;
    bool isVertical = false;
    
    if (hScrollBar == uiModule->GetHScrollBar(1)) {
        chartNumber = 1;
        isVertical = false;
    } else if (hScrollBar == uiModule->GetHScrollBar(2)) {
        chartNumber = 2;
        isVertical = false;
    } else if (hScrollBar == uiModule->GetVScrollBar(1)) {
        chartNumber = 1;
        isVertical = true;
    } else if (hScrollBar == uiModule->GetVScrollBar(2)) {
        chartNumber = 2;
        isVertical = true;
    } else {
        return; // РќРµРёР·РІРµСЃС‚РЅС‹Р№ СЃРєСЂРѕР»Р»Р±Р°СЂ
    }

    // Surf ON: X-СЃРєСЂРѕР»Р» РѕС‚РєР»СЋС‡С‘РЅ (X РІСЃРµРіРґР° Р°РІС‚Рѕ-СЃР»РµРґРёС‚ Р·Р° РїРѕСЃР»РµРґРЅРёРјРё N С‚РѕС‡РєР°РјРё).
    if (!isVertical && uiModule->IsSurfEnabled(chartNumber)) {
        return;
    }

    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
    std::vector<ZoomRange>* zoomHistory = (chartNumber == 1) ? &chart1ZoomHistory : &chart2ZoomHistory;

    const double fullMinX = 0.0;
    const double fullMaxX = static_cast<double>(dataModule->GetDataSize() - 1);
    double xLockMin = fullMinX;
    double xLockMax = fullMaxX;

    // Р•СЃР»Рё РїРѕР»СЊР·РѕРІР°С‚РµР»СЊ Р·Р°РґР°Р» X Range С‡РµСЂРµР· РїРѕР»СЏ, РЅРµ РїРѕР·РІРѕР»СЏРµРј X-scroll РІС‹С…РѕРґРёС‚СЊ Р·Р° СЌС‚Рё РіСЂР°РЅРёС†С‹.
    if (zoom->hasXRangeLock) {
        xLockMin = zoom->minX;
        xLockMax = zoom->maxX;
    } else {
        for (auto it = zoomHistory->rbegin(); it != zoomHistory->rend(); ++it) {
            if (it->hasXRangeLock) {
                xLockMin = it->minX;
                xLockMax = it->maxX;
                break;
            }
        }
    }

    if (xLockMin < fullMinX) xLockMin = fullMinX;
    if (xLockMax > fullMaxX) xLockMax = fullMaxX;
    if (xLockMax < xLockMin) {
        xLockMin = fullMinX;
        xLockMax = fullMaxX;
    }

    // РћСЃРµРІРѕР№ undo: РєРЅРѕРїРєР° "-" РЅР° СЃРєСЂРѕР»Р»Р±Р°СЂРµ РѕС‚РєР°С‚С‹РІР°РµС‚ zoom С‚РѕР»СЊРєРѕ РїРѕ СЃРѕРѕС‚РІРµС‚СЃС‚РІСѓСЋС‰РµР№ РѕСЃРё.
    // Р”Р»СЏ H-СЃРєСЂРѕР»Р»Р° СЌС‚Рѕ Р»РµРІР°СЏ СЃС‚СЂРµР»РєР°, РґР»СЏ V-СЃРєСЂРѕР»Р»Р° вЂ” РІРµСЂС…РЅСЏСЏ СЃС‚СЂРµР»РєР°.
    if ((!isVertical && scrollCode == SB_LINELEFT) || (isVertical && scrollCode == SB_LINEUP)) {
        AxisZoomRange prevAxis;

        if (isVertical) {
            if (PopAxisHistoryY(chartNumber, prevAxis)) {
                if (prevAxis.isCustom) {
                    zoom->minY = prevAxis.min;
                    zoom->maxY = prevAxis.max;
                    zoom->isYCustom = true;
                    zoom->minYRight = prevAxis.minRight;
                    zoom->maxYRight = prevAxis.maxRight;
                    zoom->hasRightAxisCustomRange = prevAxis.hasRightAxisRange;
                } else {
                    zoom->isYCustom = false;
                    ClearExplicitRightAxisRange(zoom->minYRight, zoom->maxYRight, zoom->hasRightAxisCustomRange);
                    if (uiModule->IsSurfEnabled(chartNumber)) {
                        UpdateSurfRange(chartNumber);
                    } else {
                        AutoFitYForCurrentX(chartNumber);
                    }
                }
            } else {
                zoom->isYCustom = false;
                ClearExplicitRightAxisRange(zoom->minYRight, zoom->maxYRight, zoom->hasRightAxisCustomRange);
                if (uiModule->IsSurfEnabled(chartNumber)) {
                    UpdateSurfRange(chartNumber);
                } else {
                    AutoFitYForCurrentX(chartNumber);
                }
            }
        } else {
            if (PopAxisHistoryX(chartNumber, prevAxis)) {
                if (prevAxis.isCustom) {
                    zoom->minX = prevAxis.min;
                    zoom->maxX = prevAxis.max;
                    zoom->isXCustom = true;
                    zoom->hasXRangeLock = prevAxis.hasXRangeLock;
                } else if (dataModule && dataModule->HasData()) {
                    // Р”Р»СЏ non-custom СЃРѕСЃС‚РѕСЏРЅРёСЏ РѕС‚РєР°С‚С‹РІР°РµРјСЃСЏ Рє СЂР°Р·СЂРµС€РµРЅРЅРѕРјСѓ X-РґРёР°РїР°Р·РѕРЅСѓ
                    // (РїРѕР»РЅС‹Р№ РґРёР°РїР°Р·РѕРЅ РёР»Рё РїРѕР»СЊР·РѕРІР°С‚РµР»СЊСЃРєРёР№ X Range, РµСЃР»Рё РѕРЅ Р±С‹Р» Р·Р°РґР°РЅ).
                    zoom->minX = xLockMin;
                    zoom->maxX = xLockMax;
                    zoom->isXCustom = false;
                    zoom->hasXRangeLock = false;
                    zoom->isXToLatest = false;
                }
            } else if (dataModule && dataModule->HasData()) {
                zoom->minX = xLockMin;
                zoom->maxX = xLockMax;
                zoom->isXCustom = false;
                zoom->hasXRangeLock = false;
                zoom->isXToLatest = false;
            }

            if (!zoom->isYCustom) {
                AutoFitYForCurrentX(chartNumber);
            }
        }

        UpdateZoomActiveState(chartNumber);
        UpdateScrollBars(hWnd);
        UpdateXRangeUI(hWnd);
        InvalidateChart(hWnd, chartNumber);
        return;
    }
    
    // РџРѕР»СѓС‡Р°РµРј С‚РµРєСѓС‰СѓСЋ РёРЅС„РѕСЂРјР°С†РёСЋ Рѕ СЃРєСЂРѕР»Р»Р±Р°СЂРµ
    SCROLLINFO si = {};
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_ALL;
    GetScrollInfo(hScrollBar, SB_CTL, &si);
    
    int newPos = si.nPos;
    
    // РћР±СЂР°Р±Р°С‚С‹РІР°РµРј СЂР°Р·Р»РёС‡РЅС‹Рµ РєРѕРјР°РЅРґС‹ СЃРєСЂРѕР»Р»РёРЅРіР°
    switch (scrollCode) {
        case SB_LINEUP:
            newPos = (std::max)(si.nMin, si.nPos - 1);
            break;
        case SB_LINEDOWN:
            newPos = (std::min)(si.nMax - (int)si.nPage + 1, si.nPos + 1);
            break;
        case SB_PAGEUP:
            newPos = (std::max)(si.nMin, si.nPos - (int)si.nPage);
            break;
        case SB_PAGEDOWN:
            newPos = (std::min)(si.nMax - (int)si.nPage + 1, si.nPos + (int)si.nPage);
            break;
        case SB_THUMBTRACK:
        case SB_THUMBPOSITION:
            newPos = si.nTrackPos;
            break;
        default:
            return;
    }
    
    // РћР±РЅРѕРІР»СЏРµРј РїРѕР·РёС†РёСЋ СЃРєСЂРѕР»Р»Р±Р°СЂР°
    si.nPos = newPos;
    SetScrollInfo(hScrollBar, SB_CTL, &si, TRUE);
    
    // РћР±РЅРѕРІР»СЏРµРј РґРёР°РїР°Р·РѕРЅ Р·СѓРјР° РЅР° РѕСЃРЅРѕРІРµ РЅРѕРІРѕР№ РїРѕР·РёС†РёРё СЃРєСЂРѕР»Р»Р±Р°СЂР°
    if (isVertical) {
        // Р’РµСЂС‚РёРєР°Р»СЊРЅС‹Р№ СЃРєСЂРѕР»Р»РёРЅРі (РѕСЃСЊ Y)
        if (!zoom->isZoomed) {
            // Р•СЃР»Рё РіСЂР°С„РёРє РЅРµ РІ СЂРµР¶РёРјРµ zoom, РІРµСЂС‚РёРєР°Р»СЊРЅС‹Р№ СЃРєСЂРѕР»Р»РёРЅРі РЅРµ СЂР°Р±РѕС‚Р°РµС‚
            return;
        }

        // РџРѕР»СЊР·РѕРІР°С‚РµР»СЊ РІР·Р°РёРјРѕРґРµР№СЃС‚РІСѓРµС‚ СЃ Y: С„РёРєСЃРёСЂСѓРµРј, С‡С‚Рѕ Y-zoom РїРѕР»СЊР·РѕРІР°С‚РµР»СЊСЃРєРёР№ (С‡С‚РѕР±С‹ Surf РЅРµ РїРµСЂРµР·Р°С‚РёСЂР°Р»)
        zoom->isYCustom = true;
        
        // РџРѕР»СѓС‡Р°РµРј Y РґРёР°РїР°Р·РѕРЅ РґР»СЏ РўР•РљРЈР©Р•Р“Рћ X СѓС‡Р°СЃС‚РєР° (РЅРµ РІСЃРµС… РґР°РЅРЅС‹С…)
        // Р­С‚Рѕ РїРѕР·РІРѕР»СЏРµС‚ СЃРєСЂРѕР»Р»РёС‚СЊ РІ РїСЂРµРґРµР»Р°С… РґР°РЅРЅС‹С… С‚РµРєСѓС‰РµРіРѕ РІРёРґРёРјРѕРіРѕ X РґРёР°РїР°Р·РѕРЅР°
        ChartConfiguration config = curveSelectionModule->GetChartConfiguration(chartNumber);
        std::vector<std::vector<float>> leftAxisData, rightAxisData;
        std::vector<CurveDefinition> leftAxisCurves, rightAxisCurves;
        
        // РџРѕР»СѓС‡Р°РµРј РґР°РЅРЅС‹Рµ РґР»СЏ С‚РµРєСѓС‰РµРіРѕ X РґРёР°РїР°Р·РѕРЅР°
        curveSelectionModule->CalculateCurveData(config, dataModule.get(), leftAxisData, rightAxisData,
                                                 leftAxisCurves, rightAxisCurves,
                                                 zoom->minX, zoom->maxX, -1e10, 1e10);
        
        double minYFull, maxYFull, minX, maxX, minY2, maxY2;
        chartModule->GetDataRanges(leftAxisData, rightAxisData, minX, maxX, minYFull, maxYFull, minY2, maxY2);
        
        // Р”РѕР±Р°РІР»СЏРµРј padding 5% Рє РґРёР°РїР°Р·РѕРЅСѓ РґР°РЅРЅС‹С… РґР»СЏ РІРёР·СѓР°Р»СЊРЅРѕРіРѕ РєРѕРјС„РѕСЂС‚Р°
        double dataYRange = maxYFull - minYFull;
        double padding = dataYRange * 0.05;
        minYFull -= padding;
        maxYFull += padding;
        
        double totalYRange = maxYFull - minYFull;
        double viewYRange = zoom->maxY - zoom->minY;
        
        // Р’РµСЂС‚РёРєР°Р»СЊРЅС‹Р№ СЃРєСЂРѕР»Р»Р±Р°СЂ РёРЅРІРµСЂС‚РёСЂРѕРІР°РЅ (РІРµСЂС… = 0, РЅРёР· = 100)
        // newPos = 0 РѕР·РЅР°С‡Р°РµС‚ РІРµСЂС… (РјР°РєСЃРёРјР°Р»СЊРЅС‹Р№ Y)
        // newPos = maxScrollPos РѕР·РЅР°С‡Р°РµС‚ РЅРёР· (РјРёРЅРёРјР°Р»СЊРЅС‹Р№ Y)
        // РЈС‡РёС‚С‹РІР°РµРј, С‡С‚Рѕ РјР°РєСЃРёРјР°Р»СЊРЅР°СЏ РїРѕР·РёС†РёСЏ СЃРєСЂРѕР»Р»Р±Р°СЂР° = nMax - nPage + 1
        if (totalYRange > viewYRange && totalYRange > 0) {
            int maxScrollPos = si.nMax - (int)si.nPage + 1;
            double scrollRatio = (maxScrollPos > 0) ? (double)(maxScrollPos - newPos) / (double)maxScrollPos : 0.0;
            double newMinY = minYFull + scrollRatio * (totalYRange - viewYRange);
            zoom->minY = newMinY;
            zoom->maxY = newMinY + viewYRange;

            if (zoom->hasRightAxisCustomRange && maxY2 > minY2) {
                double minYFullRight = minY2;
                double maxYFullRight = maxY2;
                double dataYRangeRight = maxYFullRight - minYFullRight;
                double paddingRight = dataYRangeRight * 0.05;
                minYFullRight -= paddingRight;
                maxYFullRight += paddingRight;

                double totalYRangeRight = maxYFullRight - minYFullRight;
                double viewYRangeRight = zoom->maxYRight - zoom->minYRight;
                if (totalYRangeRight > viewYRangeRight && totalYRangeRight > 0.0) {
                    double newMinYRight = minYFullRight + scrollRatio * (totalYRangeRight - viewYRangeRight);
                    zoom->minYRight = newMinYRight;
                    zoom->maxYRight = newMinYRight + viewYRangeRight;
                }
            }
        }
    } else {
        // Р“РѕСЂРёР·РѕРЅС‚Р°Р»СЊРЅС‹Р№ СЃРєСЂРѕР»Р»РёРЅРі (РѕСЃСЊ X)
        double totalRange = xLockMax - xLockMin;
        double viewRange = zoom->maxX - zoom->minX;
        if (viewRange > totalRange) {
            viewRange = totalRange;
        }
        
        // РЈС‡РёС‚С‹РІР°РµРј, С‡С‚Рѕ РјР°РєСЃРёРјР°Р»СЊРЅР°СЏ РїРѕР·РёС†РёСЏ СЃРєСЂРѕР»Р»Р±Р°СЂР° = nMax - nPage + 1
        // nMax = 100, nPage = (viewRange / totalRange) * 100
        // РџРѕСЌС‚РѕРјСѓ СЂРµР°Р»СЊРЅС‹Р№ РјР°РєСЃРёРјСѓРј = 100 - page + 1
        int maxScrollPos = si.nMax - (int)si.nPage + 1;
        double scrollRatio = (maxScrollPos > 0) ? (double)newPos / (double)maxScrollPos : 0.0;
        double newMinX = xLockMin + scrollRatio * (totalRange - viewRange);
        zoom->minX = newMinX;
        zoom->maxX = newMinX + viewRange;

        if (zoom->minX < xLockMin) {
            zoom->minX = xLockMin;
            zoom->maxX = xLockMin + viewRange;
        }
        if (zoom->maxX > xLockMax) {
            zoom->maxX = xLockMax;
            zoom->minX = xLockMax - viewRange;
        }

        zoom->isXCustom = true;
        zoom->hasXRangeLock = false;
        const bool wasYCustom = zoom->isYCustom;
        const double prevMinY = zoom->minY;
        const double prevMaxY = zoom->maxY;
        const double prevViewRangeY = prevMaxY - prevMinY;
        AutoFitYForCurrentX(chartNumber);
        if (wasYCustom) {
            // РЎРѕС…СЂР°РЅСЏРµРј РЅРµР·Р°РІРёСЃРёРјРѕСЃС‚СЊ РѕСЃРµР№: РѕСЃС‚Р°РІР»СЏРµРј РїРѕР»СЊР·РѕРІР°С‚РµР»СЊСЃРєСѓСЋ РІС‹СЃРѕС‚Сѓ Y-РѕРєРЅР°,
            // РЅРѕ Р·Р°Р¶РёРјР°РµРј РµС‘ РІ Р°РєС‚СѓР°Р»СЊРЅС‹Р№ Y-РґРёР°РїР°Р·РѕРЅ РґР»СЏ РЅРѕРІРѕРіРѕ X-СѓС‡Р°СЃС‚РєР°.
            const double autoMinY = zoom->minY;
            const double autoMaxY = zoom->maxY;
            const double autoRangeY = autoMaxY - autoMinY;

            if (prevViewRangeY > 0.0 && autoRangeY > prevViewRangeY) {
                double candidateMinY = prevMinY;
                if (candidateMinY < autoMinY) {
                    candidateMinY = autoMinY;
                }
                const double maxAllowedMinY = autoMaxY - prevViewRangeY;
                if (candidateMinY > maxAllowedMinY) {
                    candidateMinY = maxAllowedMinY;
                }

                zoom->minY = candidateMinY;
                zoom->maxY = candidateMinY + prevViewRangeY;
            }

            zoom->isYCustom = true;
        }
    }

    UpdateZoomActiveState(chartNumber);
    UpdateScrollBars(hWnd);
    UpdateXRangeUI(hWnd);
    
    // РџРµСЂРµСЂРёСЃРѕРІС‹РІР°РµРј С‚РѕР»СЊРєРѕ РЅСѓР¶РЅС‹Р№ РіСЂР°С„РёРє (РёР·Р±РµРіР°РµРј РјРёРіР°РЅРёСЏ)
    InvalidateChart(hWnd, chartNumber);
}

void ChartManager::UpdateScrollBars(HWND hWnd) {
    uiModule->SetUndoZoomStepEnabled(1, !chart1ZoomHistory.empty());
    uiModule->SetUndoZoomStepEnabled(2, uiModule->IsDualMode() && !chart2ZoomHistory.empty());

    // РћР±РЅРѕРІР»СЏРµРј СЃРєСЂРѕР»Р»Р±Р°СЂС‹ РґР»СЏ РїРµСЂРІРѕРіРѕ РіСЂР°С„РёРєР°
    if (chart1Zoom.isZoomed) {
        uiModule->ShowScrollBars(1, true);
        uiModule->UpdateScrollBarPositions(1, layoutModule->GetChart1Rect());

        const bool showHScroll1 = !uiModule->IsSurfEnabled(1) && chart1Zoom.isXCustom && !chart1Zoom.hasXRangeLock;
        const bool showVScroll1 = chart1Zoom.isYCustom;
        if (HWND hH = uiModule->GetHScrollBar(1)) {
            ShowWindow(hH, showHScroll1 ? SW_SHOW : SW_HIDE);
        }
        if (HWND hV = uiModule->GetVScrollBar(1)) {
            ShowWindow(hV, showVScroll1 ? SW_SHOW : SW_HIDE);
        }
        uiModule->ShowAxisUndoButtons(1, showHScroll1, showVScroll1);

        // Surf ON: РїСЂСЏС‡РµРј РіРѕСЂРёР·РѕРЅС‚Р°Р»СЊРЅС‹Р№ СЃРєСЂРѕР»Р»Р±Р°СЂ Рё РЅРµ РЅР°СЃС‚СЂР°РёРІР°РµРј X-СЃРєСЂРѕР»Р»
        if (uiModule->IsSurfEnabled(1)) {
            if (HWND hH = uiModule->GetHScrollBar(1)) {
                ShowWindow(hH, SW_HIDE);
            }
        } else {
            std::vector<ZoomRange>* zoomHistory = &chart1ZoomHistory;
            const double fullMinX = 0.0;
            const double fullMaxX = static_cast<double>(dataModule->GetDataSize() - 1);
            double xLockMin = fullMinX;
            double xLockMax = fullMaxX;

            if (chart1Zoom.hasXRangeLock) {
                xLockMin = chart1Zoom.minX;
                xLockMax = chart1Zoom.maxX;
            } else {
                for (auto it = zoomHistory->rbegin(); it != zoomHistory->rend(); ++it) {
                    if (it->hasXRangeLock) {
                        xLockMin = it->minX;
                        xLockMax = it->maxX;
                        break;
                    }
                }
            }

            if (xLockMin < fullMinX) xLockMin = fullMinX;
            if (xLockMax > fullMaxX) xLockMax = fullMaxX;
            if (xLockMax < xLockMin) {
                xLockMin = fullMinX;
                xLockMax = fullMaxX;
            }

            // РќР°СЃС‚СЂР°РёРІР°РµРј РґРёР°РїР°Р·РѕРЅ РіРѕСЂРёР·РѕРЅС‚Р°Р»СЊРЅРѕРіРѕ СЃРєСЂРѕР»Р»Р±Р°СЂР° (РѕСЃСЊ X)
            double totalDataRange = xLockMax - xLockMin;
            double viewRangeX = chart1Zoom.maxX - chart1Zoom.minX;
            if (viewRangeX > totalDataRange) {
                viewRangeX = totalDataRange;
            }
            
            // Р’С‹С‡РёСЃР»СЏРµРј page size Рё РјР°РєСЃРёРјР°Р»СЊРЅСѓСЋ РїРѕР·РёС†РёСЋ СЃРєСЂРѕР»Р»Р±Р°СЂР°
            int pageSize = (totalDataRange > 0.0)
                ? static_cast<int>((viewRangeX / totalDataRange) * 100)
                : 100;
            if (pageSize < 1) pageSize = 1;
            if (pageSize > 100) pageSize = 100;
            int maxScrollPos = 100 - pageSize + 1;
            
            // Р’С‹С‡РёСЃР»СЏРµРј РїРѕР·РёС†РёСЋ СЃРєСЂРѕР»Р»Р±Р°СЂР° РїСЂРѕРїРѕСЂС†РёРѕРЅР°Р»СЊРЅРѕ
            double scrollableRange = totalDataRange - viewRangeX;
            int hScrollPos = (scrollableRange > 0)
                ? static_cast<int>(((chart1Zoom.minX - xLockMin) / scrollableRange) * maxScrollPos)
                : 0;
            uiModule->SetScrollBarRange(1, 0, 100, pageSize, hScrollPos);
        }
        
        // РќР°СЃС‚СЂР°РёРІР°РµРј РґРёР°РїР°Р·РѕРЅ РІРµСЂС‚РёРєР°Р»СЊРЅРѕРіРѕ СЃРєСЂРѕР»Р»Р±Р°СЂР° (РѕСЃСЊ Y)
        if (showVScroll1) {
        ChartConfiguration config = curveSelectionModule->GetChartConfiguration(1);
        const CachedCurveSnapshot* curveSnapshot = nullptr;
        if (!TryGetCurveSnapshot(1, config, chart1Zoom, !ShouldUseAsyncCurveWarmup(), curveSnapshot)) {
            ScheduleAsyncCurveSnapshotWarmup(hWnd, 1, config, chart1Zoom);
            uiModule->SetVScrollBarRange(1, 0, 100, 100, 50);
        } else {
            double minYFull = curveSnapshot->minY1;
            double maxYFull = curveSnapshot->maxY1;
            
            // Р”РѕР±Р°РІР»СЏРµРј padding 5% Рє РґРёР°РїР°Р·РѕРЅСѓ РґР°РЅРЅС‹С… РґР»СЏ СЃРѕРіР»Р°СЃРѕРІР°РЅРЅРѕСЃС‚Рё СЃ HandleScrollEvent
            double dataYRange = maxYFull - minYFull;
            double padding = dataYRange * 0.05;
            minYFull -= padding;
            maxYFull += padding;
            
            double totalYRange = maxYFull - minYFull;
            double viewRangeY = chart1Zoom.maxY - chart1Zoom.minY;
            
            // Р’РµСЂС‚РёРєР°Р»СЊРЅС‹Р№ СЃРєСЂРѕР»Р»Р±Р°СЂ РёРЅРІРµСЂС‚РёСЂРѕРІР°РЅ (РІРµСЂС… = 0, РЅРёР· = 100)
            // Р—Р°С‰РёС‚Р° РѕС‚ РґРµР»РµРЅРёСЏ РЅР° РЅРѕР»СЊ
            int vScrollPos = 50;
            int vScrollPage = 100;
            if (totalYRange > viewRangeY && totalYRange > 0) {
                vScrollPage = static_cast<int>((viewRangeY / totalYRange) * 100);
                int maxScrollPos = 100 - vScrollPage + 1;
                
                // Р’С‹С‡РёСЃР»СЏРµРј РїРѕР·РёС†РёСЋ: 0 = РІРµСЂС… (maxY), maxScrollPos = РЅРёР· (minY)
                double scrollableRange = totalYRange - viewRangeY;
                double scrollRatio = (chart1Zoom.minY - minYFull) / scrollableRange;
                vScrollPos = static_cast<int>((1.0 - scrollRatio) * maxScrollPos);
            }
            uiModule->SetVScrollBarRange(1, 0, 100, vScrollPage, vScrollPos);
        }
        }
    } else {
        uiModule->ShowScrollBars(1, false);
    }
    
    // РћР±РЅРѕРІР»СЏРµРј СЃРєСЂРѕР»Р»Р±Р°СЂС‹ РґР»СЏ РІС‚РѕСЂРѕРіРѕ РіСЂР°С„РёРєР° (РµСЃР»Рё РІ СЂРµР¶РёРјРµ РґРІСѓС… РіСЂР°С„РёРєРѕРІ)
    if (uiModule->IsDualMode()) {
        if (chart2Zoom.isZoomed) {
            uiModule->ShowScrollBars(2, true);
            uiModule->UpdateScrollBarPositions(2, layoutModule->GetChart2Rect());

            const bool showHScroll2 = !uiModule->IsSurfEnabled(2) && chart2Zoom.isXCustom && !chart2Zoom.hasXRangeLock;
            const bool showVScroll2 = chart2Zoom.isYCustom;
            if (HWND hH = uiModule->GetHScrollBar(2)) {
                ShowWindow(hH, showHScroll2 ? SW_SHOW : SW_HIDE);
            }
            if (HWND hV = uiModule->GetVScrollBar(2)) {
                ShowWindow(hV, showVScroll2 ? SW_SHOW : SW_HIDE);
            }
            uiModule->ShowAxisUndoButtons(2, showHScroll2, showVScroll2);

            // Surf ON: РїСЂСЏС‡РµРј РіРѕСЂРёР·РѕРЅС‚Р°Р»СЊРЅС‹Р№ СЃРєСЂРѕР»Р»Р±Р°СЂ Рё РЅРµ РЅР°СЃС‚СЂР°РёРІР°РµРј X-СЃРєСЂРѕР»Р»
            if (uiModule->IsSurfEnabled(2)) {
                if (HWND hH = uiModule->GetHScrollBar(2)) {
                    ShowWindow(hH, SW_HIDE);
                }
            } else {
                std::vector<ZoomRange>* zoomHistory = &chart2ZoomHistory;
                const double fullMinX = 0.0;
                const double fullMaxX = static_cast<double>(dataModule->GetDataSize() - 1);
                double xLockMin = fullMinX;
                double xLockMax = fullMaxX;

                if (chart2Zoom.hasXRangeLock) {
                    xLockMin = chart2Zoom.minX;
                    xLockMax = chart2Zoom.maxX;
                } else {
                    for (auto it = zoomHistory->rbegin(); it != zoomHistory->rend(); ++it) {
                        if (it->hasXRangeLock) {
                            xLockMin = it->minX;
                            xLockMax = it->maxX;
                            break;
                        }
                    }
                }

                if (xLockMin < fullMinX) xLockMin = fullMinX;
                if (xLockMax > fullMaxX) xLockMax = fullMaxX;
                if (xLockMax < xLockMin) {
                    xLockMin = fullMinX;
                    xLockMax = fullMaxX;
                }

                // РќР°СЃС‚СЂР°РёРІР°РµРј РґРёР°РїР°Р·РѕРЅ РіРѕСЂРёР·РѕРЅС‚Р°Р»СЊРЅРѕРіРѕ СЃРєСЂРѕР»Р»Р±Р°СЂР° (РѕСЃСЊ X)
                double totalDataRange = xLockMax - xLockMin;
                double viewRangeX = chart2Zoom.maxX - chart2Zoom.minX;
                if (viewRangeX > totalDataRange) {
                    viewRangeX = totalDataRange;
                }
                
                // Р’С‹С‡РёСЃР»СЏРµРј page size Рё РјР°РєСЃРёРјР°Р»СЊРЅСѓСЋ РїРѕР·РёС†РёСЋ СЃРєСЂРѕР»Р»Р±Р°СЂР°
                int pageSize = (totalDataRange > 0.0)
                    ? static_cast<int>((viewRangeX / totalDataRange) * 100)
                    : 100;
                if (pageSize < 1) pageSize = 1;
                if (pageSize > 100) pageSize = 100;
                int maxScrollPos = 100 - pageSize + 1;
                
                // Р’С‹С‡РёСЃР»СЏРµРј РїРѕР·РёС†РёСЋ СЃРєСЂРѕР»Р»Р±Р°СЂР° РїСЂРѕРїРѕСЂС†РёРѕРЅР°Р»СЊРЅРѕ
                double scrollableRange = totalDataRange - viewRangeX;
                int hScrollPos = (scrollableRange > 0)
                    ? static_cast<int>(((chart2Zoom.minX - xLockMin) / scrollableRange) * maxScrollPos)
                    : 0;
                uiModule->SetScrollBarRange(2, 0, 100, pageSize, hScrollPos);
            }
            
            // РќР°СЃС‚СЂР°РёРІР°РµРј РґРёР°РїР°Р·РѕРЅ РІРµСЂС‚РёРєР°Р»СЊРЅРѕРіРѕ СЃРєСЂРѕР»Р»Р±Р°СЂР° (РѕСЃСЊ Y)
            if (showVScroll2) {
            ChartConfiguration config = curveSelectionModule->GetChartConfiguration(2);
            const CachedCurveSnapshot* curveSnapshot = nullptr;
            if (!TryGetCurveSnapshot(2, config, chart2Zoom, !ShouldUseAsyncCurveWarmup(), curveSnapshot)) {
                ScheduleAsyncCurveSnapshotWarmup(hWnd, 2, config, chart2Zoom);
                uiModule->SetVScrollBarRange(2, 0, 100, 100, 50);
            } else {
                double minYFull = curveSnapshot->minY1;
                double maxYFull = curveSnapshot->maxY1;
                
                // Р”РѕР±Р°РІР»СЏРµРј padding 5% Рє РґРёР°РїР°Р·РѕРЅСѓ РґР°РЅРЅС‹С… РґР»СЏ СЃРѕРіР»Р°СЃРѕРІР°РЅРЅРѕСЃС‚Рё СЃ HandleScrollEvent
                double dataYRange = maxYFull - minYFull;
                double padding = dataYRange * 0.05;
                minYFull -= padding;
                maxYFull += padding;
                
                double totalYRange = maxYFull - minYFull;
                double viewRangeY = chart2Zoom.maxY - chart2Zoom.minY;
                
                // Р’РµСЂС‚РёРєР°Р»СЊРЅС‹Р№ СЃРєСЂРѕР»Р»Р±Р°СЂ РёРЅРІРµСЂС‚РёСЂРѕРІР°РЅ (РІРµСЂС… = 0, РЅРёР· = 100)
                // Р—Р°С‰РёС‚Р° РѕС‚ РґРµР»РµРЅРёСЏ РЅР° РЅРѕР»СЊ
                int vScrollPos = 50;
                int vScrollPage = 100;
                if (totalYRange > viewRangeY && totalYRange > 0) {
                    vScrollPage = static_cast<int>((viewRangeY / totalYRange) * 100);
                    int maxScrollPos = 100 - vScrollPage + 1;
                    
                    // Р’С‹С‡РёСЃР»СЏРµРј РїРѕР·РёС†РёСЋ: 0 = РІРµСЂС… (maxY), maxScrollPos = РЅРёР· (minY)
                    double scrollableRange = totalYRange - viewRangeY;
                    double scrollRatio = (chart2Zoom.minY - minYFull) / scrollableRange;
                    vScrollPos = static_cast<int>((1.0 - scrollRatio) * maxScrollPos);
                }
                uiModule->SetVScrollBarRange(2, 0, 100, vScrollPage, vScrollPos);
            }
            }
        } else {
            uiModule->ShowScrollBars(2, false);
            uiModule->ShowAxisUndoButtons(2, false, false);
        }
    } else {
        uiModule->ShowScrollBars(2, false);
        uiModule->ShowAxisUndoButtons(2, false, false);
    }
}

// РњРµС‚РѕРґ РґР»СЏ С‡Р°СЃС‚РёС‡РЅРѕР№ РїРµСЂРµСЂРёСЃРѕРІРєРё РєРѕРЅРєСЂРµС‚РЅРѕРіРѕ РіСЂР°С„РёРєР°
void ChartManager::InvalidateChart(HWND hWnd, int chartNumber) {
    RECT chartRect;
    
    if (chartNumber == 1) {
        chartRect = layoutModule->GetChart1Rect();
    } else if (chartNumber == 2) {
        chartRect = layoutModule->GetChart2Rect();
    } else {
        return; // РќРµРІРµСЂРЅС‹Р№ РЅРѕРјРµСЂ РіСЂР°С„РёРєР°
    }

    if (IsUsingNewCurveSystem(chartNumber) && curveSelectionModule) {
        ScheduleAsyncCurveSnapshotWarmup(
            hWnd,
            chartNumber,
            curveSelectionModule->GetChartConfiguration(chartNumber),
            (chartNumber == 1) ? chart1Zoom : chart2Zoom);
    }
    
    // РџРµСЂРµСЂРёСЃРѕРІС‹РІР°РµРј С‚РѕР»СЊРєРѕ РѕР±Р»Р°СЃС‚СЊ РєРѕРЅРєСЂРµС‚РЅРѕРіРѕ РіСЂР°С„РёРєР°
    // FALSE - РЅРµ СЃС‚РёСЂР°РµРј С„РѕРЅ, С‚.Рє. РёСЃРїРѕР»СЊР·СѓРµРј double buffering РґР»СЏ СѓСЃС‚СЂР°РЅРµРЅРёСЏ РјРёРіР°РЅРёСЏ
    InvalidateRect(hWnd, &chartRect, FALSE);
}

// РњРµС‚РѕРґ РґР»СЏ РѕС‚РѕР±СЂР°Р¶РµРЅРёСЏ СЃС…РµРјС‹ РїСЂРёР±РѕСЂР° (v2.9.4: РЅРµРјРѕРґР°Р»СЊРЅРѕРµ РѕРєРЅРѕ)
void ChartManager::ShowSchematicDialog(HWND hWnd, int chartNumber) {
    if (!schematicModule || !curveSelectionModule) {
        return;
    }
    
    // РџРѕРєР°Р·С‹РІР°РµРј РЅРµРјРѕРґР°Р»СЊРЅРѕРµ РґРёР°Р»РѕРіРѕРІРѕРµ РѕРєРЅРѕ СЃС…РµРјС‹ РїСЂРёР±РѕСЂР°
    // РћРєРЅРѕ РѕСЃС‚Р°С‘С‚СЃСЏ РѕС‚РєСЂС‹С‚С‹Рј РїРѕСЃР»Рµ Apply, РїРѕСЌС‚РѕРјСѓ РЅРµ РЅСѓР¶РЅРѕ РѕР±СЂР°Р±Р°С‚С‹РІР°С‚СЊ РІРѕР·РІСЂР°С‰Р°РµРјРѕРµ Р·РЅР°С‡РµРЅРёРµ
    schematicModule->ShowSchematicDialog(hWnd, chartNumber);
    
    // РџСЂРёРјРµС‡Р°РЅРёРµ: РџСЂРёРјРµРЅРµРЅРёРµ РєРѕРЅС„РёРіСѓСЂР°С†РёРё С‚РµРїРµСЂСЊ РїСЂРѕРёСЃС…РѕРґРёС‚ РІРЅСѓС‚СЂРё SchematicModule
    // РїСЂРё РЅР°Р¶Р°С‚РёРё РєРЅРѕРїРєРё Apply, Рё РѕРєРЅРѕ РѕСЃС‚Р°С‘С‚СЃСЏ РѕС‚РєСЂС‹С‚С‹Рј РґР»СЏ Р±С‹СЃС‚СЂРѕРіРѕ РїРµСЂРµРєР»СЋС‡РµРЅРёСЏ РєСЂРёРІС‹С…
}

// РњРµС‚РѕРґС‹ РґР»СЏ РЅРѕРІРѕР№ СЃРёСЃС‚РµРјС‹ РєСЂРёРІС‹С…
void ChartManager::DrawChartWithCurves(HDC hdc, const RECT& chartRect, int chartNumber) {
    if (!curveSelectionModule || !dataModule) return;
    
    // РџРѕР»СѓС‡Р°РµРј РєРѕРЅС„РёРіСѓСЂР°С†РёСЋ РєСЂРёРІС‹С…
    ChartConfiguration config = curveSelectionModule->GetChartConfiguration(chartNumber);
    if (config.curves.empty()) return;
    
    // РџРѕР»СѓС‡Р°РµРј РїР°СЂР°РјРµС‚СЂС‹ zoom
    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;

    const CachedCurveSnapshot* curveSnapshot = nullptr;
    const CachedCurveSnapshot* staleCurveCache = (chartNumber == 1) ? &chart1SnapshotCache : &chart2SnapshotCache;
    // Р¤РѕСЂРјРёСЂСѓРµРј Р·Р°РіРѕР»РѕРІРѕРє СЃ С‡Р°СЃС‚РѕС‚РѕР№
    // "Р“СЂР°С„РёРє " РІ Unicode escape
    std::wstring title = L"\x0413\x0440\x0430\x0444\x0438\x043A " + std::to_wstring(chartNumber);
    
    // Р”РѕР±Р°РІР»СЏРµРј С‡Р°СЃС‚РѕС‚Сѓ, РµСЃР»Рё РµСЃС‚СЊ РєСЂРёРІС‹Рµ
    if (!config.curves.empty()) {
        FrequencyBand frequency = config.curves[0].frequency;
        title += (frequency == FrequencyBand::FREQ_400_KHZ) ? L" (400 kHz)" : L" (2000 kHz)";
    }

    if (!TryGetCurveSnapshot(chartNumber, config, *zoom, !ShouldUseAsyncCurveWarmup(), curveSnapshot)) {
        if (ShouldUseAsyncCurveWarmup() && staleCurveCache->valid) {
            curveSnapshot = staleCurveCache;
        } else {
            chartModule->DrawDualAxisBackground(hdc, chartRect);
            chartModule->DrawChartBorder(hdc, chartRect);
            chartModule->DrawChartTitle(hdc, chartRect, title);
            chartModule->DrawNoDataMessage(hdc, chartRect, L"Подготовка графика...");
            return;
        }
    }
    auto& leftAxisData = curveSnapshot->leftAxisData;
    auto& rightAxisData = curveSnapshot->rightAxisData;
    auto& leftAxisCurves = curveSnapshot->leftAxisCurves;
    auto& rightAxisCurves = curveSnapshot->rightAxisCurves;
    
    // v2.10.0: РџСЂРѕРІРµСЂСЏРµРј С‚РёРї РѕСЃРё X
    if (config.xAxisType == XAxisType::TEMPERATURE) {
        // Р РµР¶РёРј РєСЂРѕСЃСЃРїР»РѕС‚Р°: С‚РµРјРїРµСЂР°С‚СѓСЂР° РЅР° РѕСЃРё X

        const CachedTemperatureAxisData* temperatureAxis = nullptr;
        const CachedTemperatureAxisData* staleTemperatureCache = (chartNumber == 1) ? &chart1TemperatureCache : &chart2TemperatureCache;
        if (!TryGetTemperatureAxisData(chartNumber, *zoom, !ShouldUseAsyncCurveWarmup(), temperatureAxis)) {
            if (ShouldUseAsyncCurveWarmup() && staleTemperatureCache->valid) {
                temperatureAxis = staleTemperatureCache;
            } else {
                chartModule->DrawDualAxisBackground(hdc, chartRect);
                chartModule->DrawChartBorder(hdc, chartRect);
                chartModule->DrawChartTitle(hdc, chartRect, title);
                chartModule->DrawNoDataMessage(hdc, chartRect, L"Подготовка графика...");
                return;
            }
        }

        // РџРѕР»СѓС‡Р°РµРј РґРёР°РїР°Р·РѕРЅС‹ Y РёР· РґР°РЅРЅС‹С…
        const double minYFull = curveSnapshot->minY1;
        const double maxYFull = curveSnapshot->maxY1;
        
        double minYData = 0.0;
        double maxYData = 0.0;
        BuildDisplayedLeftAxisRange(zoom->isYCustom, zoom->minY, zoom->maxY, minYFull, maxYFull, minYData, maxYData);

        double displayMinYRight = curveSnapshot->minY2;
        double displayMaxYRight = curveSnapshot->maxY2;
        const bool useExplicitRightAxisRange = config.hasDualAxis && !rightAxisData.empty() &&
                                               zoom->isYCustom && zoom->hasRightAxisCustomRange;
        if (config.hasDualAxis && !rightAxisData.empty()) {
            BuildDisplayedRightAxisRange(zoom->isYCustom,
                                         zoom->hasRightAxisCustomRange,
                                         zoom->minYRight,
                                         zoom->maxYRight,
                                         minYFull,
                                         maxYFull,
                                         curveSnapshot->minY2,
                                         curveSnapshot->maxY2,
                                         minYData,
                                         maxYData,
                                         displayMinYRight,
                                         displayMaxYRight);
        }

        // Р’С‹Р·С‹РІР°РµРј РЅРѕРІС‹Р№ РјРµС‚РѕРґ РѕС‚СЂРёСЃРѕРІРєРё СЃ С‚РµРјРїРµСЂР°С‚СѓСЂРѕР№ Рё РїСЂР°РІРёР»СЊРЅРѕР№ Р»РµРіРµРЅРґРѕР№
        // v2.15.0: РџРµСЂРµРґР°С‘Рј config РґР»СЏ РїРѕРґРґРµСЂР¶РєРё РєРѕРјРїР°РєС‚РЅРѕР№ Р»РµРіРµРЅРґС‹ РїСЂРµСЃРµС‚РѕРІ
        chartModule->DrawDualAxisChart(hdc, chartRect, config.curves,
                                       leftAxisData, rightAxisData,
                                       title, true, temperatureAxis->temperatureData,
                                       temperatureAxis->minXData, temperatureAxis->maxXData, minYData, maxYData,
                                       config, true,
                                       useExplicitRightAxisRange,
                                       displayMinYRight,
                                       displayMaxYRight);
    } else {
        // РћР±С‹С‡РЅС‹Р№ СЂРµР¶РёРј: РЅРѕРјРµСЂ РєР°РґСЂР° РЅР° РѕСЃРё X
        
        // РџРѕР»СѓС‡Р°РµРј РґРёР°РїР°Р·РѕРЅС‹ Y РёР· РґР°РЅРЅС‹С…
        const double minYData = curveSnapshot->minY1;
        const double maxYData = curveSnapshot->maxY1;
        VisibleFrameAxisRange visibleXRange = {};
        BuildVisibleFrameAxisRange(zoom->isZoomed, zoom->minX, zoom->maxX, dataModule->GetDataSize(), visibleXRange);
        
        // РџСЂРѕРІРµСЂСЏРµРј, РЅСѓР¶РЅС‹ Р»Рё РґРІРѕР№РЅС‹Рµ РѕСЃРё
        if (config.hasDualAxis && !rightAxisData.empty()) {
            // РСЃРїРѕР»СЊР·СѓРµРј РЅРѕРІС‹Р№ РјРµС‚РѕРґ РґР»СЏ РґРІРѕР№РЅС‹С… РѕСЃРµР№ СЃ РїРѕРґРґРµСЂР¶РєРѕР№ zoom
            // Р’СЃРµРіРґР° РїРѕРєР°Р·С‹РІР°РµРј РѕСЃРё (РІРєР»СЋС‡Р°СЏ РѕСЃСЊ X)
            bool showAxes = true;
            
            double displayMinY = 0.0;
            double displayMaxY = 0.0;
            BuildDisplayedLeftAxisRange(zoom->isYCustom, zoom->minY, zoom->maxY, minYData, maxYData, displayMinY, displayMaxY);

            double displayMinYRight = curveSnapshot->minY2;
            double displayMaxYRight = curveSnapshot->maxY2;
            const bool useExplicitRightAxisRange = zoom->isYCustom && zoom->hasRightAxisCustomRange;
            BuildDisplayedRightAxisRange(zoom->isYCustom,
                                         zoom->hasRightAxisCustomRange,
                                         zoom->minYRight,
                                         zoom->maxYRight,
                                         minYData,
                                         maxYData,
                                         curveSnapshot->minY2,
                                         curveSnapshot->maxY2,
                                         displayMinY,
                                         displayMaxY,
                                         displayMinYRight,
                                         displayMaxYRight);

            DrawFrameAxisChart(*chartModule, hdc, chartRect, config.curves,
                               leftAxisData, rightAxisData,
                               title, dataModule->HasData(), visibleXRange,
                               displayMinY, displayMaxY,
                               config, showAxes,
                               useExplicitRightAxisRange,
                               displayMinYRight,
                               displayMaxYRight);
        } else {
            // РСЃРїРѕР»СЊР·СѓРµРј РѕР±С‹С‡РЅС‹Р№ РјРµС‚РѕРґ РґР»СЏ РѕРґРЅРѕР№ РѕСЃРё
            if (!leftAxisData.empty()) {
                // РћС‚СЂРёСЃРѕРІС‹РІР°РµРј РіСЂР°С„РёРє СЃ РєР°СЃС‚РѕРјРЅС‹РјРё РєСЂРёРІС‹РјРё
                // РСЃРїРѕР»СЊР·СѓРµРј РјРµС‚РѕРґ DrawDualAxisChart РґР°Р¶Рµ РґР»СЏ РѕРґРЅРѕР№ РѕСЃРё, С‡С‚РѕР±С‹ РїСЂР°РІРёР»СЊРЅРѕ РѕС‚РѕР±СЂР°Р·РёС‚СЊ Р»РµРіРµРЅРґСѓ
                bool showAxes = true;
                
                double displayMinY = 0.0;
                double displayMaxY = 0.0;
                BuildDisplayedLeftAxisRange(zoom->isYCustom, zoom->minY, zoom->maxY, minYData, maxYData, displayMinY, displayMaxY);

                DrawFrameAxisChart(*chartModule, hdc, chartRect, leftAxisCurves,
                                   leftAxisData, std::vector<std::vector<float>>(),
                                   title, dataModule->HasData(), visibleXRange,
                                   displayMinY, displayMaxY,
                                   config, showAxes,
                                   false, 0.0, 0.0);
            }
        }
    }
}

void ChartManager::DrawChartLegacy(HDC hdc, const RECT& chartRect, int chartNumber) {
    // РџСЂРѕРІРµСЂСЏРµРј, Р·Р°РіСЂСѓР¶РµРЅС‹ Р»Рё РґР°РЅРЅС‹Рµ
    bool hasData = dataModule && dataModule->HasData();
    
    // "Р“СЂР°С„РёРє " РІ Unicode escape
    std::wstring title = L"\x0413\x0440\x0430\x0444\x0438\x043A " + std::to_wstring(chartNumber);
    
    // РћС‚СЂРёСЃРѕРІС‹РІР°РµРј С„РѕРЅ Рё СЂР°РјРєСѓ РіСЂР°С„РёРєР° РІСЂСѓС‡РЅСѓСЋ (Р±РµР· РїРѕРґРїРёСЃРµР№ РѕСЃРµР№)
    chartModule->DrawChartBackground(hdc, chartRect);
    chartModule->DrawChartBorder(hdc, chartRect);
    chartModule->DrawChartTitle(hdc, chartRect, title);
    
    // Р Р°СЃС‡РµС‚ РѕР±Р»Р°СЃС‚Рё РґР»СЏ РіСЂР°С„РёРєР°
    // Р’РђР–РќРћ: РСЃРїРѕР»СЊР·СѓРµРј С‚Рµ Р¶Рµ РѕС‚СЃС‚СѓРїС‹, С‡С‚Рѕ Рё РІ DrawDualAxisChart (CalculateDualAxisPlotRect)
    RECT plotRect = chartRect;
    plotRect.left += 60;
    plotRect.top += 45;
    plotRect.right -= 140;
    plotRect.bottom -= 50;
    
    // Р РёСЃСѓРµРј РѕСЃРё
    chartModule->DrawChartAxes(hdc, plotRect, true);
    
    // Р•СЃР»Рё РґР°РЅРЅС‹Рµ Р·Р°РіСЂСѓР¶РµРЅС‹, СЂРёСЃСѓРµРј С‚РѕР»СЊРєРѕ СЃРµС‚РєСѓ Р‘Р•Р— РїРѕРґРїРёСЃРµР№ РѕСЃРµР№
    if (hasData) {
        chartModule->DrawGridOnly(hdc, plotRect);
    }
    
    // Р РёСЃСѓРµРј С‚РµРєСЃС‚ СЃ РёРЅСЃС‚СЂСѓРєС†РёРµР№
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(100, 100, 100));
    HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    
    // Р’С‹Р±РёСЂР°РµРј СЃРѕРѕР±С‰РµРЅРёРµ РІ Р·Р°РІРёСЃРёРјРѕСЃС‚Рё РѕС‚ РЅР°Р»РёС‡РёСЏ РґР°РЅРЅС‹С…
    std::wstring message;
    if (hasData) {
        // "РќР°Р¶РјРёС‚Рµ 'РЎС…РµРјР° РїСЂРёР±РѕСЂР°' РґР»СЏ РЅР°СЃС‚СЂРѕР№РєРё РіСЂР°С„РёРєР°" РІ Unicode
        message = L"\x041D\x0430\x0436\x043C\x0438\x0442\x0435 '\x0421\x0445\x0435\x043C\x0430 \x043F\x0440\x0438\x0431\x043E\x0440\x0430' \x0434\x043B\x044F \x043D\x0430\x0441\x0442\x0440\x043E\x0439\x043A\x0438 \x0433\x0440\x0430\x0444\x0438\x043A\x0430";
    } else {
        // "РЎРЅР°С‡Р°Р»Р° РѕС‚РєСЂРѕР№С‚Рµ С„Р°Р№Р» РґР°РЅРЅС‹С…" РІ Unicode
        message = L"\x0421\x043D\x0430\x0447\x0430\x043B\x0430 \x043E\x0442\x043A\x0440\x043E\x0439\x0442\x0435 \x0444\x0430\x0439\x043B \x0434\x0430\x043D\x043D\x044B\x0445";
    }
    
    RECT textRect = chartRect;
    DrawText(hdc, message.c_str(), -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

// РњРµС‚РѕРґС‹ РґР»СЏ СЂР°Р±РѕС‚С‹ СЃ РєРѕРЅС„РёРіСѓСЂР°С†РёСЏРјРё РєСЂРёРІС‹С…
void ChartManager::ApplyCurveConfiguration(int chartNumber, const ChartConfiguration& config) {
    if (!curveSelectionModule) return;
    
    // Р’Р°Р»РёРґРёСЂСѓРµРј РєРѕРЅС„РёРіСѓСЂР°С†РёСЋ
    if (!ValidateChartConfiguration(config)) {
        return;
    }
    
    // РџСЂРёРјРµРЅСЏРµРј РєРѕРЅС„РёРіСѓСЂР°С†РёСЋ
    curveSelectionModule->ApplyCurveConfiguration(chartNumber, config);
    
    // РЎРёРЅС…СЂРѕРЅРёР·РёСЂСѓРµРј UI СЃ РЅРѕРІРѕР№ РєРѕРЅС„РёРіСѓСЂР°С†РёРµР№
    SynchronizeUIWithConfiguration(chartNumber);
}

ChartConfiguration ChartManager::GetChartConfiguration(int chartNumber) const {
    if (!curveSelectionModule) {
        return ChartConfiguration();
    }
    
    return curveSelectionModule->GetChartConfiguration(chartNumber);
}

bool ChartManager::HasCurveConfiguration(int chartNumber) const {
    if (!curveSelectionModule) {
        return false;
    }
    
    return curveSelectionModule->HasConfiguration(chartNumber);
}

// РњРµС‚РѕРґС‹ РґР»СЏ РєРѕРїРёСЂРѕРІР°РЅРёСЏ РЅР°СЃС‚СЂРѕРµРє РјРµР¶РґСѓ РіСЂР°С„РёРєР°РјРё
void ChartManager::CopyChartSettings(int fromChart, int toChart) {
    if (!curveSelectionModule) return;
    
    // РџСЂРѕРІРµСЂСЏРµРј РІР°Р»РёРґРЅРѕСЃС‚СЊ РЅРѕРјРµСЂРѕРІ РіСЂР°С„РёРєРѕРІ
    if (fromChart < 1 || fromChart > 2 || toChart < 1 || toChart > 2 || fromChart == toChart) {
        return;
    }
    
    // РџСЂРѕРІРµСЂСЏРµРј, С‡С‚Рѕ РёСЃС…РѕРґРЅС‹Р№ РіСЂР°С„РёРє РёРјРµРµС‚ РєРѕРЅС„РёРіСѓСЂР°С†РёСЋ
    if (!curveSelectionModule->HasConfiguration(fromChart)) {
        return;
    }
    
    // РљРѕРїРёСЂСѓРµРј РЅР°СЃС‚СЂРѕР№РєРё
    curveSelectionModule->CopySettings(fromChart, toChart);
    
    // РЎРёРЅС…СЂРѕРЅРёР·РёСЂСѓРµРј UI СЃ РЅРѕРІРѕР№ РєРѕРЅС„РёРіСѓСЂР°С†РёРµР№
    SynchronizeUIWithConfiguration(toChart);
}

// РњРµС‚РѕРґС‹ РґР»СЏ РїСЂРёРјРµРЅРµРЅРёСЏ РїСЂРµСЃРµС‚РѕРІ
void ChartManager::ApplyPreset(int chartNumber, PresetType preset, FrequencyBand frequency) {
    if (!curveSelectionModule) return;
    
    // РџСЂРёРјРµРЅСЏРµРј РїСЂРµСЃРµС‚
    curveSelectionModule->ApplyPreset(preset, chartNumber, frequency);
    
    // РЎРёРЅС…СЂРѕРЅРёР·РёСЂСѓРµРј UI СЃ РЅРѕРІРѕР№ РєРѕРЅС„РёРіСѓСЂР°С†РёРµР№
    SynchronizeUIWithConfiguration(chartNumber);
}

// Р’СЃРїРѕРјРѕРіР°С‚РµР»СЊРЅС‹Рµ РјРµС‚РѕРґС‹ РґР»СЏ РёРЅС‚РµРіСЂР°С†РёРё
void ChartManager::UpdateChartAfterConfigurationChange(HWND hWnd, int chartNumber) {
    // РЎР±СЂР°СЃС‹РІР°РµРј РјР°СЃС€С‚Р°Р±РёСЂРѕРІР°РЅРёРµ РїСЂРё РёР·РјРµРЅРµРЅРёРё РєРѕРЅС„РёРіСѓСЂР°С†РёРё
    if (chartNumber == 1) {
        chart1Zoom.isZoomed = false;
        chart1Zoom.isXCustom = false;
        chart1Zoom.hasXRangeLock = false;
        chart1Zoom.isXToLatest = false;
        chart1Zoom.isYCustom = false;
        chart1ZoomHistory.clear(); // v2.12.5: РћС‡РёС‰Р°РµРј РёСЃС‚РѕСЂРёСЋ zoom
        ClearAxisHistory(1);
    } else if (chartNumber == 2) {
        chart2Zoom.isZoomed = false;
        chart2Zoom.isXCustom = false;
        chart2Zoom.hasXRangeLock = false;
        chart2Zoom.isXToLatest = false;
        chart2Zoom.isYCustom = false;
        chart2ZoomHistory.clear(); // v2.12.5: РћС‡РёС‰Р°РµРј РёСЃС‚РѕСЂРёСЋ zoom
        ClearAxisHistory(2);
    }

    uiModule->ResetXRangeControls(chartNumber);
    
    // РћР±РЅРѕРІР»СЏРµРј СЃРєСЂРѕР»Р»Р±Р°СЂС‹
    ScheduleAsyncCurveSnapshotWarmup(hWnd, chartNumber, curveSelectionModule->GetChartConfiguration(chartNumber), (chartNumber == 1) ? chart1Zoom : chart2Zoom);
    UpdateScrollBars(hWnd);
    
    // РџРµСЂРµСЂРёСЃРѕРІС‹РІР°РµРј РіСЂР°С„РёРє
    InvalidateChart(hWnd, chartNumber);
}

void ChartManager::SynchronizeUIWithConfiguration(int chartNumber) {
    if (!curveSelectionModule) return;
    
    // РџРѕР»СѓС‡Р°РµРј РєРѕРЅС„РёРіСѓСЂР°С†РёСЋ
    ChartConfiguration config = curveSelectionModule->GetChartConfiguration(chartNumber);
    
    // Р•СЃР»Рё РєРѕРЅС„РёРіСѓСЂР°С†РёСЏ РїСѓСЃС‚Р°, РІРѕР·РІСЂР°С‰Р°РµРјСЃСЏ Рє СЃС‚Р°РЅРґР°СЂС‚РЅРѕРјСѓ UI
    if (config.curves.empty()) {
        return;
    }
    
    // РЎРёРЅС…СЂРѕРЅРёР·РёСЂСѓРµРј СЃС‚Р°СЂС‹Рµ СЌР»РµРјРµРЅС‚С‹ UI СЃ РЅРѕРІРѕР№ СЃРёСЃС‚РµРјРѕР№, РµСЃР»Рё РІРѕР·РјРѕР¶РЅРѕ
    SynchronizeLegacyUIWithNewSystem(chartNumber);
    
    // РќРѕРІР°СЏ СЃРёСЃС‚РµРјР° СЂР°Р±РѕС‚Р°РµС‚ РЅРµР·Р°РІРёСЃРёРјРѕ РѕС‚ СЃС‚Р°СЂС‹С… СЌР»РµРјРµРЅС‚РѕРІ UI,
    // РЅРѕ РјС‹ СЃС‚Р°СЂР°РµРјСЃСЏ РїРѕРґРґРµСЂР¶РёРІР°С‚СЊ РёС… РІ СЃРёРЅС…СЂРѕРЅРёР·РёСЂРѕРІР°РЅРЅРѕРј СЃРѕСЃС‚РѕСЏРЅРёРё
    // РґР»СЏ Р»СѓС‡С€РµРіРѕ РїРѕР»СЊР·РѕРІР°С‚РµР»СЊСЃРєРѕРіРѕ РѕРїС‹С‚Р°
}

bool ChartManager::ValidateChartConfiguration(const ChartConfiguration& config) const {
    if (!curveSelectionModule) return false;
    
    // РСЃРїРѕР»СЊР·СѓРµРј РІР°Р»РёРґР°С†РёСЋ РёР· CurveSelectionModule
    return curveSelectionModule->ValidateConfiguration(config);
}

// РњРµС‚РѕРґС‹ РґР»СЏ РѕР±СЂР°Р±РѕС‚РєРё СЃРѕР±С‹С‚РёР№ РѕС‚ РґРёР°Р»РѕРіРѕРІРѕРіРѕ РѕРєРЅР° СЃС…РµРјС‹
void ChartManager::HandleSchematicDialogResult(HWND hWnd, int chartNumber, bool applied) {
    if (!applied) {
        return; // РџРѕР»СЊР·РѕРІР°С‚РµР»СЊ РѕС‚РјРµРЅРёР» РёР·РјРµРЅРµРЅРёСЏ
    }
    
    // РћР±РЅРѕРІР»СЏРµРј РіСЂР°С„РёРє РїРѕСЃР»Рµ РїСЂРёРјРµРЅРµРЅРёСЏ РЅР°СЃС‚СЂРѕРµРє
    UpdateChartAfterConfigurationChange(hWnd, chartNumber);
    
    // РћС‚РїСЂР°РІР»СЏРµРј РїРѕР»СЊР·РѕРІР°С‚РµР»СЊСЃРєРѕРµ СЃРѕРѕР±С‰РµРЅРёРµ РіР»Р°РІРЅРѕРјСѓ РѕРєРЅСѓ РґР»СЏ РѕР±РЅРѕРІР»РµРЅРёСЏ
    PostMessage(hWnd, WM_USER + 100, static_cast<WPARAM>(chartNumber), 0);
}

void ChartManager::HandlePresetCommand(HWND hWnd, int chartNumber, int presetId, FrequencyBand frequency) {
    PresetType preset;
    
    // РћРїСЂРµРґРµР»СЏРµРј С‚РёРї РїСЂРµСЃРµС‚Р° РїРѕ ID РєРѕРјР°РЅРґС‹
    switch (presetId) {
        case IDC_PRESET_ALL_AMP_R1:
            preset = PresetType::ALL_AMPLITUDES_R1;
            break;
        case IDC_PRESET_ALL_AMP_R2:
            preset = PresetType::ALL_AMPLITUDES_R2;
            break;
        case IDC_PRESET_RATIOS:
            preset = PresetType::AMPLITUDE_RATIOS;
            break;
        case IDC_PRESET_DIFFERENCES:
            preset = PresetType::PHASE_DIFFERENCES;
            break;
        default:
            return; // РќРµРёР·РІРµСЃС‚РЅС‹Р№ РїСЂРµСЃРµС‚
    }
    
    // РџСЂРёРјРµРЅСЏРµРј РїСЂРµСЃРµС‚
    ApplyPreset(chartNumber, preset, frequency);
    
    // РћР±РЅРѕРІР»СЏРµРј РіСЂР°С„РёРє
    UpdateChartAfterConfigurationChange(hWnd, chartNumber);
}

void ChartManager::HandleCopySettingsCommand(HWND hWnd, int fromChart, int toChart) {
    // РљРѕРїРёСЂСѓРµРј РЅР°СЃС‚СЂРѕР№РєРё РјРµР¶РґСѓ РіСЂР°С„РёРєР°РјРё
    CopyChartSettings(fromChart, toChart);
    
    // РћР±РЅРѕРІР»СЏРµРј С†РµР»РµРІРѕР№ РіСЂР°С„РёРє
    UpdateChartAfterConfigurationChange(hWnd, toChart);
}

// РњРµС‚РѕРґС‹ РґР»СЏ РѕР±РµСЃРїРµС‡РµРЅРёСЏ СЃРѕРІРјРµСЃС‚РёРјРѕСЃС‚Рё
void ChartManager::ResetToLegacyMode(int chartNumber) {
    if (!curveSelectionModule) return;
    
    // РћС‡РёС‰Р°РµРј РєРѕРЅС„РёРіСѓСЂР°С†РёСЋ РєСЂРёРІС‹С… РґР»СЏ РІРѕР·РІСЂР°С‚Р° Рє СЃС‚Р°СЂРѕР№ СЃРёСЃС‚РµРјРµ
    ChartConfiguration emptyConfig;
    curveSelectionModule->ApplyCurveConfiguration(chartNumber, emptyConfig);
    
    // РЎРёРЅС…СЂРѕРЅРёР·РёСЂСѓРµРј UI
    SynchronizeUIWithConfiguration(chartNumber);
}

bool ChartManager::IsUsingNewCurveSystem(int chartNumber) const {
    if (!curveSelectionModule) return false;
    
    return curveSelectionModule->HasConfiguration(chartNumber);
}

void ChartManager::SynchronizeLegacyUIWithNewSystem(int chartNumber) {
    // РњРµС‚РѕРґ РѕСЃС‚Р°РІР»РµРЅ РґР»СЏ СЃРѕРІРјРµСЃС‚РёРјРѕСЃС‚Рё API, РЅРѕ Р±РѕР»СЊС€Рµ РЅРёС‡РµРіРѕ РЅРµ РґРµР»Р°РµС‚
    // Р’СЃРµ СѓРїСЂР°РІР»СЏРµС‚СЃСЏ С‚РѕР»СЊРєРѕ С‡РµСЂРµР· Device Schematic
}

void ChartManager::HandleLegacyUIChange(HWND hWnd, int chartNumber) {
    // Р•СЃР»Рё РїРѕР»СЊР·РѕРІР°С‚РµР»СЊ РёР·РјРµРЅРёР» РЅР°СЃС‚СЂРѕР№РєРё С‡РµСЂРµР· СЃС‚Р°СЂС‹Рµ СЌР»РµРјРµРЅС‚С‹ UI,
    // СЃР±СЂР°СЃС‹РІР°РµРј РЅРѕРІСѓСЋ СЃРёСЃС‚РµРјСѓ РєСЂРёРІС‹С… РґР»СЏ СЌС‚РѕРіРѕ РіСЂР°С„РёРєР°
    if (IsUsingNewCurveSystem(chartNumber)) {
        ResetToLegacyMode(chartNumber);
        
        // РћР±РЅРѕРІР»СЏРµРј РіСЂР°С„РёРє
        InvalidateChart(hWnd, chartNumber);
    }
}

// РњРµС‚РѕРґС‹ РґР»СЏ РѕР±СЂР°Р±РѕС‚РєРё X Range
void ChartManager::HandleXRangeApply(HWND hWnd, int chartNumber) {
    if (!dataModule || !dataModule->HasData()) {
        wchar_t message[256];
        swprintf_s(message, L"No data loaded for Chart %d", chartNumber);
        MessageBox(hWnd, message, L"X Range", MB_OK | MB_ICONWARNING);
        return;
    }

    // Surf ON: X РґРёР°РїР°Р·РѕРЅ С„РёРєСЃРёСЂРѕРІР°РЅ РЅР° РїРѕСЃР»РµРґРЅРёРµ N С‚РѕС‡РµРє
    if (uiModule->IsSurfEnabled(chartNumber)) {
        MessageBox(hWnd,
            L"X Range is locked in Surf mode.\nDisable Surf to change X range.",
            L"X Range",
            MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    // РџРѕР»СѓС‡Р°РµРј РјР°РєСЃРёРјР°Р»СЊРЅРѕРµ Р·РЅР°С‡РµРЅРёРµ РґР°РЅРЅС‹С…
    double dataSize = static_cast<double>(dataModule->GetDataSize() - 1);
    
    // РџРѕР»СѓС‡Р°РµРј Р·РЅР°С‡РµРЅРёСЏ РёР· РїРѕР»РµР№ РІРІРѕРґР° РґР»СЏ СѓРєР°Р·Р°РЅРЅРѕРіРѕ РіСЂР°С„РёРєР°
    double fromX, toX;
    uiModule->GetXRangeValues(chartNumber, fromX, toX);
    const bool isToEmpty = uiModule->IsXRangeToEmpty(chartNumber);
    
    // Р•СЃР»Рё РїРѕР»Рµ "To" РїСѓСЃС‚РѕРµ, РёСЃРїРѕР»СЊР·СѓРµРј РјР°РєСЃРёРјР°Р»СЊРЅРѕРµ Р·РЅР°С‡РµРЅРёРµ РґР°РЅРЅС‹С…
    if (isToEmpty) {
        toX = dataSize;
    }
    
    // Р’Р°Р»РёРґР°С†РёСЏ РІС…РѕРґРЅС‹С… РґР°РЅРЅС‹С…
    if (fromX >= toX) {
        wchar_t message[256];
        swprintf_s(message, L"Chart %d: 'From' value must be less than 'To' value", chartNumber);
        MessageBox(hWnd, message, L"X Range Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    if (fromX < 0 || toX > dataSize || fromX > dataSize || toX < 0) {
        wchar_t message[256];
        swprintf_s(message, L"Chart %d: X Range must be between 0 and %.0f", chartNumber, dataSize);
        MessageBox(hWnd, message, L"X Range Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // РџСЂРѕРІРµСЂСЏРµРј РјРёРЅРёРјР°Р»СЊРЅС‹Р№ РґРёР°РїР°Р·РѕРЅ: РїРѕРґРїРёСЃРё РґРѕР»Р¶РЅС‹ Р±С‹С‚СЊ С†РµР»С‹РјРё Рё РѕС‚Р»РёС‡Р°С‚СЊСЃСЏ РјРёРЅРёРјСѓРј РЅР° 10
    const double MIN_X_RANGE = 10.0;
    double xRange = toX - fromX;
    
    if (xRange < MIN_X_RANGE) {
        wchar_t message[256];
        swprintf_s(message, L"Chart %d: X Range must be at least %.0f points.", 
                   chartNumber, MIN_X_RANGE);
        MessageBox(hWnd, message, L"X Range Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // РџСЂРёРјРµРЅСЏРµРј РґРёР°РїР°Р·РѕРЅ Рє СѓРєР°Р·Р°РЅРЅРѕРјСѓ РіСЂР°С„РёРєСѓ
    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
    std::vector<ZoomRange>* zoomHistory = (chartNumber == 1) ? &chart1ZoomHistory : &chart2ZoomHistory;
    zoomHistory->clear();
    PushAxisHistory(chartNumber);
    zoom->minX = fromX;
    zoom->maxX = toX;
    
    ChartConfiguration config = curveSelectionModule->GetChartConfiguration(chartNumber);
    zoom->isZoomed = true;
    zoom->isXCustom = true;
    zoom->hasXRangeLock = true;
    zoom->isXToLatest = isToEmpty;
    zoom->isYCustom = false; // Y РїРѕРґРѕР±СЂР°РЅ Р°РІС‚РѕРјР°С‚РёС‡РµСЃРєРё РїРѕРґ РІС‹Р±СЂР°РЅРЅС‹Р№ X РґРёР°РїР°Р·РѕРЅ
    ClearExplicitRightAxisRange(zoom->minYRight, zoom->maxYRight, zoom->hasRightAxisCustomRange);

    const CachedCurveSnapshot* curveSnapshot = nullptr;
    if (TryGetCurveSnapshot(chartNumber, config, *zoom, !ShouldUseAsyncCurveWarmup(), curveSnapshot)) {
        const double yRange = curveSnapshot->maxY1 - curveSnapshot->minY1;
        const double padding = yRange * 0.05;
        zoom->minY = curveSnapshot->minY1 - padding;
        zoom->maxY = curveSnapshot->maxY1 + padding;
    }
    
    // РћР±РЅРѕРІР»СЏРµРј СЃРєСЂРѕР»Р»Р±Р°СЂС‹
    ScheduleAsyncCurveSnapshotWarmup(hWnd, chartNumber, config, *zoom);
    UpdateScrollBars(hWnd);

    // РџРѕСЃР»Рµ Apply РїРѕР»Рµ To РјРѕР¶РµС‚ Р±С‹С‚СЊ РЅР°РјРµСЂРµРЅРЅРѕ РїСѓСЃС‚С‹Рј (СЂРµР¶РёРј From..latest),
    // РїРѕСЌС‚РѕРјСѓ РЅРµ РїРµСЂРµР·Р°РїРёСЃС‹РІР°РµРј РїРѕР»СЏ РІРІРѕРґР° Р°РІС‚РѕРјР°С‚РёС‡РµСЃРєРё.
    uiModule->SetXRangeResetEnabled(chartNumber, true);
    
    // РџРµСЂРµСЂРёСЃРѕРІС‹РІР°РµРј РіСЂР°С„РёРєРё (FALSE РґР»СЏ double buffering)
    InvalidateRect(hWnd, NULL, FALSE);
}

void ChartManager::HandleXRangeReset(HWND hWnd, int chartNumber) {
    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
    
    // Surf ON: Reset С‚СЂР°РєС‚СѓРµРј РєР°Рє "СЃР±СЂРѕСЃРёС‚СЊ РїРѕР»СЊР·РѕРІР°С‚РµР»СЊСЃРєРёР№ Y-zoom" Рё РІРµСЂРЅСѓС‚СЊ Р°РІС‚Рѕ-Y,
    // РїСЂРё СЌС‚РѕРј X РѕСЃС‚Р°С‘С‚СЃСЏ С„РёРєСЃРёСЂРѕРІР°РЅРЅС‹Рј РЅР° РѕРєРЅРµ Surf.
    if (uiModule->IsSurfEnabled(chartNumber)) {
        zoom->isXCustom = false;
        zoom->hasXRangeLock = false;
        zoom->isXToLatest = false;
        zoom->isYCustom = false;
        UpdateSurfRange(chartNumber);
    } else {
        // РЎР±СЂР°СЃС‹РІР°РµРј РјР°СЃС€С‚Р°Р±РёСЂРѕРІР°РЅРёРµ РґР»СЏ СѓРєР°Р·Р°РЅРЅРѕРіРѕ РіСЂР°С„РёРєР°
        zoom->isZoomed = false;
        zoom->isXCustom = false;
        zoom->hasXRangeLock = false;
        zoom->isXToLatest = false;
        zoom->isYCustom = false;
    }

    ClearAxisHistory(chartNumber);

    uiModule->ResetXRangeControls(chartNumber);
    
    // РћР±РЅРѕРІР»СЏРµРј СЃРєСЂРѕР»Р»Р±Р°СЂС‹
    if (curveSelectionModule && IsUsingNewCurveSystem(chartNumber)) {
        ScheduleAsyncCurveSnapshotWarmup(hWnd, chartNumber, curveSelectionModule->GetChartConfiguration(chartNumber), *zoom);
    }
    UpdateScrollBars(hWnd);
    
    // РћР±РЅРѕРІР»СЏРµРј UI
    UpdateXRangeUI(hWnd);
    
    // РџРµСЂРµСЂРёСЃРѕРІС‹РІР°РµРј РіСЂР°С„РёРєРё (FALSE РґР»СЏ double buffering)
    InvalidateRect(hWnd, NULL, FALSE);
}

void ChartManager::HandleResetZoom(HWND hWnd, int chartNumber) {
    // РЎР±СЂР°СЃС‹РІР°РµРј zoom РґР»СЏ СѓРєР°Р·Р°РЅРЅРѕРіРѕ РіСЂР°С„РёРєР° (РїРѕР»РЅС‹Р№ СЃР±СЂРѕСЃ, РЅРµ РїРѕС€Р°РіРѕРІС‹Р№)
    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
    
    // Surf ON: Reset Zoom СЃР±СЂР°СЃС‹РІР°РµС‚ С‚РѕР»СЊРєРѕ РїРѕР»СЊР·РѕРІР°С‚РµР»СЊСЃРєРёР№ Y-zoom,
    // РЅРѕ РЅРµ РІС‹РєР»СЋС‡Р°РµС‚ Surf-РѕРєРЅРѕ РїРѕ X.
    if (uiModule->IsSurfEnabled(chartNumber)) {
        zoom->isXCustom = false;
        zoom->hasXRangeLock = false;
        zoom->isXToLatest = false;
        zoom->isYCustom = false;
        ClearExplicitRightAxisRange(zoom->minYRight, zoom->maxYRight, zoom->hasRightAxisCustomRange);
        UpdateSurfRange(chartNumber);
    } else {
        zoom->isZoomed = false;
        zoom->isXCustom = false;
        zoom->hasXRangeLock = false;
        zoom->isXToLatest = false;
        zoom->isYCustom = false;
        ClearExplicitRightAxisRange(zoom->minYRight, zoom->maxYRight, zoom->hasRightAxisCustomRange);
    }
    
    // v2.12.5: РћС‡РёС‰Р°РµРј РёСЃС‚РѕСЂРёСЋ zoom РїСЂРё РїРѕР»РЅРѕРј СЃР±СЂРѕСЃРµ С‡РµСЂРµР· РєРЅРѕРїРєСѓ
    if (chartNumber == 1) {
        chart1ZoomHistory.clear();
        ClearAxisHistory(1);
    } else if (chartNumber == 2) {
        chart2ZoomHistory.clear();
        ClearAxisHistory(2);
    }

    uiModule->ResetXRangeControls(chartNumber);
    
    // РћР±РЅРѕРІР»СЏРµРј СЃРєСЂРѕР»Р»Р±Р°СЂС‹
    if (curveSelectionModule && IsUsingNewCurveSystem(chartNumber)) {
        ScheduleAsyncCurveSnapshotWarmup(hWnd, chartNumber, curveSelectionModule->GetChartConfiguration(chartNumber), *zoom);
    }
    UpdateScrollBars(hWnd);
    
    // РћР±РЅРѕРІР»СЏРµРј UI (X Range controls)
    UpdateXRangeUI(hWnd);
    
    // РџРµСЂРµСЂРёСЃРѕРІС‹РІР°РµРј С‚РѕР»СЊРєРѕ РёР·РјРµРЅРµРЅРЅС‹Р№ РіСЂР°С„РёРє
    InvalidateChart(hWnd, chartNumber);
}

void ChartManager::HandleUndoZoomStep(HWND hWnd, int chartNumber) {
    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
    std::vector<ZoomRange>* zoomHistory = (chartNumber == 1) ? &chart1ZoomHistory : &chart2ZoomHistory;
    CrosshairPosition* crosshair = (chartNumber == 1) ? &chart1Crosshair : &chart2Crosshair;

    if (!zoom || !zoomHistory || zoomHistory->empty()) {
        return;
    }

    crosshair->isVisible = false;
    *zoom = zoomHistory->back();
    zoomHistory->pop_back();

    AxisZoomRange axisState;
    PopAxisHistoryX(chartNumber, axisState);
    PopAxisHistoryY(chartNumber, axisState);

    if (uiModule->IsSurfEnabled(chartNumber) && dataModule && dataModule->HasData()) {
        if (!zoom->isYCustom) {
            UpdateSurfRange(chartNumber);
        } else {
            size_t dataSize = dataModule->GetDataSize();
            int surfWindow = uiModule->GetSurfWindowSize(chartNumber);
            zoom->minX = (dataSize > static_cast<size_t>(surfWindow))
                ? static_cast<double>(dataSize - surfWindow) : 0.0;
            zoom->maxX = static_cast<double>(dataSize - 1);
            zoom->isZoomed = true;
        }
    }

    UpdateZoomActiveState(chartNumber);
    UpdateScrollBars(hWnd);
    UpdateXRangeUI(hWnd);
    InvalidateChart(hWnd, chartNumber);
}

void ChartManager::UpdateXRangeUI(HWND hWnd) {
    if (!dataModule || !dataModule->HasData()) {
        uiModule->ResetXRangeControls(1);
        uiModule->ResetXRangeControls(2);
        return;
    }

    const bool isSerialRealtime = serialModule && serialModule->IsPolling();
    const double fullMaxX = static_cast<double>(dataModule->GetDataSize() - 1);

    auto updateChartXRange = [this, isSerialRealtime, fullMaxX](int chartNumber, ZoomRange& zoom) {
        const bool surfEnabled = uiModule->IsSurfEnabled(chartNumber);

        if (surfEnabled) {
            // Р’ Surf СЂРµР¶РёРјРµ РЅРµ РїРµСЂРµР·Р°РїРёСЃС‹РІР°РµРј РїРѕР»СЏ From/To.
            uiModule->SetXRangeResetEnabled(chartNumber, false);
            return;
        }

        if (zoom.isXCustom) {
            if (zoom.isXToLatest && isSerialRealtime) {
                if (zoom.maxX != fullMaxX) {
                    zoom.maxX = fullMaxX;
                    if (!zoom.isYCustom) {
                        AutoFitYForCurrentX(chartNumber);
                    }
                }
            }

            uiModule->SetXRangeResetEnabled(chartNumber, true);
            return;
        }

        // COM realtime + РЅРµС‚ РїСЂРёРјРµРЅС‘РЅРЅРѕРіРѕ РєР°СЃС‚РѕРјРЅРѕРіРѕ РґРёР°РїР°Р·РѕРЅР°:
        // РЅРµ РїРµСЂРµР·Р°РїРёСЃС‹РІР°РµРј РїРѕР»СЊР·РѕРІР°С‚РµР»СЊСЃРєРёР№ СЂСѓС‡РЅРѕР№ РІРІРѕРґ РІ РїРѕР»СЏС… X Range.
        if (isSerialRealtime) {
            return;
        }

        uiModule->SetXRangeResetEnabled(chartNumber, false);
    };
    
    // РћР±РЅРѕРІР»СЏРµРј СЌР»РµРјРµРЅС‚С‹ СѓРїСЂР°РІР»РµРЅРёСЏ РґР»СЏ Chart 1
    updateChartXRange(1, chart1Zoom);
    
    // РћР±РЅРѕРІР»СЏРµРј СЌР»РµРјРµРЅС‚С‹ СѓРїСЂР°РІР»РµРЅРёСЏ РґР»СЏ Chart 2 (РµСЃР»Рё РІ Dual Mode)
    if (uiModule->IsDualMode()) {
        updateChartXRange(2, chart2Zoom);
    }
}

void ChartManager::HandleSaveRange(HWND hWnd, int chartNumber) {
    if (!dataModule || !dataModule->HasData()) {
        MessageBox(hWnd, L"No data loaded!", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    const size_t dataSize = dataModule->GetDataSize();
    size_t startIndex = 0;
    size_t endIndex = dataSize - 1;
    
    // РџРѕР»СѓС‡Р°РµРј С‚РµРєСѓС‰РёР№ РґРёР°РїР°Р·РѕРЅ РґР»СЏ СѓРєР°Р·Р°РЅРЅРѕРіРѕ РіСЂР°С„РёРєР°
    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
    
    if (zoom->isZoomed) {
        // Zoom РїСЂРёРјРµРЅС‘РЅ С‡РµСЂРµР· Apply вЂ” РёСЃРїРѕР»СЊР·СѓРµРј РµРіРѕ РґРёР°РїР°Р·РѕРЅ
        startIndex = static_cast<size_t>(zoom->minX);
        endIndex = static_cast<size_t>(zoom->maxX);
    } else {
        // Zoom РЅРµ РїСЂРёРјРµРЅС‘РЅ вЂ” С‡РёС‚Р°РµРј From/To РёР· РїРѕР»РµР№ РІРІРѕРґР° РЅР°РїСЂСЏРјСѓСЋ
        const bool fromEmpty = uiModule->IsXRangeFromEmpty(chartNumber);
        const bool toEmpty = uiModule->IsXRangeToEmpty(chartNumber);
        
        if (fromEmpty && toEmpty) {
            // РћР±Р° РїРѕР»СЏ РїСѓСЃС‚С‹ вЂ” СЃРѕС…СЂР°РЅСЏРµРј Р’РЎР• РґР°РЅРЅС‹Рµ (0 .. dataSize-1)
            startIndex = 0;
            endIndex = dataSize - 1;
        } else {
            double fromX = 0.0, toX = 0.0;
            uiModule->GetXRangeValues(chartNumber, fromX, toX);
            
            if (fromEmpty) {
                startIndex = 0;
            } else {
                startIndex = static_cast<size_t>(fromX > 0.0 ? fromX : 0.0);
            }
            
            if (toEmpty) {
                endIndex = dataSize - 1;
            } else {
                endIndex = static_cast<size_t>(toX > 0.0 ? toX : 0.0);
            }
        }
    }
    
    // Clamp РёРЅРґРµРєСЃРѕРІ Рє РіСЂР°РЅРёС†Р°Рј РґР°РЅРЅС‹С…
    if (startIndex >= dataSize) startIndex = dataSize - 1;
    if (endIndex >= dataSize) endIndex = dataSize - 1;
    
    // РџСЂРѕРІРµСЂСЏРµРј РєРѕСЂСЂРµРєС‚РЅРѕСЃС‚СЊ РґРёР°РїР°Р·РѕРЅР°
    if (startIndex > endIndex) {
        MessageBox(hWnd, L"'From' value must be less than or equal to 'To' value!", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // РћС‚РєСЂС‹РІР°РµРј РґРёР°Р»РѕРі СЃРѕС…СЂР°РЅРµРЅРёСЏ С„Р°Р№Р»Р°
    OPENFILENAME ofn;
    wchar_t szFile[260] = {0};
    
    // Р¤РѕСЂРјРёСЂСѓРµРј РїСЂРµРґР»РѕР¶РµРЅРЅРѕРµ РёРјСЏ С„Р°Р№Р»Р°
    // Р¤РѕСЂРјР°С‚: [РёСЃС…РѕРґРЅРѕРµ_РёРјСЏ]_range_[РЅР°С‡Р°Р»Рѕ]_[РєРѕРЅРµС†]_[Р”Р”_РњРњ_Р“Р“Р“Р“].bin
    std::wstring baseFileName = dataModule->GetLoadedFileName();
    
    // РџРѕР»СѓС‡Р°РµРј С‚РµРєСѓС‰СѓСЋ РґР°С‚Сѓ
    SYSTEMTIME st;
    GetLocalTime(&st);
    
    const std::wstring suggestedFileName = BuildRangeExportSuggestedFileName(baseFileName, startIndex, endIndex, st);
    wcsncpy_s(szFile, suggestedFileName.c_str(), _TRUNCATE);
    
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    ofn.lpstrFilter = L"Binary Files (*.bin)\0*.bin\0All Files (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = L"Save Data Range";
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    
    if (GetSaveFileName(&ofn) == TRUE) {
        DataRangeExportAnalysis exportAnalysis = {};
        const bool hasExportAnalysis = dataModule->TryAnalyzeExportRange(startIndex, endIndex, exportAnalysis);

        // РЎРѕС…СЂР°РЅСЏРµРј РґРёР°РїР°Р·РѕРЅ РґР°РЅРЅС‹С…
        if (dataModule->SaveDataRange(szFile, startIndex, endIndex)) {
            // Р¤РѕСЂРјРёСЂСѓРµРј СЃРѕРѕР±С‰РµРЅРёРµ РѕР± СѓСЃРїРµС…Рµ
            size_t recordCount = endIndex - startIndex + 1;
            const std::wstring successMessage = BuildRangeExportSuccessMessage(recordCount, startIndex, endIndex, szFile, hasExportAnalysis, exportAnalysis);
            MessageBox(hWnd, successMessage.c_str(), L"Success", MB_OK | MB_ICONINFORMATION);
        } else {
            MessageBox(hWnd, L"Failed to save data range!", L"Error", MB_OK | MB_ICONERROR);
        }
    }
}

void ChartManager::HandleCutRange(HWND hWnd, int chartNumber) {
    if (!dataModule || !dataModule->HasData()) {
        MessageBox(hWnd, L"Нет загруженных данных!", L"Ошибка", MB_OK | MB_ICONERROR);
        return;
    }

    if (currentDataFilePath.empty()) {
        MessageBox(hWnd, L"Не найден путь к текущему файлу данных.", L"Ошибка", MB_OK | MB_ICONERROR);
        return;
    }

    const size_t dataSize = dataModule->GetDataSize();
    size_t startIndex = 0;
    size_t endIndex = dataSize - 1;

    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;

    if (zoom->isZoomed) {
        startIndex = static_cast<size_t>(zoom->minX);
        endIndex = static_cast<size_t>(zoom->maxX);
    } else {
        const bool fromEmpty = uiModule->IsXRangeFromEmpty(chartNumber);
        const bool toEmpty = uiModule->IsXRangeToEmpty(chartNumber);

        if (fromEmpty && toEmpty) {
            startIndex = 0;
            endIndex = dataSize - 1;
        } else {
            double fromX = 0.0;
            double toX = 0.0;
            uiModule->GetXRangeValues(chartNumber, fromX, toX);

            startIndex = fromEmpty ? 0 : static_cast<size_t>(fromX > 0.0 ? fromX : 0.0);
            endIndex = toEmpty ? (dataSize - 1) : static_cast<size_t>(toX > 0.0 ? toX : 0.0);
        }
    }

    if (startIndex >= dataSize) startIndex = dataSize - 1;
    if (endIndex >= dataSize) endIndex = dataSize - 1;

    if (startIndex > endIndex) {
        MessageBox(hWnd, L"Значение 'От' должно быть меньше или равно значению 'До'.", L"Ошибка", MB_OK | MB_ICONERROR);
        return;
    }

    if (startIndex == 0 && endIndex == dataSize - 1) {
        MessageBox(hWnd, L"Нельзя вырезать весь файл целиком.", L"Ошибка", MB_OK | MB_ICONERROR);
        return;
    }

    const int cutMode = MessageBox(
        hWnd,
        L"Выберите действие для вырезанного отрезка:\n\n"
        L"Да — Сохранить файл с вырезанным отрезком\n"
        L"Нет — Создать копию текущего файла с вырезанным отрезком\n"
        L"Отмена — отменить операцию.",
        L"Вырезание отрезка",
        MB_YESNOCANCEL | MB_ICONQUESTION);

    if (cutMode == IDCANCEL) {
        return;
    }

    std::wstring extension = L".bin";
    const size_t dotPos = currentDataFilePath.find_last_of(L'.');
    if (dotPos != std::wstring::npos) {
        extension = currentDataFilePath.substr(dotPos);
    }

    const FileFormat currentFormat = DataModule::DetectFileFormat(currentDataFilePath);
    const wchar_t* filter = L"Binary Files (*.bin)\0*.bin\0All Files (*.*)\0*.*\0";
    const wchar_t* dialogTitle = L"Сохранить копию файла с вырезанным отрезком";
    const wchar_t* defExt = L"bin";
    if (currentFormat == FileFormat::FORMAT_BIV) {
        filter = L"BIV Files (*.biv)\0*.biv\0All Files (*.*)\0*.*\0";
        defExt = L"biv";
    } else if (currentFormat == FileFormat::FORMAT_DEV) {
        filter = L"DEV Files (*.DEV)\0*.DEV\0All Files (*.*)\0*.*\0";
        defExt = L"DEV";
    }

    if (cutMode == IDYES) {
        const size_t extensionPos = currentDataFilePath.find_last_of(L'.');
        const std::wstring tempPath = (extensionPos != std::wstring::npos)
            ? (currentDataFilePath.substr(0, extensionPos) + L".cut_tmp" + currentDataFilePath.substr(extensionPos))
            : (currentDataFilePath + L".cut_tmp.bin");

        if (!DataModule::SaveFileExcludingRange(currentDataFilePath, tempPath, startIndex, endIndex)) {
            DeleteFileW(tempPath.c_str());
            MessageBox(hWnd, L"Не удалось сохранить файл без выбранного отрезка.", L"Ошибка", MB_OK | MB_ICONERROR);
            return;
        }

        DataLoadResult reloadedData;
        std::wstring reloadError;
        if (!DataModule::LoadDataFromFileResult(tempPath, reloadedData, reloadError)) {
            DeleteFileW(tempPath.c_str());
            MessageBox(hWnd,
                       reloadError.empty() ? L"Не удалось перечитать обновлённый файл." : reloadError.c_str(),
                       L"Ошибка",
                       MB_OK | MB_ICONERROR);
            return;
        }

        if (!MoveFileExW(tempPath.c_str(), currentDataFilePath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED)) {
            DeleteFileW(tempPath.c_str());
            MessageBox(hWnd, L"Не удалось заменить исходный файл обновлённой версией.", L"Ошибка", MB_OK | MB_ICONERROR);
            return;
        }

        if (!ApplyPreparedSerialDisplayData(hWnd, currentDataFilePath, std::move(reloadedData))) {
            MessageBox(hWnd, L"Файл обновлён, но не удалось перезагрузить данные в приложении.", L"Ошибка", MB_OK | MB_ICONERROR);
            return;
        }

        wchar_t message[512] = {0};
        swprintf_s(message, L"Отрезок [%zu..%zu] успешно вырезан из текущего файла.", startIndex, endIndex);
        MessageBox(hWnd, message, L"Готово", MB_OK | MB_ICONINFORMATION);
        return;
    }

    OPENFILENAME ofn = {};
    wchar_t szFile[260] = {0};
    SYSTEMTIME st;
    GetLocalTime(&st);
    const std::wstring suggestedFileName = BuildCutRangeSuggestedFileName(dataModule->GetLoadedFileName(), startIndex, endIndex, st, extension);
    wcsncpy_s(szFile, suggestedFileName.c_str(), _TRUNCATE);

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile) / sizeof(wchar_t);
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = defExt;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.lpstrTitle = dialogTitle;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    if (GetSaveFileName(&ofn) != TRUE) {
        return;
    }

    if (!DataModule::SaveFileExcludingRange(currentDataFilePath, szFile, startIndex, endIndex)) {
        MessageBox(hWnd, L"Не удалось создать копию файла без выбранного отрезка.", L"Ошибка", MB_OK | MB_ICONERROR);
        return;
    }

    wchar_t message[768] = {0};
    swprintf_s(message, L"Создана копия файла без отрезка [%zu..%zu]:\n%s", startIndex, endIndex, szFile);
    MessageBox(hWnd, message, L"Готово", MB_OK | MB_ICONINFORMATION);
}

// РњРµС‚РѕРґ РґР»СЏ РѕР±СЂР°Р±РѕС‚РєРё РґРІРёР¶РµРЅРёСЏ РјС‹С€Рё Рё РѕР±РЅРѕРІР»РµРЅРёСЏ РєРѕРѕСЂРґРёРЅР°С‚ РІ СЃС‚Р°С‚СѓСЃРЅРѕР№ СЃС‚СЂРѕРєРµ
void ChartManager::HandleMouseMoveForCoordinates(HWND hWnd, int x, int y) {
    if (!dataModule || !dataModule->HasData()) {
        uiModule->ClearStatusBarCoordinates();
        ResetCoordinateTracking();
        return;
    }

    // РћРїСЂРµРґРµР»СЏРµРј, РЅР°Рґ РєР°РєРёРј РіСЂР°С„РёРєРѕРј РЅР°С…РѕРґРёС‚СЃСЏ РєСѓСЂСЃРѕСЂ
    int chartNum = GetChartAtPoint(x, y);
    
    if (chartNum == 0) {
        // РљСѓСЂСЃРѕСЂ РІРЅРµ РіСЂР°С„РёРєРѕРІ
        uiModule->ClearStatusBarCoordinates();
        ResetCoordinateTracking();
        return;
    }

    POINT currentPoint = { x, y };
    DWORD now = GetTickCount();
    DWORD* lastTick = (chartNum == 1) ? &lastChart1CoordinateTick : &lastChart2CoordinateTick;
    POINT* lastPoint = (chartNum == 1) ? &lastChart1CoordinatePoint : &lastChart2CoordinatePoint;
    if (lastPoint->x == currentPoint.x &&
        lastPoint->y == currentPoint.y &&
        now - *lastTick < 16) {
        return;
    }
    *lastPoint = currentPoint;
    *lastTick = now;

    // РџСЂРµРѕР±СЂР°Р·СѓРµРј СЌРєСЂР°РЅРЅС‹Рµ РєРѕРѕСЂРґРёРЅР°С‚С‹ РІ РєРѕРѕСЂРґРёРЅР°С‚С‹ РґР°РЅРЅС‹С…
    double dataX, dataY1, dataY2;
    std::wstring y1Unit, y2Unit;
    bool hasDualAxis;
    
    if (ConvertScreenToChartCoords(chartNum, x, y, dataX, dataY1, dataY2, y1Unit, y2Unit, hasDualAxis)) {
        // РћР±РЅРѕРІР»СЏРµРј СЃС‚Р°С‚СѓСЃРЅСѓСЋ СЃС‚СЂРѕРєСѓ СЃ РєРѕРѕСЂРґРёРЅР°С‚Р°РјРё
        uiModule->UpdateStatusBarCoordinates(dataX, dataY1, y1Unit, dataY2, y2Unit, hasDualAxis);
        
        // РћР±РЅРѕРІР»СЏРµРј РёРЅС„РѕСЂРјР°С†РёСЋ Рѕ С„Р°Р№Р»Рµ РІ РїРѕСЃР»РµРґРЅРµР№ С‡Р°СЃС‚Рё СЃС‚Р°С‚СѓСЃРЅРѕР№ СЃС‚СЂРѕРєРё
        std::wstring fileName = dataModule->GetLoadedFileName();
        if (!fileName.empty()) {
            size_t dataSize = dataModule->GetDataSize();
            wchar_t fileInfo[256];
            swprintf_s(fileInfo, L"Points: %zu  |  %s", dataSize, fileName.c_str());
            SendMessage(uiModule->GetStatusBar(), SB_SETTEXT, 3, (LPARAM)fileInfo);
        }
    } else {
        uiModule->ClearStatusBarCoordinates();
    }
}

// РџСЂРµРѕР±СЂР°Р·РѕРІР°РЅРёРµ СЌРєСЂР°РЅРЅС‹С… РєРѕРѕСЂРґРёРЅР°С‚ РІ РєРѕРѕСЂРґРёРЅР°С‚С‹ РіСЂР°С„РёРєР°
bool ChartManager::ConvertScreenToChartCoords(int chartNum, int screenX, int screenY,
                                              double& dataX, double& dataY1, double& dataY2,
                                              std::wstring& y1Unit, std::wstring& y2Unit,
                                              bool& hasDualAxis) const {
    if (!dataModule || !dataModule->HasData()) {
        return false;
    }

    // РџРѕР»СѓС‡Р°РµРј РѕР±Р»Р°СЃС‚СЊ РіСЂР°С„РёРєР°
    RECT chartRect = (chartNum == 1) ? layoutModule->GetChart1Rect() : layoutModule->GetChart2Rect();
    
    // РџСЂРѕРІРµСЂСЏРµРј, С‡С‚Рѕ РєСѓСЂСЃРѕСЂ РІРЅСѓС‚СЂРё РѕР±Р»Р°СЃС‚Рё РіСЂР°С„РёРєР°
    if (screenX < chartRect.left || screenX > chartRect.right ||
        screenY < chartRect.top || screenY > chartRect.bottom) {
        return false;
    }

    // РћРїСЂРµРґРµР»СЏРµРј, РёСЃРїРѕР»СЊР·СѓРµС‚СЃСЏ Р»Рё РЅРѕРІР°СЏ СЃРёСЃС‚РµРјР° РєСЂРёРІС‹С…
    bool usingNewSystem = IsUsingNewCurveSystem(chartNum);
    
    if (usingNewSystem) {
        // РќРѕРІР°СЏ СЃРёСЃС‚РµРјР° РєСЂРёРІС‹С… СЃ РІРѕР·РјРѕР¶РЅС‹РјРё РґРІРѕР№РЅС‹РјРё РѕСЃСЏРјРё
        ChartConfiguration config = GetChartConfiguration(chartNum);
        
        if (config.curves.empty()) {
            return false;
        }

        // РџРѕР»СѓС‡Р°РµРј РїР°СЂР°РјРµС‚СЂС‹ zoom
        ZoomRange zoom = (chartNum == 1) ? chart1Zoom : chart2Zoom;

        // РџРѕР»СѓС‡Р°РµРј РґР°РЅРЅС‹Рµ РґР»СЏ РєСЂРёРІС‹С… СЃ СѓС‡РµС‚РѕРј zoom РїРѕ X (РєР°Рє РІ DrawChartWithCurves)
        const CachedCurveSnapshot* curveSnapshot = nullptr;
        if (!TryGetCurveSnapshot(chartNum, config, zoom, !ShouldUseAsyncCurveWarmup(), curveSnapshot)) {
            return false;
        }
        auto& leftAxisData = curveSnapshot->leftAxisData;
        auto& rightAxisData = curveSnapshot->rightAxisData;
        double minX = curveSnapshot->minX;
        double maxX = curveSnapshot->maxX;
        double minY1Full = curveSnapshot->minY1;
        double maxY1Full = curveSnapshot->maxY1;
        double minY2Full = curveSnapshot->minY2;
        double maxY2Full = curveSnapshot->maxY2;
        
        // РўРµРєСѓС‰РёРµ РґРёР°РїР°Р·РѕРЅС‹ РґР»СЏ РѕС‚РѕР±СЂР°Р¶РµРЅРёСЏ (РјРѕРіСѓС‚ Р±С‹С‚СЊ РёР·РјРµРЅРµРЅС‹ zoom РїРѕ Y)
        double minY1 = minY1Full, maxY1 = maxY1Full;
        double minY2 = minY2Full, maxY2 = maxY2Full;
        
        // РџСЂРёРјРµРЅСЏРµРј zoom РїРѕ Y РµСЃР»Рё РµСЃС‚СЊ (zoom РїРѕ X СѓР¶Рµ РїСЂРёРјРµРЅС‘РЅ РїСЂРё РїРѕР»СѓС‡РµРЅРёРё РґР°РЅРЅС‹С…)
        if (zoom.isZoomed) {
            minY1 = zoom.minY;
            maxY1 = zoom.maxY;

            // Р’РђР–РќРћ: СЃРёРЅС…СЂРѕРЅРёР·Р°С†РёСЏ СЃ С„Р°РєС‚РёС‡РµСЃРєРѕР№ РѕС‚СЂРёСЃРѕРІРєРѕР№ (DrawChartWithCurves),
            // РіРґРµ РїСЂРё zoom РґРѕР±Р°РІР»СЏРµС‚СЃСЏ display padding +/-5%.
            ApplyDisplayYPadding(minY1, maxY1);
        }

        BuildDisplayedRightAxisRange(zoom.isYCustom,
                                     zoom.hasRightAxisCustomRange,
                                     zoom.minYRight,
                                     zoom.maxYRight,
                                     minY1Full,
                                     maxY1Full,
                                     minY2Full,
                                     maxY2Full,
                                     minY1,
                                     maxY1,
                                     minY2,
                                     maxY2);
        
        // РџСЂРµРѕР±СЂР°Р·СѓРµРј СЌРєСЂР°РЅРЅС‹Рµ РєРѕРѕСЂРґРёРЅР°С‚С‹ РІ РєРѕРѕСЂРґРёРЅР°С‚С‹ РґР°РЅРЅС‹С…
        // Р’РђР–РќРћ: РСЃРїРѕР»СЊР·СѓРµРј С‚Рµ Р¶Рµ РѕС‚СЃС‚СѓРїС‹, С‡С‚Рѕ Рё РІ CalculateDualAxisPlotRect
        DualAxisChartGeometry geometry = {};
        if (!TryBuildDualAxisChartGeometry(chartRect, geometry)) {
            return false;
        }

        // Р’РђР–РќРћ: РєР°Рє РІ DrawDualAxisDataWithZoom вЂ” РґР°РЅРЅС‹Рµ СЂРёСЃСѓСЋС‚СЃСЏ РІРЅСѓС‚СЂРё dataRect,
        // Р° РЅРµ РїРѕ РІСЃРµР№ РІС‹СЃРѕС‚Рµ plotRect.
        const RECT& dataRect = geometry.dataRect;
        const int plotWidth = geometry.plotWidth;
        const int dataHeight = geometry.dataHeight;
        if (dataHeight <= 0) {
            return false;
        }
        
        int relX = screenX - geometry.plotRect.left;
        double xRatio = static_cast<double>(relX) / static_cast<double>(plotWidth);
        if (xRatio < 0.0) xRatio = 0.0;
        if (xRatio > 1.0) xRatio = 1.0;

        // X РєРѕРѕСЂРґРёРЅР°С‚Р°
        // Р’ СЂРµР¶РёРјРµ TEMPERATURE РѕСЃСЊ X РЅРµР»РёРЅРµР№РЅР° РѕС‚РЅРѕСЃРёС‚РµР»СЊРЅРѕ РёРЅРґРµРєСЃР° РєР°РґСЂР°,
        // РїРѕСЌС‚РѕРјСѓ РЅСѓР¶РЅР° РїСЂРёРІСЏР·РєР° Рє Р±Р»РёР¶Р°Р№С€РµРјСѓ РёРЅРґРµРєСЃСѓ РїРѕ С‚РµРјРїРµСЂР°С‚СѓСЂРµ.
        const double linearDataX = minX + xRatio * (maxX - minX);
        if (config.xAxisType == XAxisType::TEMPERATURE) {
            dataX = ResolveTemperatureAxisDataX(chartNum, zoom, xRatio, linearDataX, !ShouldUseAsyncCurveWarmup());
        } else {
            dataX = linearDataX;
        }

        int clampedY = static_cast<int>((std::max)(dataRect.top, (std::min)(dataRect.bottom, static_cast<LONG>(screenY))));
        double yRatio = (double)(dataRect.bottom - clampedY) / dataHeight;

        double leftDrawMin = 0.0;
        double leftDrawMax = 0.0;
        if (!TryBuildDrawnAxisRange(minY1, maxY1, leftDrawMin, leftDrawMax)) {
            leftDrawMin = minY1;
            leftDrawMax = maxY1;
        }
        
        // Y РєРѕРѕСЂРґРёРЅР°С‚С‹
        hasDualAxis = !rightAxisData.empty();
        
        if (hasDualAxis) {
            // Р”РІРѕР№РЅС‹Рµ РѕСЃРё
            dataY1 = leftDrawMin + yRatio * (leftDrawMax - leftDrawMin); // Р›РµРІР°СЏ РѕСЃСЊ

            double rightDrawMin = 0.0;
            double rightDrawMax = 0.0;
            if (!TryBuildDrawnAxisRange(minY2, maxY2, rightDrawMin, rightDrawMax)) {
                rightDrawMin = minY2;
                rightDrawMax = maxY2;
            }
            dataY2 = rightDrawMin + yRatio * (rightDrawMax - rightDrawMin); // РџСЂР°РІР°СЏ РѕСЃСЊ
            
            // РћРїСЂРµРґРµР»СЏРµРј РµРґРёРЅРёС†С‹ РёР·РјРµСЂРµРЅРёСЏ РёР· РїРµСЂРІС‹С… РєСЂРёРІС‹С… РєР°Р¶РґРѕР№ РѕСЃРё
            // РџСЂРµРѕР±СЂР°Р·СѓРµРј std::string РІ std::wstring
            std::string units1 = config.curves[0].units;
            y1Unit = std::wstring(units1.begin(), units1.end());
            
            for (const auto& curve : config.curves) {
                if (curve.yAxisGroup == 1) {
                    std::string units2 = curve.units;
                    y2Unit = std::wstring(units2.begin(), units2.end());
                    break;
                }
            }
        } else {
            // РћРґРЅР° РѕСЃСЊ
            dataY1 = leftDrawMin + yRatio * (leftDrawMax - leftDrawMin);
            std::string units1 = config.curves[0].units;
            y1Unit = std::wstring(units1.begin(), units1.end());
            y2Unit = L"";
        }
        
        return true;
        
    } else {
        // РЎС‚Р°СЂР°СЏ СЃРёСЃС‚РµРјР° (legacy mode)
        // Р—РґРµСЃСЊ РјРѕР¶РЅРѕ РґРѕР±Р°РІРёС‚СЊ РїРѕРґРґРµСЂР¶РєСѓ СЃС‚Р°СЂРѕР№ СЃРёСЃС‚РµРјС‹, РµСЃР»Рё РЅСѓР¶РЅРѕ
        // РџРѕРєР° РІРѕР·РІСЂР°С‰Р°РµРј false
        return false;
    }
}

// РџРѕР»СѓС‡РµРЅРёРµ РµРґРёРЅРёС† РёР·РјРµСЂРµРЅРёСЏ РґР»СЏ С‚РёРїР° РґР°РЅРЅС‹С…
std::wstring ChartManager::GetUnitForDataType(DataType dataType) const {
    switch (dataType) {
        case DataType::PHASE_DIFFERENCE:
        case DataType::PHASE_SMT:
        case DataType::SILENT_PHASE_DIFFERENCE:
            return L"mdeg";
        
        case DataType::AMPLITUDE_RX1:
        case DataType::AMPLITUDE_RX2:
        case DataType::SILENT_AMPLITUDE_RX1:
        case DataType::SILENT_AMPLITUDE_RX2:
            return L"mV";
        
        case DataType::AMPLITUDE_RATIO:
            return L"V/V";
        
        case DataType::TEMPERATURE:
            return L"В°C";
        
        default:
            return L"";
    }
}

// РћР±РЅРѕРІР»РµРЅРёРµ РёРЅС„РѕСЂРјР°С†РёРё Рѕ С„Р°Р№Р»Рµ РІ СЃС‚Р°С‚СѓСЃРЅРѕР№ СЃС‚СЂРѕРєРµ
void ChartManager::UpdateStatusBarFileInfo() {
    if (!uiModule) return;
    
    if (dataModule && dataModule->HasData()) {
        std::wstring fileName = dataModule->GetLoadedFileName();
        size_t dataSize = dataModule->GetDataSize();
        
        wchar_t fileInfo[256];
        swprintf_s(fileInfo, L"Points: %zu  |  %s", dataSize, fileName.c_str());
        SendMessage(uiModule->GetStatusBar(), SB_SETTEXT, 3, (LPARAM)fileInfo);
        
        // РЈСЃС‚Р°РЅР°РІР»РёРІР°РµРј РЅР°С‡Р°Р»СЊРЅРѕРµ СЃРѕРѕР±С‰РµРЅРёРµ
        uiModule->UpdateStatusBar(L"Ready");
    } else {
        SendMessage(uiModule->GetStatusBar(), SB_SETTEXT, 3, (LPARAM)L"No file loaded");
        uiModule->ClearStatusBarCoordinates();
    }
}

// РћС‚СЂРёСЃРѕРІРєР° РїРµСЂРµРєСЂРµСЃС‚РёСЏ (crosshair) РЅР° РіСЂР°С„РёРєРµ
void ChartManager::DrawCrosshair(HDC hdc, const RECT& chartRect, int chartNumber) const {
    // РџРѕР»СѓС‡Р°РµРј РїРµСЂРµРєСЂРµСЃС‚РёРµ РґР»СЏ СѓРєР°Р·Р°РЅРЅРѕРіРѕ РіСЂР°С„РёРєР°
    const CrosshairPosition* crosshair = (chartNumber == 1) ? &chart1Crosshair : &chart2Crosshair;
    
    // РџСЂРѕРІРµСЂСЏРµРј, РЅСѓР¶РЅРѕ Р»Рё РѕС‚РѕР±СЂР°Р¶Р°С‚СЊ РїРµСЂРµРєСЂРµСЃС‚РёРµ
    if (!crosshair->isVisible || crosshair->chartNumber != chartNumber) {
        return;
    }
    
    // Р’С‹С‡РёСЃР»СЏРµРј РѕР±Р»Р°СЃС‚СЊ РіСЂР°С„РёРєР° (plotRect) СЃ СѓС‡С‘С‚РѕРј РѕС‚СЃС‚СѓРїРѕРІ
    // Р’РђР–РќРћ: Р’СЃРµРіРґР° РёСЃРїРѕР»СЊР·СѓРµРј РѕС‚СЃС‚СѓРїС‹ РґР»СЏ РґРІРѕР№РЅС‹С… РѕСЃРµР№, РїРѕС‚РѕРјСѓ С‡С‚Рѕ DrawDualAxisChart
    // РІС‹Р·С‹РІР°РµС‚СЃСЏ РґР»СЏ РІСЃРµС… РіСЂР°С„РёРєРѕРІ Рё РёСЃРїРѕР»СЊР·СѓРµС‚ CalculateDualAxisPlotRect
    DualAxisChartGeometry geometry = {};
    if (!TryBuildDualAxisChartGeometry(chartRect, geometry)) {
        return;
    }
    const RECT& plotRect = geometry.plotRect;
    
    // РџСЂРѕРІРµСЂСЏРµРј, С‡С‚Рѕ РїРµСЂРµРєСЂРµСЃС‚РёРµ РЅР°С…РѕРґРёС‚СЃСЏ РІРЅСѓС‚СЂРё РѕР±Р»Р°СЃС‚Рё РіСЂР°С„РёРєР°
    if (crosshair->screenX < plotRect.left || crosshair->screenX > plotRect.right ||
        crosshair->screenY < plotRect.top || crosshair->screenY > plotRect.bottom) {
        return;
    }
    
    // РЎРѕР·РґР°С‘Рј РєСЂР°СЃРЅРѕРµ РїРµСЂРѕ РґР»СЏ РїРµСЂРµРєСЂРµСЃС‚РёСЏ
    HPEN hCrosshairPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hCrosshairPen);
    
    // Р РёСЃСѓРµРј РІРµСЂС‚РёРєР°Р»СЊРЅСѓСЋ Р»РёРЅРёСЋ (РѕС‚ РІРµСЂС…Р° РґРѕ РЅРёР·Р° plotRect)
    MoveToEx(hdc, crosshair->screenX, plotRect.top, NULL);
    LineTo(hdc, crosshair->screenX, plotRect.bottom);
    
    // Р РёСЃСѓРµРј РіРѕСЂРёР·РѕРЅС‚Р°Р»СЊРЅСѓСЋ Р»РёРЅРёСЋ (РѕС‚ Р»РµРІРѕРіРѕ РґРѕ РїСЂР°РІРѕРіРѕ РєСЂР°СЏ plotRect)
    MoveToEx(hdc, plotRect.left, crosshair->screenY, NULL);
    LineTo(hdc, plotRect.right, crosshair->screenY);
    
    // Р’РѕСЃСЃС‚Р°РЅР°РІР»РёРІР°РµРј СЃС‚Р°СЂРѕРµ РїРµСЂРѕ Рё СѓРґР°Р»СЏРµРј СЃРѕР·РґР°РЅРЅРѕРµ
    SelectObject(hdc, hOldPen);
    DeleteObject(hCrosshairPen);
}

// v2.14.2: РЎР±СЂРѕСЃ РїРµСЂРµРєСЂРµСЃС‚РёСЏ (РІС‹Р·С‹РІР°РµС‚СЃСЏ РїСЂРё РєР»РёРєРµ РЅР° РёРєРѕРЅРєСѓ рџ“Ќ)
void ChartManager::ClearCrosshair(HWND hWnd, int chartNumber) {
    if (chartNumber == 1) {
        chart1Crosshair.isVisible = false;
        uiModule->ClearClickCoordinates(1);
    } else if (chartNumber == 2) {
        chart2Crosshair.isVisible = false;
        uiModule->ClearClickCoordinates(2);
    }
    
    // РџРµСЂРµСЂРёСЃРѕРІС‹РІР°РµРј РѕРєРЅРѕ
    InvalidateRect(hWnd, NULL, TRUE);
}

void ChartManager::DrawNoteMarkers(HDC hdc, const RECT& chartRect, int chartNumber) {
    if (!hdc || !notesModule || !notesModule->IsLoaded() || !dataModule || !dataModule->HasData() || !curveSelectionModule || !chartModule) {
        return;
    }

    ChartConfiguration config = GetChartConfiguration(chartNumber);
    if (config.curves.empty()) return;

    const uint64_t cfgKey = NotesModule::ComputeConfigKey(config);
    std::vector<size_t> noteIdx = notesModule->GetNotesForConfig(cfgKey);
    if (noteIdx.empty()) return;

    // Plot rect (РґРѕР»Р¶РµРЅ СЃРѕРІРїР°РґР°С‚СЊ СЃ CalculateDualAxisPlotRect/ConvertScreenToChartCoords)
    DualAxisChartGeometry geometry = {};
    if (!TryBuildDualAxisChartGeometry(chartRect, geometry)) return;
    const RECT& plotRect = geometry.plotRect;
    const RECT& dataRect = geometry.dataRect;
    if (geometry.dataHeight <= 0) return;
    const int plotW = geometry.plotWidth;
    const int dataH = geometry.dataHeight;

    // Zoom & X range
    ZoomRange zoom = (chartNumber == 1) ? chart1Zoom : chart2Zoom;
    VisibleFrameAxisRange visibleXRange = {};
    BuildVisibleFrameAxisRange(zoom.isZoomed, zoom.minX, zoom.maxX, dataModule->GetDataSize(), visibleXRange);
    const double minXData = visibleXRange.minX;
    const double maxXData = visibleXRange.maxX;
    const double xRange = (maxXData - minXData);
    if (!(xRange > 0.0)) return;

    // РџРѕР»СѓС‡Р°РµРј РґРёР°РїР°Р·РѕРЅС‹ Y (РєР°Рє РІ ConvertScreenToChartCoords / DrawChartWithCurves)
    const CachedCurveSnapshot* curveSnapshot = nullptr;
    if (!TryGetCurveSnapshot(chartNumber, config, zoom, !ShouldUseAsyncCurveWarmup(), curveSnapshot)) {
        return;
    }
    auto& leftAxisData = curveSnapshot->leftAxisData;
    auto& rightAxisData = curveSnapshot->rightAxisData;
    const double minY1Full = curveSnapshot->minY1;
    const double maxY1Full = curveSnapshot->maxY1;
    const double minY2Full = curveSnapshot->minY2;
    const double maxY2Full = curveSnapshot->maxY2;

    double minY1 = zoom.isYCustom ? zoom.minY : minY1Full;
    double maxY1 = zoom.isYCustom ? zoom.maxY : maxY1Full;

    // v2.15.x: РїСЂРё zoom РґРѕР±Р°РІР»СЏРµРј padding РєР°Рє РІ DrawChartWithCurves
    BuildDisplayedLeftAxisRange(zoom.isYCustom, zoom.minY, zoom.maxY, minY1Full, maxY1Full, minY1, maxY1);

    // Р’ DrawDualAxisDataWithZoom РґРѕРїРѕР»РЅРёС‚РµР»СЊРЅРѕ РїСЂРёРјРµРЅСЏРµС‚СЃСЏ РЅРёР¶РЅРёР№ padding 2%.
    double drawMinY1 = 0.0;
    double drawMaxY1 = 0.0;
    if (!TryBuildDrawnAxisRange(minY1, maxY1, drawMinY1, drawMaxY1)) return;

    double minY2 = minY2Full;
    double maxY2 = maxY2Full;
    BuildDisplayedRightAxisRange(zoom.isYCustom,
                                 zoom.hasRightAxisCustomRange,
                                 zoom.minYRight,
                                 zoom.maxYRight,
                                 minY1Full,
                                 maxY1Full,
                                 minY2Full,
                                 maxY2Full,
                                 minY1,
                                 maxY1,
                                 minY2,
                                 maxY2);

    double drawMinY2 = 0.0;
    double drawMaxY2 = 0.0;
    const bool hasRightDrawRange = TryBuildDrawnAxisRange(minY2, maxY2, drawMinY2, drawMaxY2);

    const double yRangeLeft = (drawMaxY1 - drawMinY1);
    if (!(yRangeLeft > 0.0)) return;
    const double yRangeRight = hasRightDrawRange ? (drawMaxY2 - drawMinY2) : 0.0;

    // Р”Р»СЏ TEMPERATURE РЅСѓР¶РЅС‹ minTemp/maxTemp РїРѕ РІРёРґРёРјРѕРјСѓ РґРёР°РїР°Р·РѕРЅСѓ РёРЅРґРµРєСЃРѕРІ
    float minTemp = 0.0f, maxTemp = 0.0f;
    bool hasTempRange = false;
    const size_t dataSize = dataModule->GetDataSize();
    if (config.xAxisType == XAxisType::TEMPERATURE && dataSize > 0) {
        const CachedTemperatureAxisData* temperatureAxis = nullptr;
        if (TryGetTemperatureAxisData(chartNumber, zoom, !ShouldUseAsyncCurveWarmup(), temperatureAxis) && !temperatureAxis->temperatureData.empty()) {
            auto tempRange = std::minmax_element(temperatureAxis->temperatureData.begin(), temperatureAxis->temperatureData.end());
            minTemp = *tempRange.first;
            maxTemp = *tempRange.second;
            if (maxTemp <= minTemp) {
                maxTemp = minTemp + 1.0f;
            }
            hasTempRange = true;
        }
    }

    struct NoteLabelCandidate {
        int anchorX = 0;
        int anchorY = 0;
        std::wstring text;
    };

    auto BuildNotePreviewText = [](const std::wstring& source) -> std::wstring {
        constexpr size_t kMaxWords = 6;
        constexpr size_t kMaxChars = 32;

        std::wstring normalized = source;
        for (auto& ch : normalized) {
            if (ch == L'\r' || ch == L'\n' || ch == L'\t') {
                ch = L' ';
            }
        }

        std::wistringstream wss(normalized);
        std::wstring token;
        std::wstring result;
        size_t words = 0;

        while (wss >> token) {
            if (words >= kMaxWords) {
                break;
            }
            if (!result.empty()) {
                result += L" ";
            }
            result += token;
            ++words;
        }

        std::wstring trailing;
        if (wss >> trailing) {
            result += L"...";
        }

        if (result.empty()) {
            return L"(РїСѓСЃС‚Рѕ)";
        }

        if (result.length() > kMaxChars) {
            if (kMaxChars > 3) {
                result = result.substr(0, kMaxChars - 3) + L"...";
            } else {
                result = result.substr(0, kMaxChars);
            }
        }

        return result;
    };

    std::vector<NoteLabelCandidate> labelCandidates;
    labelCandidates.reserve(8);

    // Р§РµСЂРЅР°СЏ С‚РѕС‡РєР°
    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HBRUSH hBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

    const int r = 3;
    for (size_t idx : noteIdx) {
        const auto& n = notesModule->GetAllNotes()[idx];

        // РћС‚СЃРµРєР°РµРј РїРѕ X РґРёР°РїР°Р·РѕРЅСѓ (РёРЅРґРµРєСЃ РєР°РґСЂР°)
        if (n.xIndex < 0) continue;
        if (n.xIndex < (int)floor(minXData) - 1) continue;
        if (n.xIndex > (int)ceil(maxXData) + 1) continue;

        const bool preferredRightAxis = n.hasYRight && n.anchorAxisGroup == 1 && hasRightDrawRange && yRangeRight > 0.0;

        auto TryResolveMarkerScreenY = [&](bool useRightAxis, int& outY) -> bool {
            const bool canUseRightAxis = n.hasYRight && hasRightDrawRange && yRangeRight > 0.0;
            const double anchorY = (useRightAxis && canUseRightAxis) ? n.yRight : n.yLeft;
            const double drawMinY = (useRightAxis && canUseRightAxis) ? drawMinY2 : drawMinY1;
            const double drawMaxY = (useRightAxis && canUseRightAxis) ? drawMaxY2 : drawMaxY1;
            const double yRange = (useRightAxis && canUseRightAxis) ? yRangeRight : yRangeLeft;
            if (!(yRange > 0.0)) {
                return false;
            }

            double yRatio = (drawMaxY - anchorY) / yRange;
            outY = dataRect.top + (int)lround(yRatio * dataH);
            return outY >= dataRect.top && outY <= dataRect.bottom;
        };

        int x = 0;
        if (!TryResolveNoteMarkerScreenX(*dataModule, config.xAxisType, n.xIndex,
                                         minXData, maxXData, xRange,
                                         minTemp, maxTemp, hasTempRange, dataSize,
                                         plotRect, plotW, x)) {
            continue;
        }

        int y = 0;
        bool yResolved = TryResolveMarkerScreenY(preferredRightAxis, y);
        if (!yResolved && n.hasYRight) {
            yResolved = TryResolveMarkerScreenY(!preferredRightAxis, y);
        }
        if (!yResolved) {
            continue;
        }

        if (x < plotRect.left || x > plotRect.right || y < dataRect.top || y > dataRect.bottom) {
            continue;
        }

        Ellipse(hdc, x - r, y - r, x + r + 1, y + r + 1);

        NotesModule::RenderedMarker m;
        m.noteIndex = idx;
        m.chartNumber = chartNumber;
        m.screenX = x;
        m.screenY = y;
        noteFrameMarkers.push_back(m);

        if (labelCandidates.size() < 8) {
            NoteLabelCandidate candidate;
            candidate.anchorX = x;
            candidate.anchorY = y;
            candidate.text = BuildNotePreviewText(n.text);
            labelCandidates.push_back(candidate);
        }
    }

    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);

    if (labelCandidates.empty()) {
        return;
    }

    // РљРѕРјРїР°РєС‚РЅС‹Рµ С‚РµРєСЃС‚РѕРІС‹Рµ РїР»Р°С€РєРё РІРѕР·Р»Рµ С‚РѕС‡РµРє Р·Р°РјРµС‚РѕРє (С‚РѕР»СЊРєРѕ РїРµСЂРІС‹Рµ 8).
    constexpr int kLabelWidth = 130;
    constexpr int kLabelHeight = 18;
    constexpr int kOffsetX = 7;
    constexpr int kOffsetY = 8;
    constexpr int kInnerPadX = 5;

    HFONT hLabelFont = CreateFont(
        -11, 0, 0, 0,
        FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        L"Segoe UI"
    );
    HFONT hOldFont = (HFONT)SelectObject(hdc, hLabelFont);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(30, 30, 30));

    HPEN hLabelPen = CreatePen(PS_SOLID, 1, RGB(145, 145, 145));
    HBRUSH hLabelBrush = CreateSolidBrush(RGB(255, 255, 240));
    HPEN hOldLabelPen = (HPEN)SelectObject(hdc, hLabelPen);
    HBRUSH hOldLabelBrush = (HBRUSH)SelectObject(hdc, hLabelBrush);

    std::vector<RECT> drawnLabelRects;
    drawnLabelRects.reserve(labelCandidates.size());

    for (const auto& candidate : labelCandidates) {
        RECT labelRect;
        labelRect.left = candidate.anchorX + kOffsetX;
        labelRect.right = labelRect.left + kLabelWidth;
        labelRect.bottom = candidate.anchorY - kOffsetY;
        labelRect.top = labelRect.bottom - kLabelHeight;

        // Р•СЃР»Рё РЅРµ РїРѕРјРµС‰Р°РµС‚СЃСЏ СЃРІРµСЂС…Сѓ вЂ” РїРµСЂРµРЅРѕСЃРёРј РІРЅРёР· РѕС‚ РјР°СЂРєРµСЂР°.
        if (labelRect.top < dataRect.top) {
            labelRect.top = candidate.anchorY + kOffsetY;
            labelRect.bottom = labelRect.top + kLabelHeight;
        }

        // РћРіСЂР°РЅРёС‡РёРІР°РµРј РїСЂСЏРјРѕСѓРіРѕР»СЊРЅРёРє РѕР±Р»Р°СЃС‚СЊСЋ РґР°РЅРЅС‹С…, С‡С‚РѕР±С‹ РЅРµ РїРµСЂРµРєСЂС‹РІР°С‚СЊ РѕСЃРё/Р»РµРіРµРЅРґСѓ.
        if (labelRect.right > dataRect.right) {
            int dx = labelRect.right - dataRect.right;
            labelRect.left -= dx;
            labelRect.right -= dx;
        }
        if (labelRect.left < dataRect.left) {
            int dx = dataRect.left - labelRect.left;
            labelRect.left += dx;
            labelRect.right += dx;
        }
        if (labelRect.bottom > dataRect.bottom) {
            int dy = labelRect.bottom - dataRect.bottom;
            labelRect.top -= dy;
            labelRect.bottom -= dy;
        }

        // РџСЂРѕСЃС‚РµР№С€Р°СЏ Р·Р°С‰РёС‚Р° РѕС‚ РЅР°Р»РѕР¶РµРЅРёСЏ: СЃРґРІРёРіР°РµРј РІРЅРёР· РїРѕ С€Р°РіСѓ, РёРЅР°С‡Рµ РїСЂРѕРїСѓСЃРєР°РµРј.
        bool placed = false;
        for (int attempt = 0; attempt < 4 && !placed; ++attempt) {
            RECT testRect = labelRect;
            if (attempt > 0) {
                const int shift = attempt * (kLabelHeight + 2);
                testRect.top += shift;
                testRect.bottom += shift;
                if (testRect.bottom > dataRect.bottom) {
                    continue;
                }
            }

            bool overlaps = false;
            for (const auto& existing : drawnLabelRects) {
                RECT inter;
                if (IntersectRect(&inter, &testRect, &existing)) {
                    overlaps = true;
                    break;
                }
            }

            if (!overlaps) {
                labelRect = testRect;
                placed = true;
            }
        }

        if (!placed) {
            continue;
        }

        Rectangle(hdc, labelRect.left, labelRect.top, labelRect.right, labelRect.bottom);

        RECT textRect = labelRect;
        textRect.left += kInnerPadX;
        textRect.right -= kInnerPadX;
        DrawText(hdc, candidate.text.c_str(), -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);

        drawnLabelRects.push_back(labelRect);
    }

    SelectObject(hdc, hOldLabelBrush);
    SelectObject(hdc, hOldLabelPen);
    SelectObject(hdc, hOldFont);
    DeleteObject(hLabelBrush);
    DeleteObject(hLabelPen);
    DeleteObject(hLabelFont);
}


// === РњРµС‚РѕРґС‹ РґР»СЏ СЂР°Р±РѕС‚С‹ СЃ COM-РїРѕСЂС‚РѕРј (v2.13.0) ===

void ChartManager::HandleSerialConnect(HWND hWnd) {
    // v2.13.0: РџРѕРґРєР»СЋС‡РµРЅРёРµ С‚РµРїРµСЂСЊ РїСЂРѕРёСЃС…РѕРґРёС‚ С‡РµСЂРµР· РґРёР°Р»РѕРі IDD_SERIAL_STATUS
    // Р­С‚РѕС‚ РјРµС‚РѕРґ РѕСЃС‚Р°РІР»РµРЅ РґР»СЏ СЃРѕРІРјРµСЃС‚РёРјРѕСЃС‚Рё СЃ РјРµРЅСЋ, РЅРѕ РЅРµ РёСЃРїРѕР»СЊР·СѓРµС‚СЃСЏ РЅР°РїСЂСЏРјСѓСЋ
    // Р’РјРµСЃС‚Рѕ СЌС‚РѕРіРѕ РѕС‚РєСЂС‹РІР°РµС‚СЃСЏ РґРёР°Р»РѕРі С‡РµСЂРµР· IDM_DEVICE_CONNECT
}

void ChartManager::HandleSerialDisconnect(HWND hWnd) {
    if (!serialModule) return;
    
    // РћСЃС‚Р°РЅР°РІР»РёРІР°РµРј РѕРїСЂРѕСЃ РїРµСЂРµРґ РѕС‚РєР»СЋС‡РµРЅРёРµРј
    if (serialModule->IsPolling()) {
        serialModule->StopPolling();
    }
    
    serialModule->Disconnect();
    
    if (uiModule) {
        uiModule->SetSerialConnected(false);
        uiModule->SetSerialPolling(false);
    }
    
    UpdateSerialUI(hWnd);
}

void ChartManager::HandleSerialStart(HWND hWnd) {
    if (!serialModule || !serialModule->IsConnected()) return;
    
    // Р—Р°РїСѓСЃРєР°РµРј РїРµСЂРёРѕРґРёС‡РµСЃРєРёР№ РѕРїСЂРѕСЃ (РєР°Р¶РґС‹Рµ 2 СЃРµРєСѓРЅРґС‹)
    // РџСЂРёРјРµС‡Р°РЅРёРµ: РћС‡РёСЃС‚РєР° РґР°РЅРЅС‹С… РїСЂРѕРёСЃС…РѕРґРёС‚ РІ РґРёР°Р»РѕРіРµ SerialStatusDialogProc
    // РїРµСЂРµРґ РІС‹Р·РѕРІРѕРј StartPolling С‡РµСЂРµР· HandleSerialClear
    serialModule->StartPolling(hWnd, 2000);
    
    if (uiModule) {
        uiModule->SetSerialPolling(true);
    }
}

void ChartManager::HandleSerialStop(HWND hWnd) {
    if (!serialModule) return;
    
    serialModule->StopPolling();
    
    if (uiModule) {
        uiModule->SetSerialPolling(false);
    }

    // РџРѕСЃР»Рµ РѕСЃС‚Р°РЅРѕРІРєРё realtime СЃРёРЅС…СЂРѕРЅРёР·РёСЂСѓРµРј X Range СЃ С‚РµРєСѓС‰РёРј РїРѕР»РЅС‹Рј РґРёР°РїР°Р·РѕРЅРѕРј.
    UpdateXRangeUI(hWnd);
}

void ChartManager::HandleSerialClear(HWND hWnd) {
    if (!dataModule) return;
    
    // РћС‡РёС‰Р°РµРј РґР°РЅРЅС‹Рµ
    dataModule->ClearData();
    InvalidateCachedSnapshots();
    ResetCoordinateTracking();

    // v2.16.x: РїСЂРё РїРµСЂРµС…РѕРґРµ Рє COM-СЂРµР¶РёРјСѓ РЅРµ РґРѕР»Р¶РЅС‹ РїРёСЃР°С‚СЊ Р·Р°РјРµС‚РєРё РІ sidecar РїРѕСЃР»РµРґРЅРµРіРѕ РѕС‚РєСЂС‹С‚РѕРіРѕ С„Р°Р№Р»Р°
    currentDataFilePath.clear();
    
    // РЎР±СЂР°СЃС‹РІР°РµРј СЃС‚Р°С‚РёСЃС‚РёРєСѓ
    if (serialModule) {
        serialModule->ResetStatistics();
    }
    
    // РЎР±СЂР°СЃС‹РІР°РµРј zoom
    chart1Zoom.isZoomed = false;
    chart2Zoom.isZoomed = false;
    chart1Zoom.isXCustom = false;
    chart2Zoom.isXCustom = false;
    chart1Zoom.hasXRangeLock = false;
    chart2Zoom.hasXRangeLock = false;
    chart1Zoom.isXToLatest = false;
    chart2Zoom.isXToLatest = false;
    chart1Zoom.isYCustom = false;
    chart2Zoom.isYCustom = false;
    chart1ZoomHistory.clear();
    chart2ZoomHistory.clear();
    ClearAxisHistory(1);
    ClearAxisHistory(2);
    
    // РћР±РЅРѕРІР»СЏРµРј UI
    if (uiModule) {
        uiModule->UpdateSerialFrameInfo(0, 0, 0);
        uiModule->ResetXRangeControls(1);
        uiModule->ResetXRangeControls(2);
    }
    
    // РџРµСЂРµСЂРёСЃРѕРІС‹РІР°РµРј РіСЂР°С„РёРєРё
    InvalidateRect(hWnd, nullptr, TRUE);
    
    UpdateStatusBarFileInfo();
}

void ChartManager::HandleSerialTimer(HWND hWnd) {
    if (!serialModule) return;
    
    // Р’С‹Р·С‹РІР°РµРј РѕР±СЂР°Р±РѕС‚С‡РёРє С‚Р°Р№РјРµСЂР° РІ SerialModule
    serialModule->OnTimer();
}

void ChartManager::UpdateSerialUI(HWND hWnd) {
    // v2.13.0: UI РѕР±РЅРѕРІР»СЏРµС‚СЃСЏ С‡РµСЂРµР· РґРёР°Р»РѕРі IDD_SERIAL_STATUS
    // Р­С‚РѕС‚ РјРµС‚РѕРґ РѕСЃС‚Р°РІР»РµРЅ РґР»СЏ СЃРѕРІРјРµСЃС‚РёРјРѕСЃС‚Рё, РЅРѕ РЅРµ РІС‹РїРѕР»РЅСЏРµС‚ РґРµР№СЃС‚РІРёР№
}

void ChartManager::RefreshPortList(HWND hWnd) {
    // v2.13.0: РЎРїРёСЃРѕРє РїРѕСЂС‚РѕРІ РѕР±РЅРѕРІР»СЏРµС‚СЃСЏ С‡РµСЂРµР· РґРёР°Р»РѕРі IDD_SERIAL_STATUS
    // Р­С‚РѕС‚ РјРµС‚РѕРґ РѕСЃС‚Р°РІР»РµРЅ РґР»СЏ СЃРѕРІРјРµСЃС‚РёРјРѕСЃС‚Рё, РЅРѕ РЅРµ РІС‹РїРѕР»РЅСЏРµС‚ РґРµР№СЃС‚РІРёР№
}

bool ChartManager::IsSerialConnected() const {
    return serialModule && serialModule->IsConnected();
}

bool ChartManager::IsSerialPolling() const {
    return serialModule && serialModule->IsPolling();
}

// === РњРµС‚РѕРґС‹ РґР»СЏ СЂРµР¶РёРјР° Surf (v2.14.0) ===

void ChartManager::HandleSurfToggle(HWND hWnd, int chartNumber) {
    if (!dataModule || !dataModule->HasData()) {
        return;
    }
    
    bool surfEnabled = uiModule->IsSurfEnabled(chartNumber);
    
    if (surfEnabled) {
        // Surf РІРєР»СЋС‡С‘РЅ - СѓСЃС‚Р°РЅР°РІР»РёРІР°РµРј РґРёР°РїР°Р·РѕРЅ РЅР° РїРѕСЃР»РµРґРЅРёРµ N С‚РѕС‡РµРє
        // Рё СЃР±СЂР°СЃС‹РІР°РµРј РїРѕР»СЊР·РѕРІР°С‚РµР»СЊСЃРєРёР№ Y-zoom (Surf РЅР°С‡РёРЅР°РµС‚ СЃ Р°РІС‚Рѕ-РјР°СЃС€С‚Р°Р±Р° РїРѕ Y)
        ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
        zoom->isXCustom = false;
        zoom->hasXRangeLock = false;
        zoom->isXToLatest = false;
        zoom->isYCustom = false;
        UpdateSurfRange(chartNumber);
    } else {
        // Surf РІС‹РєР»СЋС‡С‘РЅ - СЃР±СЂР°СЃС‹РІР°РµРј zoom Рє РїРѕР»РЅРѕРјСѓ РґРёР°РїР°Р·РѕРЅСѓ
        ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
        zoom->isZoomed = false;
        zoom->isXCustom = false;
        zoom->hasXRangeLock = false;
        zoom->isXToLatest = false;
        zoom->isYCustom = false;
        ClearAxisHistory(chartNumber);
        
        // РќРµ Р·Р°РїРѕР»РЅСЏРµРј РїРѕР»СЏ X Range Р°РІС‚РѕРјР°С‚РёС‡РµСЃРєРё вЂ” С‚РѕР»СЊРєРѕ РѕС‡РёС‰Р°РµРј РёС…
        uiModule->ResetXRangeControls(chartNumber);
    }
    
    // РћР±РЅРѕРІР»СЏРµРј СЃС‚Р°С‚СѓСЃРЅСѓСЋ СЃС‚СЂРѕРєСѓ
    UpdateSurfStatusBar();
    
    // РџРµСЂРµСЂРёСЃРѕРІС‹РІР°РµРј РіСЂР°С„РёРє
    InvalidateChart(hWnd, chartNumber);
    UpdateScrollBars(hWnd);
}

void ChartManager::UpdateSurfRange(int chartNumber) {
    if (!dataModule || !dataModule->HasData()) {
        return;
    }
    
    if (!uiModule->IsSurfEnabled(chartNumber)) {
        return;
    }
    
    size_t dataSize = dataModule->GetDataSize();
    int surfWindow = uiModule->GetSurfWindowSize(chartNumber);
    
    double surfMinX = (dataSize > static_cast<size_t>(surfWindow)) 
        ? static_cast<double>(dataSize - surfWindow) : 0.0;
    double surfMaxX = static_cast<double>(dataSize - 1);
    
    // РЈСЃС‚Р°РЅР°РІР»РёРІР°РµРј zoom РЅР° РѕР±Р»Р°СЃС‚СЊ Surf (С‚РѕР»СЊРєРѕ РїРѕ X)
    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
    zoom->minX = surfMinX;
    zoom->maxX = surfMaxX;
    zoom->isXCustom = false;
    zoom->hasXRangeLock = false;
    zoom->isXToLatest = false;
    zoom->isZoomed = true;
    
    // Surf ON + РїРѕР»СЊР·РѕРІР°С‚РµР»СЊСЃРєРёР№ Y-zoom:
    // X РѕРєРЅРѕ РґРІРёРіР°РµС‚СЃСЏ, РЅРѕ Y РґРёР°РїР°Р·РѕРЅ РЅРµ РїРµСЂРµР·Р°С‚РёСЂР°РµРј РїСЂРё РїСЂРёС…РѕРґРµ РЅРѕРІС‹С… С‚РѕС‡РµРє.
    if (!zoom->isYCustom) {
        // Р’С‹С‡РёСЃР»СЏРµРј РґРёР°РїР°Р·РѕРЅ Y РґР»СЏ РґР°РЅРЅС‹С… РІ РѕРєРЅРµ Surf (Р°РІС‚Рѕ-РјР°СЃС€С‚Р°Р±)
        ChartConfiguration config = curveSelectionModule->GetChartConfiguration(chartNumber);
        std::vector<std::vector<float>> leftAxisData, rightAxisData;
        std::vector<CurveDefinition> leftAxisCurves, rightAxisCurves;
        
        // РџРµСЂРµРґР°С‘Рј Р±РѕР»СЊС€РѕР№ РґРёР°РїР°Р·РѕРЅ Y, С‡С‚РѕР±С‹ РЅРµ С„РёР»СЊС‚СЂРѕРІР°С‚СЊ РїРѕ Y
        curveSelectionModule->CalculateCurveData(config, dataModule.get(),
                                                leftAxisData, rightAxisData,
                                                leftAxisCurves, rightAxisCurves,
                                                surfMinX, surfMaxX, -1e30, 1e30);
        
        double minX, maxX, minYData, maxYData, minY2, maxY2;
        chartModule->GetDataRanges(leftAxisData, rightAxisData, minX, maxX, minYData, maxYData, minY2, maxY2);
        
        double yRange = maxYData - minYData;
        double padding = yRange * 0.05;
        zoom->minY = minYData - padding;
        zoom->maxY = maxYData + padding;
        ClearExplicitRightAxisRange(zoom->minYRight, zoom->maxYRight, zoom->hasRightAxisCustomRange);
    }
    
    // Р’ Surf СЂРµР¶РёРјРµ РЅРµ Р°РІС‚РѕР·Р°РїРѕР»РЅСЏРµРј РїРѕР»СЏ X Range (From/To),
    // С‡С‚РѕР±С‹ РЅРµ РїРµСЂРµС‚РёСЂР°С‚СЊ РїРѕР»СЊР·РѕРІР°С‚РµР»СЊСЃРєРёР№ РІРІРѕРґ.
    uiModule->SetXRangeResetEnabled(chartNumber, false);
}

void ChartManager::UpdateSurfStatusBar() {
    bool surf1Enabled = uiModule->IsSurfEnabled(1);
    int surf1Window = uiModule->GetSurfWindowSize(1);
    bool surf2Enabled = uiModule->IsSurfEnabled(2);
    int surf2Window = uiModule->GetSurfWindowSize(2);
    
    uiModule->UpdateStatusBarSurfInfo(surf1Enabled, surf1Window, surf2Enabled, surf2Window);
}


