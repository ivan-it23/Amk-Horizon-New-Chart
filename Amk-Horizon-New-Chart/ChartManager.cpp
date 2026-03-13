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

// === Заметки: диалог просмотра/удаления (v2.16.x) ===
namespace {
    struct NoteViewDialogContext {
        std::wstring text;
        std::string noteId;
        bool deleteRequested = false;
        bool saveRequested = false;
    };

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

    bool ShouldSnapNoteToCurve(const ChartConfiguration& config) {
        switch (config.activePreset) {
        case PresetType::PHASE_DIFFERENCES:
        case PresetType::SILENT_PHASE_DIFFERENCE:
            return false;
        default:
            return true;
        }
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

// Реализация класса ChartManager с модульной архитектурой

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
    
    // Устанавливаем связь между модулями
    schematicModule->SetCurveSelectionModule(curveSelectionModule.get());
    curveSelectionModule->SetMetrologyModule(metrologyModule.get());
    
    // Устанавливаем callback для обновления графика после применения настроек (v2.9.4)
    schematicModule->SetOnConfigurationAppliedCallback([this](int chartNumber) {
        // Получаем handle главного окна из родительского окна диалога
        HWND hSchematicDlg = schematicModule->GetSchematicDialogHandle();
        if (hSchematicDlg) {
            HWND hMainWnd = GetParent(hSchematicDlg);
            if (hMainWnd) {
                // Сбрасываем X Range при смене функции/пресета
                if (dataModule && dataModule->HasData()) {
                    uiModule->ResetXRangeControls(chartNumber);
                    
                    // Сбрасываем zoom для соответствующего графика
                    if (chartNumber == 1) {
                        chart1Zoom.isZoomed = false;
                        chart1Zoom.isXCustom = false;
                        chart1Zoom.isXToLatest = false;
                        chart1Zoom.isYCustom = false;
                        chart1ZoomHistory.clear(); // v2.12.5: Очищаем историю zoom
                        ClearAxisHistory(1);
                    } else if (chartNumber == 2) {
                        chart2Zoom.isZoomed = false;
                        chart2Zoom.isXCustom = false;
                        chart2Zoom.isXToLatest = false;
                        chart2Zoom.isYCustom = false;
                        chart2ZoomHistory.clear(); // v2.12.5: Очищаем историю zoom
                        ClearAxisHistory(2);
                    }
                }
                
                // Перерисовываем соответствующий график
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

    // Если пользователь задал X Range через поля, не позволяем X-scroll выходить за эти границы.
    if (zoom->isXCustom && !zoom->isYCustom) {
        xLockMin = zoom->minX;
        xLockMax = zoom->maxX;
    } else {
        for (auto it = zoomHistory->rbegin(); it != zoomHistory->rend(); ++it) {
            if (it->isXCustom && !it->isYCustom) {
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

    // Определяем путь для заметок: либо открыт файл, либо активна запись COM
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
    const bool snapNoteToCurve = ShouldSnapNoteToCurve(config);

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

    // Координаты заметки: X берём из перекрестия.
    // Для большинства пресетов дополнительно привязываем Y к ближайшей кривой,
    // но для phase-difference пресетов сохраняем точные координаты перекрестия:
    // там много производных/рваных каналов, и snap-to-curve может уводить
    // заметку далеко от фактической точки, выбранной пользователем.
    int xIndex = static_cast<int>(llround(resolvedDataX));
    if (xIndex < 0) xIndex = 0;
    if (dataModule->HasData()) {
        int maxIdx = static_cast<int>(dataModule->GetDataSize() > 0 ? (dataModule->GetDataSize() - 1) : 0);
        if (xIndex > maxIdx) xIndex = maxIdx;
    }

    double y1 = resolvedDataY1;
    double y2 = resolvedDataY2;
    int anchorAxisGroup = 0;

    double leftDrawMin = 0.0;
    double leftDrawMax = 0.0;
    double rightDrawMin = 0.0;
    double rightDrawMax = 0.0;
    bool hasLeftDrawRange = false;
    bool hasRightDrawRange = false;

    {
        ZoomRange zoom = (chartNumber == 1) ? chart1Zoom : chart2Zoom;
        const CachedCurveSnapshot* visibleSnapshot = nullptr;
        if (TryGetCurveSnapshot(chartNumber, config, zoom, !ShouldUseAsyncCurveWarmup(), visibleSnapshot)) {
            double displayMinYLeft = 0.0;
            double displayMaxYLeft = 0.0;
            BuildDisplayedLeftAxisRange(zoom.isZoomed, zoom.minY, zoom.maxY,
                                        visibleSnapshot->minY1, visibleSnapshot->maxY1,
                                        displayMinYLeft, displayMaxYLeft);
            hasLeftDrawRange = TryBuildDrawnAxisRange(displayMinYLeft, displayMaxYLeft, leftDrawMin, leftDrawMax);

            if (hasDualAxis && !visibleSnapshot->rightAxisData.empty()) {
                double displayMinYRight = visibleSnapshot->minY2;
                double displayMaxYRight = visibleSnapshot->maxY2;
                BuildDisplayedRightAxisRange(zoom.isYCustom,
                                             zoom.hasRightAxisCustomRange,
                                             zoom.minYRight,
                                             zoom.maxYRight,
                                             visibleSnapshot->minY1,
                                             visibleSnapshot->maxY1,
                                             visibleSnapshot->minY2,
                                             visibleSnapshot->maxY2,
                                             displayMinYLeft,
                                             displayMaxYLeft,
                                             displayMinYRight,
                                             displayMaxYRight);
                hasRightDrawRange = TryBuildDrawnAxisRange(displayMinYRight, displayMaxYRight, rightDrawMin, rightDrawMax);
            }
        }
    }

    // Snap-to-curve: вычисляем данные кривых (полный диапазон, без zoom) и находим
    // ближайшее к выбранному Y значение кривой в кадре xIndex.
    if (curveSelectionModule) {
        const CachedCurveSnapshot* fullRangeSnapshot = nullptr;
        ZoomRange unzoomedRange = {};
        std::vector<std::vector<float>> fallbackLeftAxisData;
        std::vector<std::vector<float>> fallbackRightAxisData;
        const std::vector<std::vector<float>>* leftAxisData = nullptr;
        const std::vector<std::vector<float>>* rightAxisData = nullptr;
        if (TryGetCurveSnapshot(chartNumber, config, unzoomedRange, true, fullRangeSnapshot)) {
            leftAxisData = &fullRangeSnapshot->leftAxisData;
            rightAxisData = &fullRangeSnapshot->rightAxisData;
        } else {
            std::vector<CurveDefinition> leftAxisCurves, rightAxisCurves;
            curveSelectionModule->CalculateCurveData(config, dataModule.get(),
                                                     fallbackLeftAxisData, fallbackRightAxisData,
                                                     leftAxisCurves, rightAxisCurves);
            leftAxisData = &fallbackLeftAxisData;
            rightAxisData = &fallbackRightAxisData;
        }

        bool hasLeftCandidate = false;
        double bestLeftDist = 1e30;
        double bestLeftVal = y1;
        for (const auto& channel : *leftAxisData) {
            if (xIndex < 0 || static_cast<size_t>(xIndex) >= channel.size()) {
                continue;
            }
            double v = static_cast<double>(channel[static_cast<size_t>(xIndex)]);
            if (!std::isfinite(v)) {
                continue;
            }
            double d = fabs(v - resolvedDataY1);
            if (d < bestLeftDist) {
                bestLeftDist = d;
                bestLeftVal = v;
                hasLeftCandidate = true;
            }
        }
        if (snapNoteToCurve && hasLeftCandidate) {
            y1 = bestLeftVal;
        }

        // Правая ось
        hasDualAxis = rightAxisData && !rightAxisData->empty();
        bool hasRightCandidate = false;
        double bestRightDist = 1e30;
        double bestRightVal = y2;
        if (hasDualAxis) {
            for (const auto& channel : *rightAxisData) {
                if (xIndex < 0 || static_cast<size_t>(xIndex) >= channel.size()) {
                    continue;
                }
                double v = static_cast<double>(channel[static_cast<size_t>(xIndex)]);
                if (!std::isfinite(v)) {
                    continue;
                }
                double d = fabs(v - resolvedDataY2);
                if (d < bestRightDist) {
                    bestRightDist = d;
                    bestRightVal = v;
                    hasRightCandidate = true;
                }
            }
            if (snapNoteToCurve && hasRightCandidate) {
                y2 = bestRightVal;
            }
        }

        if (hasDualAxis && hasRightCandidate) {
            const double leftRange = hasLeftDrawRange ? (leftDrawMax - leftDrawMin) : 1.0;
            const double rightRange = hasRightDrawRange ? (rightDrawMax - rightDrawMin) : 1.0;
            const double leftNormDist = hasLeftCandidate ? (bestLeftDist / ((leftRange > 1e-9) ? leftRange : 1.0)) : 1e30;
            const double rightNormDist = bestRightDist / ((rightRange > 1e-9) ? rightRange : 1.0);
            if (rightNormDist < leftNormDist) {
                anchorAxisGroup = 1;
            }
        }

        // Единицы измерения
        if (!config.curves.empty()) {
            std::string units1 = config.curves[0].units;
            y1Unit = std::wstring(units1.begin(), units1.end());
        }
        if (hasDualAxis) {
            for (const auto& curve : config.curves) {
                if (curve.yAxisGroup == 1) {
                    std::string units2 = curve.units;
                    y2Unit = std::wstring(units2.begin(), units2.end());
                    break;
                }
            }
        }
    }

    AppendDebugLog(L"[AddNoteFinal] chart=%d preset=%d snap=%d stored=(xIndex %d y1 %.3f y2 %.3f anchor=%d)\r\n",
                   chartNumber,
                   static_cast<int>(config.activePreset),
                   snapNoteToCurve ? 1 : 0,
                   xIndex,
                   y1,
                   y2,
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
    
    // v2.13.0: Устанавливаем callback для получения данных с прибора
    // Элементы управления COM-портом теперь в меню "Прибор" и диалоге IDD_SERIAL_STATUS
    serialModule->SetParsedDataCallback([this, hWnd](const ParsedToolFrame& parsedData) {
        // v2.16.x: если идёт запись в файл, привязываем заметки к текущему recordFilePath
        if (notesModule && serialModule && serialModule->IsRecordingEnabled() && serialModule->IsRecordingActive()) {
            std::wstring recordPath = serialModule->GetRecordFilePath();
            if (!recordPath.empty()) {
                EnsureNotesLoadedForPath(recordPath + L".notes.jsonl");
            }
        }

        // Добавляем данные в DataModule
        dataModule->AddParsedData(parsedData);
        InvalidateCachedSnapshots();
        
        // v2.14.0: Обновляем диапазон для режима Surf
        if (dataModule->HasData()) {
            // ВАЖНО: при поступлении новых точек в режиме реального времени
            // НЕ сбрасываем пользовательский zoom/кастомный X-range.
            //
            // Surf (auto-follow) по-прежнему обновляет диапазон на последние N точек.
            if (uiModule->IsSurfEnabled(1)) {
                UpdateSurfRange(1);
            }

            if (uiModule->IsDualMode() && uiModule->IsSurfEnabled(2)) {
                UpdateSurfRange(2);
            }

            // Обновляем X Range controls из текущего состояния zoom
            // (или полного диапазона, если zoom не активен).
            UpdateXRangeUI(hWnd);

            // Обновляем статусную строку с информацией о Surf
            UpdateSurfStatusBar();

            // Обновляем скроллбары (total range меняется при росте данных)
            UpdateScrollBars(hWnd);
        }
        
        // Перерисовываем графики
        InvalidateChart(hWnd, 1);
        if (uiModule->IsDualMode()) {
            InvalidateChart(hWnd, 2);
        }
    });
    
    // Устанавливаем callback для изменения состояния подключения
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
    
    // Валидация ограничений компоновки
    if (!layoutModule->ValidateLayoutConstraints(width, height, dualMode))
        return;
    
    // Расчет компоновки
    layoutModule->CalculateLayout(width, height, dualMode);
    
    // Управление видимостью и позиционированием элементов управления
    uiModule->UpdateControlsVisibility(dualMode);
    
    if (dualMode) {
        int panel2Y = layoutModule->GetChart2PanelY();
        uiModule->PositionChart2Controls(panel2Y);
        // Позиционируем элементы координат к правому краю для обоих графиков
        uiModule->PositionCursorControls(width, panel2Y);
    } else {
        // Позиционируем элементы координат к правому краю только для Chart 1
        uiModule->PositionCursorControls(width, 0);
    }

    // Позиционируем компактные чекбоксы отображения заметок в правом верхнем углу графиков
    uiModule->PositionNotesVisibilityControls(layoutModule->GetChart1Rect(), layoutModule->GetChart2Rect(), dualMode);
    
    // Обновляем скроллбары после изменения компоновки
    UpdateScrollBars(hWnd);
    
    // Обновляем элементы управления X Range
    UpdateXRangeUI(hWnd);
    
    // Обновляем размер статусной строки
    uiModule->ResizeStatusBar();
}

void ChartManager::ToggleDualMode()
{
    uiModule->ToggleDualMode();
}

void ChartManager::RefreshChart1()
{
    // Метод оставлен для совместимости API, но больше ничего не делает
    // Настройки управляются через Device Schematic
}

void ChartManager::RefreshChart2()
{
    // Метод оставлен для совместимости API, но больше ничего не делает
    // Настройки управляются через Device Schematic
}

void ChartManager::DrawChart1(HDC hdc)
{
    // v2.16.x: начинаем новый набор маркеров заметок на кадр отрисовки
    noteFrameMarkers.clear();

    const RECT& chart1Rect = layoutModule->GetChart1Rect();
    
    // Проверяем, есть ли конфигурация кривых для первого графика
    if (IsUsingNewCurveSystem(1)) {
        // Используем новую систему кривых
        DrawChartWithCurves(hdc, chart1Rect, 1);
    } else {
        // Используем старую систему (для обратной совместимости)
        DrawChartLegacy(hdc, chart1Rect, 1);
    }

    // v2.16.x: Отрисовываем точки заметок поверх графика (если включено)
    if (uiModule->IsNotesVisible(1)) {
        DrawNoteMarkers(hdc, chart1Rect, 1);
    }

    // Отрисовываем перекрестие поверх графика
    DrawCrosshair(hdc, chart1Rect, 1);

    // Если второй график не рисуется - фиксируем маркеры для hit-test
    if (!uiModule->IsDualMode() && notesModule) {
        notesModule->SetRenderedMarkers(std::move(noteFrameMarkers));
    }
}

void ChartManager::DrawChart2(HDC hdc)
{
    if (!uiModule->IsDualMode())
        return;
        
    const RECT& chart2Rect = layoutModule->GetChart2Rect();
    
    // Проверяем, есть ли конфигурация кривых для второго графика
    if (IsUsingNewCurveSystem(2)) {
        // Используем новую систему кривых
        DrawChartWithCurves(hdc, chart2Rect, 2);
    } else {
        // Используем старую систему (для обратной совместимости)
        DrawChartLegacy(hdc, chart2Rect, 2);
    }

    // v2.16.x: Отрисовываем точки заметок поверх графика (если включено)
    if (uiModule->IsNotesVisible(2)) {
        DrawNoteMarkers(hdc, chart2Rect, 2);
    }

    // Отрисовываем перекрестие поверх графика
    DrawCrosshair(hdc, chart2Rect, 2);

    // В dual-mode после отрисовки обоих графиков фиксируем маркеры для hit-test
    if (notesModule) {
        notesModule->SetRenderedMarkers(std::move(noteFrameMarkers));
    }
}

void ChartManager::LoadDataFromFile(HWND hWnd, const std::wstring& filename)
{
    // v2.14.1: При открытии файла останавливаем опрос COM-порта
    // Режимы работы с файлом и COM-портом взаимоисключающие
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
    
    // Обработка команд от диалогового окна схемы прибора
    switch (wmId) {
        case IDC_CHART1_SCHEMATIC_BTN:
            ShowSchematicDialog(hWnd, 1);
            return true;
            
        case IDC_CHART2_SCHEMATIC_BTN:
            ShowSchematicDialog(hWnd, 2);
            return true;
            
        case IDC_APPLY_SETTINGS:
            // Команда применения настроек из диалогового окна схемы
            // Обработка происходит в самом диалоговом окне
            return true;
            
        case IDC_PRESET_ALL_AMP_R1:
        case IDC_PRESET_ALL_AMP_R2:
        case IDC_PRESET_RATIOS:
        case IDC_PRESET_DIFFERENCES:
            // Команды пресетов обрабатываются в диалоговом окне схемы
            return true;
            
        case IDC_DUAL_MODE_CHECK:
            // Переключение режима двух графиков
            needsRedraw = uiModule->HandleCommand(wParam, lParam, hWnd);
            UpdateLayout(hWnd);
            return needsRedraw;
            
        case IDC_XRANGE_APPLY_BTN:
            // Применение пользовательского диапазона X для Chart 1
            HandleXRangeApply(hWnd, 1);
            return true;
            
        case IDC_XRANGE_RESET_BTN:
            // Сброс диапазона X к полному для Chart 1
            HandleXRangeReset(hWnd, 1);
            return true;
            
        case IDC_XRANGE2_APPLY_BTN:
            // Применение пользовательского диапазона X для Chart 2
            HandleXRangeApply(hWnd, 2);
            return true;
            
        case IDC_XRANGE2_RESET_BTN:
            // Сброс диапазона X к полному для Chart 2
            HandleXRangeReset(hWnd, 2);
            return true;
            
        case IDC_RESET_ZOOM1_BTN:
            // Сброс zoom для Chart 1 (аналогично правому клику мыши)
            HandleResetZoom(hWnd, 1);
            return true;
            
        case IDC_RESET_ZOOM2_BTN:
            // Сброс zoom для Chart 2 (аналогично правому клику мыши)
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
            
        // === Команды COM-порта (v2.13.0) ===
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
            
        // === Команды режима Surf (v2.14.0) ===
        case IDC_CHART1_SURF_CHECK:
            HandleSurfToggle(hWnd, 1);
            return true;
            
        case IDC_CHART2_SURF_CHECK:
            HandleSurfToggle(hWnd, 2);
            return true;

        case IDC_CHART1_SHOW_NOTES_CHECK:
        case IDC_CHART2_SHOW_NOTES_CHECK:
            // Переключение отображения маркеров заметок
            return true;
            
        case IDC_CHART1_SURF_EDIT:
            // При изменении размера окна Surf для Chart 1
            if (HIWORD(wParam) == EN_CHANGE && uiModule->IsSurfEnabled(1)) {
                UpdateSurfRange(1);
                UpdateSurfStatusBar();
                InvalidateChart(hWnd, 1);
            }
            return true;
            
        case IDC_CHART2_SURF_EDIT:
            // При изменении размера окна Surf для Chart 2
            if (HIWORD(wParam) == EN_CHANGE && uiModule->IsSurfEnabled(2)) {
                UpdateSurfRange(2);
                UpdateSurfStatusBar();
                InvalidateChart(hWnd, 2);
            }
            return true;
            
        default:
            // Передаем остальные команды в UIModule
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
    // Локализованное отображение частоты в заголовке (кГц)
    title += (frequency == FrequencyBand::FREQ_400_KHZ) ? L"400 кГц" : L"2000 кГц";
    title += L")";
    return title;
}

void ChartManager::HandleLegendCheckboxClick(HWND hWnd, int x, int y, int chartId) {
    // Метод оставлен для совместимости API, но больше ничего не делает
    // Управление кривыми происходит только через Device Schematic
}

// Обработчики событий мыши
void ChartManager::HandleMouseDown(HWND hWnd, int x, int y) {
    if (!dataModule->HasData())
        return;

    RECT chart1Rect = layoutModule->GetChart1Rect();
    RECT chart2Rect = layoutModule->GetChart2Rect();

    int clickedChart = GetChartAtPoint(x, y);

    // Начинаем выделение только если клик был по графику
    if (clickedChart == 0) {
        return;
    }

    // v2.16.x: если кликнули по маркеру заметки — открываем заметку (с возможностью удаления)
    // и не начинаем zoom/selection
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
    // v2.14.2: Координаты и перекрестие обновляются в HandleMouseUp,
    // только если это был клик, а не drag (выделение области для zoom)

    isSelecting = true;
    selectionStart.x = x;
    selectionStart.y = y;
    selectionEnd.x = x;
    selectionEnd.y = y;

    // Готовим быстрый оверлей выделения (снимок области графика)
    RECT chartRect = (activeChart == 2) ? layoutModule->GetChart2Rect() : layoutModule->GetChart1Rect();
    BeginSelectionOverlay(hWnd, chartRect);

    SetCapture(hWnd);
}

void ChartManager::HandleMouseMove(HWND hWnd, int x, int y)
{
    if (!isSelecting)
        return;

    // Быстрая отрисовка выделения поверх уже нарисованного графика (без полного WM_PAINT)
    HDC hdc = GetDC(hWnd);
    if (!hdc)
        return;

    // Обновляем конечную точку выделения
    selectionEnd.x = x;
    selectionEnd.y = y;

    RECT newRect;
    newRect.left = (std::min)(selectionStart.x, selectionEnd.x);
    newRect.top = (std::min)(selectionStart.y, selectionEnd.y);
    newRect.right = (std::max)(selectionStart.x, selectionEnd.x);
    newRect.bottom = (std::max)(selectionStart.y, selectionEnd.y);

    // Область обновления = union(предыдущего нарисованного rect, нового rect)
    RECT updateRect = newRect;
    if (hasLastSelectionDrawRect) {
        UnionRect(&updateRect, &lastSelectionDrawRect, &newRect);
    }
    InflateRect(&updateRect, 2, 2);

    // Восстанавливаем фон по "снимку" и рисуем новый оверлей
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

    // Убираем оверлей: восстанавливаем фон и освобождаем ресурсы
    if (hasSelectionBase) {
        HDC hdc = GetDC(hWnd);
        if (hdc) {
            RECT restoreRect = selectionBaseRect;
            RestoreSelectionBackground(hdc, restoreRect);
            ReleaseDC(hWnd, hdc);
        }
    }
    EndSelectionOverlay();
    
    // Проверяем, что выделена достаточно большая область
    int deltaX = abs(selectionEnd.x - selectionStart.x);
    int deltaY = abs(selectionEnd.y - selectionStart.y);
    
    const bool surfEnabled = (activeChart != 0) ? uiModule->IsSurfEnabled(activeChart) : false;
    const bool isZoomGesture = surfEnabled ? (deltaY > 10) : (deltaX > 10 && deltaY > 10);
    
    if (isZoomGesture) {
        // Это zoom — скрываем перекрестие
        if (activeChart == 1) {
            chart1Crosshair.isVisible = false;
        } else if (activeChart == 2) {
            chart2Crosshair.isVisible = false;
        }
        
        // Преобразуем экранные координаты в координаты данных
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
        
        // Surf ON: zoom только по оси Y, X всегда фиксирован на окне Surf (последние N точек).
        if (surfEnabled) {
            // Фиксируем X диапазон на окно Surf (последние N точек)
            size_t dataSize = dataModule->GetDataSize();
            int surfWindow = uiModule->GetSurfWindowSize(activeChart);
            double surfMinX = (dataSize > static_cast<size_t>(surfWindow))
                ? static_cast<double>(dataSize - surfWindow) : 0.0;
            double surfMaxX = (dataSize > 0) ? static_cast<double>(dataSize - 1) : 0.0;
            
            // Проверяем минимальный диапазон для Y (чтобы не уходить в "бесконечный" zoom)
            double yRange = maxDataY - minDataY;
            if (yRange < MIN_Y_RANGE) {
                MessageBox(hWnd,
                    L"Cannot zoom further: Y-axis range must be at least 10 units.\nMinimum range is 10 units.",
                    L"Zoom Limit Reached",
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
                chart2Zoom.isXToLatest = false;
                chart2Zoom.isYCustom = true;
            }
        } else {
            // Обычный режим: zoom по X и Y
            double xRange = maxDataX - minDataX;
            if (xRange < MIN_X_RANGE) {
                MessageBox(hWnd,
                    L"Cannot zoom further: X-axis labels must be integers with minimum spacing of 1.\nMinimum range is 10 points.",
                    L"Zoom Limit Reached",
                    MB_OK | MB_ICONINFORMATION);
                return;
            }
            
            // v2.14.0: Ограничиваем zoom областью Surf, если режим Surf активен (на всякий случай)
            // (в этой ветке surfEnabled == false, но оставляем логику будущей совместимости)
            
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
                chart2Zoom.isYCustom = true;
            }
        }
    } else {
        // v2.14.2: Это клик (не drag) — обновляем координаты и перекрестие
        if (activeChart != 0) {
            double dataX, dataY1, dataY2;
            std::wstring y1Unit, y2Unit;
            bool hasDualAxis;
            
            if (ConvertScreenToChartCoords(activeChart, selectionStart.x, selectionStart.y, 
                                           dataX, dataY1, dataY2, y1Unit, y2Unit, hasDualAxis)) {
                uiModule->UpdateClickCoordinates(activeChart, dataX, dataY1, y1Unit, dataY2, y2Unit, hasDualAxis);
                
                // Сохраняем позицию перекрестия только если screen->chart конвертация успешна.
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
    
    // Обновляем скроллбары после изменения зума
    UpdateScrollBars(hWnd);
    
    // Перерисовываем только измененный график (избегаем мигания)
    if (activeChart > 0) {
        InvalidateChart(hWnd, activeChart);
    } else {
        InvalidateRect(hWnd, NULL, FALSE); // Если не определен график, перерисовываем всё (FALSE для double buffering)
    }
}

void ChartManager::HandleRightClick(HWND hWnd, int x, int y)
{
    (void)hWnd;
    (void)x;
    (void)y;
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

    // Получаем конфигурацию графика для определения типа оси X
    ChartConfiguration config = curveSelectionModule->GetChartConfiguration(chartNum);
    
    // Вычисляем область графика (plotRect) с учётом отступов
    // ВАЖНО: Всегда используем отступы для двойных осей, потому что DrawDualAxisChart
    // вызывается для всех графиков (даже с одной осью) и использует CalculateDualAxisPlotRect
    DualAxisChartGeometry geometry = {};
    if (!TryBuildDualAxisChartGeometry(*chartRect, geometry)) {
        dataX = 0.0;
        dataY = 0.0;
        return;
    }

    const RECT& plotRect = geometry.plotRect;
    const RECT& dataRect = geometry.dataRect;
    double plotWidth = static_cast<double>(geometry.plotWidth);

    // ВАЖНО: синхронизируем Y-геометрию с реальной отрисовкой в DrawDualAxisDataWithZoom,
    // где данные рисуются не по всему plotRect, а по dataRect с вертикальными margin.
    double dataHeight = static_cast<double>(geometry.dataHeight);
    if (!(dataHeight > 0.0)) {
        dataX = 0.0;
        dataY = 0.0;
        return;
    }

    double xRatio = static_cast<double>(screenX - plotRect.left) / plotWidth;
    if (xRatio < 0.0) xRatio = 0.0;
    if (xRatio > 1.0) xRatio = 1.0;

    // v2.10.0: Для кроссплота (температура на оси X) нужно преобразовать координаты по-другому
    VisibleFrameAxisRange visibleXRange = {};
    BuildVisibleFrameAxisRange(zoom->isZoomed, zoom->minX, zoom->maxX, dataModule->GetDataSize(), visibleXRange);
    const double linearDataX = visibleXRange.minX + xRatio * (visibleXRange.maxX - visibleXRange.minX);
    if (config.xAxisType == XAxisType::TEMPERATURE) {
        // В режиме кроссплота экранная X координата соответствует температуре
        // Но zoom всё равно работает с индексами кадров
        dataX = ResolveTemperatureAxisDataX(chartNum, *zoom, xRatio, linearDataX, !ShouldUseAsyncCurveWarmup());
    } else {
        // Обычный режим: номер кадра на оси X
        dataX = linearDataX;
    }
    
    // Y координата обрабатывается одинаково для обоих режимов
    // Получаем реальный диапазон Y из данных
    ChartConfiguration config2 = curveSelectionModule->GetChartConfiguration(chartNum);
    const CachedCurveSnapshot* curveSnapshot = nullptr;
    ZoomRange fullRangeSnapshot = {};
    if (!TryGetCurveSnapshot(chartNum, config2, fullRangeSnapshot, !ShouldUseAsyncCurveWarmup(), curveSnapshot)) {
        dataY = 0.0;
        return;
    }

    double displayMinY = 0.0;
    double displayMaxY = 0.0;
    BuildDisplayedLeftAxisRange(zoom->isZoomed, zoom->minY, zoom->maxY, curveSnapshot->minY1, curveSnapshot->maxY1, displayMinY, displayMaxY);

    // В DrawDualAxisDataWithZoom дополнительно применяется нижний padding 2%.
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


// Методы для обработки скроллинга
void ChartManager::HandleScrollEvent(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    HWND hScrollBar = (HWND)lParam;
    int scrollCode = LOWORD(wParam);
    
    // Определяем, какой скроллбар был использован
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
        return; // Неизвестный скроллбар
    }

    // Surf ON: X-скролл отключён (X всегда авто-следит за последними N точками).
    if (!isVertical && uiModule->IsSurfEnabled(chartNumber)) {
        return;
    }

    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
    std::vector<ZoomRange>* zoomHistory = (chartNumber == 1) ? &chart1ZoomHistory : &chart2ZoomHistory;

    const double fullMinX = 0.0;
    const double fullMaxX = static_cast<double>(dataModule->GetDataSize() - 1);
    double xLockMin = fullMinX;
    double xLockMax = fullMaxX;

    // Если пользователь задал X Range через поля, не позволяем X-scroll выходить за эти границы.
    if (zoom->isXCustom && !zoom->isYCustom) {
        xLockMin = zoom->minX;
        xLockMax = zoom->maxX;
    } else {
        for (auto it = zoomHistory->rbegin(); it != zoomHistory->rend(); ++it) {
            if (it->isXCustom && !it->isYCustom) {
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

    // Осевой undo: кнопка "-" на скроллбаре откатывает zoom только по соответствующей оси.
    // Для H-скролла это левая стрелка, для V-скролла — верхняя стрелка.
    if ((!isVertical && scrollCode == SB_LINELEFT) || (isVertical && scrollCode == SB_LINEUP)) {
        AxisZoomRange prevAxis;

        if (isVertical) {
            if (PopAxisHistoryY(chartNumber, prevAxis)) {
                zoom->minY = prevAxis.min;
                zoom->maxY = prevAxis.max;
                zoom->isYCustom = prevAxis.isCustom;
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
            if (PopAxisHistoryX(chartNumber, prevAxis)) {
                if (prevAxis.isCustom) {
                    zoom->minX = prevAxis.min;
                    zoom->maxX = prevAxis.max;
                    zoom->isXCustom = true;
                } else if (dataModule && dataModule->HasData()) {
                    // Для non-custom состояния откатываемся к разрешенному X-диапазону
                    // (полный диапазон или пользовательский X Range, если он был задан).
                    zoom->minX = xLockMin;
                    zoom->maxX = xLockMax;
                    zoom->isXCustom = false;
                    zoom->isXToLatest = false;
                }
            } else if (dataModule && dataModule->HasData()) {
                zoom->minX = xLockMin;
                zoom->maxX = xLockMax;
                zoom->isXCustom = false;
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
    
    // Получаем текущую информацию о скроллбаре
    SCROLLINFO si = {};
    si.cbSize = sizeof(SCROLLINFO);
    si.fMask = SIF_ALL;
    GetScrollInfo(hScrollBar, SB_CTL, &si);
    
    int newPos = si.nPos;
    
    // Обрабатываем различные команды скроллинга
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
    
    // Обновляем позицию скроллбара
    si.nPos = newPos;
    SetScrollInfo(hScrollBar, SB_CTL, &si, TRUE);
    
    // Обновляем диапазон зума на основе новой позиции скроллбара
    if (isVertical) {
        // Вертикальный скроллинг (ось Y)
        if (!zoom->isZoomed) {
            // Если график не в режиме zoom, вертикальный скроллинг не работает
            return;
        }

        // Пользователь взаимодействует с Y: фиксируем, что Y-zoom пользовательский (чтобы Surf не перезатирал)
        zoom->isYCustom = true;
        
        // Получаем Y диапазон для ТЕКУЩЕГО X участка (не всех данных)
        // Это позволяет скроллить в пределах данных текущего видимого X диапазона
        ChartConfiguration config = curveSelectionModule->GetChartConfiguration(chartNumber);
        std::vector<std::vector<float>> leftAxisData, rightAxisData;
        std::vector<CurveDefinition> leftAxisCurves, rightAxisCurves;
        
        // Получаем данные для текущего X диапазона
        curveSelectionModule->CalculateCurveData(config, dataModule.get(), leftAxisData, rightAxisData,
                                                 leftAxisCurves, rightAxisCurves,
                                                 zoom->minX, zoom->maxX, -1e10, 1e10);
        
        double minYFull, maxYFull, minX, maxX, minY2, maxY2;
        chartModule->GetDataRanges(leftAxisData, rightAxisData, minX, maxX, minYFull, maxYFull, minY2, maxY2);
        
        // Добавляем padding 5% к диапазону данных для визуального комфорта
        double dataYRange = maxYFull - minYFull;
        double padding = dataYRange * 0.05;
        minYFull -= padding;
        maxYFull += padding;
        
        double totalYRange = maxYFull - minYFull;
        double viewYRange = zoom->maxY - zoom->minY;
        
        // Вертикальный скроллбар инвертирован (верх = 0, низ = 100)
        // newPos = 0 означает верх (максимальный Y)
        // newPos = maxScrollPos означает низ (минимальный Y)
        // Учитываем, что максимальная позиция скроллбара = nMax - nPage + 1
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
        // Горизонтальный скроллинг (ось X)
        double totalRange = xLockMax - xLockMin;
        double viewRange = zoom->maxX - zoom->minX;
        if (viewRange > totalRange) {
            viewRange = totalRange;
        }
        
        // Учитываем, что максимальная позиция скроллбара = nMax - nPage + 1
        // nMax = 100, nPage = (viewRange / totalRange) * 100
        // Поэтому реальный максимум = 100 - page + 1
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
        const bool wasYCustom = zoom->isYCustom;
        const double prevMinY = zoom->minY;
        const double prevMaxY = zoom->maxY;
        const double prevViewRangeY = prevMaxY - prevMinY;
        AutoFitYForCurrentX(chartNumber);
        if (wasYCustom) {
            // Сохраняем независимость осей: оставляем пользовательскую высоту Y-окна,
            // но зажимаем её в актуальный Y-диапазон для нового X-участка.
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
    
    // Перерисовываем только нужный график (избегаем мигания)
    InvalidateChart(hWnd, chartNumber);
}

void ChartManager::UpdateScrollBars(HWND hWnd) {
    uiModule->SetUndoZoomStepEnabled(1, !chart1ZoomHistory.empty());
    uiModule->SetUndoZoomStepEnabled(2, uiModule->IsDualMode() && !chart2ZoomHistory.empty());

    // Обновляем скроллбары для первого графика
    if (chart1Zoom.isZoomed) {
        uiModule->ShowScrollBars(1, true);
        uiModule->UpdateScrollBarPositions(1, layoutModule->GetChart1Rect());

        const bool showHScroll1 = !uiModule->IsSurfEnabled(1) && chart1Zoom.isXCustom && chart1Zoom.isYCustom;
        const bool showVScroll1 = chart1Zoom.isYCustom;
        if (HWND hH = uiModule->GetHScrollBar(1)) {
            ShowWindow(hH, showHScroll1 ? SW_SHOW : SW_HIDE);
        }
        if (HWND hV = uiModule->GetVScrollBar(1)) {
            ShowWindow(hV, showVScroll1 ? SW_SHOW : SW_HIDE);
        }
        uiModule->ShowAxisUndoButtons(1, showHScroll1, showVScroll1);

        // Surf ON: прячем горизонтальный скроллбар и не настраиваем X-скролл
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

            if (chart1Zoom.isXCustom && !chart1Zoom.isYCustom) {
                xLockMin = chart1Zoom.minX;
                xLockMax = chart1Zoom.maxX;
            } else {
                for (auto it = zoomHistory->rbegin(); it != zoomHistory->rend(); ++it) {
                    if (it->isXCustom && !it->isYCustom) {
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

            // Настраиваем диапазон горизонтального скроллбара (ось X)
            double totalDataRange = xLockMax - xLockMin;
            double viewRangeX = chart1Zoom.maxX - chart1Zoom.minX;
            if (viewRangeX > totalDataRange) {
                viewRangeX = totalDataRange;
            }
            
            // Вычисляем page size и максимальную позицию скроллбара
            int pageSize = (totalDataRange > 0.0)
                ? static_cast<int>((viewRangeX / totalDataRange) * 100)
                : 100;
            if (pageSize < 1) pageSize = 1;
            if (pageSize > 100) pageSize = 100;
            int maxScrollPos = 100 - pageSize + 1;
            
            // Вычисляем позицию скроллбара пропорционально
            double scrollableRange = totalDataRange - viewRangeX;
            int hScrollPos = (scrollableRange > 0)
                ? static_cast<int>(((chart1Zoom.minX - xLockMin) / scrollableRange) * maxScrollPos)
                : 0;
            uiModule->SetScrollBarRange(1, 0, 100, pageSize, hScrollPos);
        }
        
        // Настраиваем диапазон вертикального скроллбара (ось Y)
        if (showVScroll1) {
        ChartConfiguration config = curveSelectionModule->GetChartConfiguration(1);
        const CachedCurveSnapshot* curveSnapshot = nullptr;
        if (!TryGetCurveSnapshot(1, config, chart1Zoom, !ShouldUseAsyncCurveWarmup(), curveSnapshot)) {
            ScheduleAsyncCurveSnapshotWarmup(hWnd, 1, config, chart1Zoom);
            uiModule->SetVScrollBarRange(1, 0, 100, 100, 50);
        } else {
            double minYFull = curveSnapshot->minY1;
            double maxYFull = curveSnapshot->maxY1;
            
            // Добавляем padding 5% к диапазону данных для согласованности с HandleScrollEvent
            double dataYRange = maxYFull - minYFull;
            double padding = dataYRange * 0.05;
            minYFull -= padding;
            maxYFull += padding;
            
            double totalYRange = maxYFull - minYFull;
            double viewRangeY = chart1Zoom.maxY - chart1Zoom.minY;
            
            // Вертикальный скроллбар инвертирован (верх = 0, низ = 100)
            // Защита от деления на ноль
            int vScrollPos = 50;
            int vScrollPage = 100;
            if (totalYRange > viewRangeY && totalYRange > 0) {
                vScrollPage = static_cast<int>((viewRangeY / totalYRange) * 100);
                int maxScrollPos = 100 - vScrollPage + 1;
                
                // Вычисляем позицию: 0 = верх (maxY), maxScrollPos = низ (minY)
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
    
    // Обновляем скроллбары для второго графика (если в режиме двух графиков)
    if (uiModule->IsDualMode()) {
        if (chart2Zoom.isZoomed) {
            uiModule->ShowScrollBars(2, true);
            uiModule->UpdateScrollBarPositions(2, layoutModule->GetChart2Rect());

            const bool showHScroll2 = !uiModule->IsSurfEnabled(2) && chart2Zoom.isXCustom && chart2Zoom.isYCustom;
            const bool showVScroll2 = chart2Zoom.isYCustom;
            if (HWND hH = uiModule->GetHScrollBar(2)) {
                ShowWindow(hH, showHScroll2 ? SW_SHOW : SW_HIDE);
            }
            if (HWND hV = uiModule->GetVScrollBar(2)) {
                ShowWindow(hV, showVScroll2 ? SW_SHOW : SW_HIDE);
            }
            uiModule->ShowAxisUndoButtons(2, showHScroll2, showVScroll2);

            // Surf ON: прячем горизонтальный скроллбар и не настраиваем X-скролл
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

                if (chart2Zoom.isXCustom && !chart2Zoom.isYCustom) {
                    xLockMin = chart2Zoom.minX;
                    xLockMax = chart2Zoom.maxX;
                } else {
                    for (auto it = zoomHistory->rbegin(); it != zoomHistory->rend(); ++it) {
                        if (it->isXCustom && !it->isYCustom) {
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

                // Настраиваем диапазон горизонтального скроллбара (ось X)
                double totalDataRange = xLockMax - xLockMin;
                double viewRangeX = chart2Zoom.maxX - chart2Zoom.minX;
                if (viewRangeX > totalDataRange) {
                    viewRangeX = totalDataRange;
                }
                
                // Вычисляем page size и максимальную позицию скроллбара
                int pageSize = (totalDataRange > 0.0)
                    ? static_cast<int>((viewRangeX / totalDataRange) * 100)
                    : 100;
                if (pageSize < 1) pageSize = 1;
                if (pageSize > 100) pageSize = 100;
                int maxScrollPos = 100 - pageSize + 1;
                
                // Вычисляем позицию скроллбара пропорционально
                double scrollableRange = totalDataRange - viewRangeX;
                int hScrollPos = (scrollableRange > 0)
                    ? static_cast<int>(((chart2Zoom.minX - xLockMin) / scrollableRange) * maxScrollPos)
                    : 0;
                uiModule->SetScrollBarRange(2, 0, 100, pageSize, hScrollPos);
            }
            
            // Настраиваем диапазон вертикального скроллбара (ось Y)
            if (showVScroll2) {
            ChartConfiguration config = curveSelectionModule->GetChartConfiguration(2);
            const CachedCurveSnapshot* curveSnapshot = nullptr;
            if (!TryGetCurveSnapshot(2, config, chart2Zoom, !ShouldUseAsyncCurveWarmup(), curveSnapshot)) {
                ScheduleAsyncCurveSnapshotWarmup(hWnd, 2, config, chart2Zoom);
                uiModule->SetVScrollBarRange(2, 0, 100, 100, 50);
            } else {
                double minYFull = curveSnapshot->minY1;
                double maxYFull = curveSnapshot->maxY1;
                
                // Добавляем padding 5% к диапазону данных для согласованности с HandleScrollEvent
                double dataYRange = maxYFull - minYFull;
                double padding = dataYRange * 0.05;
                minYFull -= padding;
                maxYFull += padding;
                
                double totalYRange = maxYFull - minYFull;
                double viewRangeY = chart2Zoom.maxY - chart2Zoom.minY;
                
                // Вертикальный скроллбар инвертирован (верх = 0, низ = 100)
                // Защита от деления на ноль
                int vScrollPos = 50;
                int vScrollPage = 100;
                if (totalYRange > viewRangeY && totalYRange > 0) {
                    vScrollPage = static_cast<int>((viewRangeY / totalYRange) * 100);
                    int maxScrollPos = 100 - vScrollPage + 1;
                    
                    // Вычисляем позицию: 0 = верх (maxY), maxScrollPos = низ (minY)
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

// Метод для частичной перерисовки конкретного графика
void ChartManager::InvalidateChart(HWND hWnd, int chartNumber) {
    RECT chartRect;
    
    if (chartNumber == 1) {
        chartRect = layoutModule->GetChart1Rect();
    } else if (chartNumber == 2) {
        chartRect = layoutModule->GetChart2Rect();
    } else {
        return; // Неверный номер графика
    }

    if (IsUsingNewCurveSystem(chartNumber) && curveSelectionModule) {
        ScheduleAsyncCurveSnapshotWarmup(
            hWnd,
            chartNumber,
            curveSelectionModule->GetChartConfiguration(chartNumber),
            (chartNumber == 1) ? chart1Zoom : chart2Zoom);
    }
    
    // Перерисовываем только область конкретного графика
    // FALSE - не стираем фон, т.к. используем double buffering для устранения мигания
    InvalidateRect(hWnd, &chartRect, FALSE);
}

// Метод для отображения схемы прибора (v2.9.4: немодальное окно)
void ChartManager::ShowSchematicDialog(HWND hWnd, int chartNumber) {
    if (!schematicModule || !curveSelectionModule) {
        return;
    }
    
    // Показываем немодальное диалоговое окно схемы прибора
    // Окно остаётся открытым после Apply, поэтому не нужно обрабатывать возвращаемое значение
    schematicModule->ShowSchematicDialog(hWnd, chartNumber);
    
    // Примечание: Применение конфигурации теперь происходит внутри SchematicModule
    // при нажатии кнопки Apply, и окно остаётся открытым для быстрого переключения кривых
}

// Методы для новой системы кривых
void ChartManager::DrawChartWithCurves(HDC hdc, const RECT& chartRect, int chartNumber) {
    if (!curveSelectionModule || !dataModule) return;
    
    // Получаем конфигурацию кривых
    ChartConfiguration config = curveSelectionModule->GetChartConfiguration(chartNumber);
    if (config.curves.empty()) return;
    
    // Получаем параметры zoom
    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;

    const CachedCurveSnapshot* curveSnapshot = nullptr;
    const CachedCurveSnapshot* staleCurveCache = (chartNumber == 1) ? &chart1SnapshotCache : &chart2SnapshotCache;
    // Формируем заголовок с частотой
    // "График " в Unicode escape
    std::wstring title = L"\x0413\x0440\x0430\x0444\x0438\x043A " + std::to_wstring(chartNumber);
    
    // Добавляем частоту, если есть кривые
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
    
    // v2.10.0: Проверяем тип оси X
    if (config.xAxisType == XAxisType::TEMPERATURE) {
        // Режим кроссплота: температура на оси X

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

        // Получаем диапазоны Y из данных
        const double minYFull = curveSnapshot->minY1;
        const double maxYFull = curveSnapshot->maxY1;
        
        double minYData = 0.0;
        double maxYData = 0.0;
        BuildDisplayedLeftAxisRange(zoom->isZoomed, zoom->minY, zoom->maxY, minYFull, maxYFull, minYData, maxYData);

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

        // Вызываем новый метод отрисовки с температурой и правильной легендой
        // v2.15.0: Передаём config для поддержки компактной легенды пресетов
        chartModule->DrawDualAxisChart(hdc, chartRect, config.curves,
                                       leftAxisData, rightAxisData,
                                       title, true, temperatureAxis->temperatureData,
                                       temperatureAxis->minXData, temperatureAxis->maxXData, minYData, maxYData,
                                       config, true,
                                       useExplicitRightAxisRange,
                                       displayMinYRight,
                                       displayMaxYRight);
    } else {
        // Обычный режим: номер кадра на оси X
        
        // Получаем диапазоны Y из данных
        const double minYData = curveSnapshot->minY1;
        const double maxYData = curveSnapshot->maxY1;
        VisibleFrameAxisRange visibleXRange = {};
        BuildVisibleFrameAxisRange(zoom->isZoomed, zoom->minX, zoom->maxX, dataModule->GetDataSize(), visibleXRange);
        
        // Проверяем, нужны ли двойные оси
        if (config.hasDualAxis && !rightAxisData.empty()) {
            // Используем новый метод для двойных осей с поддержкой zoom
            // Всегда показываем оси (включая ось X)
            bool showAxes = true;
            
            double displayMinY = 0.0;
            double displayMaxY = 0.0;
            BuildDisplayedLeftAxisRange(zoom->isZoomed, zoom->minY, zoom->maxY, minYData, maxYData, displayMinY, displayMaxY);

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
            // Используем обычный метод для одной оси
            if (!leftAxisData.empty()) {
                // Отрисовываем график с кастомными кривыми
                // Используем метод DrawDualAxisChart даже для одной оси, чтобы правильно отобразить легенду
                bool showAxes = true;
                
                double displayMinY = 0.0;
                double displayMaxY = 0.0;
                BuildDisplayedLeftAxisRange(zoom->isZoomed, zoom->minY, zoom->maxY, minYData, maxYData, displayMinY, displayMaxY);

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
    // Проверяем, загружены ли данные
    bool hasData = dataModule && dataModule->HasData();
    
    // "График " в Unicode escape
    std::wstring title = L"\x0413\x0440\x0430\x0444\x0438\x043A " + std::to_wstring(chartNumber);
    
    // Отрисовываем фон и рамку графика вручную (без подписей осей)
    chartModule->DrawChartBackground(hdc, chartRect);
    chartModule->DrawChartBorder(hdc, chartRect);
    chartModule->DrawChartTitle(hdc, chartRect, title);
    
    // Расчет области для графика
    // ВАЖНО: Используем те же отступы, что и в DrawDualAxisChart (CalculateDualAxisPlotRect)
    RECT plotRect = chartRect;
    plotRect.left += 60;
    plotRect.top += 45;
    plotRect.right -= 140;
    plotRect.bottom -= 50;
    
    // Рисуем оси
    chartModule->DrawChartAxes(hdc, plotRect, true);
    
    // Если данные загружены, рисуем только сетку БЕЗ подписей осей
    if (hasData) {
        chartModule->DrawGridOnly(hdc, plotRect);
    }
    
    // Рисуем текст с инструкцией
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(100, 100, 100));
    HFONT hFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    
    // Выбираем сообщение в зависимости от наличия данных
    std::wstring message;
    if (hasData) {
        // "Нажмите 'Схема прибора' для настройки графика" в Unicode
        message = L"\x041D\x0430\x0436\x043C\x0438\x0442\x0435 '\x0421\x0445\x0435\x043C\x0430 \x043F\x0440\x0438\x0431\x043E\x0440\x0430' \x0434\x043B\x044F \x043D\x0430\x0441\x0442\x0440\x043E\x0439\x043A\x0438 \x0433\x0440\x0430\x0444\x0438\x043A\x0430";
    } else {
        // "Сначала откройте файл данных" в Unicode
        message = L"\x0421\x043D\x0430\x0447\x0430\x043B\x0430 \x043E\x0442\x043A\x0440\x043E\x0439\x0442\x0435 \x0444\x0430\x0439\x043B \x0434\x0430\x043D\x043D\x044B\x0445";
    }
    
    RECT textRect = chartRect;
    DrawText(hdc, message.c_str(), -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

// Методы для работы с конфигурациями кривых
void ChartManager::ApplyCurveConfiguration(int chartNumber, const ChartConfiguration& config) {
    if (!curveSelectionModule) return;
    
    // Валидируем конфигурацию
    if (!ValidateChartConfiguration(config)) {
        return;
    }
    
    // Применяем конфигурацию
    curveSelectionModule->ApplyCurveConfiguration(chartNumber, config);
    
    // Синхронизируем UI с новой конфигурацией
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

// Методы для копирования настроек между графиками
void ChartManager::CopyChartSettings(int fromChart, int toChart) {
    if (!curveSelectionModule) return;
    
    // Проверяем валидность номеров графиков
    if (fromChart < 1 || fromChart > 2 || toChart < 1 || toChart > 2 || fromChart == toChart) {
        return;
    }
    
    // Проверяем, что исходный график имеет конфигурацию
    if (!curveSelectionModule->HasConfiguration(fromChart)) {
        return;
    }
    
    // Копируем настройки
    curveSelectionModule->CopySettings(fromChart, toChart);
    
    // Синхронизируем UI с новой конфигурацией
    SynchronizeUIWithConfiguration(toChart);
}

// Методы для применения пресетов
void ChartManager::ApplyPreset(int chartNumber, PresetType preset, FrequencyBand frequency) {
    if (!curveSelectionModule) return;
    
    // Применяем пресет
    curveSelectionModule->ApplyPreset(preset, chartNumber, frequency);
    
    // Синхронизируем UI с новой конфигурацией
    SynchronizeUIWithConfiguration(chartNumber);
}

// Вспомогательные методы для интеграции
void ChartManager::UpdateChartAfterConfigurationChange(HWND hWnd, int chartNumber) {
    // Сбрасываем масштабирование при изменении конфигурации
    if (chartNumber == 1) {
        chart1Zoom.isZoomed = false;
        chart1Zoom.isXCustom = false;
        chart1Zoom.isXToLatest = false;
        chart1Zoom.isYCustom = false;
        chart1ZoomHistory.clear(); // v2.12.5: Очищаем историю zoom
        ClearAxisHistory(1);
    } else if (chartNumber == 2) {
        chart2Zoom.isZoomed = false;
        chart2Zoom.isXCustom = false;
        chart2Zoom.isXToLatest = false;
        chart2Zoom.isYCustom = false;
        chart2ZoomHistory.clear(); // v2.12.5: Очищаем историю zoom
        ClearAxisHistory(2);
    }

    uiModule->ResetXRangeControls(chartNumber);
    
    // Обновляем скроллбары
    ScheduleAsyncCurveSnapshotWarmup(hWnd, chartNumber, curveSelectionModule->GetChartConfiguration(chartNumber), (chartNumber == 1) ? chart1Zoom : chart2Zoom);
    UpdateScrollBars(hWnd);
    
    // Перерисовываем график
    InvalidateChart(hWnd, chartNumber);
}

void ChartManager::SynchronizeUIWithConfiguration(int chartNumber) {
    if (!curveSelectionModule) return;
    
    // Получаем конфигурацию
    ChartConfiguration config = curveSelectionModule->GetChartConfiguration(chartNumber);
    
    // Если конфигурация пуста, возвращаемся к стандартному UI
    if (config.curves.empty()) {
        return;
    }
    
    // Синхронизируем старые элементы UI с новой системой, если возможно
    SynchronizeLegacyUIWithNewSystem(chartNumber);
    
    // Новая система работает независимо от старых элементов UI,
    // но мы стараемся поддерживать их в синхронизированном состоянии
    // для лучшего пользовательского опыта
}

bool ChartManager::ValidateChartConfiguration(const ChartConfiguration& config) const {
    if (!curveSelectionModule) return false;
    
    // Используем валидацию из CurveSelectionModule
    return curveSelectionModule->ValidateConfiguration(config);
}

// Методы для обработки событий от диалогового окна схемы
void ChartManager::HandleSchematicDialogResult(HWND hWnd, int chartNumber, bool applied) {
    if (!applied) {
        return; // Пользователь отменил изменения
    }
    
    // Обновляем график после применения настроек
    UpdateChartAfterConfigurationChange(hWnd, chartNumber);
    
    // Отправляем пользовательское сообщение главному окну для обновления
    PostMessage(hWnd, WM_USER + 100, static_cast<WPARAM>(chartNumber), 0);
}

void ChartManager::HandlePresetCommand(HWND hWnd, int chartNumber, int presetId, FrequencyBand frequency) {
    PresetType preset;
    
    // Определяем тип пресета по ID команды
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
            return; // Неизвестный пресет
    }
    
    // Применяем пресет
    ApplyPreset(chartNumber, preset, frequency);
    
    // Обновляем график
    UpdateChartAfterConfigurationChange(hWnd, chartNumber);
}

void ChartManager::HandleCopySettingsCommand(HWND hWnd, int fromChart, int toChart) {
    // Копируем настройки между графиками
    CopyChartSettings(fromChart, toChart);
    
    // Обновляем целевой график
    UpdateChartAfterConfigurationChange(hWnd, toChart);
}

// Методы для обеспечения совместимости
void ChartManager::ResetToLegacyMode(int chartNumber) {
    if (!curveSelectionModule) return;
    
    // Очищаем конфигурацию кривых для возврата к старой системе
    ChartConfiguration emptyConfig;
    curveSelectionModule->ApplyCurveConfiguration(chartNumber, emptyConfig);
    
    // Синхронизируем UI
    SynchronizeUIWithConfiguration(chartNumber);
}

bool ChartManager::IsUsingNewCurveSystem(int chartNumber) const {
    if (!curveSelectionModule) return false;
    
    return curveSelectionModule->HasConfiguration(chartNumber);
}

void ChartManager::SynchronizeLegacyUIWithNewSystem(int chartNumber) {
    // Метод оставлен для совместимости API, но больше ничего не делает
    // Все управляется только через Device Schematic
}

void ChartManager::HandleLegacyUIChange(HWND hWnd, int chartNumber) {
    // Если пользователь изменил настройки через старые элементы UI,
    // сбрасываем новую систему кривых для этого графика
    if (IsUsingNewCurveSystem(chartNumber)) {
        ResetToLegacyMode(chartNumber);
        
        // Обновляем график
        InvalidateChart(hWnd, chartNumber);
    }
}

// Методы для обработки X Range
void ChartManager::HandleXRangeApply(HWND hWnd, int chartNumber) {
    if (!dataModule || !dataModule->HasData()) {
        wchar_t message[256];
        swprintf_s(message, L"No data loaded for Chart %d", chartNumber);
        MessageBox(hWnd, message, L"X Range", MB_OK | MB_ICONWARNING);
        return;
    }

    // Surf ON: X диапазон фиксирован на последние N точек
    if (uiModule->IsSurfEnabled(chartNumber)) {
        MessageBox(hWnd,
            L"X Range is locked in Surf mode.\nDisable Surf to change X range.",
            L"X Range",
            MB_OK | MB_ICONINFORMATION);
        return;
    }
    
    // Получаем максимальное значение данных
    double dataSize = static_cast<double>(dataModule->GetDataSize() - 1);
    
    // Получаем значения из полей ввода для указанного графика
    double fromX, toX;
    uiModule->GetXRangeValues(chartNumber, fromX, toX);
    const bool isToEmpty = uiModule->IsXRangeToEmpty(chartNumber);
    
    // Если поле "To" пустое, используем максимальное значение данных
    if (isToEmpty) {
        toX = dataSize;
    }
    
    // Валидация входных данных
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
    
    // Проверяем минимальный диапазон: подписи должны быть целыми и отличаться минимум на 10
    const double MIN_X_RANGE = 10.0;
    double xRange = toX - fromX;
    
    if (xRange < MIN_X_RANGE) {
        wchar_t message[256];
        swprintf_s(message, L"Chart %d: X Range must be at least %.0f points.", 
                   chartNumber, MIN_X_RANGE);
        MessageBox(hWnd, message, L"X Range Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Применяем диапазон к указанному графику
    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
    std::vector<ZoomRange>* zoomHistory = (chartNumber == 1) ? &chart1ZoomHistory : &chart2ZoomHistory;
    zoomHistory->clear();
    PushAxisHistory(chartNumber);
    zoom->minX = fromX;
    zoom->maxX = toX;
    
    ChartConfiguration config = curveSelectionModule->GetChartConfiguration(chartNumber);
    zoom->isZoomed = true;
    zoom->isXCustom = true;
    zoom->isXToLatest = isToEmpty;
    zoom->isYCustom = false; // Y подобран автоматически под выбранный X диапазон
    ClearExplicitRightAxisRange(zoom->minYRight, zoom->maxYRight, zoom->hasRightAxisCustomRange);

    const CachedCurveSnapshot* curveSnapshot = nullptr;
    if (TryGetCurveSnapshot(chartNumber, config, *zoom, !ShouldUseAsyncCurveWarmup(), curveSnapshot)) {
        const double yRange = curveSnapshot->maxY1 - curveSnapshot->minY1;
        const double padding = yRange * 0.05;
        zoom->minY = curveSnapshot->minY1 - padding;
        zoom->maxY = curveSnapshot->maxY1 + padding;
    }
    
    // Обновляем скроллбары
    ScheduleAsyncCurveSnapshotWarmup(hWnd, chartNumber, config, *zoom);
    UpdateScrollBars(hWnd);

    // После Apply поле To может быть намеренно пустым (режим From..latest),
    // поэтому не перезаписываем поля ввода автоматически.
    uiModule->SetXRangeResetEnabled(chartNumber, true);
    
    // Перерисовываем графики (FALSE для double buffering)
    InvalidateRect(hWnd, NULL, FALSE);
}

void ChartManager::HandleXRangeReset(HWND hWnd, int chartNumber) {
    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
    
    // Surf ON: Reset трактуем как "сбросить пользовательский Y-zoom" и вернуть авто-Y,
    // при этом X остаётся фиксированным на окне Surf.
    if (uiModule->IsSurfEnabled(chartNumber)) {
        zoom->isXCustom = false;
        zoom->isXToLatest = false;
        zoom->isYCustom = false;
        UpdateSurfRange(chartNumber);
    } else {
        // Сбрасываем масштабирование для указанного графика
        zoom->isZoomed = false;
        zoom->isXCustom = false;
        zoom->isXToLatest = false;
        zoom->isYCustom = false;
    }

    ClearAxisHistory(chartNumber);

    uiModule->ResetXRangeControls(chartNumber);
    
    // Обновляем скроллбары
    if (curveSelectionModule && IsUsingNewCurveSystem(chartNumber)) {
        ScheduleAsyncCurveSnapshotWarmup(hWnd, chartNumber, curveSelectionModule->GetChartConfiguration(chartNumber), *zoom);
    }
    UpdateScrollBars(hWnd);
    
    // Обновляем UI
    UpdateXRangeUI(hWnd);
    
    // Перерисовываем графики (FALSE для double buffering)
    InvalidateRect(hWnd, NULL, FALSE);
}

void ChartManager::HandleResetZoom(HWND hWnd, int chartNumber) {
    // Сбрасываем zoom для указанного графика (полный сброс, не пошаговый)
    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
    
    // Surf ON: Reset Zoom сбрасывает только пользовательский Y-zoom,
    // но не выключает Surf-окно по X.
    if (uiModule->IsSurfEnabled(chartNumber)) {
        zoom->isXCustom = false;
        zoom->isXToLatest = false;
        zoom->isYCustom = false;
        ClearExplicitRightAxisRange(zoom->minYRight, zoom->maxYRight, zoom->hasRightAxisCustomRange);
        UpdateSurfRange(chartNumber);
    } else {
        zoom->isZoomed = false;
        zoom->isXCustom = false;
        zoom->isXToLatest = false;
        zoom->isYCustom = false;
        ClearExplicitRightAxisRange(zoom->minYRight, zoom->maxYRight, zoom->hasRightAxisCustomRange);
    }
    
    // v2.12.5: Очищаем историю zoom при полном сбросе через кнопку
    if (chartNumber == 1) {
        chart1ZoomHistory.clear();
        ClearAxisHistory(1);
    } else if (chartNumber == 2) {
        chart2ZoomHistory.clear();
        ClearAxisHistory(2);
    }

    uiModule->ResetXRangeControls(chartNumber);
    
    // Обновляем скроллбары
    if (curveSelectionModule && IsUsingNewCurveSystem(chartNumber)) {
        ScheduleAsyncCurveSnapshotWarmup(hWnd, chartNumber, curveSelectionModule->GetChartConfiguration(chartNumber), *zoom);
    }
    UpdateScrollBars(hWnd);
    
    // Обновляем UI (X Range controls)
    UpdateXRangeUI(hWnd);
    
    // Перерисовываем только измененный график
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
            // В Surf режиме не перезаписываем поля From/To.
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

        // COM realtime + нет применённого кастомного диапазона:
        // не перезаписываем пользовательский ручной ввод в полях X Range.
        if (isSerialRealtime) {
            return;
        }

        uiModule->SetXRangeResetEnabled(chartNumber, false);
    };
    
    // Обновляем элементы управления для Chart 1
    updateChartXRange(1, chart1Zoom);
    
    // Обновляем элементы управления для Chart 2 (если в Dual Mode)
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
    
    // Получаем текущий диапазон для указанного графика
    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
    
    if (zoom->isZoomed) {
        // Zoom применён через Apply — используем его диапазон
        startIndex = static_cast<size_t>(zoom->minX);
        endIndex = static_cast<size_t>(zoom->maxX);
    } else {
        // Zoom не применён — читаем From/To из полей ввода напрямую
        const bool fromEmpty = uiModule->IsXRangeFromEmpty(chartNumber);
        const bool toEmpty = uiModule->IsXRangeToEmpty(chartNumber);
        
        if (fromEmpty && toEmpty) {
            // Оба поля пусты — сохраняем ВСЕ данные (0 .. dataSize-1)
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
    
    // Clamp индексов к границам данных
    if (startIndex >= dataSize) startIndex = dataSize - 1;
    if (endIndex >= dataSize) endIndex = dataSize - 1;
    
    // Проверяем корректность диапазона
    if (startIndex > endIndex) {
        MessageBox(hWnd, L"'From' value must be less than or equal to 'To' value!", L"Error", MB_OK | MB_ICONERROR);
        return;
    }
    
    // Открываем диалог сохранения файла
    OPENFILENAME ofn;
    wchar_t szFile[260] = {0};
    
    // Формируем предложенное имя файла
    // Формат: [исходное_имя]_range_[начало]_[конец]_[ДД_ММ_ГГГГ].bin
    std::wstring baseFileName = dataModule->GetLoadedFileName();
    
    // Получаем текущую дату
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

        // Сохраняем диапазон данных
        if (dataModule->SaveDataRange(szFile, startIndex, endIndex)) {
            // Формируем сообщение об успехе
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

// Метод для обработки движения мыши и обновления координат в статусной строке
void ChartManager::HandleMouseMoveForCoordinates(HWND hWnd, int x, int y) {
    if (!dataModule || !dataModule->HasData()) {
        uiModule->ClearStatusBarCoordinates();
        ResetCoordinateTracking();
        return;
    }

    // Определяем, над каким графиком находится курсор
    int chartNum = GetChartAtPoint(x, y);
    
    if (chartNum == 0) {
        // Курсор вне графиков
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

    // Преобразуем экранные координаты в координаты данных
    double dataX, dataY1, dataY2;
    std::wstring y1Unit, y2Unit;
    bool hasDualAxis;
    
    if (ConvertScreenToChartCoords(chartNum, x, y, dataX, dataY1, dataY2, y1Unit, y2Unit, hasDualAxis)) {
        // Обновляем статусную строку с координатами
        uiModule->UpdateStatusBarCoordinates(dataX, dataY1, y1Unit, dataY2, y2Unit, hasDualAxis);
        
        // Обновляем информацию о файле в последней части статусной строки
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

// Преобразование экранных координат в координаты графика
bool ChartManager::ConvertScreenToChartCoords(int chartNum, int screenX, int screenY,
                                              double& dataX, double& dataY1, double& dataY2,
                                              std::wstring& y1Unit, std::wstring& y2Unit,
                                              bool& hasDualAxis) const {
    if (!dataModule || !dataModule->HasData()) {
        return false;
    }

    // Получаем область графика
    RECT chartRect = (chartNum == 1) ? layoutModule->GetChart1Rect() : layoutModule->GetChart2Rect();
    
    // Проверяем, что курсор внутри области графика
    if (screenX < chartRect.left || screenX > chartRect.right ||
        screenY < chartRect.top || screenY > chartRect.bottom) {
        return false;
    }

    // Определяем, используется ли новая система кривых
    bool usingNewSystem = IsUsingNewCurveSystem(chartNum);
    
    if (usingNewSystem) {
        // Новая система кривых с возможными двойными осями
        ChartConfiguration config = GetChartConfiguration(chartNum);
        
        if (config.curves.empty()) {
            return false;
        }

        // Получаем параметры zoom
        ZoomRange zoom = (chartNum == 1) ? chart1Zoom : chart2Zoom;

        // Получаем данные для кривых с учетом zoom по X (как в DrawChartWithCurves)
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
        
        // Текущие диапазоны для отображения (могут быть изменены zoom по Y)
        double minY1 = minY1Full, maxY1 = maxY1Full;
        double minY2 = minY2Full, maxY2 = maxY2Full;
        
        // Применяем zoom по Y если есть (zoom по X уже применён при получении данных)
        if (zoom.isZoomed) {
            minY1 = zoom.minY;
            maxY1 = zoom.maxY;

            // ВАЖНО: синхронизация с фактической отрисовкой (DrawChartWithCurves),
            // где при zoom добавляется display padding +/-5%.
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
        
        // Преобразуем экранные координаты в координаты данных
        // ВАЖНО: Используем те же отступы, что и в CalculateDualAxisPlotRect
        DualAxisChartGeometry geometry = {};
        if (!TryBuildDualAxisChartGeometry(chartRect, geometry)) {
            return false;
        }

        // ВАЖНО: как в DrawDualAxisDataWithZoom — данные рисуются внутри dataRect,
        // а не по всей высоте plotRect.
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

        // X координата
        // В режиме TEMPERATURE ось X нелинейна относительно индекса кадра,
        // поэтому нужна привязка к ближайшему индексу по температуре.
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
        
        // Y координаты
        hasDualAxis = !rightAxisData.empty();
        
        if (hasDualAxis) {
            // Двойные оси
            dataY1 = leftDrawMin + yRatio * (leftDrawMax - leftDrawMin); // Левая ось

            double rightDrawMin = 0.0;
            double rightDrawMax = 0.0;
            if (!TryBuildDrawnAxisRange(minY2, maxY2, rightDrawMin, rightDrawMax)) {
                rightDrawMin = minY2;
                rightDrawMax = maxY2;
            }
            dataY2 = rightDrawMin + yRatio * (rightDrawMax - rightDrawMin); // Правая ось
            
            // Определяем единицы измерения из первых кривых каждой оси
            // Преобразуем std::string в std::wstring
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
            // Одна ось
            dataY1 = leftDrawMin + yRatio * (leftDrawMax - leftDrawMin);
            std::string units1 = config.curves[0].units;
            y1Unit = std::wstring(units1.begin(), units1.end());
            y2Unit = L"";
        }
        
        return true;
        
    } else {
        // Старая система (legacy mode)
        // Здесь можно добавить поддержку старой системы, если нужно
        // Пока возвращаем false
        return false;
    }
}

// Получение единиц измерения для типа данных
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
            return L"°C";
        
        default:
            return L"";
    }
}

// Обновление информации о файле в статусной строке
void ChartManager::UpdateStatusBarFileInfo() {
    if (!uiModule) return;
    
    if (dataModule && dataModule->HasData()) {
        std::wstring fileName = dataModule->GetLoadedFileName();
        size_t dataSize = dataModule->GetDataSize();
        
        wchar_t fileInfo[256];
        swprintf_s(fileInfo, L"Points: %zu  |  %s", dataSize, fileName.c_str());
        SendMessage(uiModule->GetStatusBar(), SB_SETTEXT, 3, (LPARAM)fileInfo);
        
        // Устанавливаем начальное сообщение
        uiModule->UpdateStatusBar(L"Ready");
    } else {
        SendMessage(uiModule->GetStatusBar(), SB_SETTEXT, 3, (LPARAM)L"No file loaded");
        uiModule->ClearStatusBarCoordinates();
    }
}

// Отрисовка перекрестия (crosshair) на графике
void ChartManager::DrawCrosshair(HDC hdc, const RECT& chartRect, int chartNumber) const {
    // Получаем перекрестие для указанного графика
    const CrosshairPosition* crosshair = (chartNumber == 1) ? &chart1Crosshair : &chart2Crosshair;
    
    // Проверяем, нужно ли отображать перекрестие
    if (!crosshair->isVisible || crosshair->chartNumber != chartNumber) {
        return;
    }
    
    // Вычисляем область графика (plotRect) с учётом отступов
    // ВАЖНО: Всегда используем отступы для двойных осей, потому что DrawDualAxisChart
    // вызывается для всех графиков и использует CalculateDualAxisPlotRect
    DualAxisChartGeometry geometry = {};
    if (!TryBuildDualAxisChartGeometry(chartRect, geometry)) {
        return;
    }
    const RECT& plotRect = geometry.plotRect;
    
    // Проверяем, что перекрестие находится внутри области графика
    if (crosshair->screenX < plotRect.left || crosshair->screenX > plotRect.right ||
        crosshair->screenY < plotRect.top || crosshair->screenY > plotRect.bottom) {
        return;
    }
    
    // Создаём красное перо для перекрестия
    HPEN hCrosshairPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hCrosshairPen);
    
    // Рисуем вертикальную линию (от верха до низа plotRect)
    MoveToEx(hdc, crosshair->screenX, plotRect.top, NULL);
    LineTo(hdc, crosshair->screenX, plotRect.bottom);
    
    // Рисуем горизонтальную линию (от левого до правого края plotRect)
    MoveToEx(hdc, plotRect.left, crosshair->screenY, NULL);
    LineTo(hdc, plotRect.right, crosshair->screenY);
    
    // Восстанавливаем старое перо и удаляем созданное
    SelectObject(hdc, hOldPen);
    DeleteObject(hCrosshairPen);
}

// v2.14.2: Сброс перекрестия (вызывается при клике на иконку 📍)
void ChartManager::ClearCrosshair(HWND hWnd, int chartNumber) {
    if (chartNumber == 1) {
        chart1Crosshair.isVisible = false;
        uiModule->ClearClickCoordinates(1);
    } else if (chartNumber == 2) {
        chart2Crosshair.isVisible = false;
        uiModule->ClearClickCoordinates(2);
    }
    
    // Перерисовываем окно
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

    // Plot rect (должен совпадать с CalculateDualAxisPlotRect/ConvertScreenToChartCoords)
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

    // Получаем диапазоны Y (как в ConvertScreenToChartCoords / DrawChartWithCurves)
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

    double minY1 = zoom.isZoomed ? zoom.minY : minY1Full;
    double maxY1 = zoom.isZoomed ? zoom.maxY : maxY1Full;

    // v2.15.x: при zoom добавляем padding как в DrawChartWithCurves
    BuildDisplayedLeftAxisRange(zoom.isZoomed, zoom.minY, zoom.maxY, minY1Full, maxY1Full, minY1, maxY1);

    // В DrawDualAxisDataWithZoom дополнительно применяется нижний padding 2%.
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

    // Для TEMPERATURE нужны minTemp/maxTemp по видимому диапазону индексов
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
            return L"(пусто)";
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

    // Черная точка
    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
    HBRUSH hBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, hBrush);

    const int r = 3;
    for (size_t idx : noteIdx) {
        const auto& n = notesModule->GetAllNotes()[idx];

        // Отсекаем по X диапазону (индекс кадра)
        if (n.xIndex < 0) continue;
        if (n.xIndex < (int)floor(minXData) - 1) continue;
        if (n.xIndex > (int)ceil(maxXData) + 1) continue;

        const bool anchorOnRightAxis = n.hasYRight && n.anchorAxisGroup == 1 && hasRightDrawRange && yRangeRight > 0.0;
        const double anchorY = anchorOnRightAxis ? n.yRight : n.yLeft;
        const double drawMinY = anchorOnRightAxis ? drawMinY2 : drawMinY1;
        const double drawMaxY = anchorOnRightAxis ? drawMaxY2 : drawMaxY1;
        const double yRange = anchorOnRightAxis ? yRangeRight : yRangeLeft;
        double yRatio = (drawMaxY - anchorY) / yRange;

        int x = 0;
        if (!TryResolveNoteMarkerScreenX(*dataModule, config.xAxisType, n.xIndex,
                                         minXData, maxXData, xRange,
                                         minTemp, maxTemp, hasTempRange, dataSize,
                                         plotRect, plotW, x)) {
            continue;
        }
        int y = dataRect.top + (int)lround(yRatio * dataH);

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

    // Компактные текстовые плашки возле точек заметок (только первые 8).
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

        // Если не помещается сверху — переносим вниз от маркера.
        if (labelRect.top < dataRect.top) {
            labelRect.top = candidate.anchorY + kOffsetY;
            labelRect.bottom = labelRect.top + kLabelHeight;
        }

        // Ограничиваем прямоугольник областью данных, чтобы не перекрывать оси/легенду.
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

        // Простейшая защита от наложения: сдвигаем вниз по шагу, иначе пропускаем.
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


// === Методы для работы с COM-портом (v2.13.0) ===

void ChartManager::HandleSerialConnect(HWND hWnd) {
    // v2.13.0: Подключение теперь происходит через диалог IDD_SERIAL_STATUS
    // Этот метод оставлен для совместимости с меню, но не используется напрямую
    // Вместо этого открывается диалог через IDM_DEVICE_CONNECT
}

void ChartManager::HandleSerialDisconnect(HWND hWnd) {
    if (!serialModule) return;
    
    // Останавливаем опрос перед отключением
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
    
    // Запускаем периодический опрос (каждые 2 секунды)
    // Примечание: Очистка данных происходит в диалоге SerialStatusDialogProc
    // перед вызовом StartPolling через HandleSerialClear
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

    // После остановки realtime синхронизируем X Range с текущим полным диапазоном.
    UpdateXRangeUI(hWnd);
}

void ChartManager::HandleSerialClear(HWND hWnd) {
    if (!dataModule) return;
    
    // Очищаем данные
    dataModule->ClearData();
    InvalidateCachedSnapshots();
    ResetCoordinateTracking();

    // v2.16.x: при переходе к COM-режиму не должны писать заметки в sidecar последнего открытого файла
    currentDataFilePath.clear();
    
    // Сбрасываем статистику
    if (serialModule) {
        serialModule->ResetStatistics();
    }
    
    // Сбрасываем zoom
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
    
    // Обновляем UI
    if (uiModule) {
        uiModule->UpdateSerialFrameInfo(0, 0, 0);
        uiModule->ResetXRangeControls(1);
        uiModule->ResetXRangeControls(2);
    }
    
    // Перерисовываем графики
    InvalidateRect(hWnd, nullptr, TRUE);
    
    UpdateStatusBarFileInfo();
}

void ChartManager::HandleSerialTimer(HWND hWnd) {
    if (!serialModule) return;
    
    // Вызываем обработчик таймера в SerialModule
    serialModule->OnTimer();
}

void ChartManager::UpdateSerialUI(HWND hWnd) {
    // v2.13.0: UI обновляется через диалог IDD_SERIAL_STATUS
    // Этот метод оставлен для совместимости, но не выполняет действий
}

void ChartManager::RefreshPortList(HWND hWnd) {
    // v2.13.0: Список портов обновляется через диалог IDD_SERIAL_STATUS
    // Этот метод оставлен для совместимости, но не выполняет действий
}

bool ChartManager::IsSerialConnected() const {
    return serialModule && serialModule->IsConnected();
}

bool ChartManager::IsSerialPolling() const {
    return serialModule && serialModule->IsPolling();
}

// === Методы для режима Surf (v2.14.0) ===

void ChartManager::HandleSurfToggle(HWND hWnd, int chartNumber) {
    if (!dataModule || !dataModule->HasData()) {
        return;
    }
    
    bool surfEnabled = uiModule->IsSurfEnabled(chartNumber);
    
    if (surfEnabled) {
        // Surf включён - устанавливаем диапазон на последние N точек
        // и сбрасываем пользовательский Y-zoom (Surf начинает с авто-масштаба по Y)
        ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
        zoom->isXCustom = false;
        zoom->isXToLatest = false;
        zoom->isYCustom = false;
        UpdateSurfRange(chartNumber);
    } else {
        // Surf выключён - сбрасываем zoom к полному диапазону
        ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
        zoom->isZoomed = false;
        zoom->isXCustom = false;
        zoom->isXToLatest = false;
        zoom->isYCustom = false;
        ClearAxisHistory(chartNumber);
        
        // Не заполняем поля X Range автоматически — только очищаем их
        uiModule->ResetXRangeControls(chartNumber);
    }
    
    // Обновляем статусную строку
    UpdateSurfStatusBar();
    
    // Перерисовываем график
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
    
    // Устанавливаем zoom на область Surf (только по X)
    ZoomRange* zoom = (chartNumber == 1) ? &chart1Zoom : &chart2Zoom;
    zoom->minX = surfMinX;
    zoom->maxX = surfMaxX;
    zoom->isXCustom = false;
    zoom->isXToLatest = false;
    zoom->isZoomed = true;
    
    // Surf ON + пользовательский Y-zoom:
    // X окно двигается, но Y диапазон не перезатираем при приходе новых точек.
    if (!zoom->isYCustom) {
        // Вычисляем диапазон Y для данных в окне Surf (авто-масштаб)
        ChartConfiguration config = curveSelectionModule->GetChartConfiguration(chartNumber);
        std::vector<std::vector<float>> leftAxisData, rightAxisData;
        std::vector<CurveDefinition> leftAxisCurves, rightAxisCurves;
        
        // Передаём большой диапазон Y, чтобы не фильтровать по Y
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
    
    // В Surf режиме не автозаполняем поля X Range (From/To),
    // чтобы не перетирать пользовательский ввод.
    uiModule->SetXRangeResetEnabled(chartNumber, false);
}

void ChartManager::UpdateSurfStatusBar() {
    bool surf1Enabled = uiModule->IsSurfEnabled(1);
    int surf1Window = uiModule->GetSurfWindowSize(1);
    bool surf2Enabled = uiModule->IsSurfEnabled(2);
    int surf2Window = uiModule->GetSurfWindowSize(2);
    
    uiModule->UpdateStatusBarSurfInfo(surf1Enabled, surf1Window, surf2Enabled, surf2Window);
}
