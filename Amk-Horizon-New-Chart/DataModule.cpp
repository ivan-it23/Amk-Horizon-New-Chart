#include "framework.h"
#include "DataModule.h"
#include "SchematicModule.h"
#include "ToolDataAccess.h"
#include <fstream>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cwctype>  // v2.15.1: для towlower

namespace {
constexpr size_t kCanonicalFrameSize = sizeof(GP_TOOL_DATA);
constexpr size_t kDevHeaderSize = 11;

size_t GetPhysicalFrameSize(FileFormat format) {
    return (format == FileFormat::FORMAT_DEV) ? (kCanonicalFrameSize + kDevHeaderSize) : kCanonicalFrameSize;
}

ToolRecordMetadata BuildParsedFrameMetadata(const ParsedToolFrame& parsedFrame) {
    ToolRecordMetadata metadata = {};
    metadata.kind = parsedFrame.kind;
    metadata.capabilities = parsedFrame.capabilities;
    return metadata;
}

bool TryExtractCanonicalData(const ParsedToolFrame& parsedFrame, GP_TOOL_DATA& outData) {
    outData = parsedFrame.canonicalData;
    return true;
}

bool ReadParsedFrameAt(std::ifstream& file, FileFormat format, size_t frameIndex, ParsedToolFrame& outFrame) {
    outFrame = {};

    const std::streamoff frameOffset = static_cast<std::streamoff>(frameIndex * GetPhysicalFrameSize(format));
    file.clear();
    file.seekg(frameOffset, std::ios::beg);
    if (!file.good()) {
        return false;
    }

    if (format == FileFormat::FORMAT_DEV) {
        file.seekg(static_cast<std::streamoff>(kDevHeaderSize), std::ios::cur);
        if (!file.good()) {
            return false;
        }
    }

    char rawFrameBuffer[sizeof(GP_TOOL_DATA)] = {};
    return file.read(rawFrameBuffer, sizeof(rawFrameBuffer)) &&
           DataModule::ParseRawFrameDetailed(rawFrameBuffer, sizeof(rawFrameBuffer), outFrame);
}

void AppendCanonicalRecord(const GP_TOOL_DATA& record,
                           const ToolRecordMetadata& metadata,
                           std::vector<GP_TOOL_DATA>& data,
                           std::vector<ToolRecordMetadata>& recordMetadata) {
    data.push_back(record);
    recordMetadata.push_back(metadata);
}

void AppendCanonicalRecord(const GP_TOOL_DATA& record,
                           std::vector<GP_TOOL_DATA>& data,
                           std::vector<ToolRecordMetadata>& recordMetadata) {
    ToolRecordMetadata metadata = {};
    const IToolDataAdapter* adapter = ToolDataAdapterRegistry::Get().FindAdapterBySignature(record.signature);
    if (adapter) {
        metadata.kind = adapter->GetKind();
        metadata.capabilities = adapter->GetCapabilities();
    }

    AppendCanonicalRecord(record, metadata, data, recordMetadata);
}

void AppendParsedFrame(const ParsedToolFrame& parsedFrame,
                       std::vector<GP_TOOL_DATA>& data,
                       std::vector<ToolRecordMetadata>& recordMetadata) {
    GP_TOOL_DATA record = {};
    if (!TryExtractCanonicalData(parsedFrame, record)) {
        return;
    }

    AppendCanonicalRecord(record, BuildParsedFrameMetadata(parsedFrame), data, recordMetadata);
}

bool HasSameRecordMetadata(const ToolRecordMetadata& left, const ToolRecordMetadata& right) {
    return std::memcmp(&left, &right, sizeof(ToolRecordMetadata)) == 0;
}

bool IsSupportedDataFileFormat(FileFormat format) {
    return format == FileFormat::FORMAT_BIN ||
           format == FileFormat::FORMAT_BIV ||
           format == FileFormat::FORMAT_DEV;
}

void LoadParsedFrames(std::ifstream& file,
                      FileFormat format,
                      size_t frameCount,
                      std::vector<GP_TOOL_DATA>& data,
                      std::vector<ToolRecordMetadata>& recordMetadata) {
    data.reserve(frameCount);
    recordMetadata.reserve(frameCount);

    for (size_t n = 0; n < frameCount; ++n) {
        ParsedToolFrame parsedFrame = {};
        if (ReadParsedFrameAt(file, format, n, parsedFrame)) {
            AppendParsedFrame(parsedFrame, data, recordMetadata);
        }
    }
}

bool WriteCanonicalDataRange(std::ofstream& file,
                             const DataModule& dataModule,
                             size_t startIndex,
                             size_t endIndex) {
    for (size_t i = startIndex; i <= endIndex; ++i) {
        const GP_TOOL_DATA* record = dataModule.TryGetRecord(i);
        if (!record) {
            return false;
        }

        file.write(reinterpret_cast<const char*>(record), sizeof(GP_TOOL_DATA));
        if (!file) {
            return false;
        }
    }

    return true;
}

struct DataIndexWindow {
    size_t startIndex = 0;
    size_t endIndex = 0;
};

struct ActiveCalculationContext {
    DataIndexWindow window = {};
    int frequencyIndex = -1;
    int transmitterIndex = -1;
};

struct SilentCalculationContext {
    DataIndexWindow window = {};
    int frequencyIndex = -1;
};

DataIndexWindow BuildDataIndexWindow(const DataModule& dataModule, double minX, double maxX) {
    if (!dataModule.HasData()) {
        return {};
    }

    const size_t dataSize = dataModule.GetDataSize();
    size_t startIndex = static_cast<size_t>(std::max(0.0, minX));
    size_t endIndex = static_cast<size_t>(std::min(static_cast<double>(dataSize), maxX + 1));

    startIndex = std::min(startIndex, dataSize);
    endIndex = std::min(endIndex, dataSize);

    if (startIndex >= endIndex) {
        return {};
    }

    return { startIndex, endIndex };
}

bool TryBuildActiveCalculationContext(const DataModule& dataModule,
                                      int transmitterIndex,
                                      FrequencyBand frequency,
                                      double minX,
                                      double maxX,
                                      ActiveCalculationContext& outContext) {
    outContext = {};
    outContext.window = BuildDataIndexWindow(dataModule, minX, maxX);
    outContext.frequencyIndex = (frequency == FrequencyBand::FREQ_400_KHZ) ? 0 : 1;
    outContext.transmitterIndex = dataModule.GetActiveTransmitterIndex(static_cast<Transmitter>(transmitterIndex), frequency);
    return outContext.transmitterIndex != -1;
}

bool TryBuildSilentCalculationContext(const DataModule& dataModule,
                                      FrequencyBand frequency,
                                      double minX,
                                      double maxX,
                                      SilentCalculationContext& outContext) {
    outContext = {};
    outContext.window = BuildDataIndexWindow(dataModule, minX, maxX);
    outContext.frequencyIndex = dataModule.GetSilentTransmitterIndex(frequency);
    return outContext.frequencyIndex != -1;
}

void ReserveForWindow(std::vector<float>& outData, const DataIndexWindow& window) {
    outData.reserve(window.endIndex - window.startIndex);
}
}

bool DataModule::ReadFileSignature(std::ifstream& file, FileFormat format, uint32_t& outSignature) {
    const std::streamoff signatureOffset = (format == FileFormat::FORMAT_DEV) ? 11 : 0;

    file.clear();
    file.seekg(signatureOffset, std::ios::beg);
    if (!file.good()) {
        return false;
    }

    file.read(reinterpret_cast<char*>(&outSignature), sizeof(outSignature));
    return static_cast<bool>(file);
}

uint32_t DataModule::DecodeToolCode(uint32_t signature) {
    return ToolDataAdapterRegistry::DecodeToolCode(signature);
}

bool DataModule::IsDirectlySupportedToolCode(uint32_t toolCode) {
    return toolCode == 151u || toolCode == 152u || toolCode == 241u;
}

uint32_t DataModule::DecodeToolCodeFromSignature(uint32_t signature) {
    return DecodeToolCode(signature);
}

ToolDataKind DataModule::DetermineToolDataKindFromSignature(uint32_t signature) {
    return ToolDataAdapterRegistry::DetermineToolDataKind(signature);
}

bool DataModule::ParseRawFrameDetailed(const void* rawFrame, size_t rawFrameSize, ParsedToolFrame& outFrame) {
    outFrame = {};
    if (!rawFrame || rawFrameSize != sizeof(GP_TOOL_DATA)) {
        return false;
    }

    return ToolDataAdapterRegistry::Get().ParseRawFrame(rawFrame, rawFrameSize, outFrame);
}

bool DataModule::ParseRawFrame(const void* rawFrame, size_t rawFrameSize, GP_TOOL_DATA& outData) {
    ParsedToolFrame parsedFrame = {};
    if (!ParseRawFrameDetailed(rawFrame, rawFrameSize, parsedFrame)) {
        outData = {};
        return false;
    }

    return TryExtractCanonicalData(parsedFrame, outData);
}

bool DataModule::ValidateDataFile(const std::wstring& filename, std::wstring& outError) {
    outError.clear();

    const FileFormat format = DetectFileFormat(filename);
    if (format == FileFormat::FORMAT_UNKNOWN) {
        outError = L"Неподдерживаемый формат файла";
        return false;
    }

    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        outError = L"Не удалось открыть файл данных";
        return false;
    }

    file.seekg(0, std::ios::end);
    const std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    if (fileSize <= 0) {
        outError = L"Файл слишком короткий или не содержит корректную сигнатуру";
        return false;
    }

    uint32_t signature = 0;
    if (!ReadFileSignature(file, format, signature)) {
        outError = L"Файл слишком короткий или не содержит корректную сигнатуру";
        return false;
    }

    const IToolDataAdapter* adapter = ToolDataAdapterRegistry::Get().FindAdapterBySignature(signature);
    if (!adapter || !adapter->SupportsFileFormat(format)) {
        outError = L"Файл содержит неподдерживаемую структуру данных.\n"
                   L"Приложение поддерживает только структуры AUTONOM_5Tx или LWD legacy (240 байт на кадр).";
        return false;
    }

    const size_t physicalFrameSize = GetPhysicalFrameSize(format);

    if (static_cast<size_t>(fileSize) < physicalFrameSize ||
        (static_cast<size_t>(fileSize) % physicalFrameSize) != 0) {
        outError = L"Файл содержит неподдерживаемую структуру данных.\n"
                   L"Приложение поддерживает только структуры AUTONOM_5Tx или LWD legacy (240 байт на кадр).";
        return false;
    }

    return true;
}

DataModule::DataModule() {
}

DataModule::~DataModule() {
}

// v2.15.1: Определение формата файла по расширению
FileFormat DataModule::DetectFileFormat(const std::wstring& filename) {
    // Находим расширение файла
    size_t lastDot = filename.find_last_of(L'.');
    if (lastDot == std::wstring::npos) {
        return FileFormat::FORMAT_UNKNOWN;
    }
    
    std::wstring extension = filename.substr(lastDot);
    
    // Приводим к нижнему регистру для сравнения
    for (auto& c : extension) {
        c = towlower(c);
    }
    
    if (extension == L".bin") {
        return FileFormat::FORMAT_BIN;
    } else if (extension == L".biv") {
        return FileFormat::FORMAT_BIV;
    } else if (extension == L".dev") {
        return FileFormat::FORMAT_DEV;
    }
    
    return FileFormat::FORMAT_UNKNOWN;
}

bool DataModule::LoadDataFromFile(const std::wstring& filename) {
    std::wstring error;
    return LoadDataFromFile(filename, error);
}

bool DataModule::LoadDataFromFile(const std::wstring& filename, std::wstring& outError) {
    outError.clear();

    if (!ValidateDataFile(filename, outError)) {
        data.clear();
        recordMetadata.clear();
        loadedFileName.clear();
        return false;
    }

    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        outError = L"Не удалось открыть файл данных";
        return false;
    }
        
    data.clear();
    recordMetadata.clear();
    
    // v2.15.1: Определяем формат файла
    FileFormat format = DetectFileFormat(filename);
    if (!IsSupportedDataFileFormat(format)) {
        outError = L"Неподдерживаемый формат файла";
        file.close();
        return false;
    }
    
    // Определяем размер файла
    file.seekg(0, std::ios::end);
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    
    if (fileSize <= 0) {
        outError = L"Файл слишком короткий или не содержит корректную сигнатуру";
        file.close();
        return false;
    }
    
    const size_t frameCount = static_cast<size_t>(fileSize) / GetPhysicalFrameSize(format);
    if (frameCount == 0) {
        outError = L"Файл содержит неподдерживаемую структуру данных.\n"
                   L"Приложение поддерживает только структуры AUTONOM_5Tx или LWD legacy (240 байт на кадр).";
        file.close();
        return false;
    }

    LoadParsedFrames(file, format, frameCount, data, recordMetadata);
    
    file.close();

    if (!HasData()) {
        outError = L"Файл содержит неподдерживаемую структуру данных.\n"
                   L"Приложение поддерживает только структуры AUTONOM_5Tx или LWD legacy (240 байт на кадр).";
        loadedFileName.clear();
        return false;
    }
    
    // Сохраняем имя файла без пути и расширения
    if (HasData()) {
        // Извлекаем имя файла из полного пути
        size_t lastSlash = filename.find_last_of(L"\\/");
        std::wstring fileNameOnly = (lastSlash != std::wstring::npos) 
            ? filename.substr(lastSlash + 1) 
            : filename;
        
        // Удаляем расширение
        size_t lastDot = fileNameOnly.find_last_of(L'.');
        loadedFileName = (lastDot != std::wstring::npos) 
            ? fileNameOnly.substr(0, lastDot) 
            : fileNameOnly;
    }
    
    return HasData();
}

void DataModule::ClearData() {
    data.clear();
    recordMetadata.clear();
    loadedFileName.clear();
}

std::wstring DataModule::GetLoadedFileName() const {
    return loadedFileName;
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

const GP_TOOL_DATA* DataModule::TryGetRecord(size_t index) const {
    return (index < data.size()) ? &data[index] : nullptr;
}

bool DataModule::TryGetRecordMetadata(size_t index, ToolRecordMetadata& outMetadata) const {
    if (index >= recordMetadata.size()) {
        outMetadata = {};
        return false;
    }

    outMetadata = recordMetadata[index];
    return true;
}

bool DataModule::TryDescribeRangeMetadata(size_t startIndex, size_t endIndex, DataRangeMetadataSummary& outSummary) const {
    outSummary = {};

    if (!HasData() || startIndex > endIndex || endIndex >= GetDataSize()) {
        return false;
    }

    ToolRecordMetadata firstMetadata = {};
    if (!TryGetRecordMetadata(startIndex, firstMetadata)) {
        return false;
    }

    outSummary.metadata = firstMetadata;
    outSummary.recordCount = endIndex - startIndex + 1;
    outSummary.hasMetadata = true;
    outSummary.isHomogeneous = true;

    for (size_t index = startIndex + 1; index <= endIndex; ++index) {
        ToolRecordMetadata currentMetadata = {};
        if (!TryGetRecordMetadata(index, currentMetadata)) {
            outSummary = {};
            return false;
        }

        if (!HasSameRecordMetadata(currentMetadata, firstMetadata)) {
            outSummary.isHomogeneous = false;
        }
    }

    return true;
}

bool DataModule::TryAnalyzeExportRange(size_t startIndex, size_t endIndex, DataRangeExportAnalysis& outAnalysis) const {
    outAnalysis = {};

    DataRangeMetadataSummary metadataSummary = {};
    if (!TryDescribeRangeMetadata(startIndex, endIndex, metadataSummary)) {
        return false;
    }

    outAnalysis.metadataSummary = metadataSummary;
    outAnalysis.isValidRange = true;
    outAnalysis.canExportAsCanonicalBin = metadataSummary.hasMetadata;
    outAnalysis.requiresAdapterAwarePolicy = !metadataSummary.isHomogeneous;
    return true;
}

bool DataModule::TryGetRangeMetadataSummary(size_t startIndex, size_t endIndex, ToolRecordMetadata& outMetadata, bool& outIsHomogeneous) const {
    outMetadata = {};
    outIsHomogeneous = false;

    DataRangeMetadataSummary summary = {};
    if (!TryDescribeRangeMetadata(startIndex, endIndex, summary) || !summary.hasMetadata) {
        return false;
    }

    outMetadata = summary.metadata;
    outIsHomogeneous = summary.isHomogeneous;
    return true;
}

bool DataModule::TryGetTemperatureRange(double minX, double maxX, float& outMinTemperature, float& outMaxTemperature) const {
    outMinTemperature = 0.0f;
    outMaxTemperature = 0.0f;

    if (!HasData()) {
        return false;
    }

    const auto indexRange = GetDataIndexRange(minX, maxX);
    const size_t startIndex = indexRange.first;
    const size_t endIndex = indexRange.second;
    if (startIndex >= endIndex) {
        return false;
    }

    outMinTemperature = FLT_MAX;
    outMaxTemperature = -FLT_MAX;
    for (size_t i = startIndex; i < endIndex; ++i) {
        const MeasurementView measurement = GetMeasurementView(i);
        const float value = measurement.GetTemperature();
        if (value < outMinTemperature) {
            outMinTemperature = value;
        }
        if (value > outMaxTemperature) {
            outMaxTemperature = value;
        }
    }

    if (outMaxTemperature <= outMinTemperature) {
        outMaxTemperature = outMinTemperature + 1.0f;
    }

    return true;
}

MeasurementView DataModule::GetMeasurementView(size_t index) const {
    const GP_TOOL_DATA* record = TryGetRecord(index);
    if (!record) {
        return MeasurementView();
    }

    ToolRecordMetadata metadata;
    if (TryGetRecordMetadata(index, metadata)) {
        return MeasurementView(*record, metadata);
    }

    return MeasurementView(*record);
}

ToolDataAccessView DataModule::GetDataView(size_t index) const {
    return GetMeasurementView(index).GetAccessView();
}

// v2.13.0: Добавление данных в реальном времени
void DataModule::AddParsedData(const ParsedToolFrame& parsedData) {
    AppendParsedFrame(parsedData, data, recordMetadata);
}

void DataModule::AddData(const GP_TOOL_DATA& newData) {
    AppendCanonicalRecord(newData, data, recordMetadata);
}

void DataModule::AddDataBatch(const std::vector<GP_TOOL_DATA>& newDataBatch) {
    data.reserve(data.size() + newDataBatch.size());
    recordMetadata.reserve(recordMetadata.size() + newDataBatch.size());
    for (const auto& record : newDataBatch) {
        AppendCanonicalRecord(record, data, recordMetadata);
    }
}

bool DataModule::SaveDataRange(const std::wstring& filename, size_t startIndex, size_t endIndex) const {
    // Проверка корректности индексов
    if (!HasData()) {
        return false;
    }
    
    if (startIndex >= GetDataSize() || endIndex >= GetDataSize() || startIndex > endIndex) {
        return false;
    }
    
    // Открываем файл для записи
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    if (!WriteCanonicalDataRange(file, *this, startIndex, endIndex)) {
        file.close();
        return false;
    }
    
    file.close();
    
    return true;
}

void DataModule::CalculateChartData(DataType type, FrequencyBand freq, std::vector<std::vector<float>>& outData) const {
    // Вызываем перегруженную версию без ограничений диапазона
    std::vector<bool> allTransmittersEnabled(static_cast<size_t>(GetAvailableTransmitterCount()), true);
    CalculateChartData(type, freq, outData, allTransmittersEnabled, 0.0, static_cast<double>(GetDataSize() - 1), -1e6, 1e6);
}

void DataModule::CalculateChartData(DataType type, FrequencyBand freq, std::vector<std::vector<float>>& outData,
                                   double minX, double maxX, double minY, double maxY) const {
    std::vector<bool> allTransmittersEnabled(static_cast<size_t>(GetAvailableTransmitterCount()), true);
    CalculateChartData(type, freq, outData, allTransmittersEnabled, minX, maxX, minY, maxY);
}

void DataModule::CalculateChartData(DataType type, FrequencyBand freq, std::vector<std::vector<float>>& outData,
                                   const std::vector<bool>& enabledTransmitters,
                                   double minX, double maxX, double minY, double maxY) const {
    outData.clear();
    
    if (!HasData())
        return;
    
    // Используем оптимизированный метод получения диапазона индексов
    auto indexRange = GetDataIndexRange(minX, maxX);
    size_t startIndex = indexRange.first;
    size_t endIndex = indexRange.second;
    
    if (startIndex >= endIndex) 
        return;
    
    const int freqIdx = (freq == FrequencyBand::FREQ_400_KHZ) ? 0 : 1;

    switch (type) {
    case DataType::PHASE_DIFFERENCE:
    case DataType::AMPLITUDE_RATIO:
    case DataType::AMPLITUDE_RX1:
    case DataType::AMPLITUDE_RX2:
    case DataType::PHASE_SMT:
    case DataType::RHO_SMT:
        {
            const int availableTransmitterCount = GetAvailableTransmitterCount();
            outData.resize(static_cast<size_t>(availableTransmitterCount));

            std::vector<bool> validTransmitters(static_cast<size_t>(availableTransmitterCount), true);
            for (int t = 0; t < availableTransmitterCount; ++t) {
                validTransmitters[static_cast<size_t>(t)] = (t < static_cast<int>(enabledTransmitters.size())) ? enabledTransmitters[static_cast<size_t>(t)] : true;
            }

            size_t expectedSize = endIndex - startIndex;
            for (int t = 0; t < availableTransmitterCount; ++t) {
                if (validTransmitters[static_cast<size_t>(t)]) {
                    outData[static_cast<size_t>(t)].reserve(expectedSize);
                }
            }

            for (size_t i = startIndex; i < endIndex; ++i) {
                const ToolDataAccessView view = GetDataView(i);
                const int transmitterCount = std::min(availableTransmitterCount, view.GetTransmitterCount());

                for (int t = 0; t < transmitterCount; ++t) {
                    if (!validTransmitters[static_cast<size_t>(t)]) continue;

                    float value = 0.0f;
                    bool hasValue = false;

                    switch (type) {
                        case DataType::PHASE_DIFFERENCE:
                            {
                                float phaseDiff = 0.0f;
                                if (!view.TryGetDeltaPhase(freqIdx, t, phaseDiff)) {
                                    break;
                                }
                                phaseDiff = NormalizePhase(phaseDiff);
                                value = phaseDiff * 57295.7795f; // рад -> мдег
                                hasValue = true;
                            }
                            break;
                        case DataType::AMPLITUDE_RATIO:
                            {
                                float rx1 = 0.0f;
                                float rx2 = 0.0f;
                                if (!view.TryGetAmplitudeRx1(freqIdx, t, rx1) || !view.TryGetAmplitudeRx2(freqIdx, t, rx2)) {
                                    break;
                                }
                                value = (rx1 != 0.0f) ? (rx2 / rx1) : 0.0f;
                                hasValue = true;
                            }
                            break;
                        case DataType::AMPLITUDE_RX1:
                            {
                                float amplitude = 0.0f;
                                if (!view.TryGetAmplitudeRx1(freqIdx, t, amplitude)) {
                                    break;
                                }
                                value = amplitude * 4000.0f / pow(2.0f, 24.0f);
                                hasValue = true;
                            }
                            break;
                        case DataType::AMPLITUDE_RX2:
                            {
                                float amplitude = 0.0f;
                                if (!view.TryGetAmplitudeRx2(freqIdx, t, amplitude)) {
                                    break;
                                }
                                value = amplitude * 4000.0f / pow(2.0f, 24.0f);
                                hasValue = true;
                            }
                            break;
                        case DataType::PHASE_SMT:
                            {
                                float phase = 0.0f;
                                if (!view.TryGetPhaseSmt(freqIdx, t, phase)) {
                                    break;
                                }
                                phase = NormalizePhase(phase);
                                value = phase * 57295.7795f; // рад -> мдег
                                hasValue = true;
                            }
                            break;
                        case DataType::RHO_SMT:
                            hasValue = view.TryGetRhoSmt(freqIdx, t, value);
                            break;
                        default:
                            continue;
                    }

                    if (hasValue && value >= minY && value <= maxY) {
                        outData[static_cast<size_t>(t)].push_back(value);
                    }
                }
            }
        }
        break;
    
    case DataType::SILENT_AMPLITUDE_RX1:
    case DataType::SILENT_AMPLITUDE_RX2:
        {
            // 1 канал для молчащих передатчиков
            outData.resize(1);
            
            
            // Резервируем память для оптимизации
            outData[0].reserve(endIndex - startIndex);
            
            for (size_t i = startIndex; i < endIndex; ++i) {
                const ToolDataAccessView view = GetDataView(i);
                float amplitude = 0.0f;
                const bool hasValue = (type == DataType::SILENT_AMPLITUDE_RX1) ?
                    view.TryGetSilentAmplitudeRx1(freqIdx, amplitude) :
                    view.TryGetSilentAmplitudeRx2(freqIdx, amplitude);
                if (!hasValue) {
                    continue;
                }
                float value = amplitude * 4000.0f / pow(2.0f, 24.0f);
                
                // Проверяем диапазон значений для фильтрации
                if (value >= minY && value <= maxY) {
                    outData[0].push_back(value);
                }
            }
        }
        break;

    case DataType::SILENT_PHASE_DIFFERENCE:
        {
            // 1 канал: разность фаз молчащих передатчиков (R2−R1) по ZERO_dPH[freqIdx]
            outData.resize(1);
            outData[0].reserve(endIndex - startIndex);

            for (size_t i = startIndex; i < endIndex; ++i) {
                const ToolDataAccessView view = GetDataView(i);
                float diff = 0.0f;
                if (!view.TryGetSilentDeltaPhase(freqIdx, diff)) {
                    continue;
                }
                diff = NormalizePhase(diff);
                float value = diff * 57295.7795f; // рад -> мдег

                if (value >= minY && value <= maxY) {
                    outData[0].push_back(value);
                }
            }
        }
        break;

    case DataType::TEMPERATURE:
        {
            // 1 канал для температуры
            outData.resize(1);
            outData[0].reserve(endIndex - startIndex);
            
            for (size_t i = startIndex; i < endIndex; ++i) {
                const MeasurementView measurement = GetMeasurementView(i);
                float value = measurement.GetTemperature();
                
                // Проверяем диапазон значений для фильтрации
                if (value >= minY && value <= maxY) {
                    outData[0].push_back(value);
                }
            }
        }
        break;
    }
}

std::wstring DataModule::GetDataTypeName(DataType type) const {
    switch (type) {
    case DataType::PHASE_DIFFERENCE: return L"Разность фаз";
    case DataType::AMPLITUDE_RATIO: return L"Отношение амплитуд";
    case DataType::AMPLITUDE_RX1: return L"Амплитуда RX1";
    case DataType::AMPLITUDE_RX2: return L"Амплитуда RX2";
    case DataType::PHASE_SMT: return L"Фаза";
    case DataType::RHO_SMT: return L"УЭС";
    case DataType::SILENT_AMPLITUDE_RX1: return L"Молчащий Амплитуда RX1";
    case DataType::SILENT_AMPLITUDE_RX2: return L"Молчащий Амплитуда RX2";
    case DataType::SILENT_PHASE_DIFFERENCE: return L"Молчащий Разность фаз";
    case DataType::TEMPERATURE: return L"Temperature";
    default: return L"Unknown Type";
    }
}

// Вспомогательные методы
int DataModule::GetActiveTransmitterIndex(Transmitter transmitter, FrequencyBand freq) const {
    (void)freq;
    const int idx = static_cast<int>(transmitter);
    return (idx >= 0 && idx < GetAvailableTransmitterCount()) ? idx : -1;
}

int DataModule::GetSilentTransmitterIndex(FrequencyBand freq) const {
    // Для нового формата возвращаем индекс частоты (0=400, 1=2000)
    return (freq == FrequencyBand::FREQ_400_KHZ) ? 0 : 1;
}

int DataModule::GetAvailableTransmitterCount() const {
    int maxCount = 0;
    for (size_t i = 0; i < GetDataSize(); ++i) {
        const ToolDataAccessView view = GetDataView(i);
        maxCount = std::max(maxCount, view.GetTransmitterCount());
    }

    return (maxCount > 0) ? maxCount : kLegacyFallbackTransmitterCount;
}

// Новые методы для работы с произвольными кривыми
void DataModule::CalculateCustomCurveData(const std::vector<CurveDefinition>& curves,
                                         std::vector<std::vector<float>>& leftAxisData,
                                         std::vector<std::vector<float>>& rightAxisData) const {
    // Вызываем перегруженную версию без ограничений диапазона
    if (!HasData()) {
        leftAxisData.clear();
        rightAxisData.clear();
        return;
    }

    CalculateCustomCurveData(curves, leftAxisData, rightAxisData, 
                            0.0, static_cast<double>(GetDataSize() - 1), -1e6, 1e6);
}

void DataModule::CalculateCustomCurveData(const std::vector<CurveDefinition>& curves,
                                         std::vector<std::vector<float>>& leftAxisData,
                                         std::vector<std::vector<float>>& rightAxisData,
                                         double minX, double maxX, double minY, double maxY) const {
    leftAxisData.clear();
    rightAxisData.clear();
    
    if (!HasData() || curves.empty()) {
        return;
    }
    
    // Группируем кривые по осям
    std::vector<CurveDefinition> leftAxisCurves, rightAxisCurves;
    GroupCurvesByAxis(curves, leftAxisCurves, rightAxisCurves);
    
    // Вычисляем данные для левой оси
    leftAxisData.resize(leftAxisCurves.size());
    for (size_t i = 0; i < leftAxisCurves.size(); ++i) {
        CalculateSingleCurveData(leftAxisCurves[i], leftAxisData[i], minX, maxX, minY, maxY);
    }
    
    // Вычисляем данные для правой оси
    rightAxisData.resize(rightAxisCurves.size());
    for (size_t i = 0; i < rightAxisCurves.size(); ++i) {
        CalculateSingleCurveData(rightAxisCurves[i], rightAxisData[i], minX, maxX, minY, maxY);
    }
}

void DataModule::CalculateSingleCurveData(const CurveDefinition& curve, 
                                         std::vector<float>& outData) const {
    // Вызываем перегруженную версию без ограничений диапазона
    if (!HasData()) {
        outData.clear();
        return;
    }

    CalculateSingleCurveData(curve, outData, 0.0, static_cast<double>(GetDataSize() - 1), -1e6, 1e6);
}

void DataModule::CalculateSingleCurveData(const CurveDefinition& curve, 
                                         std::vector<float>& outData,
                                         double minX, double maxX, double minY, double maxY) const {
    outData.clear();
    
    if (!HasData()) {
        return;
    }
    
    // Определяем диапазон данных
    auto indexRange = GetDataIndexRange(minX, maxX);
    size_t startIndex = indexRange.first;
    size_t endIndex = indexRange.second;
    
    if (startIndex >= endIndex) {
        return;
    }
    
    // Вычисляем данные в зависимости от типа кривой
    switch (curve.dataType) {
        case DataType::AMPLITUDE_RATIO:
            CalculateRatioData(curve.elementIndex, curve.frequency, outData, minX, maxX);
            break;
            
        case DataType::PHASE_DIFFERENCE:
            CalculateDifferenceData(curve.elementIndex, curve.frequency, outData, minX, maxX);
            break;
            
        case DataType::AMPLITUDE_RX1:
            CalculateAmplitudeData(curve.elementIndex, 0, curve.frequency, outData, minX, maxX);
            break;
            
        case DataType::AMPLITUDE_RX2:
            CalculateAmplitudeData(curve.elementIndex, 1, curve.frequency, outData, minX, maxX);
            break;
            
        case DataType::PHASE_SMT:
            CalculatePhaseData(curve.elementIndex, 0, curve.frequency, outData, minX, maxX);
            break;

        case DataType::RHO_SMT:
            {
                auto indexRange = GetDataIndexRange(minX, maxX);
                size_t startIndex = indexRange.first;
                size_t endIndex = indexRange.second;

                const int freqIdx = (curve.frequency == FrequencyBand::FREQ_400_KHZ) ? 0 : 1;
                const int txIdx = GetActiveTransmitterIndex(static_cast<Transmitter>(curve.elementIndex), curve.frequency);
                if (txIdx == -1) break;

                outData.reserve(endIndex - startIndex);
                for (size_t i = startIndex; i < endIndex; ++i) {
                    const ToolDataAccessView view = GetDataView(i);
                    float value = 0.0f;
                    if (!view.TryGetRhoSmt(freqIdx, txIdx, value)) {
                        continue;
                    }
                    if (value >= minY && value <= maxY) {
                        outData.push_back(value);
                    }
                }
            }
            break;
            
        case DataType::SILENT_AMPLITUDE_RX1:
            CalculateSilentAmplitudeData(0, curve.frequency, outData, minX, maxX);
            break;
            
        case DataType::SILENT_AMPLITUDE_RX2:
            CalculateSilentAmplitudeData(1, curve.frequency, outData, minX, maxX);
            break;

        case DataType::SILENT_PHASE_DIFFERENCE:
            CalculateSilentPhaseDifferenceData(curve.frequency, outData, minX, maxX);
            break;
            
        case DataType::TEMPERATURE:
            {
                std::vector<float> temperatureData;
                GetTemperatureData(temperatureData, minX, maxX);
                outData.reserve(temperatureData.size());
                for (float value : temperatureData) {
                    if (value >= minY && value <= maxY) {
                        outData.push_back(value);
                    }
                }
            }
            break;
            
        default:
            // Неизвестный тип данных
            break;
    }
}

void DataModule::GroupCurvesByAxis(const std::vector<CurveDefinition>& curves,
                                  std::vector<CurveDefinition>& leftAxisCurves,
                                  std::vector<CurveDefinition>& rightAxisCurves) const {
    leftAxisCurves.clear();
    rightAxisCurves.clear();
    
    for (size_t i = 0; i < curves.size(); i++) {
        const auto& curve = curves[i];
        
        if (!curve.enabled) {
            continue; // Пропускаем отключенные кривые
        }
        
        if (curve.yAxisGroup == 0) {
            leftAxisCurves.push_back(curve);
        } else if (curve.yAxisGroup == 1) {
            rightAxisCurves.push_back(curve);
        }
    }
}

// Внутренние методы для вычисления различных типов данных
void DataModule::CalculateRatioData(int transmitterIndex, FrequencyBand frequency,
                                   std::vector<float>& outData, 
                                   double minX, double maxX) const {
    ActiveCalculationContext context = {};
    if (!TryBuildActiveCalculationContext(*this, transmitterIndex, frequency, minX, maxX, context)) return;
    
    ReserveForWindow(outData, context.window);
    
    // Константа для проверки деления на ноль
    const float EPSILON = 1e-10f;
    
    for (size_t i = context.window.startIndex; i < context.window.endIndex; ++i) {
        const ToolDataAccessView view = GetDataView(i);
        float denominator = 0.0f;
        float numerator = 0.0f;
        if (!view.TryGetAmplitudeRx1(context.frequencyIndex, context.transmitterIndex, denominator) ||
            !view.TryGetAmplitudeRx2(context.frequencyIndex, context.transmitterIndex, numerator)) {
            continue;
        }
        
        if (std::abs(denominator) > EPSILON) { // Избегаем деления на ноль
            float ratio = numerator / denominator;
            outData.push_back(ratio);
        } else {
            outData.push_back(0.0f); // Или можно использовать std::numeric_limits<float>::quiet_NaN()
        }
    }
}

void DataModule::CalculateDifferenceData(int transmitterIndex, FrequencyBand frequency,
                                        std::vector<float>& outData,
                                        double minX, double maxX) const {
    ActiveCalculationContext context = {};
    if (!TryBuildActiveCalculationContext(*this, transmitterIndex, frequency, minX, maxX, context)) return;
    
    ReserveForWindow(outData, context.window);
    
    for (size_t i = context.window.startIndex; i < context.window.endIndex; ++i) {
        const ToolDataAccessView view = GetDataView(i);
        float difference = 0.0f;
        if (!view.TryGetDeltaPhase(context.frequencyIndex, context.transmitterIndex, difference)) {
            continue;
        }
        difference = NormalizePhase(difference);
        outData.push_back(difference * 57295.7795f);
    }
}

void DataModule::CalculateAmplitudeData(int transmitterIndex, int receiverIndex, 
                                       FrequencyBand frequency, std::vector<float>& outData,
                                       double minX, double maxX) const {
    ActiveCalculationContext context = {};
    if (!TryBuildActiveCalculationContext(*this, transmitterIndex, frequency, minX, maxX, context)) return;
    
    ReserveForWindow(outData, context.window);
    
    for (size_t i = context.window.startIndex; i < context.window.endIndex; ++i) {
        const ToolDataAccessView view = GetDataView(i);
        float amplitude = 0.0f;
        const bool hasValue = (receiverIndex == 0) ?
            view.TryGetAmplitudeRx1(context.frequencyIndex, context.transmitterIndex, amplitude) :
            view.TryGetAmplitudeRx2(context.frequencyIndex, context.transmitterIndex, amplitude);
        if (!hasValue) {
            continue;
        }
        outData.push_back(amplitude * 4000.0f / pow(2.0f, 24.0f)); // Преобразование В -> мВ
    }
}

void DataModule::CalculatePhaseData(int transmitterIndex, int receiverIndex, 
                                   FrequencyBand frequency, std::vector<float>& outData,
                                   double minX, double maxX) const {
    (void)receiverIndex;
    ActiveCalculationContext context = {};
    if (!TryBuildActiveCalculationContext(*this, transmitterIndex, frequency, minX, maxX, context)) return;
    
    ReserveForWindow(outData, context.window);
    
    for (size_t i = context.window.startIndex; i < context.window.endIndex; ++i) {
        const ToolDataAccessView view = GetDataView(i);
        float phase = 0.0f;
        if (!view.TryGetPhaseSmt(context.frequencyIndex, context.transmitterIndex, phase)) {
            continue;
        }
        phase = NormalizePhase(phase);
        outData.push_back(phase * 57295.7795f);
    }
}

void DataModule::CalculateSilentAmplitudeData(int receiverIndex, FrequencyBand frequency,
                                             std::vector<float>& outData,
                                             double minX, double maxX) const {
    SilentCalculationContext context = {};
    if (!TryBuildSilentCalculationContext(*this, frequency, minX, maxX, context)) return;
    
    ReserveForWindow(outData, context.window);
    
    for (size_t i = context.window.startIndex; i < context.window.endIndex; ++i) {
        const ToolDataAccessView view = GetDataView(i);
        float amplitude = 0.0f;
        const bool hasValue = (receiverIndex == 0) ?
            view.TryGetSilentAmplitudeRx1(context.frequencyIndex, amplitude) :
            view.TryGetSilentAmplitudeRx2(context.frequencyIndex, amplitude);
        if (!hasValue) {
            continue;
        }
        outData.push_back(amplitude * 4000.0f / pow(2.0f, 24.0f)); // Преобразование В -> мВ
    }
}

void DataModule::CalculateSilentPhaseDifferenceData(FrequencyBand frequency,
                                                   std::vector<float>& outData,
                                                   double minX, double maxX) const {
    SilentCalculationContext context = {};
    if (!TryBuildSilentCalculationContext(*this, frequency, minX, maxX, context)) return;

    ReserveForWindow(outData, context.window);

    for (size_t i = context.window.startIndex; i < context.window.endIndex; ++i) {
        const ToolDataAccessView view = GetDataView(i);
        float diff = 0.0f;
        if (!view.TryGetSilentDeltaPhase(context.frequencyIndex, diff)) {
            continue;
        }
        diff = NormalizePhase(diff);
        outData.push_back(diff * 57295.7795f); // рад -> мдег
    }
}

// Метод нормализации фазы в диапазон (-π, π]
float DataModule::NormalizePhase(float phaseRadians) const {
    const float PI = 3.14159265358979323846f;
    
    // Приводим фазу к диапазону (-π, π]
    while (phaseRadians > PI) {
        phaseRadians -= 2.0f * PI;
    }
    while (phaseRadians <= -PI) {
        phaseRadians += 2.0f * PI;
    }
    
    return phaseRadians;
}

// Вспомогательные методы
int DataModule::GetDataArrayIndex(int transmitterIndex, int receiverIndex, 
                                 FrequencyBand frequency) const {
    // Этот метод может быть использован для более сложной логики индексации
    if (receiverIndex == 0 || receiverIndex == 1) {
        return GetActiveTransmitterIndex(static_cast<Transmitter>(transmitterIndex), frequency);
    }
    return -1;
}

bool DataModule::IsValidDataRange(double minX, double maxX) const {
    return minX >= 0 && maxX < static_cast<double>(GetDataSize()) && minX <= maxX;
}

std::pair<size_t, size_t> DataModule::GetDataIndexRange(double minX, double maxX) const {
    if (!HasData()) {
        return {0, 0};
    }
    
    const size_t dataSize = GetDataSize();
    size_t startIndex = static_cast<size_t>(std::max(0.0, minX));
    size_t endIndex = static_cast<size_t>(std::min(static_cast<double>(dataSize), maxX + 1));
    
    // Убеждаемся, что индексы корректны
    startIndex = std::min(startIndex, dataSize);
    endIndex = std::min(endIndex, dataSize);
    
    if (startIndex >= endIndex) {
        return {0, 0};
    }
    
    return {startIndex, endIndex};
}

// Новые оптимизированные методы для работы с конфигурациями кривых
void DataModule::CalculateChartDataFromConfiguration(const ChartConfiguration& config,
                                                   std::vector<std::vector<float>>& leftAxisData,
                                                   std::vector<std::vector<float>>& rightAxisData) const {
    // Вызываем перегруженную версию без ограничений диапазона
    if (!HasData()) {
        leftAxisData.clear();
        rightAxisData.clear();
        return;
    }

    CalculateChartDataFromConfiguration(config, leftAxisData, rightAxisData,
                                      0.0, static_cast<double>(GetDataSize() - 1), -1e6, 1e6);
}

void DataModule::CalculateChartDataFromConfiguration(const ChartConfiguration& config,
                                                   std::vector<std::vector<float>>& leftAxisData,
                                                   std::vector<std::vector<float>>& rightAxisData,
                                                   double minX, double maxX, double minY, double maxY) const {
    leftAxisData.clear();
    rightAxisData.clear();
    
    if (!HasData() || config.curves.empty()) {
        return;
    }
    
    // Используем новый метод для вычисления данных кривых
    CalculateCustomCurveData(config.curves, leftAxisData, rightAxisData, minX, maxX, minY, maxY);
}


// v2.10.0: Получение температурных данных для оси X
void DataModule::GetTemperatureData(std::vector<float>& outData) const {
    outData.clear();
    if (!HasData()) return;
    
    outData.reserve(GetDataSize());
    for (size_t i = 0; i < GetDataSize(); ++i) {
        const MeasurementView measurement = GetMeasurementView(i);
        outData.push_back(measurement.GetTemperature());
    }
}

void DataModule::GetTemperatureData(std::vector<float>& outData, double minX, double maxX) const {
    outData.clear();
    if (!HasData()) return;
    
    // Получаем диапазон индексов
    auto indexRange = GetDataIndexRange(minX, maxX);
    size_t startIndex = indexRange.first;
    size_t endIndex = indexRange.second;
    
    outData.reserve(endIndex - startIndex);
    for (size_t i = startIndex; i < endIndex; ++i) {
        const MeasurementView measurement = GetMeasurementView(i);
        outData.push_back(measurement.GetTemperature());
    }
}
