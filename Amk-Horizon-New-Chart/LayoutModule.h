#pragma once

#include "framework.h"

// Класс для управления компоновкой интерфейса
class LayoutModule {
private:
    // Константы компоновки (фиксированные размеры)
    enum {
        MENU_HEIGHT = 10,           // Высота главного меню (финальная оптимизация)
        PANEL_HEIGHT = 25,          // Высота панели управления
        STATUSBAR_HEIGHT = 25,      // Высота статусной строки
        MARGIN = 5,                 // Боковые отступы
        ELEMENT_SPACING = 2,        // Минимальный отступ между элементами
        MIN_CHART_HEIGHT = 100      // Минимальная высота графика
    };
    
    // Процентное распределение для графиков (в Dual Mode)
    static constexpr float CHART1_HEIGHT_PERCENT = 0.50f;  // 50% для первого графика
    static constexpr float CHART2_HEIGHT_PERCENT = 0.50f;  // 50% для второго графика
    // Графики занимают всё доступное пространство (100%)
    
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