#include "framework.h"
#include "ChartModule.h"
#include "SchematicModule.h"
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <sstream> // Для std::wstringstream
#include <iomanip> // Для std::fixed и std::setprecision
#include <set>     // Для std::set (отслеживание уникальных значений)

// GDI+ Flat C API: прямые forward-declarations вместо gdiplus.h,
// чтобы обойти несовместимость Windows 8.1 SDK + VS 2026.
extern "C" {
    typedef int GpStatus;
    enum { GpOk = 0 };

    struct GdiplusStartupInput {
        UINT32 GdiplusVersion;
        void*  DebugEventCallback;
        BOOL   SuppressBackgroundThread;
        BOOL   SuppressExternalCodecs;
    };

    GpStatus WINAPI GdiplusStartup(ULONG_PTR* token, const GdiplusStartupInput* input, void* output);
    void     WINAPI GdiplusShutdown(ULONG_PTR token);

    // Graphics
    GpStatus WINAPI GdipCreateFromHDC(HDC hdc, void** graphics);
    GpStatus WINAPI GdipDeleteGraphics(void* graphics);
    GpStatus WINAPI GdipSetSmoothingMode(void* graphics, int smoothingMode);
    GpStatus WINAPI GdipSetPixelOffsetMode(void* graphics, int pixelOffsetMode);

    // Pen
    GpStatus WINAPI GdipCreatePen1(UINT32 color, float width, int unit, void** pen);
    GpStatus WINAPI GdipDeletePen(void* pen);
    GpStatus WINAPI GdipSetPenLineJoin(void* pen, int lineJoin);
    GpStatus WINAPI GdipSetPenStartCap(void* pen, int startCap);
    GpStatus WINAPI GdipSetPenEndCap(void* pen, int endCap);
    GpStatus WINAPI GdipSetPenDashStyle(void* pen, int dashStyle);

    // Draw
    GpStatus WINAPI GdipDrawLinesI(void* graphics, void* pen, const POINT* points, int count);
}

// GDI+ enum constants
namespace GpConst {
    enum {
        SmoothingModeAntiAlias = 4,
        PixelOffsetModeHalf    = 4,
        UnitPixel              = 2,
        LineJoinRound          = 2,
        LineCapRound           = 2,
        DashStyleDash          = 1,
    };
}
#include <cstdarg>
#include <cstdio>

// Для AlphaBlend (полупрозрачное выделение области)
#pragma comment(lib, "Msimg32.lib")
#pragma comment(lib, "gdiplus.lib")

static void AppendDebugLog(const wchar_t* fmt, ...) {
     wchar_t buffer[2048];
     va_list args;
     va_start(args, fmt);
     _vsnwprintf_s(buffer, _countof(buffer), _TRUNCATE, fmt, args);
     va_end(args);

     OutputDebugStringW(buffer);

     FILE* f = nullptr;
     if (_wfopen_s(&f, L"\u0432\u044b\u0432\u043e\u0434.txt", L"a+, ccs=UTF-8") == 0 && f) {
         fputws(buffer, f);
         fclose(f);
     }
 }

// Вспомогательная функция для конвертации UTF-8 в UTF-16 (локальная для этого файла)
static std::wstring Utf8ToWide(const std::string& utf8str) {
    if (utf8str.empty()) return std::wstring();

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8str.c_str(), (int)utf8str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8str.c_str(), (int)utf8str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// Определение цветов для каналов
const COLORREF ChartModule::CHANNEL_COLORS[5] = {
    // Спокойная "офисная" палитра (без неоновых цветов), хорошо читается при 1px линиях.
    RGB(0, 128, 0),      // T1 - тёмно-зелёный
    RGB(139, 69, 19),    // T2 - коричневый
    RGB(200, 50, 50),    // T3 - приглушённый красный
    RGB(50, 90, 190),    // T4 - приглушённый синий
    RGB(170, 80, 140)    // T5 - приглушённый пурпурный
};

const wchar_t* ChartModule::CHANNEL_NAMES[5] = {
    L"T1", L"T2", L"T3", L"T4", L"T5"
};

ChartModule::ChartModule() {
}

ChartModule::~ChartModule() {
}

namespace {
    // Ненавязчивый "корпоративный" стиль: лёгкий фон, мягкая сетка, читаемые подписи.
    constexpr COLORREF kChartBg = RGB(248, 249, 251);     // общий фон области графика
    constexpr COLORREF kPlotBg = RGB(255, 255, 255);      // фон plot-области
    constexpr COLORREF kBorder = RGB(210, 214, 219);      // рамка графика
    constexpr COLORREF kAxis = RGB(85, 85, 85);           // оси/засечки
    constexpr COLORREF kText = RGB(45, 45, 45);           // подписи
    constexpr COLORREF kGridMajor = RGB(226, 230, 235);   // основные линии сетки

    // Толщина КРИВЫХ (сетка/оси остаются тонкими).
    constexpr int kCurveWidthPx = 2;

    struct GdiPlusScope {
        ULONG_PTR token = 0;
        bool initialized = false;

        GdiPlusScope() {
            GdiplusStartupInput input = { 1, nullptr, FALSE, FALSE };
            initialized = (GdiplusStartup(&token, &input, nullptr) == GpOk);
        }

        ~GdiPlusScope() {
            if (initialized) {
                GdiplusShutdown(token);
            }
        }
    };

    static bool IsGdiPlusReady() {
        static GdiPlusScope scope;
        return scope.initialized;
    }

    static HPEN CreateCurvePen(int lineStyle, COLORREF color) {
        if (lineStyle == PS_DASH) {
            if (kCurveWidthPx <= 1) {
                return CreatePen(PS_DASH, 1, color);
            }

            // Для толщины > 1px используем геометрический pen, иначе GDI часто "ломает" пунктир.
            LOGBRUSH lb = {};
            lb.lbStyle = BS_SOLID;
            lb.lbColor = color;

            // Ненавязчивый пунктир (в пикселях). При желании можно подстроить под толщину.
            DWORD pattern[2] = { 6, 4 }; // dash, gap
            return ExtCreatePen(
                PS_GEOMETRIC | PS_USERSTYLE | PS_ENDCAP_ROUND | PS_JOIN_ROUND,
                kCurveWidthPx,
                &lb,
                2,
                pattern
            );
        }

        return CreatePen(PS_SOLID, kCurveWidthPx, color);
    }

    static void DrawCurvePolyline(HDC hdc, const std::vector<POINT>& points, COLORREF color, int lineStyle) {
        if (points.size() < 2) {
            return;
        }

        if (IsGdiPlusReady()) {
            void* pGraphics = nullptr;
            if (GdipCreateFromHDC(hdc, &pGraphics) == GpOk && pGraphics) {
                GdipSetSmoothingMode(pGraphics, GpConst::SmoothingModeAntiAlias);
                GdipSetPixelOffsetMode(pGraphics, GpConst::PixelOffsetModeHalf);

                // ARGB: 0xAARRGGBB
                UINT32 argb = (0xFFu << 24) | (GetRValue(color) << 16) | (GetGValue(color) << 8) | GetBValue(color);
                void* pPen = nullptr;
                if (GdipCreatePen1(argb, static_cast<float>(kCurveWidthPx), GpConst::UnitPixel, &pPen) == GpOk && pPen) {
                    GdipSetPenLineJoin(pPen, GpConst::LineJoinRound);
                    GdipSetPenStartCap(pPen, GpConst::LineCapRound);
                    GdipSetPenEndCap(pPen, GpConst::LineCapRound);
                    if (lineStyle == PS_DASH) {
                        GdipSetPenDashStyle(pPen, GpConst::DashStyleDash);
                    }

                    GdipDrawLinesI(pGraphics, pPen, points.data(), static_cast<int>(points.size()));
                    GdipDeletePen(pPen);
                }
                GdipDeleteGraphics(pGraphics);
            }
            return;
        }

        // Fallback на GDI (если GDI+ недоступен)
        HPEN hPen = CreateCurvePen(lineStyle, color);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        Polyline(hdc, points.data(), static_cast<int>(points.size()));
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
    }
}

void ChartModule::DrawChart(HDC hdc, const RECT& rect, const std::vector<std::vector<float>>& chartData,
                           DataType type, const std::wstring& title, bool hasData, bool showAxes) const {
    // По умолчанию все передатчики включены
    std::vector<bool> allTransmittersEnabled = {true, true, true, true, true};
    DrawChart(hdc, rect, chartData, type, title, hasData, allTransmittersEnabled, nullptr, 0.0, 0.0, 0.0, 0.0, showAxes);
}

void ChartModule::DrawChart(HDC hdc, const RECT& rect, const std::vector<std::vector<float>>& chartData,
                           DataType type, const std::wstring& title, bool hasData,
                           const std::vector<bool>& enabledTransmitters, const DataModule* dataModule,
                           double minXData, double maxXData, double minYData, double maxYData, bool showAxes) const {
    if (rect.right <= rect.left || rect.bottom <= rect.top)
        return;

    // Отрисовка фона и рамки
    DrawChartBackground(hdc, rect);
    DrawChartBorder(hdc, rect);
    
    // Отрисовка заголовка
    DrawChartTitle(hdc, rect, title);
    
    // Расчет области для графика
    RECT plotRect = CalculatePlotRect(rect);

    // Фон plot-области (лёгкое визуальное разделение от "карточки" графика)
    {
        HBRUSH hPlotBrush = CreateSolidBrush(kPlotBg);
        FillRect(hdc, &plotRect, hPlotBrush);
        DeleteObject(hPlotBrush);
    }

    // Отрисовка осей (только если не заменены скроллбарами)
    DrawChartAxes(hdc, plotRect, showAxes);

    if (plotRect.right <= plotRect.left || plotRect.bottom <= plotRect.top)
        return;
    
    // Отрисовка сетки и осей с метками
    DrawGridAndAxes(hdc, plotRect, chartData, dataModule, 
                    minXData, maxXData, minYData, maxYData); // Передаем параметры зума
    
    // Отрисовка данных с учетом zoom
    if (hasData) {
        DrawChartData(hdc, plotRect, chartData, minXData, maxXData);
    }
    
    // Отрисовка легенды для многоканальных данных
    if (chartData.size() > 1) {
        DrawChartLegend(hdc, rect, chartData, type, enabledTransmitters);
    }
}

void ChartModule::DrawChartBackground(HDC hdc, const RECT& rect) const {
    HBRUSH hBrush = CreateSolidBrush(kChartBg);
    FillRect(hdc, &rect, hBrush);
    DeleteObject(hBrush);
}

void ChartModule::DrawChartBorder(HDC hdc, const RECT& rect) const {
    HPEN hPen = CreatePen(PS_SOLID, 1, kBorder);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

    Rectangle(hdc, rect.left, rect.top, rect.right, rect.bottom);

    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

void ChartModule::DrawChartTitle(HDC hdc, const RECT& rect, const std::wstring& title) const {
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    SetTextColor(hdc, kText);
    SetBkMode(hdc, TRANSPARENT);
    TextOut(hdc, rect.left + 10, rect.top + 10, title.c_str(), (int)title.length());
    SelectObject(hdc, hOldFont);
}


void ChartModule::DrawChartAxes(HDC hdc, const RECT& plotRect, bool showAxes) const {
    if (!showAxes) return; // Не рисуем оси, если они заменены скроллбарами
    
    HPEN hPen = CreatePen(PS_SOLID, 1, kAxis); // Сплошная линия для осей
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

    // X-ось
    MoveToEx(hdc, plotRect.left, plotRect.bottom, NULL);
    LineTo(hdc, plotRect.right, plotRect.bottom);
    
    // Y-ось
    MoveToEx(hdc, plotRect.left, plotRect.top, NULL);
    LineTo(hdc, plotRect.left, plotRect.bottom);

    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

void ChartModule::DrawChartData(HDC hdc, const RECT& plotRect, const std::vector<std::vector<float>>& chartData) const {
    // Находим минимальные и максимальные значения с padding
    float minVal, maxVal;
    FindMinMaxValues(chartData, minVal, maxVal);
    
    // Рисуем данные для каждого канала
    for (size_t channelIdx = 0; channelIdx < chartData.size(); channelIdx++) {
        const auto& channel = chartData[channelIdx];
        if (channel.empty())
            continue;

        std::vector<POINT> points;
        points.reserve(channel.size());
        const double xDenominator = (channel.size() > 1) ? static_cast<double>(channel.size() - 1) : 1.0;
        for (size_t i = 0; i < channel.size(); i++) {
            int x = plotRect.left + (int)((double)i / xDenominator * (plotRect.right - plotRect.left));
            int y = plotRect.bottom - (int)((channel[i] - minVal) / (maxVal - minVal) * (plotRect.bottom - plotRect.top));
            points.push_back({ x, y });
        }

        DrawCurvePolyline(hdc, points, CHANNEL_COLORS[channelIdx % 5], PS_SOLID);
    }
}

// Перегрузка с поддержкой zoom для правильного отображения X координат
void ChartModule::DrawChartData(HDC hdc, const RECT& plotRect, const std::vector<std::vector<float>>& chartData,
                                double minXData, double maxXData) const {
    // Находим минимальные и максимальные значения с padding
    float minVal, maxVal;
    FindMinMaxValues(chartData, minVal, maxVal);
    
    if (maxXData <= minXData) {
        maxXData = minXData + 1; // Избегаем деления на ноль
    }
    
    // Полный диапазон данных (от 0 до размера массива)
    double fullXRange = static_cast<double>(chartData.empty() || chartData[0].empty() ? 1 : chartData[0].size() - 1);
    
    // Рисуем данные для каждого канала
    for (size_t channelIdx = 0; channelIdx < chartData.size(); channelIdx++) {
        const auto& channel = chartData[channelIdx];
        if (channel.empty())
            continue;

        std::vector<POINT> points;
        points.reserve(channel.size());
        for (size_t i = 0; i < channel.size(); i++) {
            // Реальная X координата в данных (индекс i соответствует реальной позиции в данных)
            double realX = static_cast<double>(i);
            
            // Проверяем, находится ли точка в видимом диапазоне zoom
            if (realX < minXData || realX > maxXData) {
                continue; // Пропускаем точки вне видимого диапазона
            }
            
            // Вычисляем экранные координаты относительно видимого диапазона
            int x = plotRect.left + (int)((realX - minXData) / (maxXData - minXData) * (plotRect.right - plotRect.left));
            int y = plotRect.bottom - (int)((channel[i] - minVal) / (maxVal - minVal) * (plotRect.bottom - plotRect.top));

            points.push_back({ x, y });
        }

        DrawCurvePolyline(hdc, points, CHANNEL_COLORS[channelIdx % 5], PS_SOLID);
    }
}

void ChartModule::DrawChartLegend(HDC hdc, const RECT& rect, const std::vector<std::vector<float>>& chartData,
                                  DataType type, const std::vector<bool>& enabledTransmitters) const {
    // Определяем, является ли тип данных многоканальным
    bool isMultiChannel = false;
    if (type == DataType::PHASE_DIFFERENCE || type == DataType::AMPLITUDE_RATIO ||
        type == DataType::AMPLITUDE_RX1 || type == DataType::AMPLITUDE_RX2 ||
        type == DataType::PHASE_SMT || type == DataType::RHO_SMT) {
        isMultiChannel = true;
    }

    if (!isMultiChannel) {
        // Для одноканальных данных легенда отображается без чекбоксов
        // Пока не отрисовываем легенду для одноканальных данных вообще
        return;
    }

    // Параметры легенды
    int checkboxSize = 12;        // Размер чекбокса
    int elementSpacing = 15;      // Промежуток между элементами легенды
    int lineHeight = 20;          // Высота строки легенды
    
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, kText);
    
    // Вычисляем ширину каждого элемента легенды
    struct LegendElement {
        int width;
        int channelIdx;
    };
    
    std::vector<LegendElement> elements;
    int totalWidth = 0;
    int lineLength = 15; // Длина цветной линии
    
    for (size_t channelIdx = 0; channelIdx < 5; channelIdx++) {
        if (isMultiChannel && channelIdx < enabledTransmitters.size()) {
            // Ширина элемента = чекбокс + отступ + линия + отступ + текст
            SIZE textSize;
            GetTextExtentPoint32(hdc, CHANNEL_NAMES[channelIdx], (int)wcslen(CHANNEL_NAMES[channelIdx]), &textSize);
            int elementWidth = checkboxSize + 5 + lineLength + 5 + textSize.cx;
            
            elements.push_back({elementWidth, (int)channelIdx});
            totalWidth += elementWidth;
            
            if (channelIdx < 5) { // Добавляем промежуток между элементами (кроме последнего)
                totalWidth += elementSpacing;
            }
        }
    }
    
    // Вычисляем начальную X позицию для центрирования
    int chartWidth = rect.right - rect.left;
    int startX = rect.left + (chartWidth - totalWidth) / 2;
    
    // Позиция Y - под заголовком
    int legendY = rect.top + 35;
    
    // Отрисовка элементов легенды
    int currentX = startX;
    
    for (const auto& elem : elements) {
        int channelIdx = elem.channelIdx;
        COLORREF color = CHANNEL_COLORS[channelIdx % 5];
        
        // Отрисовка чекбокса
        RECT checkboxRect = {currentX,
                             legendY + (lineHeight - checkboxSize) / 2,
                             currentX + checkboxSize,
                             legendY + (lineHeight + checkboxSize) / 2};
        DrawFrameControl(hdc, &checkboxRect, DFC_BUTTON, DFCS_BUTTONCHECK | (enabledTransmitters[channelIdx] ? DFCS_CHECKED : 0));
        
        currentX += checkboxSize + 5;
        
        // Отрисовка цветной линии
        HPEN hPen = CreateCurvePen(PS_SOLID, color);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        
        MoveToEx(hdc, currentX, legendY + lineHeight / 2, NULL);
        LineTo(hdc, currentX + lineLength, legendY + lineHeight / 2);
        
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
        
        currentX += lineLength + 5;
        
        // Отрисовка названия канала (CHANNEL_NAMES уже wide-строки)
        const wchar_t* name = CHANNEL_NAMES[channelIdx];
        TextOut(hdc, currentX, legendY, name, (int)wcslen(name));
        
        SIZE textSize;
        GetTextExtentPoint32(hdc, name, (int)wcslen(name), &textSize);
        currentX += textSize.cx + elementSpacing;
    }
    
    SelectObject(hdc, hOldFont);
}

// Обновленная DrawChartLegend (для совместимости)
void ChartModule::DrawChartLegend(HDC hdc, const RECT& rect, const std::vector<std::vector<float>>& chartData) const {
    // По умолчанию все передатчики включены
    std::vector<bool> allTransmittersEnabled = {true, true, true, true, true};
    DrawChartLegend(hdc, rect, chartData, DataType::PHASE_DIFFERENCE, allTransmittersEnabled); // Используем PHASE_DIFFERENCE как заглушку для многоканального типа
}


int ChartModule::HitTestLegendCheckbox(const RECT& chartRect, const POINT& pt, DataType type) const {
    // Определяем, является ли тип данных многоканальным
    bool isMultiChannel = false;
    if (type == DataType::PHASE_DIFFERENCE || type == DataType::AMPLITUDE_RATIO ||
        type == DataType::AMPLITUDE_RX1 || type == DataType::AMPLITUDE_RX2 ||
        type == DataType::PHASE_SMT || type == DataType::RHO_SMT) {
        isMultiChannel = true;
    }

    if (!isMultiChannel) {
        return -1; // Для одноканальных данных нет чекбоксов
    }

    // Параметры легенды (должны совпадать с DrawChartLegend)
    int checkboxSize = 12;
    int lineLength = 15;
    int elementSpacing = 15;
    int lineHeight = 20;
    
    // Создаем временный DC для вычисления размеров текста
    HDC hdc = GetDC(NULL);
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    
    // Вычисляем ширину каждого элемента и общую ширину
    struct LegendElement {
        int width;
        int channelIdx;
        int checkboxX;
    };
    
    std::vector<LegendElement> elements;
    int totalWidth = 0;
    
    for (int channelIdx = 0; channelIdx < 5; channelIdx++) {
        if (isMultiChannel) {
            SIZE textSize;
            GetTextExtentPoint32(hdc, CHANNEL_NAMES[channelIdx], (int)wcslen(CHANNEL_NAMES[channelIdx]), &textSize);
            int elementWidth = checkboxSize + 5 + lineLength + 5 + textSize.cx;
            
            elements.push_back({elementWidth, channelIdx, 0});
            totalWidth += elementWidth;
            
            if (channelIdx < 5) {
                totalWidth += elementSpacing;
            }
        }
    }
    
    SelectObject(hdc, hOldFont);
    ReleaseDC(NULL, hdc);
    
    // Вычисляем начальную X позицию для центрирования
    int chartWidth = chartRect.right - chartRect.left;
    int startX = chartRect.left + (chartWidth - totalWidth) / 2;
    int legendY = chartRect.top + 35;
    
    // Вычисляем позиции чекбоксов
    int currentX = startX;
    for (auto& elem : elements) {
        elem.checkboxX = currentX; // Чекбокс теперь первый элемент
        currentX += elem.width + elementSpacing;
    }
    
    // Проверяем попадание в чекбоксы
    for (const auto& elem : elements) {
        RECT checkboxRect = {elem.checkboxX,
                             legendY + (lineHeight - checkboxSize) / 2,
                             elem.checkboxX + checkboxSize,
                             legendY + (lineHeight + checkboxSize) / 2};
        
        if (PtInRect(&checkboxRect, pt)) {
            return elem.channelIdx;
        }
    }

    return -1; // Не попали ни в один чекбокс
}

void ChartModule::DrawGridAndAxes(HDC hdc, const RECT& plotRect, const std::vector<std::vector<float>>& chartData) const {
    // Вызываем перегруженную версию с nullptr для dataModule, если он не передан.
    // Это означает, что GetDataSize() не будет использоваться в этой перегрузке.
    DrawGridAndAxes(hdc, plotRect, chartData, nullptr, 0.0, 0.0, 0.0, 0.0); // Передаем 0.0 для min/maxX/YData
}

void ChartModule::DrawGridAndAxes(HDC hdc, const RECT& plotRect, const std::vector<std::vector<float>>& chartData, const DataModule* dataModule) const {
    // Вызываем новую перегрузку с данными о зуме по умолчанию
    double minXData = 0.0;
    double maxXData = 0.0;
    if (dataModule && dataModule->HasData()) {
        maxXData = static_cast<double>(dataModule->GetDataSize() - 1);
    }
    DrawGridAndAxes(hdc, plotRect, chartData, dataModule, minXData, maxXData, 0.0, 0.0);
}

void ChartModule::DrawGridAndAxes(HDC hdc, const RECT& plotRect, const std::vector<std::vector<float>>& chartData, 
                                const DataModule* dataModule, double minXData, double maxXData, double minYData, double maxYData) const {
    // Если данных нет, то не рисуем сетку и метки
    if (!dataModule || !dataModule->HasData()) {
        return;
    }

    float minVal, maxVal;
    FindMinMaxValues(chartData, minVal, maxVal, minYData, maxYData); // Передаем minYData и maxYData

    // Если minVal == maxVal (все значения одинаковы), устанавливаем разумный диапазон по умолчанию
    if (minVal == FLT_MAX || maxVal == -FLT_MAX || minVal == maxVal) {
        minVal = static_cast<float>(minYData);
        maxVal = static_cast<float>(maxYData);
        if (minYData == 0.0 && maxYData == 0.0) { // Если не было зума, устанавливаем диапазон по умолчанию
            minVal = 0.0f;
            maxVal = 100.0f;
        }
    }
    
    // Константа для длины засечки
    const int TICK_LENGTH = 5;
    SetTextColor(hdc, kText);
    SetBkMode(hdc, TRANSPARENT);
    
    // Отрисовка горизонтальных линий сетки и меток Y-оси
    double niceStepY = CalculateNiceStep(maxVal - minVal, (plotRect.bottom - plotRect.top) / 40); // Примерное количество шагов
    double currentYVal = CalculateNiceMin(minVal, niceStepY);

    while (currentYVal <= maxVal + niceStepY / 2) { // Добавляем niceStepY / 2 для учета ошибок округления
        if (currentYVal >= minVal - niceStepY / 2 && currentYVal <= maxVal + niceStepY / 2) { // Проверка на видимый диапазон с запасом
            long screenY = plotRect.bottom - (LONG)((currentYVal - minVal) / (maxVal - minVal) * (plotRect.bottom - plotRect.top));
            
            // Проверяем, что линия сетки находится в пределах области графика
            if (screenY >= plotRect.top && screenY <= plotRect.bottom) {
                HPEN hPen = CreatePen(PS_SOLID, 1, kGridMajor);
                HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
                
                // Рисуем горизонтальную линию сетки только в пределах plotRect
                MoveToEx(hdc, plotRect.left, screenY, NULL);
                LineTo(hdc, plotRect.right, screenY);
                
                SelectObject(hdc, hOldPen);
                DeleteObject(hPen);
                
                // Рисуем засечку (tick mark) влево от оси Y
                HPEN hTickPen = CreatePen(PS_SOLID, 1, kAxis);
                HPEN hOldTickPen = (HPEN)SelectObject(hdc, hTickPen);
                MoveToEx(hdc, plotRect.left - TICK_LENGTH, screenY, NULL);
                LineTo(hdc, plotRect.left, screenY);
                SelectObject(hdc, hOldTickPen);
                DeleteObject(hTickPen);

                // Отрисовка метки Y-оси (в области отступа слева от plotRect)
                std::wstringstream ssY;
                ssY << std::fixed << std::setprecision(1) << currentYVal;
                std::wstring yLabel = ssY.str();
                
                // Вычисляем ширину текста для правильного выравнивания
                SIZE textSize;
                GetTextExtentPoint32(hdc, yLabel.c_str(), (int)yLabel.length(), &textSize);
                
                // Рисуем текст так, чтобы он заканчивался в 7 пикселях от оси (учитываем засечку)
                int textX = plotRect.left - TICK_LENGTH - 2 - textSize.cx;
                TextOut(hdc, textX, screenY - 7, yLabel.c_str(), (int)yLabel.length());
            }
        }
        currentYVal += niceStepY;
    }

    // Отрисовка вертикальных линий сетки и меток X-оси
    if (dataModule && dataModule->HasData()) {
        double currentMinX = minXData; // Используем переданный диапазон
        double currentMaxX = maxXData;

        if (currentMaxX == 0.0 && dataModule->GetDataSize() > 0) { // Если не был передан зум, используем полный диапазон
            currentMaxX = static_cast<double>(dataModule->GetDataSize() - 1);
        }

        if (currentMaxX <= currentMinX) currentMaxX = currentMinX + 1; // Избегаем деления на ноль

        double xRange = currentMaxX - currentMinX;
        double niceStepX = CalculateNiceStep(xRange, (plotRect.right - plotRect.left) / 60);
        double currentXVal = CalculateNiceMin(currentMinX, niceStepX);

        while (currentXVal <= currentMaxX + niceStepX / 2) { // Добавляем niceStepX / 2 для учета ошибок округления
            if (currentXVal >= currentMinX - niceStepX / 2 && currentXVal <= currentMaxX + niceStepX / 2) { // Проверка на видимый диапазон с запасом
                long screenX = plotRect.left + (LONG)((currentXVal - currentMinX) / xRange * (plotRect.right - plotRect.left));
                
                // Проверяем, что линия сетки находится в пределах области графика
                if (screenX >= plotRect.left && screenX <= plotRect.right) {
                    HPEN hPen = CreatePen(PS_SOLID, 1, kGridMajor);
                    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
                    
                    // Рисуем вертикальную линию сетки только в пределах plotRect
                    MoveToEx(hdc, screenX, plotRect.top, NULL);
                    LineTo(hdc, screenX, plotRect.bottom);
                    
                    SelectObject(hdc, hOldPen);
                    DeleteObject(hPen);
                    
                    // Рисуем засечку (tick mark) вниз от оси X
                    HPEN hTickPen = CreatePen(PS_SOLID, 1, kAxis);
                    HPEN hOldTickPen = (HPEN)SelectObject(hdc, hTickPen);
                    MoveToEx(hdc, screenX, plotRect.bottom, NULL);
                    LineTo(hdc, screenX, plotRect.bottom + TICK_LENGTH);
                    SelectObject(hdc, hOldTickPen);
                    DeleteObject(hTickPen);

                    // Отрисовка метки X-оси (в области отступа снизу от plotRect)
                    std::wstringstream ssX;
                    ssX << std::fixed << std::setprecision(1) << currentXVal;
                    std::wstring xLabel = ssX.str();
                    TextOut(hdc, screenX - 10, plotRect.bottom + TICK_LENGTH + 2, xLabel.c_str(), (int)xLabel.length());
                }
            }
            currentXVal += niceStepX;
        }
        
        // Рисуем ось X (не утолщаем: при плотных данных толстая ось визуально давит)
        HPEN hAxisPen = CreatePen(PS_SOLID, 1, kAxis);
        HPEN hOldAxisPen = (HPEN)SelectObject(hdc, hAxisPen);
        MoveToEx(hdc, plotRect.left, plotRect.bottom, NULL);
        LineTo(hdc, plotRect.right, plotRect.bottom);
        SelectObject(hdc, hOldAxisPen);
        DeleteObject(hAxisPen);
    }
}

// Метод для отрисовки только сетки без подписей осей (для состояния "кривые не выбраны")
void ChartModule::DrawGridOnly(HDC hdc, const RECT& plotRect) const {
    // Рисуем только линии сетки без подписей осей
    // Используем фиксированное количество линий (10 горизонтальных и 10 вертикальных)
    
    int plotWidth = plotRect.right - plotRect.left;
    int plotHeight = plotRect.bottom - plotRect.top;
    
    if (plotWidth <= 0 || plotHeight <= 0) return;
    
    HPEN hPen = CreatePen(PS_SOLID, 1, kGridMajor);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    
    // Горизонтальные линии сетки (10 линий)
    for (int i = 0; i <= 10; i++) {
        int y = plotRect.top + (plotHeight * i) / 10;
        MoveToEx(hdc, plotRect.left, y, NULL);
        LineTo(hdc, plotRect.right, y);
    }
    
    // Вертикальные линии сетки (10 линий)
    for (int i = 0; i <= 10; i++) {
        int x = plotRect.left + (plotWidth * i) / 10;
        MoveToEx(hdc, x, plotRect.top, NULL);
        LineTo(hdc, x, plotRect.bottom);
    }
    
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}


RECT ChartModule::CalculatePlotRect(const RECT& chartRect) const {
    RECT plotRect = chartRect;
    plotRect.left += 80;   // Увеличенный отступ для Y-оси (для длинных значений мВ и мдег)
    plotRect.top += 45;    // Отступ для заголовка + горизонтальная легенда (10px заголовок + 25px легенда + 10px отступ)
    plotRect.right -= 30;  // Увеличенный отступ справа
    plotRect.bottom -= 50; // Увеличенный отступ для X-оси и скроллбара
    return plotRect;
}

RECT ChartModule::CalculateDualAxisPlotRect(const RECT& chartRect) const {
    RECT plotRect = chartRect;
    // ВАЖНО: этот отступ должен быть согласован с вычислениями plotRect в ChartManager
    // (ConvertScreenToDataCoords / ConvertScreenToChartCoords / DrawCrosshair).
    // Увеличено с 60 → 80 → 100, чтобы подписи левой оси (например, V/V) не обрезались слева.
    plotRect.left += 100;   // Отступ для левой Y-оси
    plotRect.top += 45;    // Отступ для заголовка + горизонтальная легенда (10px заголовок + 25px легенда + 10px отступ)
    plotRect.right -= 140; // Увеличенный отступ для правой Y-оси (для мВ с 2 знаками + scrollbar)
    plotRect.bottom -= 50; // Увеличенный отступ для X-оси и скроллбара
    return plotRect;
}

void ChartModule::FindMinMaxValues(const std::vector<std::vector<float>>& chartData, float& minVal, float& maxVal) const {
    FindMinMaxValues(chartData, minVal, maxVal, 0.0, 100.0); // По умолчанию используем 0-100 для Y
}

void ChartModule::FindMinMaxValues(const std::vector<std::vector<float>>& chartData, float& minVal, float& maxVal, double minYPlot, double maxYPlot) const {
    minVal = FLT_MAX;
    maxVal = -FLT_MAX;
    
    // Находим исходные минимум и максимум
    for (const auto& channel : chartData) {
        for (float val : channel) {
            minVal = (std::min)(minVal, val);
            maxVal = (std::max)(maxVal, val);
        }
    }
    
    // Если данных нет, устанавливаем значения по умолчанию из minYPlot и maxYPlot
    if (minVal == FLT_MAX || maxVal == -FLT_MAX) {
        minVal = static_cast<float>(minYPlot);
        maxVal = static_cast<float>(maxYPlot);
        // Если minYPlot и maxYPlot тоже 0, то устанавливаем глобальный дефолт
        if (minYPlot == 0.0 && maxYPlot == 0.0) {
            minVal = 0.0f;
            maxVal = 100.0f;
        }
        return;
    }
    
    // Если minVal == maxVal (все значения одинаковы), устанавливаем разумный диапазон по умолчанию
    if (minVal == maxVal) {
        minVal = static_cast<float>(minYPlot); 
        maxVal = static_cast<float>(maxYPlot); 
        if (minYPlot == 0.0 && maxYPlot == 0.0) {
            minVal = 0.0f;
            maxVal = 100.0f;
        }
        return;
    }
    
    // Рассчитываем диапазон данных
    float range = maxVal - minVal;
    // Добавляем небольшой padding для лучшей визуализации, если диапазон достаточно большой
    if (range > 0.001f) { // Избегаем деления на ноль для очень маленьких диапазонов
        float padding = range * 0.05f; // 5% от диапазона
        minVal -= padding;
        maxVal += padding;
    }
}


double ChartModule::CalculateNiceStep(double range, int targetSteps) const {
    if (range <= 0 || targetSteps <= 0)
        return 1.0;
    
    double roughStep = range / targetSteps;
    double magnitude = pow(10.0, floor(log10(roughStep)));
    double normalizedStep = roughStep / magnitude;
    
    // Выбираем "красивый" шаг
    double niceStep;
    if (normalizedStep <= 1.0)
        niceStep = 1.0;
    else if (normalizedStep <= 2.0)
        niceStep = 2.0;
    else if (normalizedStep <= 5.0)
        niceStep = 5.0;
    else
        niceStep = 10.0;
    
    return niceStep * magnitude;
}

double ChartModule::CalculateNiceMin(double minVal, double step) const {
    return floor(minVal / step) * step;
}

void ChartModule::DrawZoomRectangle(HDC hdc, const RECT& zoomRect) const {
    if (!hdc)
        return;

    if (zoomRect.right <= zoomRect.left || zoomRect.bottom <= zoomRect.top)
        return;

    // Новый дизайн: выделение области системным серым цветом (без XOR и без оранжевого пунктира).
    // Цвет берём из системной темы Windows.
    const int w = zoomRect.right - zoomRect.left;
    const int h = zoomRect.bottom - zoomRect.top;

    // Полупрозрачный оверлей, чтобы данные под ним оставались видимыми
    bool blended = false;
    HDC memDC = CreateCompatibleDC(hdc);
    if (memDC) {
        HBITMAP bmp = CreateCompatibleBitmap(hdc, w, h);
        if (bmp) {
            HGDIOBJ oldBmp = SelectObject(memDC, bmp);

            RECT local = { 0, 0, w, h };
            HBRUSH brush = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
            if (brush) {
                FillRect(memDC, &local, brush);
                DeleteObject(brush);
            }

            BLENDFUNCTION bf = {};
            bf.BlendOp = AC_SRC_OVER;
            bf.SourceConstantAlpha = 96; // ~38% непрозрачности
            bf.AlphaFormat = 0;

            blended = (AlphaBlend(hdc, zoomRect.left, zoomRect.top, w, h,
                                  memDC, 0, 0, w, h, bf) != FALSE);

            SelectObject(memDC, oldBmp);
            DeleteObject(bmp);
        }
        DeleteDC(memDC);
    }

    // Fallback если AlphaBlend недоступен/не сработал
    if (!blended) {
        FillRect(hdc, &zoomRect, GetSysColorBrush(COLOR_BTNFACE));
    }

    // Лёгкая рамка системным цветом, чтобы область была читаемой на белом фоне.
    HPEN hPen = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_3DSHADOW));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));

    Rectangle(hdc, zoomRect.left, zoomRect.top, zoomRect.right, zoomRect.bottom);

    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

// Методы для поддержки двойных осей ординат
void ChartModule::DrawDualAxisChart(HDC hdc, const RECT& rect, 
                                   const std::vector<CurveDefinition>& curves,
                                   const std::vector<std::vector<float>>& leftAxisData,
                                   const std::vector<std::vector<float>>& rightAxisData,
                                   const std::wstring& title, bool hasData) const {
    if (rect.right <= rect.left || rect.bottom <= rect.top)
        return;

    // Отрисовка фона и рамки
    DrawDualAxisBackground(hdc, rect);
    DrawChartBorder(hdc, rect);
    
    // Отрисовка заголовка
    DrawChartTitle(hdc, rect, title);
    
    // Расчет области для графика с двойными осями
    RECT plotRect = CalculateDualAxisPlotRect(rect);
    
    if (plotRect.right <= plotRect.left || plotRect.bottom <= plotRect.top)
        return;
    
    // Отрисовка сетки и осей (без оси X для старой версии)
    DrawDualAxisGrid(hdc, plotRect, leftAxisData, rightAxisData);
    
    // Отрисовка левой и правой осей с подписями
    std::string leftUnits, rightUnits;
    for (const auto& curve : curves) {
        if (!curve.enabled) continue;
        
        if (curve.yAxisGroup == 0 && leftUnits.empty()) {
            leftUnits = curve.units;
        } else if (curve.yAxisGroup == 1 && rightUnits.empty()) {
            rightUnits = curve.units;
        }
    }
    
    if (!leftAxisData.empty()) {
        DrawLeftAxis(hdc, plotRect, leftAxisData, leftUnits);
    }
    if (!rightAxisData.empty()) {
        DrawRightAxis(hdc, plotRect, rightAxisData, rightUnits);
    }
    
    // Отрисовка данных
    if (hasData) {
        DrawDualAxisData(hdc, plotRect, curves, leftAxisData, rightAxisData);
    }
    
    // Отрисовка легенды
    if (!curves.empty()) {
        DrawDualAxisLegend(hdc, rect, curves);
    }
}

// Перегрузка с поддержкой zoom
void ChartModule::DrawDualAxisChart(HDC hdc, const RECT& rect, 
                                   const std::vector<CurveDefinition>& curves,
                                   const std::vector<std::vector<float>>& leftAxisData,
                                   const std::vector<std::vector<float>>& rightAxisData,
                                   const std::wstring& title, bool hasData,
                                   double minXData, double maxXData, 
                                   double minYData, double maxYData, bool showAxes) const {
    if (rect.right <= rect.left || rect.bottom <= rect.top)
        return;

    // Отрисовка фона и рамки
    DrawDualAxisBackground(hdc, rect);
    DrawChartBorder(hdc, rect);
    
    // Отрисовка заголовка
    DrawChartTitle(hdc, rect, title);
    
    // Расчет области для графика с двойными осями
    RECT plotRect = CalculateDualAxisPlotRect(rect);
    
    if (plotRect.right <= plotRect.left || plotRect.bottom <= plotRect.top)
        return;
    
    // Отрисовка сетки и осей с учетом zoom (включая ось X и Y)
    if (showAxes) {
        DrawDualAxisGrid(hdc, plotRect, leftAxisData, rightAxisData, minXData, maxXData, minYData, maxYData);
    }
    
    // Отрисовка левой и правой осей с подписями
    std::string leftUnits, rightUnits;
    for (const auto& curve : curves) {
        if (!curve.enabled) continue;
        
        if (curve.yAxisGroup == 0 && leftUnits.empty()) {
            leftUnits = curve.units;
        } else if (curve.yAxisGroup == 1 && rightUnits.empty()) {
            rightUnits = curve.units;
        }
    }
    
    // Вычисляем полные диапазоны из данных
    double leftMinFull = 0.0, leftMaxFull = 1.0;
    if (!leftAxisData.empty()) {
        float minVal = FLT_MAX, maxVal = -FLT_MAX;
        FindMinMaxValues(leftAxisData, minVal, maxVal);
        leftMinFull = static_cast<double>(minVal);
        leftMaxFull = static_cast<double>(maxVal);
    }
    
    double rightMinFull = 0.0, rightMaxFull = 1.0;
    if (!rightAxisData.empty()) {
        float minVal = FLT_MAX, maxVal = -FLT_MAX;
        FindMinMaxValues(rightAxisData, minVal, maxVal);
        rightMinFull = static_cast<double>(minVal);
        rightMaxFull = static_cast<double>(maxVal);
    }
    
    // Вычисляем пропорциональный zoom для правой оси
    // Если minYData/maxYData отличаются от полного диапазона левой оси, применяем ту же пропорцию к правой
    double minYRight = rightMinFull, maxYRight = rightMaxFull;
    double leftFullRange = leftMaxFull - leftMinFull;
    double rightFullRange = rightMaxFull - rightMinFull;
    
    if (leftFullRange > 0.0001 && rightFullRange > 0.0001) {
        // Вычисляем пропорцию zoom от левой оси
        double minRatio = (minYData - leftMinFull) / leftFullRange;
        double maxRatio = (maxYData - leftMinFull) / leftFullRange;
        
        // Применяем ту же пропорцию к правой оси
        minYRight = rightMinFull + minRatio * rightFullRange;
        maxYRight = rightMinFull + maxRatio * rightFullRange;
    }
    
    if (!leftAxisData.empty()) {
        DrawLeftAxisWithZoom(hdc, plotRect, leftAxisData, leftUnits, minYData, maxYData);
    }
    
    if (!rightAxisData.empty()) {
        DrawRightAxisWithZoom(hdc, plotRect, rightAxisData, rightUnits, minYRight, maxYRight);
    }
    
    // Отрисовка данных с учетом zoom по Y
    if (hasData) {
        // Используем разные диапазоны для левой и правой осей
        DrawDualAxisDataWithZoom(hdc, plotRect, curves, leftAxisData, rightAxisData, 
                                 minYData, maxYData, minYRight, maxYRight,
                                 minXData, maxXData);
    }
    
    // Отрисовка легенды
    if (!curves.empty()) {
        DrawDualAxisLegend(hdc, rect, curves);
    }
}

// v2.15.0: Перегрузка с поддержкой zoom и конфигурации (для пресетов)
void ChartModule::DrawDualAxisChart(HDC hdc, const RECT& rect, 
                                   const std::vector<CurveDefinition>& curves,
                                   const std::vector<std::vector<float>>& leftAxisData,
                                   const std::vector<std::vector<float>>& rightAxisData,
                                   const std::wstring& title, bool hasData,
                                   double minXData, double maxXData, 
                                   double minYData, double maxYData,
                                   const ChartConfiguration& config, bool showAxes) const {
    if (rect.right <= rect.left || rect.bottom <= rect.top)
        return;

    // Отрисовка фона и рамки
    DrawDualAxisBackground(hdc, rect);
    DrawChartBorder(hdc, rect);
    
    // Отрисовка заголовка
    DrawChartTitle(hdc, rect, title);
    
    // Расчет области для графика с двойными осями
    RECT plotRect = CalculateDualAxisPlotRect(rect);
    
    if (plotRect.right <= plotRect.left || plotRect.bottom <= plotRect.top)
        return;
    
    // Отрисовка сетки и осей с учетом zoom (включая ось X и Y)
    if (showAxes) {
        DrawDualAxisGrid(hdc, plotRect, leftAxisData, rightAxisData, minXData, maxXData, minYData, maxYData);
    }
    
    // Отрисовка левой и правой осей с подписями
    std::string leftUnits, rightUnits;
    for (const auto& curve : curves) {
        if (!curve.enabled) continue;
        
        if (curve.yAxisGroup == 0 && leftUnits.empty()) {
            leftUnits = curve.units;
        } else if (curve.yAxisGroup == 1 && rightUnits.empty()) {
            rightUnits = curve.units;
        }
    }
    
    // Вычисляем полные диапазоны из данных
    double leftMinFull = 0.0, leftMaxFull = 1.0;
    if (!leftAxisData.empty()) {
        float minVal = FLT_MAX, maxVal = -FLT_MAX;
        FindMinMaxValues(leftAxisData, minVal, maxVal);
        leftMinFull = static_cast<double>(minVal);
        leftMaxFull = static_cast<double>(maxVal);
    }
    
    double rightMinFull = 0.0, rightMaxFull = 1.0;
    if (!rightAxisData.empty()) {
        float minVal = FLT_MAX, maxVal = -FLT_MAX;
        FindMinMaxValues(rightAxisData, minVal, maxVal);
        rightMinFull = static_cast<double>(minVal);
        rightMaxFull = static_cast<double>(maxVal);
    }
    
    // Вычисляем пропорциональный zoom для правой оси
    double minYRight = rightMinFull, maxYRight = rightMaxFull;
    double leftFullRange = leftMaxFull - leftMinFull;
    double rightFullRange = rightMaxFull - rightMinFull;
    
    if (leftFullRange > 0.0001 && rightFullRange > 0.0001) {
        double minRatio = (minYData - leftMinFull) / leftFullRange;
        double maxRatio = (maxYData - leftMinFull) / leftFullRange;
        minYRight = rightMinFull + minRatio * rightFullRange;
        maxYRight = rightMinFull + maxRatio * rightFullRange;
    }
    
    if (!leftAxisData.empty()) {
        DrawLeftAxisWithZoom(hdc, plotRect, leftAxisData, leftUnits, minYData, maxYData);
    }
    
    if (!rightAxisData.empty()) {
        DrawRightAxisWithZoom(hdc, plotRect, rightAxisData, rightUnits, minYRight, maxYRight);
    }
    
    // Отрисовка данных с учетом zoom по Y
    if (hasData) {
        DrawDualAxisDataWithZoom(hdc, plotRect, curves, leftAxisData, rightAxisData, 
                                 minYData, maxYData, minYRight, maxYRight,
                                 minXData, maxXData);
    }
    
    // Отрисовка легенды с учетом пресета
    if (!curves.empty()) {
        DrawDualAxisLegend(hdc, rect, curves, config);
    }
}

void ChartModule::DrawDualAxisBackground(HDC hdc, const RECT& rect) const {
    // Используем тот же фон, что и для обычных графиков
    DrawChartBackground(hdc, rect);
}

void ChartModule::DrawDualAxisGrid(HDC hdc, const RECT& plotRect, 
                                  const std::vector<std::vector<float>>& leftAxisData,
                                  const std::vector<std::vector<float>>& rightAxisData) const {
    // Находим диапазоны для обеих осей
    float leftMin = FLT_MAX, leftMax = -FLT_MAX;
    float rightMin = FLT_MAX, rightMax = -FLT_MAX;
    
    // Диапазон левой оси
    for (const auto& channel : leftAxisData) {
        for (float val : channel) {
            leftMin = (std::min)(leftMin, val);
            leftMax = (std::max)(leftMax, val);
        }
    }
    
    // Диапазон правой оси
    for (const auto& channel : rightAxisData) {
        for (float val : channel) {
            rightMin = (std::min)(rightMin, val);
            rightMax = (std::max)(rightMax, val);
        }
    }
    
    // Добавляем padding
    if (leftMin != FLT_MAX && leftMax != -FLT_MAX && leftMin != leftMax) {
        float leftRange = leftMax - leftMin;
        float leftPadding = leftRange * 0.05f;
        leftMin -= leftPadding;
        leftMax += leftPadding;
    }
    
    if (rightMin != FLT_MAX && rightMax != -FLT_MAX && rightMin != rightMax) {
        float rightRange = rightMax - rightMin;
        float rightPadding = rightRange * 0.05f;
        rightMin -= rightPadding;
        rightMax += rightPadding;
    }
    
    // Отрисовка горизонтальных линий сетки (основываемся на левой оси)
    if (leftMin != FLT_MAX && leftMax != -FLT_MAX) {
        double niceStepY = CalculateNiceStep(leftMax - leftMin, (plotRect.bottom - plotRect.top) / 40);
        double currentYVal = CalculateNiceMin(leftMin, niceStepY);

        while (currentYVal <= leftMax + niceStepY / 2) {
            if (currentYVal >= leftMin - niceStepY / 2 && currentYVal <= leftMax + niceStepY / 2) {
                long screenY = plotRect.bottom - (LONG)((currentYVal - leftMin) / (leftMax - leftMin) * (plotRect.bottom - plotRect.top));
                
                if (screenY >= plotRect.top && screenY <= plotRect.bottom) {
                    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(230, 230, 230));
                    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
                    
                    MoveToEx(hdc, plotRect.left, screenY, NULL);
                    LineTo(hdc, plotRect.right, screenY);
                    
                    SelectObject(hdc, hOldPen);
                    DeleteObject(hPen);
                }
            }
            currentYVal += niceStepY;
        }
    }
}

// Перегрузка с поддержкой оси X и Y zoom
void ChartModule::DrawDualAxisGrid(HDC hdc, const RECT& plotRect, 
                                  const std::vector<std::vector<float>>& leftAxisData,
                                  const std::vector<std::vector<float>>& rightAxisData,
                                  double minXData, double maxXData,
                                  double minYData, double maxYData) const {
    // Используем переданные параметры zoom для Y вместо вычисления из данных
    float leftMin = static_cast<float>(minYData);
    float leftMax = static_cast<float>(maxYData);
    
    // Отрисовка горизонтальных линий сетки с учетом zoom по Y
    double niceStepY = CalculateNiceStep(leftMax - leftMin, (plotRect.bottom - plotRect.top) / 40);
    double currentYVal = CalculateNiceMin(leftMin, niceStepY);

    while (currentYVal <= leftMax + niceStepY / 2) {
        if (currentYVal >= leftMin - niceStepY / 2 && currentYVal <= leftMax + niceStepY / 2) {
            long screenY = plotRect.bottom - (LONG)((currentYVal - leftMin) / (leftMax - leftMin) * (plotRect.bottom - plotRect.top));
            
            if (screenY >= plotRect.top && screenY <= plotRect.bottom) {
                HPEN hPen = CreatePen(PS_SOLID, 1, kGridMajor);
                HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
                
                MoveToEx(hdc, plotRect.left, screenY, NULL);
                LineTo(hdc, plotRect.right, screenY);
                
                SelectObject(hdc, hOldPen);
                DeleteObject(hPen);
            }
        }
        currentYVal += niceStepY;
    }
    
    // Теперь добавляем ось X
    if (maxXData <= minXData) {
        maxXData = minXData + 1; // Избегаем деления на ноль
    }
    
    double xRange = maxXData - minXData;
    double niceStepX = CalculateNiceStep(xRange, (plotRect.right - plotRect.left) / 60);
    double currentXVal = CalculateNiceMin(minXData, niceStepX);
    
    SetTextColor(hdc, kText);
    SetBkMode(hdc, TRANSPARENT);
    
    // Отслеживание уже отрисованных целых значений для предотвращения дубликатов
    std::set<int> drawnLabels;
    
    // Константа для длины засечки
    const int TICK_LENGTH = 5;
    
    while (currentXVal <= maxXData + niceStepX / 2) {
        if (currentXVal >= minXData - niceStepX / 2 && currentXVal <= maxXData + niceStepX / 2) {
            long screenX = plotRect.left + (LONG)((currentXVal - minXData) / xRange * (plotRect.right - plotRect.left));
            
            if (screenX >= plotRect.left && screenX <= plotRect.right) {
                // Рисуем вертикальную линию сетки
                HPEN hPen = CreatePen(PS_SOLID, 1, kGridMajor);
                HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
                
                MoveToEx(hdc, screenX, plotRect.top, NULL);
                LineTo(hdc, screenX, plotRect.bottom);
                
                SelectObject(hdc, hOldPen);
                DeleteObject(hPen);
                
                // Рисуем засечку (tick mark) вниз от оси X
                HPEN hTickPen = CreatePen(PS_SOLID, 1, kAxis);
                HPEN hOldTickPen = (HPEN)SelectObject(hdc, hTickPen);
                MoveToEx(hdc, screenX, plotRect.bottom, NULL);
                LineTo(hdc, screenX, plotRect.bottom + TICK_LENGTH);
                SelectObject(hdc, hOldTickPen);
                DeleteObject(hTickPen);
                
                // Отрисовка метки X-оси целыми числами (только уникальные значения)
                int roundedValue = static_cast<int>(std::round(currentXVal));
                if (drawnLabels.find(roundedValue) == drawnLabels.end()) {
                    drawnLabels.insert(roundedValue);
                    
                    std::wstringstream ssX;
                    ssX << roundedValue;
                    std::wstring xLabel = ssX.str();
                    TextOut(hdc, screenX - 10, plotRect.bottom + TICK_LENGTH + 2, xLabel.c_str(), (int)xLabel.length());
                }
            }
        }
        currentXVal += niceStepX;
    }
    
    // Рисуем ось X (без утолщения)
    HPEN hAxisPen = CreatePen(PS_SOLID, 1, kAxis);
    HPEN hOldAxisPen = (HPEN)SelectObject(hdc, hAxisPen);
    MoveToEx(hdc, plotRect.left, plotRect.bottom, NULL);
    LineTo(hdc, plotRect.right, plotRect.bottom);
    SelectObject(hdc, hOldAxisPen);
    DeleteObject(hAxisPen);
}

void ChartModule::DrawLeftAxis(HDC hdc, const RECT& plotRect, 
                              const std::vector<std::vector<float>>& leftAxisData,
                              const std::string& units) const {
    if (leftAxisData.empty()) return;
    
    // Находим диапазон левой оси
    float minVal = FLT_MAX, maxVal = -FLT_MAX;
    for (const auto& channel : leftAxisData) {
        for (float val : channel) {
            minVal = (std::min)(minVal, val);
            maxVal = (std::max)(maxVal, val);
        }
    }
    
    if (minVal == FLT_MAX || maxVal == -FLT_MAX || minVal == maxVal) {
        minVal = 0.0f;
        maxVal = 100.0f;
    } else {
        float range = maxVal - minVal;
        float padding = range * 0.05f;
        minVal -= padding;
        maxVal += padding;
    }
    
    SetTextColor(hdc, kText);
    SetBkMode(hdc, TRANSPARENT);

    // Рисуем левую ось (тонко и ненавязчиво)
    HPEN hPen = CreatePen(PS_SOLID, 1, kAxis);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    
    MoveToEx(hdc, plotRect.left, plotRect.top, NULL);
    LineTo(hdc, plotRect.left, plotRect.bottom);
    
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
    
    // Рисуем метки левой оси
    double niceStepY = CalculateNiceStep(maxVal - minVal, (plotRect.bottom - plotRect.top) / 40);
    double currentYVal = CalculateNiceMin(minVal, niceStepY);
    
    SetTextColor(hdc, kText);
    SetBkMode(hdc, TRANSPARENT);
    
    // Константа для длины засечки
    const int TICK_LENGTH = 5;
    
    while (currentYVal <= maxVal + niceStepY / 2) {
        if (currentYVal >= minVal - niceStepY / 2 && currentYVal <= maxVal + niceStepY / 2) {
            long screenY = plotRect.bottom - (LONG)((currentYVal - minVal) / (maxVal - minVal) * (plotRect.bottom - plotRect.top));
            
            if (screenY >= plotRect.top && screenY <= plotRect.bottom) {
                // Рисуем засечку (tick mark) влево от оси
                HPEN hTickPen = CreatePen(PS_SOLID, 1, kAxis);
                HPEN hOldTickPen = (HPEN)SelectObject(hdc, hTickPen);
                MoveToEx(hdc, plotRect.left - TICK_LENGTH, screenY, NULL);
                LineTo(hdc, plotRect.left, screenY);
                SelectObject(hdc, hOldTickPen);
                DeleteObject(hTickPen);
                
                std::wstringstream ss;
                // Для фазовых данных (mdeg) используем целые числа, для остальных - 2 знака после запятой
                if (units == "mdeg") {
                    ss << std::fixed << std::setprecision(0) << currentYVal;
                } else {
                    ss << std::fixed << std::setprecision(2) << currentYVal;
                }
                std::wstring label = ss.str();
                
                // Вычисляем ширину текста для правильного выравнивания
                SIZE textSize;
                GetTextExtentPoint32(hdc, label.c_str(), (int)label.length(), &textSize);
                
                // Рисуем текст так, чтобы он заканчивался в 7 пикселях от оси (учитываем засечку)
                int textX = plotRect.left - TICK_LENGTH - 2 - textSize.cx;
                TextOut(hdc, textX, screenY - 7, label.c_str(), (int)label.length());
            }
        }
        currentYVal += niceStepY;
    }
    
    // Подпись единиц измерения левой оси
    if (!units.empty()) {
        std::wstring wunits(units.begin(), units.end());
        TextOut(hdc, plotRect.left - 45, plotRect.top - 20, wunits.c_str(), (int)wunits.length());
    }
}

void ChartModule::DrawRightAxis(HDC hdc, const RECT& plotRect, 
                               const std::vector<std::vector<float>>& rightAxisData,
                               const std::string& units) const {
    if (rightAxisData.empty()) return;
    
    // Находим диапазон правой оси
    float minVal = FLT_MAX, maxVal = -FLT_MAX;
    for (const auto& channel : rightAxisData) {
        for (float val : channel) {
            minVal = (std::min)(minVal, val);
            maxVal = (std::max)(maxVal, val);
        }
    }
    
    if (minVal == FLT_MAX || maxVal == -FLT_MAX || minVal == maxVal) {
        minVal = 0.0f;
        maxVal = 100.0f;
    } else {
        float range = maxVal - minVal;
        float padding = range * 0.05f;
        minVal -= padding;
        maxVal += padding;
    }
    
    // Рисуем правую ось с отступом от края
    int rightAxisX = plotRect.right + 40; // Отступ 40 пикселей от области графика
    
    SetTextColor(hdc, kText);
    SetBkMode(hdc, TRANSPARENT);

    HPEN hPen = CreatePen(PS_SOLID, 1, kAxis);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    
    MoveToEx(hdc, rightAxisX, plotRect.top, NULL);
    LineTo(hdc, rightAxisX, plotRect.bottom);
    
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
    
    // Рисуем метки правой оси
    double niceStepY = CalculateNiceStep(maxVal - minVal, (plotRect.bottom - plotRect.top) / 40);
    double currentYVal = CalculateNiceMin(minVal, niceStepY);
    
    SetTextColor(hdc, kText);
    SetBkMode(hdc, TRANSPARENT);
    
    // Константа для длины засечки
    const int TICK_LENGTH = 5;
    
    while (currentYVal <= maxVal + niceStepY / 2) {
        if (currentYVal >= minVal - niceStepY / 2 && currentYVal <= maxVal + niceStepY / 2) {
            long screenY = plotRect.bottom - (LONG)((currentYVal - minVal) / (maxVal - minVal) * (plotRect.bottom - plotRect.top));
            
            if (screenY >= plotRect.top && screenY <= plotRect.bottom) {
                // Рисуем засечку (tick mark) вправо от оси
                HPEN hTickPen = CreatePen(PS_SOLID, 1, kAxis);
                HPEN hOldTickPen = (HPEN)SelectObject(hdc, hTickPen);
                MoveToEx(hdc, rightAxisX, screenY, NULL);
                LineTo(hdc, rightAxisX + TICK_LENGTH, screenY);
                SelectObject(hdc, hOldTickPen);
                DeleteObject(hTickPen);
                
                std::wstringstream ss;
                // Для фазовых данных (mdeg) используем целые числа, для остальных - 2 знака после запятой
                if (units == "mdeg") {
                    ss << std::fixed << std::setprecision(0) << currentYVal;
                } else {
                    ss << std::fixed << std::setprecision(2) << currentYVal;
                }
                std::wstring label = ss.str();
                TextOut(hdc, rightAxisX + TICK_LENGTH + 2, screenY - 7, label.c_str(), (int)label.length());
            }
        }
        currentYVal += niceStepY;
    }
    
    // Подпись единиц измерения правой оси
    if (!units.empty()) {
        std::wstring wunits(units.begin(), units.end());
        TextOut(hdc, rightAxisX + TICK_LENGTH + 2, plotRect.top - 20, wunits.c_str(), (int)wunits.length());
    }
}

void ChartModule::DrawDualAxisData(HDC hdc, const RECT& plotRect,
                                  const std::vector<CurveDefinition>& curves,
                                  const std::vector<std::vector<float>>& leftAxisData,
                                  const std::vector<std::vector<float>>& rightAxisData) const {
    // Создаем clipping region с небольшим расширением, чтобы крайние точки не обрезались
    // Расширяем на 5 пикселей сверху и снизу
    HRGN hClipRegion = CreateRectRgn(plotRect.left, plotRect.top - 5, plotRect.right, plotRect.bottom + 5);
    HRGN hOldClipRegion = CreateRectRgn(0, 0, 0, 0);
    int regionType = GetClipRgn(hdc, hOldClipRegion);
    SelectClipRgn(hdc, hClipRegion);
    
    // Находим диапазоны для обеих осей
    float leftMin = FLT_MAX, leftMax = -FLT_MAX;
    float rightMin = FLT_MAX, rightMax = -FLT_MAX;
    
    // Диапазон левой оси
    for (const auto& channel : leftAxisData) {
        for (float val : channel) {
            leftMin = (std::min)(leftMin, val);
            leftMax = (std::max)(leftMax, val);
        }
    }
    
    // Диапазон правой оси
    for (const auto& channel : rightAxisData) {
        for (float val : channel) {
            rightMin = (std::min)(rightMin, val);
            rightMax = (std::max)(rightMax, val);
        }
    }
    
    // Добавляем padding и проверяем корректность диапазонов
    if (leftMin != FLT_MAX && leftMax != -FLT_MAX && leftMin != leftMax) {
        float leftRange = leftMax - leftMin;
        float leftPadding = leftRange * 0.05f;
        leftMin -= leftPadding;
        leftMax += leftPadding;
    } else {
        leftMin = 0.0f;
        leftMax = 100.0f;
    }
    
    if (rightMin != FLT_MAX && rightMax != -FLT_MAX && rightMin != rightMax) {
        float rightRange = rightMax - rightMin;
        float rightPadding = rightRange * 0.05f;
        rightMin -= rightPadding;
        rightMax += rightPadding;
    } else {
        rightMin = 0.0f;
        rightMax = 100.0f;
    }
    
    // Отрисовка кривых левой оси (сплошные линии)
    size_t leftIndex = 0;
    for (size_t i = 0; i < curves.size() && leftIndex < leftAxisData.size(); i++) {
        const auto& curve = curves[i];
        if (!curve.enabled || curve.yAxisGroup != 0) continue;
        
        const auto& channel = leftAxisData[leftIndex];
        if (channel.empty()) {
            leftIndex++;
            continue;
        }

        std::vector<POINT> points;
        points.reserve(channel.size());
        const double xDenominator = (channel.size() > 1) ? static_cast<double>(channel.size() - 1) : 1.0;
        for (size_t j = 0; j < channel.size(); j++) {
            int x = plotRect.left + (int)((double)j / xDenominator * (plotRect.right - plotRect.left));
            float clampedValue = (std::max)(leftMin, (std::min)(leftMax, channel[j]));
            int y = plotRect.bottom - (int)((clampedValue - leftMin) / (leftMax - leftMin) * (plotRect.bottom - plotRect.top));
            points.push_back({ x, y });
        }

        DrawCurvePolyline(hdc, points, curve.color, PS_SOLID);
        leftIndex++;
    }
    
    // Отрисовка кривых правой оси (пунктирные линии)
    size_t rightIndex = 0;
    for (size_t i = 0; i < curves.size() && rightIndex < rightAxisData.size(); i++) {
        const auto& curve = curves[i];
        if (!curve.enabled || curve.yAxisGroup != 1) continue;
        
        const auto& channel = rightAxisData[rightIndex];
        if (channel.empty()) {
            rightIndex++;
            continue;
        }

        std::vector<POINT> points;
        points.reserve(channel.size());
        const double xDenominator = (channel.size() > 1) ? static_cast<double>(channel.size() - 1) : 1.0;
        for (size_t j = 0; j < channel.size(); j++) {
            int x = plotRect.left + (int)((double)j / xDenominator * (plotRect.right - plotRect.left));
            float clampedValue = (std::max)(rightMin, (std::min)(rightMax, channel[j]));
            int y = plotRect.bottom - (int)((clampedValue - rightMin) / (rightMax - rightMin) * (plotRect.bottom - plotRect.top));
            points.push_back({ x, y });
        }

        DrawCurvePolyline(hdc, points, curve.color, curve.lineStyle);
        rightIndex++;
    }
    
    // Восстанавливаем предыдущий clipping region
    if (regionType == 1) {
        SelectClipRgn(hdc, hOldClipRegion);
    } else {
        SelectClipRgn(hdc, NULL);
    }
    DeleteObject(hClipRegion);
    DeleteObject(hOldClipRegion);
}

// Новая функция для отрисовки данных с учетом zoom по Y
void ChartModule::DrawDualAxisDataWithZoom(HDC hdc, const RECT& plotRect,
                                           const std::vector<CurveDefinition>& curves,
                                           const std::vector<std::vector<float>>& leftAxisData,
                                           const std::vector<std::vector<float>>& rightAxisData,
                                           double leftMinY, double leftMaxY,
                                           double rightMinY, double rightMaxY,
                                           double minXData, double maxXData) const {
    RECT dataRect = plotRect;
    int plotHeight = plotRect.bottom - plotRect.top;
    int yMargin = plotHeight * 3 / 100;
    yMargin = (std::max)(8, (std::min)(24, yMargin));
    dataRect.top += yMargin;
    dataRect.bottom -= yMargin;

    if (dataRect.bottom <= dataRect.top) {
        return;
    }

    // Добавляем небольшой padding только снизу (2%) для визуального комфорта
    // Сверху padding не нужен, чтобы не было лишнего пространства
    float yRangeLeft = static_cast<float>(leftMaxY - leftMinY);
    float yPaddingLeft = yRangeLeft * 0.02f;
    float leftMin = static_cast<float>(leftMinY) - yPaddingLeft;
    float leftMax = static_cast<float>(leftMaxY);
    
    float yRangeRight = static_cast<float>(rightMaxY - rightMinY);
    float yPaddingRight = yRangeRight * 0.02f;
    float rightMin = static_cast<float>(rightMinY) - yPaddingRight;
    float rightMax = static_cast<float>(rightMaxY);
    
    // Отладочный вывод
    wchar_t debugMsg[512];
    swprintf_s(debugMsg, L"=== DrawDualAxisDataWithZoom ===\n"
                         L"plotRect: left=%d, top=%d, right=%d, bottom=%d\n"
                         L"leftMinY=%.2f, leftMaxY=%.2f\n"
                         L"leftMin=%.2f, leftMax=%.2f\n",
              plotRect.left, plotRect.top, plotRect.right, plotRect.bottom,
              leftMinY, leftMaxY, leftMin, leftMax);
    AppendDebugLog(L"%ls", debugMsg);
    
    // Создаем clipping region с небольшим расширением, чтобы крайние точки не обрезались
    // Расширяем на 5 пикселей сверху и снизу
    HRGN hClipRegion = CreateRectRgn(dataRect.left, dataRect.top - 5, dataRect.right, dataRect.bottom + 5);
    HRGN hOldClipRegion = CreateRectRgn(0, 0, 0, 0);
    int regionType = GetClipRgn(hdc, hOldClipRegion);
    SelectClipRgn(hdc, hClipRegion);
    
    // Отрисовка кривых левой оси (сплошные линии)
    size_t leftIndex = 0;
    for (size_t i = 0; i < curves.size() && leftIndex < leftAxisData.size(); i++) {
        const auto& curve = curves[i];
        if (!curve.enabled || curve.yAxisGroup != 0) continue;
        
        const auto& channel = leftAxisData[leftIndex];
        if (channel.empty()) {
            leftIndex++;
            continue;
        }

        std::vector<POINT> points;
        points.reserve(channel.size());
        
        // Отладка для кривой
        float minChannelVal = *std::min_element(channel.begin(), channel.end());
        float maxChannelVal = *std::max_element(channel.begin(), channel.end());
        swprintf_s(debugMsg, L"Left Curve %zu: minVal=%.2f, maxVal=%.2f\n", leftIndex, minChannelVal, maxChannelVal);
        AppendDebugLog(L"%ls", debugMsg);
        
        int minY = INT_MAX, maxY = INT_MIN;
        int clampedToMin = 0;
        int clampedToMax = 0;
        double xRange = maxXData - minXData;
        for (size_t j = 0; j < channel.size(); j++) {
            // Вычисляем реальный индекс данных и преобразуем в экранную координату
            double dataX = minXData + (double)j;
            int x = plotRect.left + (int)((dataX - minXData) / xRange * (plotRect.right - plotRect.left));
            
            // Ограничиваем значение данных к диапазону leftMin/leftMax для корректного отображения
            float v = channel[j];
            float clampedValue = (std::max)(leftMin, (std::min)(leftMax, v));
            if (clampedValue == leftMin && v < leftMin) clampedToMin++;
            if (clampedValue == leftMax && v > leftMax) clampedToMax++;
            int y = dataRect.bottom - (int)((clampedValue - leftMin) / (leftMax - leftMin) * (dataRect.bottom - dataRect.top));
            
            // Отслеживаем min/max Y координаты
            minY = (std::min)(minY, y);
            maxY = (std::max)(maxY, y);

            points.push_back({ x, y });
        }
        
        // Отладка: выводим диапазон Y координат
        swprintf_s(debugMsg, L"  Screen Y range: minY=%d, maxY=%d (plotRect.top=%d, plotRect.bottom=%d)\n",
                  minY, maxY, plotRect.top, plotRect.bottom);
        AppendDebugLog(L"%ls", debugMsg);

        AppendDebugLog(L"  Clamp stats: toMin=%d toMax=%d (points=%zu)\n", clampedToMin, clampedToMax, channel.size());
        
        DrawCurvePolyline(hdc, points, curve.color, PS_SOLID);
        leftIndex++;
    }
    
    // Отрисовка кривых правой оси (пунктирные линии)
    size_t rightIndex = 0;
    double xRangeForRightAxis = maxXData - minXData;
    for (size_t i = 0; i < curves.size() && rightIndex < rightAxisData.size(); i++) {
        const auto& curve = curves[i];
        if (!curve.enabled || curve.yAxisGroup != 1) continue;
        
        const auto& channel = rightAxisData[rightIndex];
        if (channel.empty()) {
            rightIndex++;
            continue;
        }

        std::vector<POINT> points;
        points.reserve(channel.size());
        for (size_t j = 0; j < channel.size(); j++) {
            // Вычисляем реальный индекс данных и преобразуем в экранную координату
            double dataX = minXData + (double)j;
            int x = plotRect.left + (int)((dataX - minXData) / xRangeForRightAxis * (plotRect.right - plotRect.left));
            float clampedValue = (std::max)(rightMin, (std::min)(rightMax, channel[j]));
            int y = dataRect.bottom - (int)((clampedValue - rightMin) / (rightMax - rightMin) * (dataRect.bottom - dataRect.top));

            points.push_back({ x, y });
        }

        DrawCurvePolyline(hdc, points, curve.color, curve.lineStyle);
        rightIndex++;
    }
    
    // Восстанавливаем предыдущий clipping region
    if (regionType == 1) {
        SelectClipRgn(hdc, hOldClipRegion);
    } else {
        SelectClipRgn(hdc, NULL);
    }
    DeleteObject(hClipRegion);
    DeleteObject(hOldClipRegion);
}

void ChartModule::DrawDualAxisLegend(HDC hdc, const RECT& rect,
                                    const std::vector<CurveDefinition>& curves) const {
    // Параметры легенды
    int lineLength = 15;
    int elementSpacing = 15;
    int lineHeight = 20;
    int separatorSpacing = 25; // Дополнительный отступ для разделителя
    
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    SetBkMode(hdc, TRANSPARENT);
    
    // Группируем кривые по осям
    std::vector<const CurveDefinition*> leftAxisCurves;
    std::vector<const CurveDefinition*> rightAxisCurves;
    
    for (const auto& curve : curves) {
        if (!curve.enabled) continue;
        
        if (curve.yAxisGroup == 0) {
            leftAxisCurves.push_back(&curve);
        } else {
            rightAxisCurves.push_back(&curve);
        }
    }
    
    if (leftAxisCurves.empty() && rightAxisCurves.empty()) {
        SelectObject(hdc, hOldFont);
        return;
    }
    
    auto GetAxisFunctionLabel = [](const std::vector<const CurveDefinition*>& axisCurves) -> std::wstring {
        if (axisCurves.empty()) return L"";

        enum class Category {
            Unknown,
            PhaseDifference,
            PhaseSmt,
            Amplitude,
            AmplitudeRatio,
            RhoSmt,
            Temperature,
        };

        auto GetCategory = [](DataType t) -> Category {
            switch (t) {
                case DataType::PHASE_DIFFERENCE:
                case DataType::SILENT_PHASE_DIFFERENCE:
                    return Category::PhaseDifference;
                case DataType::PHASE_SMT:
                    return Category::PhaseSmt;
                case DataType::AMPLITUDE_RX1:
                case DataType::AMPLITUDE_RX2:
                case DataType::SILENT_AMPLITUDE_RX1:
                case DataType::SILENT_AMPLITUDE_RX2:
                    // ВАЖНО: AMPLITUDE_RX1 и AMPLITUDE_RX2 считаем одной категорией,
                    // чтобы легенда была "Амплитуда T1 R1 T1 R2 ..." (как требует UX).
                    return Category::Amplitude;
                case DataType::AMPLITUDE_RATIO:
                    return Category::AmplitudeRatio;
                case DataType::RHO_SMT:
                    return Category::RhoSmt;
                case DataType::TEMPERATURE:
                    return Category::Temperature;
                default:
                    return Category::Unknown;
            }
        };

        Category firstCat = Category::Unknown;
        for (const auto* c : axisCurves) {
            if (!c) continue;
            firstCat = GetCategory(c->dataType);
            break;
        }

        if (firstCat == Category::Unknown) return L"";

        for (const auto* c : axisCurves) {
            if (!c) continue;
            if (GetCategory(c->dataType) != firstCat) {
                return L""; // смешанные категории — не упрощаем
            }
        }

        switch (firstCat) {
            case Category::PhaseDifference: return L"Разность фаз";
            case Category::PhaseSmt:        return L"Фаза";
            case Category::Amplitude:       return L"Амплитуда";
            case Category::AmplitudeRatio:  return L"Отношение амплитуд";
            case Category::RhoSmt:          return L"УЭС";
            case Category::Temperature:     return L"Температура";
            default:                        return L"";
        }
    };

    auto GetCurveShortLabel = [](const CurveDefinition* c) -> std::wstring {
        if (!c) return L"";
        switch (c->sourceElement) {
            case ElementType::TRANSMITTER: {
                // Tn, плюс R1/R2 для амплитуд
                std::wstring s = L"T" + std::to_wstring(c->elementIndex + 1);
                if (c->dataType == DataType::AMPLITUDE_RX1) s += L" R1";
                else if (c->dataType == DataType::AMPLITUDE_RX2) s += L" R2";
                return s;
            }
            case ElementType::RECEIVER:
                return L"R" + std::to_wstring(c->elementIndex + 1);
            case ElementType::SILENT_TX:
                if (c->dataType == DataType::SILENT_PHASE_DIFFERENCE) return L"R1-R2";
                return L"R" + std::to_wstring(c->elementIndex + 1);
            default:
                return Utf8ToWide(c->displayName);
        }
    };

    // Получаем единицы измерения для каждой оси
    std::string leftUnits, rightUnits;
    if (!leftAxisCurves.empty()) {
        leftUnits = leftAxisCurves[0]->units;
    }
    if (!rightAxisCurves.empty()) {
        rightUnits = rightAxisCurves[0]->units;
    }

    // Проверяем, можно ли сократить подписи (единый тип данных на оси)
    const std::wstring leftFn = GetAxisFunctionLabel(leftAxisCurves);
    const std::wstring rightFn = GetAxisFunctionLabel(rightAxisCurves);
    const bool leftCondensed = !leftFn.empty();
    const bool rightCondensed = !rightFn.empty();
    
    // Вычисляем ширину каждого элемента легенды
    struct LegendElement {
        int width;
        const CurveDefinition* curve;
        int axisGroup; // 0 = left, 1 = right
    };
    
    std::vector<LegendElement> elements;
    int leftGroupWidth = 0;
    int rightGroupWidth = 0;
    
    // Вычисляем ширину подписи левой оси
    std::wstring leftAxisLabel = L"[Левая ось (" + std::wstring(leftUnits.begin(), leftUnits.end()) + L")]: ";
    SIZE leftLabelSize;
    GetTextExtentPoint32(hdc, leftAxisLabel.c_str(), (int)leftAxisLabel.length(), &leftLabelSize);
    leftGroupWidth = leftLabelSize.cx + 10; // +10 для отступа после подписи

    SIZE leftFnSize = {};
    if (leftCondensed) {
        std::wstring leftFnLabel = leftFn + L" ";
        GetTextExtentPoint32(hdc, leftFnLabel.c_str(), (int)leftFnLabel.length(), &leftFnSize);
        leftGroupWidth += leftFnSize.cx;
    }
    
    // Добавляем элементы левой оси
    for (const auto* curve : leftAxisCurves) {
        std::wstring name = leftCondensed ? GetCurveShortLabel(curve) : Utf8ToWide(curve->displayName);
        SIZE textSize;
        GetTextExtentPoint32(hdc, name.c_str(), (int)name.length(), &textSize);
        
        int elementWidth = lineLength + 5 + textSize.cx;
        elements.push_back({elementWidth, curve, 0});
        leftGroupWidth += elementWidth + elementSpacing;
    }
    
    // Ширина разделителя " | "
    int separatorWidth = 20;
    
    // Вычисляем ширину подписи правой оси
    std::wstring rightAxisLabel = L"[Правая ось (" + std::wstring(rightUnits.begin(), rightUnits.end()) + L")]: ";
    SIZE rightLabelSize;
    GetTextExtentPoint32(hdc, rightAxisLabel.c_str(), (int)rightAxisLabel.length(), &rightLabelSize);
    rightGroupWidth = rightLabelSize.cx + 10; // +10 для отступа после подписи

    SIZE rightFnSize = {};
    if (rightCondensed) {
        std::wstring rightFnLabel = rightFn + L" ";
        GetTextExtentPoint32(hdc, rightFnLabel.c_str(), (int)rightFnLabel.length(), &rightFnSize);
        rightGroupWidth += rightFnSize.cx;
    }
    
    // Добавляем элементы правой оси
    for (const auto* curve : rightAxisCurves) {
        std::wstring name = rightCondensed ? GetCurveShortLabel(curve) : Utf8ToWide(curve->displayName);
        SIZE textSize;
        GetTextExtentPoint32(hdc, name.c_str(), (int)name.length(), &textSize);
        
        int elementWidth = lineLength + 5 + textSize.cx;
        elements.push_back({elementWidth, curve, 1});
        rightGroupWidth += elementWidth + elementSpacing;
    }
    
    // Общая ширина легенды
    int totalWidth = leftGroupWidth + separatorSpacing + separatorWidth + separatorSpacing + rightGroupWidth;
    
    // Вычисляем начальную X позицию для центрирования
    int chartWidth = rect.right - rect.left;
    int startX = rect.left + (chartWidth - totalWidth) / 2;
    
    // Позиция Y - под заголовком
    int legendY = rect.top + 35;
    
    // Отрисовка легенды
    int currentX = startX;
    
    // Отрисовка подписи левой оси
    if (!leftAxisCurves.empty()) {
        SetTextColor(hdc, kText);
        TextOut(hdc, currentX, legendY, leftAxisLabel.c_str(), (int)leftAxisLabel.length());
        currentX += leftLabelSize.cx + 10;

        if (leftCondensed) {
            std::wstring leftFnLabel = leftFn + L" ";
            TextOut(hdc, currentX, legendY, leftFnLabel.c_str(), (int)leftFnLabel.length());
            currentX += leftFnSize.cx;
        }
        
        // Отрисовка элементов левой оси
        for (const auto& elem : elements) {
            if (elem.axisGroup != 0) continue;
            
            const CurveDefinition* curve = elem.curve;
            
            // Линия (сплошная для левой оси)
            HPEN hPen = CreateCurvePen(PS_SOLID, curve->color);
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
            
            MoveToEx(hdc, currentX, legendY + lineHeight / 2, NULL);
            LineTo(hdc, currentX + lineLength, legendY + lineHeight / 2);
            
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
            
            currentX += lineLength + 5;
            
            // Название
            std::wstring name = leftCondensed ? GetCurveShortLabel(curve) : Utf8ToWide(curve->displayName);
            TextOut(hdc, currentX, legendY, name.c_str(), (int)name.length());
            
            SIZE textSize;
            GetTextExtentPoint32(hdc, name.c_str(), (int)name.length(), &textSize);
            currentX += textSize.cx + elementSpacing;
        }
        
        currentX += separatorSpacing - elementSpacing; // Убираем лишний отступ после последнего элемента
    }
    
    // Отрисовка разделителя " | "
    if (!leftAxisCurves.empty() && !rightAxisCurves.empty()) {
        SetTextColor(hdc, RGB(130, 130, 130)); // спокойный серый для разделителя
        TextOut(hdc, currentX, legendY, L"|", 1);
        currentX += separatorWidth + separatorSpacing;
    }
    
    // Отрисовка подписи правой оси
    if (!rightAxisCurves.empty()) {
        SetTextColor(hdc, kText);
        TextOut(hdc, currentX, legendY, rightAxisLabel.c_str(), (int)rightAxisLabel.length());
        currentX += rightLabelSize.cx + 10;

        if (rightCondensed) {
            std::wstring rightFnLabel = rightFn + L" ";
            TextOut(hdc, currentX, legendY, rightFnLabel.c_str(), (int)rightFnLabel.length());
            currentX += rightFnSize.cx;
        }
        
        // Отрисовка элементов правой оси
        for (const auto& elem : elements) {
            if (elem.axisGroup != 1) continue;
            
            const CurveDefinition* curve = elem.curve;
            
            // Линия (пунктирная для правой оси)
            HPEN hPen = CreateCurvePen(curve->lineStyle, curve->color);
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
            
            MoveToEx(hdc, currentX, legendY + lineHeight / 2, NULL);
            LineTo(hdc, currentX + lineLength, legendY + lineHeight / 2);
            
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
            
            currentX += lineLength + 5;
            
            // Название (конвертируем UTF-8 в UTF-16)
            std::wstring name = rightCondensed ? GetCurveShortLabel(curve) : Utf8ToWide(curve->displayName);
            TextOut(hdc, currentX, legendY, name.c_str(), (int)name.length());
            
            SIZE textSize;
            GetTextExtentPoint32(hdc, name.c_str(), (int)name.length(), &textSize);
            currentX += textSize.cx + elementSpacing;
        }
    }
    
    SelectObject(hdc, hOldFont);
}

// v2.15.0: Перегрузка с поддержкой пресетов (компактная легенда)
void ChartModule::DrawDualAxisLegend(HDC hdc, const RECT& rect,
                                    const std::vector<CurveDefinition>& curves,
                                    const ChartConfiguration& config) const {
    // Если пресет не активен, используем стандартную отрисовку
    if (!config.IsPresetActive()) {
        DrawDualAxisLegend(hdc, rect, curves);
        return;
    }

    // --- Специальная легенда для пресетов молчащих передатчиков ---
    // Причина: стандартная "компактная легенда пресета" извлекает короткое имя из displayName по байтам,
    // что для UTF-8 (кириллица) даёт некорректные символы. Для silent-пресетов рисуем полностью заданный текст.
    if (config.activePreset == PresetType::SILENT_PHASE_DIFFERENCE ||
        config.activePreset == PresetType::SILENT_AMPLITUDES) {
        const int lineLength = 15;
        const int elementSpacing = 15;
        const int lineHeight = 20;

        HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        SetBkMode(hdc, TRANSPARENT);

        // Собираем включённые кривые (обычно только левая ось)
        std::vector<const CurveDefinition*> leftAxisCurves;
        for (const auto& curve : curves) {
            if (!curve.enabled) continue;
            if (curve.yAxisGroup == 0) {
                leftAxisCurves.push_back(&curve);
            }
        }

        if (leftAxisCurves.empty()) {
            SelectObject(hdc, hOldFont);
            return;
        }

        // Единицы оси (mV/mdeg)
        const std::string leftUnits = leftAxisCurves[0]->units;
        const std::wstring leftUnitsW(leftUnits.begin(), leftUnits.end());

        // Требуемый формат:
        // 1) "[Левая ось (mdeg)]: Разность фаз молчащих передатчиков: [цвет] R1-R2"
        // 2) "[Левая ось (mV)]: Амплитуды молчащих передатчиков: [цвет] R1  [цвет] R2"
        const std::wstring prefix = L"[Левая ось (" + leftUnitsW + L")]: ";
        const std::wstring functionLabel =
            (config.activePreset == PresetType::SILENT_PHASE_DIFFERENCE)
                ? L"Разность фаз молчащих передатчиков: "
                : L"Амплитуды молчащих передатчиков: ";

        // Готовим "элементы" (цветная линия + подпись)
        struct Item {
            const CurveDefinition* curve;
            std::wstring label;
        };

        std::vector<Item> items;
        items.reserve(leftAxisCurves.size());

        if (config.activePreset == PresetType::SILENT_PHASE_DIFFERENCE) {
            // Ожидаем одну кривую разности фаз молчащих передатчиков
            items.push_back({leftAxisCurves[0], L"R1-R2"});
        } else {
            // Амплитуды молчащих: две кривые (R1 и R2). Сортируем предсказуемо.
            const CurveDefinition* r1 = nullptr;
            const CurveDefinition* r2 = nullptr;
            for (const auto* c : leftAxisCurves) {
                if (c->dataType == DataType::SILENT_AMPLITUDE_RX1) r1 = c;
                if (c->dataType == DataType::SILENT_AMPLITUDE_RX2) r2 = c;
            }
            if (r1) items.push_back({r1, L"R1"});
            if (r2) items.push_back({r2, L"R2"});
            // fallback: если по каким-то причинам типы не совпали, просто добавим как есть
            if (items.empty()) {
                for (const auto* c : leftAxisCurves) {
                    items.push_back({c, Utf8ToWide(c->displayName)});
                }
            }
        }

        // Считаем ширину для центрирования
        SIZE prefixSize = {};
        SIZE labelSize = {};
        GetTextExtentPoint32(hdc, prefix.c_str(), (int)prefix.length(), &prefixSize);
        GetTextExtentPoint32(hdc, functionLabel.c_str(), (int)functionLabel.length(), &labelSize);

        int totalWidth = prefixSize.cx + labelSize.cx;
        for (const auto& it : items) {
            SIZE itemText = {};
            GetTextExtentPoint32(hdc, it.label.c_str(), (int)it.label.length(), &itemText);
            totalWidth += lineLength + 5 + itemText.cx + elementSpacing;
        }
        if (!items.empty()) {
            totalWidth -= elementSpacing; // убираем последний отступ
        }

        const int chartWidth = rect.right - rect.left;
        int currentX = rect.left + (chartWidth - totalWidth) / 2;
        const int legendY = rect.top + 35;

        // Рисуем префикс + полное название функции
        SetTextColor(hdc, kText);
        TextOut(hdc, currentX, legendY, prefix.c_str(), (int)prefix.length());
        currentX += prefixSize.cx;

        TextOut(hdc, currentX, legendY, functionLabel.c_str(), (int)functionLabel.length());
        currentX += labelSize.cx;

        // Рисуем элементы (цвет + подпись)
        for (size_t i = 0; i < items.size(); ++i) {
            const CurveDefinition* c = items[i].curve;
            const std::wstring& text = items[i].label;

            // Цветная линия
            HPEN hPen = CreatePen(PS_SOLID, 1, c ? c->color : kAxis);
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
            MoveToEx(hdc, currentX, legendY + lineHeight / 2, NULL);
            LineTo(hdc, currentX + lineLength, legendY + lineHeight / 2);
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);

            currentX += lineLength + 5;

            TextOut(hdc, currentX, legendY, text.c_str(), (int)text.length());
            SIZE tSize = {};
            GetTextExtentPoint32(hdc, text.c_str(), (int)text.length(), &tSize);
            currentX += tSize.cx;

            if (i + 1 < items.size()) {
                currentX += elementSpacing;
            }
        }

        SelectObject(hdc, hOldFont);
        return;
    }
    
    // Параметры легенды для пресетов
    int lineLength = 15;
    int elementSpacing = 15;
    int lineHeight = 20;
    
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    SetBkMode(hdc, TRANSPARENT);
    
    // Группируем кривые по осям
    std::vector<const CurveDefinition*> leftAxisCurves;
    std::vector<const CurveDefinition*> rightAxisCurves;
    
    for (const auto& curve : curves) {
        if (!curve.enabled) continue;
        
        if (curve.yAxisGroup == 0) {
            leftAxisCurves.push_back(&curve);
        } else {
            rightAxisCurves.push_back(&curve);
        }
    }
    
    if (leftAxisCurves.empty() && rightAxisCurves.empty()) {
        SelectObject(hdc, hOldFont);
        return;
    }
    
    // Получаем единицы измерения для каждой оси
    std::string leftUnits, rightUnits;
    if (!leftAxisCurves.empty()) {
        leftUnits = leftAxisCurves[0]->units;
    }
    if (!rightAxisCurves.empty()) {
        rightUnits = rightAxisCurves[0]->units;
    }
    
    // Вычисляем ширину элементов легенды
    int totalWidth = 0;
    
    // Подпись левой оси: [Левая ось (mdeg)]
    std::wstring leftAxisLabel = L"[Левая ось (" + std::wstring(leftUnits.begin(), leftUnits.end()) + L")]";
    SIZE leftLabelSize;
    GetTextExtentPoint32(hdc, leftAxisLabel.c_str(), (int)leftAxisLabel.length(), &leftLabelSize);
    totalWidth += leftLabelSize.cx + 10;
    
    // Название пресета с двоеточием
    std::wstring presetLabel = config.presetDisplayName + L":";
    SIZE presetLabelSize;
    GetTextExtentPoint32(hdc, presetLabel.c_str(), (int)presetLabel.length(), &presetLabelSize);
    totalWidth += presetLabelSize.cx + 10;
    
    // Вычисляем ширину кривых (только короткие имена: T1, T2, T3, T4)
    for (const auto* curve : leftAxisCurves) {
        // Извлекаем короткое имя (T1, T2, T3, T4 или Temperature)
        std::wstring shortName;
        std::string displayName = curve->displayName;
        
        // Для температуры используем полное имя
        if (curve->dataType == DataType::TEMPERATURE) {
            shortName = L"Temp";
        } else {
            // Извлекаем первые 2 символа (T1, T2, T3, T4)
            if (displayName.length() >= 2) {
                shortName = std::wstring(displayName.begin(), displayName.begin() + 2);
            } else {
                shortName = std::wstring(displayName.begin(), displayName.end());
            }
        }
        
        SIZE textSize;
        GetTextExtentPoint32(hdc, shortName.c_str(), (int)shortName.length(), &textSize);
        totalWidth += lineLength + 5 + textSize.cx + elementSpacing;
    }
    
    // Вычисляем начальную X позицию для центрирования
    int chartWidth = rect.right - rect.left;
    int startX = rect.left + (chartWidth - totalWidth) / 2;
    
    // Позиция Y - под заголовком
    int legendY = rect.top + 35;
    
    // Отрисовка легенды
    int currentX = startX;
    
    // Отрисовка подписи левой оси
    SetTextColor(hdc, kText);
    TextOut(hdc, currentX, legendY, leftAxisLabel.c_str(), (int)leftAxisLabel.length());
    currentX += leftLabelSize.cx + 10;
    
    // Отрисовка названия пресета
    TextOut(hdc, currentX, legendY, presetLabel.c_str(), (int)presetLabel.length());
    currentX += presetLabelSize.cx + 10;
    
    // Отрисовка кривых с короткими именами
    for (const auto* curve : leftAxisCurves) {
        // Линия
        HPEN hPen = CreateCurvePen(PS_SOLID, curve->color);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        
        MoveToEx(hdc, currentX, legendY + lineHeight / 2, NULL);
        LineTo(hdc, currentX + lineLength, legendY + lineHeight / 2);
        
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
        
        currentX += lineLength + 5;
        
        // Короткое имя
        std::wstring shortName;
        std::string displayName = curve->displayName;
        
        if (curve->dataType == DataType::TEMPERATURE) {
            shortName = L"Temp";
        } else {
            if (displayName.length() >= 2) {
                shortName = std::wstring(displayName.begin(), displayName.begin() + 2);
            } else {
                shortName = std::wstring(displayName.begin(), displayName.end());
            }
        }
        
        TextOut(hdc, currentX, legendY, shortName.c_str(), (int)shortName.length());
        
        SIZE textSize;
        GetTextExtentPoint32(hdc, shortName.c_str(), (int)shortName.length(), &textSize);
        currentX += textSize.cx + elementSpacing;
    }
    
    // Если есть кривые на правой оси (для пресетов обычно нет, но на всякий случай)
    if (!rightAxisCurves.empty()) {
        // Разделитель
        SetTextColor(hdc, RGB(130, 130, 130));
        TextOut(hdc, currentX, legendY, L"|", 1);
        currentX += 20;
        
        // Подпись правой оси
        std::wstring rightAxisLabel = L"[Правая ось (" + std::wstring(rightUnits.begin(), rightUnits.end()) + L")]";
        SetTextColor(hdc, kText);
        TextOut(hdc, currentX, legendY, rightAxisLabel.c_str(), (int)rightAxisLabel.length());
        
        SIZE rightLabelSize;
        GetTextExtentPoint32(hdc, rightAxisLabel.c_str(), (int)rightAxisLabel.length(), &rightLabelSize);
        currentX += rightLabelSize.cx + 10;
        
        // Кривые правой оси
        for (const auto* curve : rightAxisCurves) {
            HPEN hPen = CreateCurvePen(curve->lineStyle, curve->color);
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
            
            MoveToEx(hdc, currentX, legendY + lineHeight / 2, NULL);
            LineTo(hdc, currentX + lineLength, legendY + lineHeight / 2);
            
            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
            
            currentX += lineLength + 5;
            
            std::wstring shortName;
            std::string displayName = curve->displayName;
            if (displayName.length() >= 2) {
                shortName = std::wstring(displayName.begin(), displayName.begin() + 2);
            } else {
                shortName = std::wstring(displayName.begin(), displayName.end());
            }
            
            TextOut(hdc, currentX, legendY, shortName.c_str(), (int)shortName.length());
            
            SIZE textSize;
            GetTextExtentPoint32(hdc, shortName.c_str(), (int)shortName.length(), &textSize);
            currentX += textSize.cx + elementSpacing;
        }
    }
    
    SelectObject(hdc, hOldFont);
}

// Метод для получения диапазонов данных
void ChartModule::GetDataRanges(const std::vector<std::vector<float>>& leftAxisData,
                               const std::vector<std::vector<float>>& rightAxisData,
                               double& minX, double& maxX,
                               double& minY1, double& maxY1,
                               double& minY2, double& maxY2) const {
    minX = 0.0;
    maxX = 0.0;
    minY1 = 0.0;
    maxY1 = 0.0;
    minY2 = 0.0;
    maxY2 = 0.0;

    // Определяем диапазон X (одинаковый для обеих осей)
    if (!leftAxisData.empty() && !leftAxisData[0].empty()) {
        maxX = static_cast<double>(leftAxisData[0].size() - 1);
    } else if (!rightAxisData.empty() && !rightAxisData[0].empty()) {
        maxX = static_cast<double>(rightAxisData[0].size() - 1);
    }

    // Определяем диапазон Y для левой оси
    if (!leftAxisData.empty()) {
        float minVal = FLT_MAX;
        float maxVal = -FLT_MAX;
        FindMinMaxValues(leftAxisData, minVal, maxVal);
        minY1 = static_cast<double>(minVal);
        maxY1 = static_cast<double>(maxVal);
    }

    // Определяем диапазон Y для правой оси
    if (!rightAxisData.empty()) {
        float minVal = FLT_MAX;
        float maxVal = -FLT_MAX;
        FindMinMaxValues(rightAxisData, minVal, maxVal);
        minY2 = static_cast<double>(minVal);
        maxY2 = static_cast<double>(maxVal);
    }
}


// v2.10.0: Перегрузка DrawChart с поддержкой температуры на оси X
void ChartModule::DrawChart(HDC hdc, const RECT& rect, const std::vector<std::vector<float>>& chartData,
                           DataType type, const std::wstring& title, bool hasData,
                           const std::vector<bool>& enabledTransmitters, const DataModule* dataModule,
                           double minXData, double maxXData, double minYData, double maxYData,
                           const std::vector<float>& xAxisData, bool showAxes) const {
    if (rect.right <= rect.left || rect.bottom <= rect.top)
        return;

    // Отрисовка фона и рамки
    DrawChartBackground(hdc, rect);
    DrawChartBorder(hdc, rect);
    
    // Отрисовка заголовка
    DrawChartTitle(hdc, rect, title);
    
    // Расчет области для графика
    RECT plotRect = CalculatePlotRect(rect);

    // Отрисовка осей
    DrawChartAxes(hdc, plotRect, showAxes);

    if (plotRect.right <= plotRect.left || plotRect.bottom <= plotRect.top)
        return;
    
    // Отрисовка сетки и осей с метками (с температурой)
    DrawGridAndAxes(hdc, plotRect, chartData, dataModule, xAxisData,
                    minXData, maxXData, minYData, maxYData);
    
    // Отрисовка данных с температурой на оси X
    if (hasData && !xAxisData.empty()) {
        DrawChartData(hdc, plotRect, chartData, xAxisData, minXData, maxXData);
    }
    
    // Отрисовка легенды
    if (chartData.size() > 1) {
        DrawChartLegend(hdc, rect, chartData, type, enabledTransmitters);
    }
}

// v2.10.0: Отрисовка данных с температурой на оси X
void ChartModule::DrawChartData(HDC hdc, const RECT& plotRect, const std::vector<std::vector<float>>& chartData,
                                const std::vector<float>& xAxisData, double minXData, double maxXData) const {
    if (xAxisData.empty() || chartData.empty()) return;
    
    // Находим минимальные и максимальные значения Y
    float minVal, maxVal;
    FindMinMaxValues(chartData, minVal, maxVal);
    
    // Находим минимальные и максимальные значения температуры для отображаемого диапазона
    size_t startIdx = static_cast<size_t>(std::max(0.0, minXData));
    size_t endIdx = static_cast<size_t>(std::min(static_cast<double>(xAxisData.size()), maxXData + 1));
    
    if (startIdx >= endIdx || endIdx > xAxisData.size()) return;
    
    float minTemp = *std::min_element(xAxisData.begin() + startIdx, xAxisData.begin() + endIdx);
    float maxTemp = *std::max_element(xAxisData.begin() + startIdx, xAxisData.begin() + endIdx);
    
    if (maxTemp <= minTemp) {
        maxTemp = minTemp + 1.0f; // Избегаем деления на ноль
    }
    
    // Рисуем данные для каждого канала
    for (size_t channelIdx = 0; channelIdx < chartData.size(); channelIdx++) {
        const auto& channel = chartData[channelIdx];
        if (channel.empty()) continue;

        std::vector<POINT> points;
        points.reserve((endIdx > startIdx) ? (endIdx - startIdx) : 0);
        for (size_t i = startIdx; i < endIdx && i < channel.size() && i < xAxisData.size(); i++) {
            // X координата основана на температуре
            float temp = xAxisData[i];
            int x = plotRect.left + (int)((temp - minTemp) / (maxTemp - minTemp) * (plotRect.right - plotRect.left));
            
            // Y координата основана на значении данных
            int y = plotRect.bottom - (int)((channel[i] - minVal) / (maxVal - minVal) * (plotRect.bottom - plotRect.top));
            
            points.push_back({ x, y });
        }

        DrawCurvePolyline(hdc, points, CHANNEL_COLORS[channelIdx % 5], PS_SOLID);
    }
}

// v2.10.0: Отрисовка сетки и осей с температурой на оси X
void ChartModule::DrawGridAndAxes(HDC hdc, const RECT& plotRect, const std::vector<std::vector<float>>& chartData,
                                  const DataModule* dataModule, const std::vector<float>& xAxisData,
                                  double minXData, double maxXData, double minYData, double maxYData) const {
    if (xAxisData.empty()) return;
    
    // Находим диапазон температур для отображаемого диапазона
    size_t startIdx = static_cast<size_t>(std::max(0.0, minXData));
    size_t endIdx = static_cast<size_t>(std::min(static_cast<double>(xAxisData.size()), maxXData + 1));
    
    if (startIdx >= endIdx || endIdx > xAxisData.size()) return;
    
    float minTemp = *std::min_element(xAxisData.begin() + startIdx, xAxisData.begin() + endIdx);
    float maxTemp = *std::max_element(xAxisData.begin() + startIdx, xAxisData.begin() + endIdx);
    
    if (maxTemp <= minTemp) {
        maxTemp = minTemp + 1.0f;
    }
    
    // Находим диапазон Y
    float minVal, maxVal;
    FindMinMaxValues(chartData, minVal, maxVal, minYData, maxYData);
    
    // Рисуем сетку (мягкая, ненавязчивая)
    HPEN hGridPen = CreatePen(PS_SOLID, 1, kGridMajor);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hGridPen);
    
    // Вертикальные линии сетки (по температуре)
    int numVerticalLines = 10;
    
    // Отслеживание уже отрисованных целых значений для предотвращения дубликатов
    std::set<int> drawnTempLabels;
    
    for (int i = 0; i <= numVerticalLines; i++) {
        int x = plotRect.left + (plotRect.right - plotRect.left) * i / numVerticalLines;
        MoveToEx(hdc, x, plotRect.top, NULL);
        LineTo(hdc, x, plotRect.bottom);
        
        // Подпись температуры (только целое число, без единиц, без дубликатов)
        float temp = minTemp + (maxTemp - minTemp) * i / numVerticalLines;
        int roundedTemp = static_cast<int>(std::round(temp));
        
        if (drawnTempLabels.find(roundedTemp) == drawnTempLabels.end()) {
            drawnTempLabels.insert(roundedTemp);
            
            wchar_t label[32];
            swprintf_s(label, L"%d", roundedTemp);
            
            SIZE textSize;
            GetTextExtentPoint32(hdc, label, (int)wcslen(label), &textSize);
            TextOut(hdc, x - textSize.cx / 2, plotRect.bottom + 5, label, (int)wcslen(label));
        }
    }
    
    // Горизонтальные линии сетки (по Y) - используем "nice" алгоритм как в DrawLeftAxis
    double niceStepY = CalculateNiceStep(maxVal - minVal, (plotRect.bottom - plotRect.top) / 40);
    double currentYVal = CalculateNiceMin(minVal, niceStepY);
    
    while (currentYVal <= maxVal + niceStepY / 2) {
        if (currentYVal >= minVal - niceStepY / 2 && currentYVal <= maxVal + niceStepY / 2) {
            long screenY = plotRect.bottom - (LONG)((currentYVal - minVal) / (maxVal - minVal) * (plotRect.bottom - plotRect.top));
            
            if (screenY >= plotRect.top && screenY <= plotRect.bottom) {
                // Рисуем горизонтальную линию сетки
                MoveToEx(hdc, plotRect.left, screenY, NULL);
                LineTo(hdc, plotRect.right, screenY);
                
                // Подпись Y (только число, без единиц)
                std::wstringstream ss;
                ss << std::fixed << std::setprecision(1) << currentYVal;
                std::wstring label = ss.str();
                
                SIZE textSize;
                GetTextExtentPoint32(hdc, label.c_str(), (int)label.length(), &textSize);
                int textX = plotRect.left - 5 - textSize.cx;
                TextOut(hdc, textX, screenY - 7, label.c_str(), (int)label.length());
            }
        }
        currentYVal += niceStepY;
    }
    
    SelectObject(hdc, hOldPen);
    DeleteObject(hGridPen);
    
    // v2.10.0: Добавляем подпись единиц измерения для оси X
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, kText);
    
    // Подпись оси X (Temperature) - справа внизу, ниже числовых значений
    const wchar_t* xAxisLabel = L"\u00B0C"; // Unicode для символа градуса
    SIZE xLabelSize;
    GetTextExtentPoint32(hdc, xAxisLabel, (int)wcslen(xAxisLabel), &xLabelSize);
    int xLabelX = plotRect.right - xLabelSize.cx - 5; // Справа с отступом 5px
    int xLabelY = plotRect.bottom + 25; // Ниже числовых значений (5 + 20 отступ)
    TextOut(hdc, xLabelX, xLabelY, xAxisLabel, (int)wcslen(xAxisLabel));
}


// v2.10.0 + v2.12.0: Отрисовка графика с двойными осями и температурой на оси X с поддержкой zoom по Y
void ChartModule::DrawDualAxisChart(HDC hdc, const RECT& rect,
                                   const std::vector<CurveDefinition>& curves,
                                   const std::vector<std::vector<float>>& leftAxisData,
                                   const std::vector<std::vector<float>>& rightAxisData,
                                   const std::wstring& title, bool hasData,
                                   const std::vector<float>& xAxisData,
                                   double minXData, double maxXData,
                                   double minYData, double maxYData, bool showAxes) const {
    if (rect.right <= rect.left || rect.bottom <= rect.top)
        return;

    // Отрисовка фона и рамки
    DrawDualAxisBackground(hdc, rect);
    DrawChartBorder(hdc, rect);
    
    // Отрисовка заголовка
    DrawChartTitle(hdc, rect, title);
    
    // Расчет области для графика
    RECT plotRect = CalculateDualAxisPlotRect(rect);

    if (plotRect.right <= plotRect.left || plotRect.bottom <= plotRect.top)
        return;
    
    // Получаем единицы измерения для осей
    std::string leftUnits, rightUnits;
    for (const auto& curve : curves) {
        if (!curve.enabled) continue;
        
        if (curve.yAxisGroup == 0 && leftUnits.empty()) {
            leftUnits = curve.units;
        } else if (curve.yAxisGroup == 1 && rightUnits.empty()) {
            rightUnits = curve.units;
        }
    }
    
    // Отрисовка сетки и осей с температурой и zoom по Y
    if (hasData && !xAxisData.empty() && showAxes) {
        DrawDualAxisGridWithTemperature(hdc, plotRect, leftAxisData, rightAxisData,
                                       xAxisData, minXData, maxXData, minYData, maxYData, leftUnits);
    }
    
    // Вычисляем полные диапазоны из данных
    double leftMinFull = 0.0, leftMaxFull = 1.0;
    if (!leftAxisData.empty()) {
        float minVal = FLT_MAX, maxVal = -FLT_MAX;
        FindMinMaxValues(leftAxisData, minVal, maxVal);
        leftMinFull = static_cast<double>(minVal);
        leftMaxFull = static_cast<double>(maxVal);
    }
    
    double rightMinFull = 0.0, rightMaxFull = 1.0;
    if (!rightAxisData.empty()) {
        float minVal = FLT_MAX, maxVal = -FLT_MAX;
        FindMinMaxValues(rightAxisData, minVal, maxVal);
        rightMinFull = static_cast<double>(minVal);
        rightMaxFull = static_cast<double>(maxVal);
    }
    
    // Вычисляем пропорциональный zoom для правой оси
    double minYRight = rightMinFull, maxYRight = rightMaxFull;
    double leftFullRange = leftMaxFull - leftMinFull;
    double rightFullRange = rightMaxFull - rightMinFull;
    
    if (leftFullRange > 0.0001 && rightFullRange > 0.0001) {
        double minRatio = (minYData - leftMinFull) / leftFullRange;
        double maxRatio = (maxYData - leftMinFull) / leftFullRange;
        minYRight = rightMinFull + minRatio * rightFullRange;
        maxYRight = rightMinFull + maxRatio * rightFullRange;
    }
    
    if (!leftAxisData.empty()) {
        DrawLeftAxisWithZoom(hdc, plotRect, leftAxisData, leftUnits, minYData, maxYData);
    }
    
    if (!rightAxisData.empty()) {
        DrawRightAxisWithZoom(hdc, plotRect, rightAxisData, rightUnits, minYRight, maxYRight);
    }
    
    // Отрисовка данных с температурой на оси X и zoom по Y
    if (hasData && !xAxisData.empty()) {
        DrawDualAxisDataWithTemperatureAndZoom(hdc, plotRect, curves, leftAxisData, rightAxisData,
                                               xAxisData, minXData, maxXData, minYData, maxYData);
    }
    
    // Отрисовка легенды
    if (!curves.empty()) {
        DrawDualAxisLegend(hdc, rect, curves);
    }
}

// v2.15.0: Перегрузка с поддержкой температуры и конфигурации (для пресетов)
void ChartModule::DrawDualAxisChart(HDC hdc, const RECT& rect,
                                   const std::vector<CurveDefinition>& curves,
                                   const std::vector<std::vector<float>>& leftAxisData,
                                   const std::vector<std::vector<float>>& rightAxisData,
                                   const std::wstring& title, bool hasData,
                                   const std::vector<float>& xAxisData,
                                   double minXData, double maxXData,
                                   double minYData, double maxYData,
                                   const ChartConfiguration& config, bool showAxes) const {
    if (rect.right <= rect.left || rect.bottom <= rect.top)
        return;

    // Отрисовка фона и рамки
    DrawDualAxisBackground(hdc, rect);
    DrawChartBorder(hdc, rect);
    
    // Отрисовка заголовка
    DrawChartTitle(hdc, rect, title);
    
    // Расчет области для графика
    RECT plotRect = CalculateDualAxisPlotRect(rect);

    if (plotRect.right <= plotRect.left || plotRect.bottom <= plotRect.top)
        return;
    
    // Получаем единицы измерения для осей
    std::string leftUnits, rightUnits;
    for (const auto& curve : curves) {
        if (!curve.enabled) continue;
        
        if (curve.yAxisGroup == 0 && leftUnits.empty()) {
            leftUnits = curve.units;
        } else if (curve.yAxisGroup == 1 && rightUnits.empty()) {
            rightUnits = curve.units;
        }
    }
    
    // Отрисовка сетки и осей с температурой и zoom по Y
    if (hasData && !xAxisData.empty() && showAxes) {
        DrawDualAxisGridWithTemperature(hdc, plotRect, leftAxisData, rightAxisData,
                                       xAxisData, minXData, maxXData, minYData, maxYData, leftUnits);
    }
    
    // Вычисляем полные диапазоны из данных
    double leftMinFull = 0.0, leftMaxFull = 1.0;
    if (!leftAxisData.empty()) {
        float minVal = FLT_MAX, maxVal = -FLT_MAX;
        FindMinMaxValues(leftAxisData, minVal, maxVal);
        leftMinFull = static_cast<double>(minVal);
        leftMaxFull = static_cast<double>(maxVal);
    }
    
    double rightMinFull = 0.0, rightMaxFull = 1.0;
    if (!rightAxisData.empty()) {
        float minVal = FLT_MAX, maxVal = -FLT_MAX;
        FindMinMaxValues(rightAxisData, minVal, maxVal);
        rightMinFull = static_cast<double>(minVal);
        rightMaxFull = static_cast<double>(maxVal);
    }
    
    // Вычисляем пропорциональный zoom для правой оси
    double minYRight = rightMinFull, maxYRight = rightMaxFull;
    double leftFullRange = leftMaxFull - leftMinFull;
    double rightFullRange = rightMaxFull - rightMinFull;
    
    if (leftFullRange > 0.0001 && rightFullRange > 0.0001) {
        double minRatio = (minYData - leftMinFull) / leftFullRange;
        double maxRatio = (maxYData - leftMinFull) / leftFullRange;
        minYRight = rightMinFull + minRatio * rightFullRange;
        maxYRight = rightMinFull + maxRatio * rightFullRange;
    }
    
    if (!leftAxisData.empty()) {
        DrawLeftAxisWithZoom(hdc, plotRect, leftAxisData, leftUnits, minYData, maxYData);
    }
    
    if (!rightAxisData.empty()) {
        DrawRightAxisWithZoom(hdc, plotRect, rightAxisData, rightUnits, minYRight, maxYRight);
    }
    
    // Отрисовка данных с температурой на оси X и zoom по Y
    if (hasData && !xAxisData.empty()) {
        DrawDualAxisDataWithTemperatureAndZoom(hdc, plotRect, curves, leftAxisData, rightAxisData,
                                               xAxisData, minXData, maxXData, minYData, maxYData);
    }
    
    // Отрисовка легенды с учетом пресета
    if (!curves.empty()) {
        DrawDualAxisLegend(hdc, rect, curves, config);
    }
}


// v2.10.0: Отрисовка данных с температурой на оси X
void ChartModule::DrawDualAxisData(HDC hdc, const RECT& plotRect,
                                  const std::vector<CurveDefinition>& curves,
                                  const std::vector<std::vector<float>>& leftAxisData,
                                  const std::vector<std::vector<float>>& rightAxisData,
                                  const std::vector<float>& xAxisData,
                                  double minXData, double maxXData) const {
    // Находим диапазоны для обеих осей
    float leftMin = FLT_MAX, leftMax = -FLT_MAX;
    float rightMin = FLT_MAX, rightMax = -FLT_MAX;
    
    // Диапазон левой оси
    for (const auto& channel : leftAxisData) {
        for (float val : channel) {
            leftMin = (std::min)(leftMin, val);
            leftMax = (std::max)(leftMax, val);
        }
    }
    
    // Диапазон правой оси
    for (const auto& channel : rightAxisData) {
        for (float val : channel) {
            rightMin = (std::min)(rightMin, val);
            rightMax = (std::max)(rightMax, val);
        }
    }
    
    // Добавляем padding
    if (leftMin != FLT_MAX && leftMax != -FLT_MAX && leftMin != leftMax) {
        float leftRange = leftMax - leftMin;
        float leftPadding = leftRange * 0.05f;
        leftMin -= leftPadding;
        leftMax += leftPadding;
    } else {
        leftMin = 0.0f;
        leftMax = 100.0f;
    }
    
    if (rightMin != FLT_MAX && rightMax != -FLT_MAX && rightMin != rightMax) {
        float rightRange = rightMax - rightMin;
        float rightPadding = rightRange * 0.05f;
        rightMin -= rightPadding;
        rightMax += rightPadding;
    } else {
        rightMin = 0.0f;
        rightMax = 100.0f;
    }
    
    // Находим диапазон температур
    size_t startIdx = static_cast<size_t>(std::max(0.0, minXData));
    size_t endIdx = static_cast<size_t>(std::min(static_cast<double>(xAxisData.size()), maxXData + 1));
    
    if (startIdx >= endIdx || endIdx > xAxisData.size()) return;
    
    float minTemp = *std::min_element(xAxisData.begin() + startIdx, xAxisData.begin() + endIdx);
    float maxTemp = *std::max_element(xAxisData.begin() + startIdx, xAxisData.begin() + endIdx);
    
    if (maxTemp <= minTemp) {
        maxTemp = minTemp + 1.0f;
    }
    
    // Отрисовка кривых левой оси
    size_t leftIndex = 0;
    for (size_t i = 0; i < curves.size() && leftIndex < leftAxisData.size(); i++) {
        const auto& curve = curves[i];
        if (!curve.enabled || curve.yAxisGroup != 0) continue;
        
        const auto& channel = leftAxisData[leftIndex];
        if (channel.empty()) {
            leftIndex++;
            continue;
        }

        std::vector<POINT> points;
        points.reserve((endIdx > startIdx) ? (endIdx - startIdx) : 0);
        for (size_t j = startIdx; j < endIdx && j < channel.size() && j < xAxisData.size(); j++) {
            float temp = xAxisData[j];
            int x = plotRect.left + (int)((temp - minTemp) / (maxTemp - minTemp) * (plotRect.right - plotRect.left));
            float clampedValue = (std::max)(leftMin, (std::min)(leftMax, channel[j]));
            int y = plotRect.bottom - (int)((clampedValue - leftMin) / (leftMax - leftMin) * (plotRect.bottom - plotRect.top));

            points.push_back({ x, y });
        }

        DrawCurvePolyline(hdc, points, curve.color, PS_SOLID);
        leftIndex++;
    }
    
    // Отрисовка кривых правой оси
    size_t rightIndex = 0;
    for (size_t i = 0; i < curves.size() && rightIndex < rightAxisData.size(); i++) {
        const auto& curve = curves[i];
        if (!curve.enabled || curve.yAxisGroup != 1) continue;
        
        const auto& channel = rightAxisData[rightIndex];
        if (channel.empty()) {
            rightIndex++;
            continue;
        }

        std::vector<POINT> points;
        points.reserve((endIdx > startIdx) ? (endIdx - startIdx) : 0);
        for (size_t j = startIdx; j < endIdx && j < channel.size() && j < xAxisData.size(); j++) {
            float temp = xAxisData[j];
            int x = plotRect.left + (int)((temp - minTemp) / (maxTemp - minTemp) * (plotRect.right - plotRect.left));
            float clampedValue = (std::max)(rightMin, (std::min)(rightMax, channel[j]));
            int y = plotRect.bottom - (int)((clampedValue - rightMin) / (rightMax - rightMin) * (plotRect.bottom - plotRect.top));

            points.push_back({ x, y });
        }

        DrawCurvePolyline(hdc, points, curve.color, curve.lineStyle);
        rightIndex++;
    }
}

// v2.12.0: Вспомогательные функции для температурного режима с zoom по Y

void ChartModule::DrawDualAxisGridWithTemperature(HDC hdc, const RECT& plotRect,
                                                  const std::vector<std::vector<float>>& leftAxisData,
                                                  const std::vector<std::vector<float>>& rightAxisData,
                                                  const std::vector<float>& xAxisData,
                                                  double minXData, double maxXData,
                                                  double minYData, double maxYData,
                                                  const std::string& leftUnits) const {
    if (xAxisData.empty()) return;
    
    // Находим диапазон температур для отображаемого диапазона
    size_t startIdx = static_cast<size_t>(std::max(0.0, minXData));
    size_t endIdx = static_cast<size_t>(std::min(static_cast<double>(xAxisData.size()), maxXData + 1));
    
    if (startIdx >= endIdx || endIdx > xAxisData.size()) return;
    
    float minTemp = *std::min_element(xAxisData.begin() + startIdx, xAxisData.begin() + endIdx);
    float maxTemp = *std::max_element(xAxisData.begin() + startIdx, xAxisData.begin() + endIdx);
    
    if (maxTemp <= minTemp) {
        maxTemp = minTemp + 1.0f;
    }
    
    // Константа для длины засечки
    const int TICK_LENGTH = 5;
    
    // Рисуем сетку (мягкая, ненавязчивая)
    HPEN hGridPen = CreatePen(PS_SOLID, 1, kGridMajor);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hGridPen);
    
    // Вертикальные линии сетки (по температуре)
    int numVerticalLines = 10;
    std::set<int> drawnTempLabels;
    
    for (int i = 0; i <= numVerticalLines; i++) {
        int x = plotRect.left + (plotRect.right - plotRect.left) * i / numVerticalLines;
        MoveToEx(hdc, x, plotRect.top, NULL);
        LineTo(hdc, x, plotRect.bottom);
        
        // Рисуем засечку (tick mark) вниз от оси X
        SelectObject(hdc, hOldPen);
        HPEN hTickPen = CreatePen(PS_SOLID, 1, kAxis);
        HPEN hOldTickPen = (HPEN)SelectObject(hdc, hTickPen);
        MoveToEx(hdc, x, plotRect.bottom, NULL);
        LineTo(hdc, x, plotRect.bottom + TICK_LENGTH);
        SelectObject(hdc, hOldTickPen);
        DeleteObject(hTickPen);
        SelectObject(hdc, hGridPen);
        
        float temp = minTemp + (maxTemp - minTemp) * i / numVerticalLines;
        int roundedTemp = static_cast<int>(std::round(temp));
        
        if (drawnTempLabels.find(roundedTemp) == drawnTempLabels.end()) {
            drawnTempLabels.insert(roundedTemp);
            
            wchar_t label[32];
            swprintf_s(label, L"%d", roundedTemp);
            
            SIZE textSize;
            GetTextExtentPoint32(hdc, label, (int)wcslen(label), &textSize);
            TextOut(hdc, x - textSize.cx / 2, plotRect.bottom + TICK_LENGTH + 2, label, (int)wcslen(label));
        }
    }
    
    // Горизонтальные линии сетки (по Y) с учетом zoom
    double niceStepY = CalculateNiceStep(maxYData - minYData, (plotRect.bottom - plotRect.top) / 40);
    double currentYVal = CalculateNiceMin(minYData, niceStepY);
    
    while (currentYVal <= maxYData + niceStepY / 2) {
        if (currentYVal >= minYData - niceStepY / 2 && currentYVal <= maxYData + niceStepY / 2) {
            long screenY = plotRect.bottom - (LONG)((currentYVal - minYData) / (maxYData - minYData) * (plotRect.bottom - plotRect.top));
            
            if (screenY >= plotRect.top && screenY <= plotRect.bottom) {
                MoveToEx(hdc, plotRect.left, screenY, NULL);
                LineTo(hdc, plotRect.right, screenY);
                
                // Рисуем засечку (tick mark) влево от оси Y
                SelectObject(hdc, hOldPen);
                HPEN hTickPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
                HPEN hOldTickPen = (HPEN)SelectObject(hdc, hTickPen);
                MoveToEx(hdc, plotRect.left - TICK_LENGTH, screenY, NULL);
                LineTo(hdc, plotRect.left, screenY);
                SelectObject(hdc, hOldTickPen);
                DeleteObject(hTickPen);
                SelectObject(hdc, hGridPen);
                
                std::wstringstream ss;
                // Для фазовых данных (mdeg) используем целые числа, для остальных - 2 знака после запятой
                if (leftUnits == "mdeg") {
                    ss << std::fixed << std::setprecision(0) << currentYVal;
                } else {
                    ss << std::fixed << std::setprecision(2) << currentYVal;
                }
                std::wstring label = ss.str();
                
                SIZE textSize;
                GetTextExtentPoint32(hdc, label.c_str(), (int)label.length(), &textSize);
                int textX = plotRect.left - TICK_LENGTH - 2 - textSize.cx;
                TextOut(hdc, textX, screenY - 7, label.c_str(), (int)label.length());
            }
        }
        currentYVal += niceStepY;
    }
    
    SelectObject(hdc, hOldPen);
    DeleteObject(hGridPen);
    
    // Рисуем ось X (без утолщения)
    HPEN hAxisPen = CreatePen(PS_SOLID, 1, kAxis);
    HPEN hOldAxisPen = (HPEN)SelectObject(hdc, hAxisPen);
    MoveToEx(hdc, plotRect.left, plotRect.bottom, NULL);
    LineTo(hdc, plotRect.right, plotRect.bottom);
    SelectObject(hdc, hOldAxisPen);
    DeleteObject(hAxisPen);
}

void ChartModule::DrawLeftAxisWithZoom(HDC hdc, const RECT& plotRect,
                                      const std::vector<std::vector<float>>& leftAxisData,
                                      const std::string& units,
                                      double minYData, double maxYData) const {
    if (leftAxisData.empty()) return;
    
    // Используем переданные параметры zoom вместо вычисления из данных
    float minVal = static_cast<float>(minYData);
    float maxVal = static_cast<float>(maxYData);
    
    SetTextColor(hdc, kText);
    SetBkMode(hdc, TRANSPARENT);

    // Рисуем левую ось
    HPEN hPen = CreatePen(PS_SOLID, 1, kAxis);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    
    MoveToEx(hdc, plotRect.left, plotRect.top, NULL);
    LineTo(hdc, plotRect.left, plotRect.bottom);
    
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
    
    // Рисуем метки левой оси с учетом zoom
    double niceStepY = CalculateNiceStep(maxVal - minVal, (plotRect.bottom - plotRect.top) / 40);
    double currentYVal = CalculateNiceMin(minVal, niceStepY);
    
    SetTextColor(hdc, kText);
    SetBkMode(hdc, TRANSPARENT);
    
    // Константа для длины засечки
    const int TICK_LENGTH = 5;
    
    while (currentYVal <= maxVal + niceStepY / 2) {
        if (currentYVal >= minVal - niceStepY / 2 && currentYVal <= maxVal + niceStepY / 2) {
            long screenY = plotRect.bottom - (LONG)((currentYVal - minVal) / (maxVal - minVal) * (plotRect.bottom - plotRect.top));
            
            if (screenY >= plotRect.top && screenY <= plotRect.bottom) {
                // Рисуем засечку (tick mark) влево от оси
                HPEN hTickPen = CreatePen(PS_SOLID, 1, kAxis);
                HPEN hOldTickPen = (HPEN)SelectObject(hdc, hTickPen);
                MoveToEx(hdc, plotRect.left - TICK_LENGTH, screenY, NULL);
                LineTo(hdc, plotRect.left, screenY);
                SelectObject(hdc, hOldTickPen);
                DeleteObject(hTickPen);
                
                std::wstringstream ss;
                // Для фазовых данных (mdeg) используем целые числа, для остальных - 2 знака после запятой
                if (units == "mdeg") {
                    ss << std::fixed << std::setprecision(0) << currentYVal;
                } else {
                    ss << std::fixed << std::setprecision(2) << currentYVal;
                }
                std::wstring label = ss.str();
                
                SIZE textSize;
                GetTextExtentPoint32(hdc, label.c_str(), (int)label.length(), &textSize);
                
                int textX = plotRect.left - TICK_LENGTH - 2 - textSize.cx;
                TextOut(hdc, textX, screenY - 7, label.c_str(), (int)label.length());
            }
        }
        currentYVal += niceStepY;
    }
    
    // Подпись единиц измерения левой оси
    if (!units.empty()) {
        std::wstring wunits(units.begin(), units.end());
        TextOut(hdc, plotRect.left - 45, plotRect.top - 20, wunits.c_str(), (int)wunits.length());
    }
}

void ChartModule::DrawRightAxisWithZoom(HDC hdc, const RECT& plotRect,
                                       const std::vector<std::vector<float>>& rightAxisData,
                                       const std::string& units,
                                       double minYData, double maxYData) const {
    if (rightAxisData.empty()) return;
    
    // Используем переданные параметры zoom вместо вычисления из данных
    float minVal = static_cast<float>(minYData);
    float maxVal = static_cast<float>(maxYData);
    
    // Рисуем правую ось с отступом от края
    int rightAxisX = plotRect.right + 40;
    
    SetTextColor(hdc, kText);
    SetBkMode(hdc, TRANSPARENT);

    HPEN hPen = CreatePen(PS_SOLID, 1, kAxis);
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    
    MoveToEx(hdc, rightAxisX, plotRect.top, NULL);
    LineTo(hdc, rightAxisX, plotRect.bottom);
    
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
    
    // Рисуем метки правой оси с учетом zoom
    double niceStepY = CalculateNiceStep(maxVal - minVal, (plotRect.bottom - plotRect.top) / 40);
    double currentYVal = CalculateNiceMin(minVal, niceStepY);
    
    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkMode(hdc, TRANSPARENT);
    
    // Константа для длины засечки
    const int TICK_LENGTH = 5;
    
    while (currentYVal <= maxVal + niceStepY / 2) {
        if (currentYVal >= minVal - niceStepY / 2 && currentYVal <= maxVal + niceStepY / 2) {
            long screenY = plotRect.bottom - (LONG)((currentYVal - minVal) / (maxVal - minVal) * (plotRect.bottom - plotRect.top));
            
            if (screenY >= plotRect.top && screenY <= plotRect.bottom) {
                // Рисуем засечку (tick mark) вправо от оси
                HPEN hTickPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
                HPEN hOldTickPen = (HPEN)SelectObject(hdc, hTickPen);
                MoveToEx(hdc, rightAxisX, screenY, NULL);
                LineTo(hdc, rightAxisX + TICK_LENGTH, screenY);
                SelectObject(hdc, hOldTickPen);
                DeleteObject(hTickPen);
                
                std::wstringstream ss;
                // Для фазовых данных (mdeg) используем целые числа, для остальных - 2 знака после запятой
                if (units == "mdeg") {
                    ss << std::fixed << std::setprecision(0) << currentYVal;
                } else {
                    ss << std::fixed << std::setprecision(2) << currentYVal;
                }
                std::wstring label = ss.str();
                TextOut(hdc, rightAxisX + TICK_LENGTH + 2, screenY - 7, label.c_str(), (int)label.length());
            }
        }
        currentYVal += niceStepY;
    }
    
    // Подпись единиц измерения правой оси
    if (!units.empty()) {
        std::wstring wunits(units.begin(), units.end());
        TextOut(hdc, rightAxisX + TICK_LENGTH + 2, plotRect.top - 20, wunits.c_str(), (int)wunits.length());
    }
}

void ChartModule::DrawDualAxisDataWithTemperatureAndZoom(HDC hdc, const RECT& plotRect,
                                                         const std::vector<CurveDefinition>& curves,
                                                         const std::vector<std::vector<float>>& leftAxisData,
                                                         const std::vector<std::vector<float>>& rightAxisData,
                                                         const std::vector<float>& xAxisData,
                                                         double minXData, double maxXData,
                                                         double minYData, double maxYData) const {
    if (xAxisData.empty()) return;

    RECT dataRect = plotRect;
    int plotHeight = plotRect.bottom - plotRect.top;
    int yMargin = plotHeight * 3 / 100;
    yMargin = (std::max)(8, (std::min)(24, yMargin));
    dataRect.top += yMargin;
    dataRect.bottom -= yMargin;

    if (dataRect.bottom <= dataRect.top) {
        return;
    }
    
    // Создаем clipping region с небольшим расширением, чтобы крайние точки не обрезались
    // Расширяем на 5 пикселей сверху и снизу
    HRGN hClipRegion = CreateRectRgn(dataRect.left, dataRect.top - 5, dataRect.right, dataRect.bottom + 5);
    HRGN hOldClipRegion = CreateRectRgn(0, 0, 0, 0);
    int regionType = GetClipRgn(hdc, hOldClipRegion);
    SelectClipRgn(hdc, hClipRegion);
    
    // Находим диапазон температур
    size_t startIdx = static_cast<size_t>(std::max(0.0, minXData));
    size_t endIdx = static_cast<size_t>(std::min(static_cast<double>(xAxisData.size()), maxXData + 1));
    
    if (startIdx >= endIdx || endIdx > xAxisData.size()) {
        SelectClipRgn(hdc, regionType == 1 ? hOldClipRegion : NULL);
        DeleteObject(hClipRegion);
        DeleteObject(hOldClipRegion);
        return;
    }
    
    float minTemp = *std::min_element(xAxisData.begin() + startIdx, xAxisData.begin() + endIdx);
    float maxTemp = *std::max_element(xAxisData.begin() + startIdx, xAxisData.begin() + endIdx);
    
    if (maxTemp <= minTemp) {
        maxTemp = minTemp + 1.0f;
    }
    
    // Вычисляем полные диапазоны из данных для пропорционального zoom
    double leftMinFull = 0.0, leftMaxFull = 1.0;
    if (!leftAxisData.empty()) {
        float minVal = FLT_MAX, maxVal = -FLT_MAX;
        FindMinMaxValues(leftAxisData, minVal, maxVal);
        leftMinFull = static_cast<double>(minVal);
        leftMaxFull = static_cast<double>(maxVal);
    }
    
    double rightMinFull = 0.0, rightMaxFull = 1.0;
    if (!rightAxisData.empty()) {
        float minVal = FLT_MAX, maxVal = -FLT_MAX;
        FindMinMaxValues(rightAxisData, minVal, maxVal);
        rightMinFull = static_cast<double>(minVal);
        rightMaxFull = static_cast<double>(maxVal);
    }
    
    // Вычисляем пропорциональный zoom для правой оси
    double minYRight = rightMinFull, maxYRight = rightMaxFull;
    double leftFullRange = leftMaxFull - leftMinFull;
    double rightFullRange = rightMaxFull - rightMinFull;
    
    if (leftFullRange > 0.0001 && rightFullRange > 0.0001) {
        double minRatio = (minYData - leftMinFull) / leftFullRange;
        double maxRatio = (maxYData - leftMinFull) / leftFullRange;
        minYRight = rightMinFull + minRatio * rightFullRange;
        maxYRight = rightMinFull + maxRatio * rightFullRange;
    }
    
    // Добавляем небольшой padding только снизу (2%) для визуального комфорта
    float yRangeLeft = static_cast<float>(maxYData - minYData);
    float yPaddingLeft = yRangeLeft * 0.02f;
    float leftMin = static_cast<float>(minYData) - yPaddingLeft;
    float leftMax = static_cast<float>(maxYData);
    
    float yRangeRight = static_cast<float>(maxYRight - minYRight);
    float yPaddingRight = yRangeRight * 0.02f;
    float rightMin = static_cast<float>(minYRight) - yPaddingRight;
    float rightMax = static_cast<float>(maxYRight);
    
    // Отладочный вывод
    wchar_t debugMsg[512];
    swprintf_s(debugMsg, L"=== DrawDualAxisDataWithTemperatureAndZoom ===\n"
                         L"plotRect: left=%d, top=%d, right=%d, bottom=%d\n"
                         L"minYData=%.2f, maxYData=%.2f\n"
                         L"leftMin=%.2f, leftMax=%.2f\n",
              plotRect.left, plotRect.top, plotRect.right, plotRect.bottom,
              minYData, maxYData, leftMin, leftMax);
    AppendDebugLog(L"%ls", debugMsg);
    
    // Отрисовка кривых левой оси (сплошные линии)
    size_t leftIndex = 0;
    for (size_t i = 0; i < curves.size() && leftIndex < leftAxisData.size(); i++) {
        const auto& curve = curves[i];
        if (!curve.enabled || curve.yAxisGroup != 0) continue;
        
        const auto& channel = leftAxisData[leftIndex];
        if (channel.empty()) {
            leftIndex++;
            continue;
        }

        std::vector<POINT> points;
        points.reserve((endIdx > startIdx) ? (endIdx - startIdx) : 0);
        
        // Отладка для первой кривой
        float minChannelVal = *std::min_element(channel.begin() + startIdx, channel.begin() + std::min(endIdx, channel.size()));
        float maxChannelVal = *std::max_element(channel.begin() + startIdx, channel.begin() + std::min(endIdx, channel.size()));
        swprintf_s(debugMsg, L"Curve %zu: minVal=%.2f, maxVal=%.2f\n", leftIndex, minChannelVal, maxChannelVal);
        AppendDebugLog(L"%ls", debugMsg);
        
        int minY = INT_MAX, maxY = INT_MIN;
        int clampedToMin = 0;
        int clampedToMax = 0;
        for (size_t j = startIdx; j < endIdx && j < channel.size(); j++) {
            // X координата по температуре
            float temp = xAxisData[j];
            int x = plotRect.left + (int)((temp - minTemp) / (maxTemp - minTemp) * (plotRect.right - plotRect.left));
            
            // Y координата с учетом zoom
            float v = channel[j];
            float clampedValue = (std::max)(leftMin, (std::min)(leftMax, v));
            if (clampedValue == leftMin && v < leftMin) clampedToMin++;
            if (clampedValue == leftMax && v > leftMax) clampedToMax++;
            int y = dataRect.bottom - (int)((clampedValue - leftMin) / (leftMax - leftMin) * (dataRect.bottom - dataRect.top));
            
            // Отслеживаем min/max Y координаты
            minY = (std::min)(minY, y);
            maxY = (std::max)(maxY, y);

            points.push_back({ x, y });
        }
        
        // Отладка: выводим диапазон Y координат
        swprintf_s(debugMsg, L"  Screen Y range: minY=%d, maxY=%d (plotRect.top=%d, plotRect.bottom=%d)\n",
                  minY, maxY, plotRect.top, plotRect.bottom);
        AppendDebugLog(L"%ls", debugMsg);

        AppendDebugLog(L"  Clamp stats: toMin=%d toMax=%d (points=%zu)\n", clampedToMin, clampedToMax, (size_t)(endIdx - startIdx));
        
        DrawCurvePolyline(hdc, points, curve.color, PS_SOLID);
        leftIndex++;
    }
    
    // Отрисовка кривых правой оси (пунктирные линии)
    size_t rightIndex = 0;
    for (size_t i = 0; i < curves.size() && rightIndex < rightAxisData.size(); i++) {
        const auto& curve = curves[i];
        if (!curve.enabled || curve.yAxisGroup != 1) continue;
        
        const auto& channel = rightAxisData[rightIndex];
        if (channel.empty()) {
            rightIndex++;
            continue;
        }

        std::vector<POINT> points;
        points.reserve((endIdx > startIdx) ? (endIdx - startIdx) : 0);
        for (size_t j = startIdx; j < endIdx && j < channel.size(); j++) {
            // X координата по температуре
            float temp = xAxisData[j];
            int x = plotRect.left + (int)((temp - minTemp) / (maxTemp - minTemp) * (plotRect.right - plotRect.left));
            
            // Y координата с учетом zoom
            float clampedValue = (std::max)(rightMin, (std::min)(rightMax, channel[j]));
            int y = dataRect.bottom - (int)((clampedValue - rightMin) / (rightMax - rightMin) * (dataRect.bottom - dataRect.top));

            points.push_back({ x, y });
        }

        DrawCurvePolyline(hdc, points, curve.color, curve.lineStyle);
        rightIndex++;
    }
    
    // Восстанавливаем предыдущий clipping region
    if (regionType == 1) {
        SelectClipRgn(hdc, hOldClipRegion);
    } else {
        SelectClipRgn(hdc, NULL);
    }
    DeleteObject(hClipRegion);
    DeleteObject(hOldClipRegion);
}
