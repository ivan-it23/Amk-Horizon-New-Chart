#include "framework.h"
#include "DataModule.h"
#include <fstream>
#include <algorithm>

DataModule::DataModule() {
}

DataModule::~DataModule() {
}

bool DataModule::LoadDataFromFile(const std::wstring& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
        return false;
        
    data.clear();
    
    GP_TOOL_DATA record;
    while (file.read(reinterpret_cast<char*>(&record), sizeof(GP_TOOL_DATA))) {
        data.push_back(record);
    }
    
    file.close();
    
    return !data.empty();
}

void DataModule::ClearData() {
    data.clear();
}

bool DataModule::HasData() const {
    return !data.empty();
}

size_t DataModule::GetDataSize() const {
    return data.size();
}

const std::vector<GP_TOOL_DATA>& DataModule::GetData() const {
    return data;
}

void DataModule::CalculateChartData(DataType type, FrequencyBand freq, std::vector<std::vector<float>>& outData) const {
    // Вызываем перегруженную версию без ограничений диапазона
    std::vector<bool> allTransmittersEnabled = {true, true, true, true}; // По умолчанию все передатчики включены
    CalculateChartData(type, freq, outData, allTransmittersEnabled, 0.0, static_cast<double>(data.size() - 1), -1e6, 1e6);
}

void DataModule::CalculateChartData(DataType type, FrequencyBand freq, std::vector<std::vector<float>>& outData,
                                   double minX, double maxX, double minY, double maxY) const {
    std::vector<bool> allTransmittersEnabled = {true, true, true, true}; // По умолчанию все передатчики включены
    CalculateChartData(type, freq, outData, allTransmittersEnabled, minX, maxX, minY, maxY);
}

void DataModule::CalculateChartData(DataType type, FrequencyBand freq, std::vector<std::vector<float>>& outData,
                                   const std::vector<bool>& enabledTransmitters,
                                   double minX, double maxX, double minY, double maxY) const {
    outData.clear();
    
    if (data.empty())
        return;
    
    // Определяем диапазон индексов для отображения
    size_t startIndex = static_cast<size_t>(std::max(0.0, minX));
    size_t endIndex = static_cast<size_t>(std::min(static_cast<double>(data.size()), maxX + 1));
    
    if (startIndex >= endIndex || startIndex >= data.size()) 
        return;
    
    switch (type) {
    case DataType::PHASE_DIFFERENCE:
    case DataType::AMPLITUDE_RATIO:
    case DataType::AMPLITUDE_RX1:
    case DataType::AMPLITUDE_RX2:
    case DataType::PHASE_RX1:
    case DataType::PHASE_RX2:
        // 4 канала для активных передатчиков
        outData.resize(4); // Четыре канала T1-T4
        for (size_t i = startIndex; i < endIndex; ++i) {
            const GP_TOOL_DATA& record = data[i];
            // Общие индексы для 400кГц: T1(2), T2(3), T3(4), T4(5)
            // Общие индексы для 2000кГц: T1(6), T2(7), T3(8), T4(9)
            
            for (int t = 0; t < 4; ++t) {
                if (!enabledTransmitters[t]) continue; // Пропускаем, если передатчик отключен
                int rxIdx = GetActiveTransmitterIndex(static_cast<Transmitter>(t), freq);
                if (rxIdx == -1) continue;

                switch (type) {
                    case DataType::PHASE_DIFFERENCE:
                        outData[t].push_back(record.PH_RX_2[rxIdx] - record.PH_RX_1[rxIdx]);
                        break;
                    case DataType::AMPLITUDE_RATIO:
                        outData[t].push_back(record.AM_RX_2[rxIdx] / record.AM_RX_1[rxIdx]);
                        break;
                    case DataType::AMPLITUDE_RX1:
                        outData[t].push_back(record.AM_RX_1[rxIdx]);
                        break;
                    case DataType::AMPLITUDE_RX2:
                        outData[t].push_back(record.AM_RX_2[rxIdx]);
                        break;
                    case DataType::PHASE_RX1:
                        outData[t].push_back(record.PH_RX_1[rxIdx]);
                        break;
                    case DataType::PHASE_RX2:
                        outData[t].push_back(record.PH_RX_2[rxIdx]);
                        break;
                    default: // Should not happen
                        break;
                }
            }
        }
        break;
    
    case DataType::SILENT_AMPLITUDE_RX1:
    case DataType::SILENT_AMPLITUDE_RX2:
        // 1 канал для молчащих передатчиков (без возможности выбора)
        outData.resize(1);
        for (size_t i = startIndex; i < endIndex; ++i) {
            const GP_TOOL_DATA& record = data[i];
            int rxIdx = GetSilentTransmitterIndex(freq);
            if (rxIdx == -1) continue;

            if (type == DataType::SILENT_AMPLITUDE_RX1) {
                outData[0].push_back(record.AM_RX_1[rxIdx]);
            } else {
                outData[0].push_back(record.AM_RX_2[rxIdx]);
            }
        }
        break;

    case DataType::TEMPERATURE:
        // 1 канал для температуры (без возможности выбора)
        outData.resize(1);
        for (size_t i = startIndex; i < endIndex; ++i) {
            const GP_TOOL_DATA& record = data[i];
            outData[0].push_back(record.temperature);
        }
        break;
    }
}

std::wstring DataModule::GetDataTypeName(DataType type) const {
    switch (type) {
    case DataType::PHASE_DIFFERENCE: return L"Phase Difference";
    case DataType::AMPLITUDE_RATIO: return L"Amplitude Ratio";
    case DataType::AMPLITUDE_RX1: return L"Amplitude RX1";
    case DataType::AMPLITUDE_RX2: return L"Amplitude RX2";
    case DataType::PHASE_RX1: return L"Phase RX1";
    case DataType::PHASE_RX2: return L"Phase RX2";
    case DataType::SILENT_AMPLITUDE_RX1: return L"Silent Amplitude RX1";
    case DataType::SILENT_AMPLITUDE_RX2: return L"Silent Amplitude RX2";
    case DataType::TEMPERATURE: return L"Temperature";
    default: return L"Unknown Type";
    }
}

// Вспомогательные методы
int DataModule::GetActiveTransmitterIndex(Transmitter transmitter, FrequencyBand freq) const {
    // Индексы для активных передатчиков T1-T4
    if (freq == FrequencyBand::FREQ_400_KHZ) {
        switch (transmitter) {
            case Transmitter::T1: return 2;
            case Transmitter::T2: return 3;
            case Transmitter::T3: return 4;
            case Transmitter::T4: return 5;
            default: return -1; // Should not happen
        }
    } else { // FREQ_2000_KHZ
        switch (transmitter) {
            case Transmitter::T1: return 6;
            case Transmitter::T2: return 7;
            case Transmitter::T3: return 8;
            case Transmitter::T4: return 9;
            default: return -1; // Should not happen
        }
    }
}

int DataModule::GetSilentTransmitterIndex(FrequencyBand freq) const {
    // Индексы для молчащих передатчиков
    if (freq == FrequencyBand::FREQ_400_KHZ)
        return 0; // индекс 0
    else
        return 1; // индекс 1
}

