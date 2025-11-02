# 👨‍💻 Руководство разработчика AMK-Horizon-New-Chart v2.2.0

## 🎯 Введение

Данное руководство предназначено для разработчиков, которые хотят понять, модифицировать или расширить функциональность AMK-Horizon-New-Chart версии 2.2.0. Здесь описаны принципы архитектуры, процессы разработки и рекомендации по добавлению новых функций.

## 🆕 Новые возможности v2.2.0

### Независимые X Range Controls
- **12 новых элементов управления**: 6 для Chart 1 + 6 для Chart 2
- **Независимые ID**: IDC_XRANGE_* для Chart 1, IDC_XRANGE2_* для Chart 2
- **Динамическое управление видимостью**: Элементы Chart 2 показываются только в Dual Mode
- **Методы UIModule**: UpdateXRangeControls(), GetXRangeValues(), SetXRangeValues(), ResetXRangeControls()
- **Методы ChartManager**: HandleXRangeApply(), HandleXRangeReset(), UpdateXRangeUI()

### Улучшенный Zoom Selection
- **Диагональная штриховка**: Паттерн HS_DIAGCROSS для области выделения
- **Оранжевый цвет**: RGB(255, 165, 0) для лучшей видимости
- **Обновлённый ChartModule::DrawZoomRectangle()**: Новая логика отрисовки выделения

## 🏗️ Архитектурные принципы

### Модульность
- **Разделение ответственности** - каждый модуль отвечает за конкретную область
- **Слабая связанность** - модули взаимодействуют через четко определенные интерфейсы
- **Высокая сплоченность** - функции внутри модуля тесно связаны по назначению

### Расширяемость
- **Открыт для расширения** - легко добавлять новые типы данных и графиков
- **Закрыт для модификации** - существующий код не требует изменений
- **Полиморфизм** - использование виртуальных методов где это уместно

### Совместимость
- **Обратная совместимость** - новые версии работают со старыми данными
- **API стабильность** - публичные интерфейсы остаются неизменными
- **Миграция данных** - автоматическое обновление форматов при необходимости

## 🛠️ Настройка среды разработки

### Требования
- **Visual Studio 2019/2022** с поддержкой C++17
- **Windows SDK 10.0** или новее
- **Git** для контроля версий

### Настройка проекта
1. Клонируйте репозиторий
2. Откройте `Amk-Horizon-New-Chart.vcxproj` в Visual Studio
3. Убедитесь, что выбрана конфигурация **Debug x64**
4. Соберите проект (Build → Build Solution)

### Структура решения
```
Amk-Horizon-New-Chart/
├── 📁 Source Files/
│   ├── Amk-Horizon-New-Chart.cpp    # Точка входа
│   ├── ChartManager.cpp              # Главный координатор
│   ├── DataModule.cpp                # Модуль данных
│   ├── ChartModule.cpp               # Модуль отрисовки
│   ├── UIModule.cpp                  # Модуль интерфейса
│   ├── LayoutModule.cpp              # Модуль компоновки
│   ├── SchematicModule.cpp           # Модуль схемы прибора
│   └── CurveSelectionModule.cpp      # Модуль управления кривыми
│
├── 📁 Header Files/
│   ├── *.h                           # Заголовочные файлы модулей
│   ├── framework.h                   # Системные заголовки
│   └── Resource.h                    # Определения ресурсов
│
└── 📁 Resource Files/
    ├── Amk-Horizon-New-Chart.rc     # Ресурсы интерфейса
    └── *.ico                         # Иконки
```

## 🔧 Добавление новых функций

### Добавление нового типа данных

#### 1. Обновите перечисление DataType
```cpp
// В DataModule.h
enum class DataType {
    // ... существующие типы
    NEW_DATA_TYPE = 9    // Новый тип данных
};
```

#### 2. Добавьте обработку в DataModule
```cpp
// В DataModule.cpp
std::wstring DataModule::GetDataTypeName(DataType type) const {
    switch (type) {
        // ... существующие случаи
        case DataType::NEW_DATA_TYPE: 
            return L"New Data Type";
        default: 
            return L"Unknown Type";
    }
}

void DataModule::CalculateChartData(/* параметры */) {
    switch (type) {
        // ... существующие случаи
        case DataType::NEW_DATA_TYPE:
            // Логика вычисления нового типа данных
            CalculateNewDataType(outData, enabledTransmitters, /* другие параметры */);
            break;
    }
}
```

#### 3. Обновите UIModule
```cpp
// В UIModule.cpp
void UIModule::PopulateComboBoxes() {
    const wchar_t* dataTypes[] = {
        // ... существующие типы
        L"New Data Type"
    };
    
    // Увеличьте размер массива
    for (int i = 0; i < 10; i++) { // было 9, стало 10
        SendMessage(hChart1DataCombo, CB_ADDSTRING, 0, (LPARAM)dataTypes[i]);
        SendMessage(hChart2DataCombo, CB_ADDSTRING, 0, (LPARAM)dataTypes[i]);
    }
}
```

### Добавление нового модуля

#### 1. Создайте заголовочный файл
```cpp
// NewModule.h
#pragma once
#include "framework.h"

class NewModule {
public:
    NewModule();
    ~NewModule();
    
    // Публичные методы
    void Initialize();
    void ProcessData();
    
private:
    // Приватные члены
    bool initialized;
    
    // Приватные методы
    void InternalProcess();
};
```

#### 2. Создайте файл реализации
```cpp
// NewModule.cpp
#include "framework.h"
#include "NewModule.h"

NewModule::NewModule() : initialized(false) {
}

NewModule::~NewModule() {
}

void NewModule::Initialize() {
    // Логика инициализации
    initialized = true;
}

void NewModule::ProcessData() {
    if (!initialized) {
        Initialize();
    }
    
    // Основная логика модуля
    InternalProcess();
}

void NewModule::InternalProcess() {
    // Внутренняя логика
}
```

#### 3. Интегрируйте в ChartManager
```cpp
// В ChartManager.h
#include "NewModule.h"

class ChartManager {
private:
    std::unique_ptr<NewModule> newModule;
    // ... другие модули
};

// В ChartManager.cpp
ChartManager::ChartManager() 
    : newModule(std::make_unique<NewModule>())
    // ... другие модули
{
    // Инициализация связей между модулями
    newModule->Initialize();
}
```

### Добавление нового элемента интерфейса

#### 1. Добавьте ресурс в .rc файл
```rc
// В Amk-Horizon-New-Chart.rc
PUSHBUTTON "New Button", IDC_NEW_BUTTON, 10, 10, 80, 25
```

#### 2. Определите константу в Resource.h
```cpp
// В Resource.h
#define IDC_NEW_BUTTON    2001
```

#### 3. Создайте элемент в UIModule
```cpp
// В UIModule.h
class UIModule {
private:
    HWND hNewButton;
    // ... другие элементы
};

// В UIModule.cpp
void UIModule::CreateControls(HWND hWnd, HINSTANCE hInst) {
    // ... создание других элементов
    
    hNewButton = CreateWindow(L"BUTTON", L"New Button",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        10, 10, 80, 25, hWnd, (HMENU)IDC_NEW_BUTTON, hInst, NULL);
}

bool UIModule::HandleCommand(WPARAM wParam, LPARAM lParam, HWND hWnd) {
    int wmId = LOWORD(wParam);
    
    switch (wmId) {
        case IDC_NEW_BUTTON:
            // Обработка нажатия новой кнопки
            HandleNewButtonClick();
            return true;
        // ... другие случаи
    }
    
    return false;
}
```

## 🧪 Тестирование

### Модульное тестирование

#### Создание тестового проекта
1. Добавьте новый проект **Test** в решение
2. Настройте зависимости от основного проекта
3. Используйте фреймворк тестирования (например, Google Test)

#### Пример теста для DataModule
```cpp
// DataModuleTest.cpp
#include "gtest/gtest.h"
#include "../DataModule.h"

class DataModuleTest : public ::testing::Test {
protected:
    void SetUp() override {
        dataModule = std::make_unique<DataModule>();
    }
    
    std::unique_ptr<DataModule> dataModule;
};

TEST_F(DataModuleTest, LoadValidFile) {
    // Тест загрузки корректного файла
    bool result = dataModule->LoadDataFromFile(L"test_data.bin");
    EXPECT_TRUE(result);
    EXPECT_TRUE(dataModule->HasData());
    EXPECT_GT(dataModule->GetDataSize(), 0);
}

TEST_F(DataModuleTest, CalculatePhaseData) {
    // Тест вычисления фазовых данных
    dataModule->LoadDataFromFile(L"test_data.bin");
    
    std::vector<std::vector<float>> chartData;
    std::vector<bool> enabledTransmitters = {true, true, true, true};
    
    dataModule->CalculateChartData(DataType::PHASE_DIFFERENCE, 
                                  FrequencyBand::FREQ_400_KHZ,
                                  chartData, enabledTransmitters,
                                  0.0, 100.0, -180.0, 180.0);
    
    EXPECT_EQ(chartData.size(), 4); // 4 канала T1-T4
    EXPECT_FALSE(chartData[0].empty()); // Данные присутствуют
}
```

### Интеграционное тестирование

#### Тестирование взаимодействия модулей
```cpp
// IntegrationTest.cpp
TEST(IntegrationTest, ChartManagerModuleInteraction) {
    ChartManager manager;
    
    // Тест загрузки данных через ChartManager
    manager.LoadDataFromFile(L"test_data.bin");
    
    // Тест отрисовки графика
    HDC testDC = CreateCompatibleDC(NULL);
    RECT testRect = {0, 0, 800, 600};
    
    // Не должно вызывать исключений
    EXPECT_NO_THROW(manager.DrawChart1(testDC));
    
    DeleteDC(testDC);
}
```

### Тестирование пользовательского интерфейса

#### Автоматизированное тестирование UI
```cpp
// UITest.cpp
TEST(UITest, ButtonClickHandling) {
    UIModule uiModule;
    HWND testWindow = CreateTestWindow();
    
    uiModule.CreateControls(testWindow, GetModuleHandle(NULL));
    
    // Симуляция нажатия кнопки
    WPARAM wParam = MAKEWPARAM(IDC_CHART1_REFRESH_BTN, BN_CLICKED);
    LPARAM lParam = (LPARAM)uiModule.GetRefreshButton();
    
    bool handled = uiModule.HandleCommand(wParam, lParam, testWindow);
    EXPECT_TRUE(handled);
    
    DestroyWindow(testWindow);
}
```

## 🐛 Отладка

### Настройка отладки в Visual Studio

#### Точки останова
- Устанавливайте точки останова в ключевых местах
- Используйте условные точки останова для специфических случаев
- Применяйте точки трассировки для логирования без остановки

#### Окна отладки
- **Watch** - для отслеживания переменных
- **Call Stack** - для анализа стека вызовов
- **Memory** - для просмотра содержимого памяти
- **Disassembly** - для анализа ассемблерного кода

### Логирование

#### Добавление системы логирования
```cpp
// Logger.h
#pragma once
#include <string>
#include <fstream>

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR
};

class Logger {
public:
    static Logger& Instance();
    void Log(LogLevel level, const std::string& message);
    void SetLogFile(const std::string& filename);
    
private:
    Logger() = default;
    std::ofstream logFile;
    LogLevel minLevel = LogLevel::INFO;
};

// Макросы для удобства
#define LOG_DEBUG(msg) Logger::Instance().Log(LogLevel::DEBUG, msg)
#define LOG_INFO(msg) Logger::Instance().Log(LogLevel::INFO, msg)
#define LOG_WARNING(msg) Logger::Instance().Log(LogLevel::WARNING, msg)
#define LOG_ERROR(msg) Logger::Instance().Log(LogLevel::ERROR, msg)
```

#### Использование логирования
```cpp
// В коде модулей
void DataModule::LoadDataFromFile(const std::wstring& filename) {
    LOG_INFO("Loading data from file: " + WStringToString(filename));
    
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file: " + WStringToString(filename));
        return false;
    }
    
    // ... логика загрузки
    
    LOG_INFO("Successfully loaded " + std::to_string(data.size()) + " records");
    return true;
}
```

## 📊 Профилирование производительности

### Измерение времени выполнения
```cpp
// PerformanceTimer.h
#pragma once
#include <chrono>
#include <string>

class PerformanceTimer {
public:
    PerformanceTimer(const std::string& name);
    ~PerformanceTimer();
    
private:
    std::string timerName;
    std::chrono::high_resolution_clock::time_point startTime;
};

// Макрос для удобства
#define PERF_TIMER(name) PerformanceTimer timer(name)
```

### Использование профилировщика
```cpp
void DataModule::CalculateChartData(/* параметры */) {
    PERF_TIMER("DataModule::CalculateChartData");
    
    // Основная логика вычислений
    for (size_t i = startIndex; i < endIndex; ++i) {
        // ... вычисления
    }
}
```

## 🔄 Процесс разработки

### Git Workflow

#### Структура веток
```
main                    # Стабильная версия
├── develop            # Разработка
├── feature/new-chart  # Новые функции
├── bugfix/fix-crash   # Исправления ошибок
└── hotfix/urgent-fix  # Критические исправления
```

#### Коммиты
```bash
# Формат коммитов
git commit -m "feat: add new chart type for temperature data"
git commit -m "fix: resolve crash when loading empty files"
git commit -m "docs: update API documentation"
git commit -m "refactor: extract common chart rendering logic"
```

### Code Review

#### Чек-лист для ревью
- [ ] Код соответствует стилю проекта
- [ ] Добавлены необходимые тесты
- [ ] Обновлена документация
- [ ] Нет утечек памяти
- [ ] Обработаны все возможные ошибки
- [ ] Производительность не ухудшилась

#### Инструменты статического анализа
- **PVS-Studio** - для поиска потенциальных ошибок
- **Clang Static Analyzer** - анализ кода
- **CppCheck** - проверка стиля и ошибок

## 📋 Стандарты кодирования

### Именование
```cpp
// Классы - PascalCase
class DataModule { };

// Методы - PascalCase
void CalculateChartData();

// Переменные - camelCase
int chartNumber;
bool isEnabled;

// Константы - UPPER_CASE
const int MAX_CHANNELS = 10;

// Приватные члены - camelCase с префиксом
private:
    int m_dataSize;
    bool m_isInitialized;
```

### Комментарии
```cpp
/**
 * @brief Вычисляет данные для отображения на графике
 * @param type Тип данных для вычисления
 * @param freq Частотный диапазон
 * @param outData Выходной массив данных
 * @param enabledTransmitters Маска включенных передатчиков
 * @return true если вычисление успешно
 */
bool CalculateChartData(DataType type, FrequencyBand freq, 
                       std::vector<std::vector<float>>& outData,
                       const std::vector<bool>& enabledTransmitters);
```

### Обработка ошибок
```cpp
// Используйте RAII для управления ресурсами
class GDIResource {
public:
    GDIResource(HDC hdc) : m_hdc(hdc) {
        m_oldPen = (HPEN)SelectObject(m_hdc, m_pen);
    }
    
    ~GDIResource() {
        SelectObject(m_hdc, m_oldPen);
        DeleteObject(m_pen);
    }
    
private:
    HDC m_hdc;
    HPEN m_pen;
    HPEN m_oldPen;
};

// Проверяйте возвращаемые значения
HWND hwnd = CreateWindow(/* параметры */);
if (!hwnd) {
    LOG_ERROR("Failed to create window");
    return false;
}
```

## 🚀 Развертывание

### Сборка релиза
1. Переключитесь на конфигурацию **Release x64**
2. Очистите решение (Build → Clean Solution)
3. Пересоберите решение (Build → Rebuild Solution)
4. Проверьте отсутствие предупреждений

### Создание установщика
```cpp
// Используйте WiX Toolset или Inno Setup
// Пример скрипта Inno Setup
[Setup]
AppName=AMK-Horizon-New-Chart
AppVersion=1.0
DefaultDirName={pf}\AMK-Horizon-New-Chart
DefaultGroupName=AMK-Horizon-New-Chart

[Files]
Source: "Release\Amk-Horizon-New-Chart.exe"; DestDir: "{app}"
Source: "Release\*.dll"; DestDir: "{app}"

[Icons]
Name: "{group}\AMK-Horizon-New-Chart"; Filename: "{app}\Amk-Horizon-New-Chart.exe"
```

## 📚 Дополнительные ресурсы

### Документация
- [Windows API Reference](https://docs.microsoft.com/en-us/windows/win32/api/)
- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [Modern C++ Features](https://github.com/AnthonyCalandra/modern-cpp-features)

### Инструменты
- **Visual Studio Code** - для быстрого редактирования
- **Git Extensions** - GUI для Git
- **Dependency Walker** - анализ зависимостей DLL
- **Application Verifier** - поиск ошибок в runtime

---

**Удачной разработки!** 🚀 Следуйте принципам чистого кода и модульной архитектуры для создания качественного программного обеспечения.