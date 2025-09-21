#include "framework.h"
#include "LayoutModule.h"

LayoutModule::LayoutModule() {
    ZeroMemory(&chart1Rect, sizeof(RECT));
    ZeroMemory(&chart2Rect, sizeof(RECT));
}

LayoutModule::~LayoutModule() {
}

void LayoutModule::CalculateLayout(int windowWidth, int windowHeight, bool dualMode) {
    if (dualMode) {
        CalculateDualChartLayout(windowWidth, windowHeight);
    } else {
        CalculateSingleChartLayout(windowWidth, windowHeight);
    }
}

bool LayoutModule::ValidateLayoutConstraints(int width, int height, bool dualMode) const {
    // Минимальные размеры окна для корректной работы
    const int MIN_WINDOW_WIDTH = 600;
    const int MIN_WINDOW_HEIGHT = 400;
    
    if (width < MIN_WINDOW_WIDTH || height < MIN_WINDOW_HEIGHT)
        return false;
    
    // В режиме двух графиков нужно больше места
    if (dualMode) {
        const int MIN_DUAL_HEIGHT = 600;
        if (height < MIN_DUAL_HEIGHT)
            return false;
    }
    
    return true;
}

int LayoutModule::GetChart2PanelY() const {
    return chart1Rect.bottom + MARGIN;
}

void LayoutModule::CalculateSingleChartLayout(int windowWidth, int windowHeight) {
    // Режим одного графика - занимает всё доступное пространство
    chart1Rect.left = MARGIN;
    chart1Rect.top = MENU_AND_PANEL1_HEIGHT;
    chart1Rect.right = windowWidth - MARGIN;
    chart1Rect.bottom = windowHeight - MARGIN;
    
    // Очищаем прямоугольник второго графика
    ZeroMemory(&chart2Rect, sizeof(RECT));
}

void LayoutModule::CalculateDualChartLayout(int windowWidth, int windowHeight) {
    // Расчет для режима двух графиков
    // Доступное пространство = Общая высота - Фиксированные элементы
    int totalFixedHeight = MENU_AND_PANEL1_HEIGHT + PANEL_HEIGHT + (4 * MARGIN) + PANEL_SPACING;
    int availableHeight = windowHeight - totalFixedHeight;
    
    // Каждый график получает равную долю доступного пространства
    int chartHeight = availableHeight / 2;
    
    // Убеждаемся, что высота графика не меньше минимальной
    if (chartHeight < MIN_CHART_HEIGHT)
        chartHeight = MIN_CHART_HEIGHT;
    
    // Первый график
    chart1Rect.left = MARGIN;
    chart1Rect.top = MENU_AND_PANEL1_HEIGHT;
    chart1Rect.right = windowWidth - MARGIN;
    chart1Rect.bottom = chart1Rect.top + chartHeight;
    
    // Второй график (симметрично первому)
    chart2Rect.left = MARGIN;
    chart2Rect.top = chart1Rect.bottom + PANEL_HEIGHT + (2 * MARGIN);
    chart2Rect.right = windowWidth - MARGIN;
    chart2Rect.bottom = chart2Rect.top + chartHeight;
}