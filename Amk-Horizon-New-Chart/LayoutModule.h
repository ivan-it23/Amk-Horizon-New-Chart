#pragma once

#include "framework.h"

// Класс для управления компоновкой интерфейса
class LayoutModule {
private:
    // Константы компоновки
    enum {
        PANEL_HEIGHT = 40,
        MARGIN = 5,
        PANEL_SPACING = 5,
        MIN_CHART_HEIGHT = 100,
        MENU_AND_PANEL1_HEIGHT = 60,
        STATUSBAR_HEIGHT = 25  // Высота статусной строки
    };
    
    // Области графиков
    RECT chart1Rect;
    RECT chart2Rect;

public:
    LayoutModule();
    ~LayoutModule();

    // Основные методы компоновки
    void CalculateLayout(int windowWidth, int windowHeight, bool dualMode);
    bool ValidateLayoutConstraints(int width, int height, bool dualMode) const;

    // Получение областей графиков
    const RECT& GetChart1Rect() const { return chart1Rect; }
    const RECT& GetChart2Rect() const { return chart2Rect; }
    
    // Расчет позиций элементов управления
    int GetChart2PanelY() const;

private:
    // Вспомогательные методы
    void CalculateSingleChartLayout(int windowWidth, int windowHeight);
    void CalculateDualChartLayout(int windowWidth, int windowHeight);
};