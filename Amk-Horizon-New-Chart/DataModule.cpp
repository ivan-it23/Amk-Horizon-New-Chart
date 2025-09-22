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
    CalculateChartData(type, freq, outData, 0.0, static_cast<double>(data.size() - 1), -1e6, 1e6);
}

void DataModule::CalculateChartData(DataType type, FrequencyBand freq, std::vector<std::vector<float>>& outData, 
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
        outData.resize(4);
        for (int channel = 0; channel < 4; channel++) {
            outData[channel].reserve(endIndex - startIndex);
            for (size_t i = startIndex; i < endIndex; i++) {
                const auto& record = data[i];
                int index = GetActiveTransmitterIndex(channel, freq);
                float value = 0.0f;
                
                switch (type) {
                case DataType::PHASE_DIFFERENCE:
                    value = record.PH_RX_2[index] - record.PH_RX_1[index];
                    break;
                case DataType::AMPLITUDE_RATIO:
                    value = (record.AM_RX_1[index] != 0.0f) ? record.AM_RX_2[index] / record.AM_RX_1[index] : 0.0f;
                    break;
                case DataType::AMPLITUDE_RX1:
                    value = record.AM_RX_1[index];
                    break;
                case DataType::AMPLITUDE_RX2:
                    value = record.AM_RX_2[index];
                    break;
                case DataType::PHASE_RX1:
                    value = record.PH_RX_1[index];
                    break;
                case DataType::PHASE_RX2:
                    value = record.PH_RX_2[index];
                    break;
                }
                
                outData[channel].push_back(value);
            }
        }
        break;
        
    case DataType::SILENT_AMPLITUDE_RX1:
    case DataType::SILENT_AMPLITUDE_RX2:
        // 1 канал для молчащих передатчиков
        outData.resize(1);
        outData[0].reserve(endIndex - startIndex);
        for (size_t i = startIndex; i < endIndex; i++) {
            const auto& record = data[i];
            int index = GetSilentTransmitterIndex(freq);
            float value = (type == DataType::SILENT_AMPLITUDE_RX1) ? 
                record.AM_RX_1[index] : record.AM_RX_2[index];
            outData[0].push_back(value);
        }
        break;
        
    case DataType::TEMPERATURE:
        // 1 значение температуры
        outData.resize(1);
        outData[0].reserve(endIndex - startIndex);
        for (size_t i = startIndex; i < endIndex; i++) {
            const auto& record = data[i];
            outData[0].push_back(record.temperature);
        }
        break;
    }
}

std::wstring DataModule::GetDataTypeName(DataType type) const {
    switch (type) {
    case DataType::PHASE_DIFFERENCE: return L"Разность фаз";
    case DataType::AMPLITUDE_RATIO: return L"Отношение амплитуд";
    case DataType::AMPLITUDE_RX1: return L"Амплитуды RX1";
    case DataType::AMPLITUDE_RX2: return L"Амплитуды RX2";
    case DataType::PHASE_RX1: return L"Фазы RX1";
    case DataType::PHASE_RX2: return L"Фазы RX2";
    case DataType::SILENT_AMPLITUDE_RX1: return L"Молчащие амплитуды RX1";
    case DataType::SILENT_AMPLITUDE_RX2: return L"Молчащие амплитуды RX2";
    case DataType::TEMPERATURE: return L"Температура";
    default: return L"Неизвестный тип";
    }
}

int DataModule::GetActiveTransmitterIndex(int transmitter, FrequencyBand freq) const {
    // Индексы для активных передатчиков T1-T4
    if (freq == FrequencyBand::FREQ_400_KHZ)
        return 2 + transmitter; // индексы 2, 3, 4, 5
    else
        return 6 + transmitter; // индексы 6, 7, 8, 9
}

int DataModule::GetSilentTransmitterIndex(FrequencyBand freq) const {
    // Индексы для молчащих передатчиков
    return (freq == FrequencyBand::FREQ_400_KHZ) ? 0 : 1;
}

