# 👨‍💻 Руководство разработчика AMK-Horizon-New-Chart

## 🛠️ Настройка среды разработки

### Требования
- **Visual Studio 2019/2022** с поддержкой C++17
- **Windows SDK 10.0** или новее
- **Git** для контроля версий

### Структура проекта
```
Amk-Horizon-New-Chart/
├── Source Files/
│   ├── Amk-Horizon-New-Chart.cpp    # Точка входа
│   ├── ChartManager.cpp              # Главный координатор
│   ├── DataModule.cpp                # Модуль данных
│   ├── ChartModule.cpp               # Модуль отрисовки
│   ├── UIModule.cpp                  # Модуль интерфейса
│   ├── LayoutModule.cpp              # Модуль компоновки
│   ├── SchematicModule.cpp           # Модуль схемы прибора
│   └── CurveSelectionModule.cpp      # Модуль управления кривыми
│
├── Header Files/
│   └── *.h                           # Заголовочные файлы
│
└── Resource Files/
    └── Amk-Horizon-New-Chart.rc     # Ресурсы интерфейса
```

## 🏗️ Архитектурные принципы

### Модульность
- **Разделение ответственности** - каждый модуль отвечает за конкретную область
- **Слабая связанность** - модули взаимодействуют через четко определенные интерфейсы
- **Высокая сплоченность** - функции внутри модуля тесно связаны по назначению

### Расширяемость
- **Открыт для расширения** - легко добавлять новые типы данных и графиков
- **Закрыт для модификации** - существующий код не требует изменений

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
        case DataType::NEW_DATA_TYPE: 
            return L"New Data Type";
        // ...
    }
}

void DataModule::CalculateChartData(/* параметры */) {
    switch (type) {
        case DataType::NEW_DATA_TYPE:
            CalculateNewDataType(outData, enabledTransmitters);
            break;
        // ...
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
    
    void Initialize();
    void ProcessData();
    
private:
    bool initialized;
    void InternalProcess();
};
```

#### 2. Интегрируйте в ChartManager
```cpp
// В ChartManager.h
#include "NewModule.h"

class ChartManager {
private:
    std::unique_ptr<NewModule> newModule;
};

// В ChartManager.cpp
ChartManager::ChartManager() 
    : newModule(std::make_unique<NewModule>())
{
    newModule->Initialize();
}
```

## 🧪 Тестирование

### Пример теста для DataModule
```cpp
#include "gtest/gtest.h"
#include "../DataModule.h"

TEST(DataModuleTest, LoadValidFile) {
    DataModule dataModule;
    bool result = dataModule.LoadDataFromFile(L"test_data.bin");
    EXPECT_TRUE(result);
    EXPECT_TRUE(dataModule.HasData());
}

TEST(DataModuleTest, CalculatePhaseData) {
    DataModule dataModule;
    dataModule.LoadDataFromFile(L"test_data.bin");
    
    std::vector<std::vector<float>> chartData;
    std::vector<bool> enabledTransmitters = {true, true, true, true};
    
    dataModule.CalculateChartData(DataType::PHASE_DIFFERENCE, 
                                  FrequencyBand::FREQ_400_KHZ,
                                  chartData, enabledTransmitters,
                                  0.0, 100.0, -180.0, 180.0);
    
    EXPECT_EQ(chartData.size(), 4);
    EXPECT_FALSE(chartData[0].empty());
}
```

## 🐛 Отладка

### Добавление системы логирования
```cpp
// Logger.h
enum class LogLevel { DEBUG, INFO, WARNING, ERROR };

class Logger {
public:
    static Logger& Instance();
    void Log(LogLevel level, const std::string& message);
    
private:
    std::ofstream logFile;
};

// Макросы
#define LOG_INFO(msg) Logger::Instance().Log(LogLevel::INFO, msg)
#define LOG_ERROR(msg) Logger::Instance().Log(LogLevel::ERROR, msg)
```

### Измерение производительности
```cpp
// PerformanceTimer.h
class PerformanceTimer {
public:
    PerformanceTimer(const std::string& name);
    ~PerformanceTimer();
    
private:
    std::string timerName;
    std::chrono::high_resolution_clock::time_point startTime;
};

#define PERF_TIMER(name) PerformanceTimer timer(name)
```

## 🔄 Процесс разработки

### Git Workflow
```
main                    # Стабильная версия
├── develop            # Разработка
├── feature/new-chart  # Новые функции
├── bugfix/fix-crash   # Исправления ошибок
└── hotfix/urgent-fix  # Критические исправления
```

### Формат коммитов
```bash
git commit -m "feat: add new chart type"
git commit -m "fix: resolve crash when loading empty files"
git commit -m "docs: update API documentation"
git commit -m "refactor: extract common rendering logic"
```

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
```

### Комментарии
```cpp
/**
 * @brief Вычисляет данные для отображения на графике
 * @param type Тип данных для вычисления
 * @param freq Частотный диапазон
 * @param outData Выходной массив данных
 * @return true если вычисление успешно
 */
bool CalculateChartData(DataType type, FrequencyBand freq, 
                       std::vector<std::vector<float>>& outData);
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
    HPEN m_pen, m_oldPen;
};
```

## 🚀 Развертывание

### Сборка релиза
1. Переключитесь на конфигурацию **Release x64**
2. Очистите решение (Build → Clean Solution)
3. Пересоберите решение (Build → Rebuild Solution)
4. Проверьте отсутствие предупреждений

---

**Удачной разработки!** 🚀
