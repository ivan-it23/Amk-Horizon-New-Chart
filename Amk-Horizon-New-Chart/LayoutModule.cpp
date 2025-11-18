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
    return chart1Rect.bottom + ELEMENT_SPACING;
}

void LayoutModule::CalculateSingleChartLayout(int windowWidth, int windowHeight) {
    // Режим одного графика - занимает максимум доступного пространства
    
    // Фиксированные элементы сверху: меню + панель управления + отступ
    int topOffset = MENU_HEIGHT + PANEL_HEIGHT + ELEMENT_SPACING;
    
    // Фиксированные элементы снизу: статусная строка + отступ
    int bottomOffset = STATUSBAR_HEIGHT + ELEMENT_SPACING;
    
    // График занимает всё оставшееся пространство
    chart1Rect.left = MARGIN;
    chart1Rect.top = topOffset;
    chart1Rect.right = windowWidth - MARGIN;
    chart1Rect.bottom = windowHeight - bottomOffset;
    
    // Очищаем прямоугольник второго графика
    ZeroMemory(&chart2Rect, sizeof(RECT));
}

void LayoutModule::CalculateDualChartLayout(int windowWidth, int windowHeight) {
    // Расчет для режима двух графиков с процентным распределением
    
    // Фиксированные элементы:
    // - Меню сверху
    // - Панель управления Chart 1
    // - Панель управления Chart 2
    // - Статусная строка снизу
    // - Отступы между элементами
    int totalFixedHeight = MENU_HEIGHT + (2 * PANEL_HEIGHT) + STATUSBAR_HEIGHT + (4 * ELEMENT_SPACING);
    
    // Доступная высота для графиков
    int availableHeight = windowHeight - totalFixedHeight;
    
    // Высота каждого графика по процентам от доступной высоты
    int chart1Height = static_cast<int>(availableHeight * CHART1_HEIGHT_PERCENT);
    int chart2Height = static_cast<int>(availableHeight * CHART2_HEIGHT_PERCENT);
    
    // Убеждаемся, что высота графиков не меньше минимальной
    if (chart1Height < MIN_CHART_HEIGHT) chart1Height = MIN_CHART_HEIGHT;
    if (chart2Height < MIN_CHART_HEIGHT) chart2Height = MIN_CHART_HEIGHT;
    
    // Первый график
    int currentY = MENU_HEIGHT + PANEL_HEIGHT + ELEMENT_SPACING;
    chart1Rect.left = MARGIN;
    chart1Rect.top = currentY;
    chart1Rect.right = windowWidth - MARGIN;
    chart1Rect.bottom = currentY + chart1Height;
    
    // Второй график
    currentY = chart1Rect.bottom + ELEMENT_SPACING + PANEL_HEIGHT + ELEMENT_SPACING;
    chart2Rect.left = MARGIN;
    chart2Rect.top = currentY;
    chart2Rect.right = windowWidth - MARGIN;
    chart2Rect.bottom = currentY + chart2Height;
}