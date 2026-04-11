#include "framework.h"
#include "SchematicModule.h"
#include "CurveSelectionModule.h"
#include "MetrologyModule.h"
#include "ToolPresentation.h"
#include "Resource.h"
#include <sstream>
#include <commctrl.h>
#include <algorithm>
#include <array>
#include <map>
#include <set>

// Вспомогательная функция для конвертации UTF-8 в UTF-16 (static - локальная для этого файла)
static std::wstring Utf8ToWide(const std::string& utf8str) {
    if (utf8str.empty()) return std::wstring();
    
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8str.c_str(), (int)utf8str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8str.c_str(), (int)utf8str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

static std::wstring BuildSmtPresetDisplayName(const wchar_t* baseName, SmtComputationMode mode) {
    return std::wstring(baseName) +
           (mode == SmtComputationMode::FROM_COMPUTER ? L" (на компьютере)" : L" (на приборе)");
}

struct SmtUiBinding {
    int presetId;
    int fromSondeCheckboxId;
    int fromComputerCheckboxId;
    DataTypeFilter dataFilter;
    PresetType presetType;
    const wchar_t* baseName;
    SmtComputationMode SchematicSelection::* modeMember;
};

const SmtUiBinding* FindSmtBindingByPresetId(int presetId) {
    static const std::array<SmtUiBinding, 2> kBindings = {{
        {IDC_PRESET_RHO_SMT, IDC_RHO_SMT_FROM_SONDE, IDC_RHO_SMT_FROM_COMPUTER,
         DataTypeFilter::RHO_SMT_ONLY, PresetType::RHO_SMT, L"УЭС", &SchematicSelection::rhoSmtMode},
        {IDC_PRESET_PHASE_SMT, IDC_PHASE_SMT_FROM_SONDE, IDC_PHASE_SMT_FROM_COMPUTER,
         DataTypeFilter::PHASE_SMT_ONLY, PresetType::PHASE_SMT, L"Симм. фазы", &SchematicSelection::phaseSmtMode}
    }};

    for (const auto& binding : kBindings) {
        if (binding.presetId == presetId) {
            return &binding;
        }
    }

    return nullptr;
}

const SmtUiBinding* FindSmtBindingByCheckboxId(int checkboxId) {
    static const std::array<int, 2> kSmtPresetIds = {IDC_PRESET_RHO_SMT, IDC_PRESET_PHASE_SMT};
    for (int presetId : kSmtPresetIds) {
        const SmtUiBinding* binding = FindSmtBindingByPresetId(presetId);
        if (!binding) {
            continue;
        }

        if (binding->fromSondeCheckboxId == checkboxId || binding->fromComputerCheckboxId == checkboxId) {
            return binding;
        }
    }

    return nullptr;
}

SmtComputationMode GetDialogSmtMode(HWND hDlg, const SmtUiBinding& binding) {
    return (IsDlgButtonChecked(hDlg, binding.fromComputerCheckboxId) == BST_CHECKED)
        ? SmtComputationMode::FROM_COMPUTER
        : SmtComputationMode::FROM_SONDE;
}

void ApplySmtPresetState(HWND hDlg,
                         const SmtUiBinding& binding,
                         SchematicSelection& selection,
                         PresetType& currentPreset,
                         std::wstring& currentPresetDisplayName) {
    selection.dataFilter = binding.dataFilter;
    selection.*(binding.modeMember) = GetDialogSmtMode(hDlg, binding);
    currentPreset = binding.presetType;
    currentPresetDisplayName = BuildSmtPresetDisplayName(binding.baseName, selection.*(binding.modeMember));
}

void ApplyBasicPresetState(SchematicSelection& selection,
                           DataTypeFilter dataFilter,
                           PresetType presetType,
                           const wchar_t* displayName,
                           PresetType& currentPreset,
                           std::wstring& currentPresetDisplayName) {
    selection.dataFilter = dataFilter;
    currentPreset = presetType;
    currentPresetDisplayName = displayName;
}

void ApplySmtModesToConfigurationCurves(ChartConfiguration& config, const SchematicSelection& selection) {
    for (auto& curve : config.curves) {
        if (curve.dataType == DataType::RHO_SMT) {
            curve.smtMode = selection.rhoSmtMode;
        } else if (curve.dataType == DataType::PHASE_SMT) {
            curve.smtMode = selection.phaseSmtMode;
        }
    }
}

std::wstring BuildAppliedPresetDisplayName(PresetType currentPreset,
                                           const std::wstring& currentPresetDisplayName,
                                           const SchematicSelection& selection) {
    if (currentPreset == PresetType::RHO_SMT) {
        return BuildSmtPresetDisplayName(L"УЭС", selection.rhoSmtMode);
    }

    if (currentPreset == PresetType::PHASE_SMT) {
        return BuildSmtPresetDisplayName(L"Симм. фазы", selection.phaseSmtMode);
    }

    return currentPresetDisplayName;
}

void SyncSmtModeCheckboxes(HWND hDlg, const SmtUiBinding& binding, const SchematicSelection& selection) {
    const bool isActive = selection.dataFilter == binding.dataFilter;
    const SmtComputationMode mode = selection.*(binding.modeMember);

    CheckDlgButton(hDlg, binding.fromSondeCheckboxId,
                   isActive && mode == SmtComputationMode::FROM_SONDE ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, binding.fromComputerCheckboxId,
                   isActive && mode == SmtComputationMode::FROM_COMPUTER ? BST_CHECKED : BST_UNCHECKED);
}

void ApplySmtModeCheckboxSelection(HWND hDlg,
                                   const SmtUiBinding& binding,
                                   SchematicSelection& selection,
                                   SmtComputationMode mode) {
    selection.*(binding.modeMember) = mode;
    CheckDlgButton(hDlg, binding.fromSondeCheckboxId,
                   mode == SmtComputationMode::FROM_SONDE ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hDlg, binding.fromComputerCheckboxId,
                   mode == SmtComputationMode::FROM_COMPUTER ? BST_CHECKED : BST_UNCHECKED);
}

static constexpr size_t kAllUiTransmittersDualReceiverCurveThreshold =
    static_cast<size_t>(SchematicSelection::UI_TRANSMITTER_COUNT) *
    static_cast<size_t>(SchematicSelection::UI_RECEIVER_COUNT) * 2;

static constexpr int kSchematicElementWidth = 58;
static constexpr int kSchematicElementHeight = 35;
static constexpr int kSchematicElementSpacing = 8;
static constexpr int kFrameToSilentSpacing = 6;
static constexpr int kSchematicFrameElementsCount =
    SchematicSelection::UI_TRANSMITTER_COUNT + SchematicSelection::UI_RECEIVER_COUNT;
static constexpr std::array<int, SchematicSelection::UI_TRANSMITTER_COUNT> kUiTransmitterLayoutPositions = {2, 5, 1, 6, 0};
static constexpr int kUiReceiverLayoutStartPosition = 3;

static std::wstring BuildSilentGroupHeaderLabel(int index) {
    return L"\x041C\x043E\x043B\x0447. R" + std::to_wstring(index + 1);
}

static std::wstring BuildElementLabel(ElementType type, int index) {
    return GetElementShortLabel(type, index);
}

static std::wstring BuildElementTooltip(ElementType type, int index) {
    return GetElementTooltipText(type, index);
}

static bool HasAllUiTransmittersSelected(const std::set<int>& uniqueTransmitters, size_t transmitterCount) {
    return uniqueTransmitters.size() == transmitterCount;
}

static void SelectUiReceiver(SchematicSelection& selection, int receiverIndex) {
    if (receiverIndex >= 0 && receiverIndex < SchematicSelection::UI_RECEIVER_COUNT) {
        selection.receivers[receiverIndex] = true;
    }
}

static void SelectAllUiReceivers(SchematicSelection& selection) {
    for (int i = 0; i < SchematicSelection::UI_RECEIVER_COUNT; i++) {
        selection.receivers[i] = true;
    }
}

static bool IsUiTransmitterIndex(int index) {
    return index >= 0 && index < SchematicSelection::UI_TRANSMITTER_COUNT;
}

static bool IsUiReceiverIndex(int index) {
    return index >= 0 && index < SchematicSelection::UI_RECEIVER_COUNT;
}

static bool IsUiSilentIndex(int index) {
    return index >= 0 && index < SchematicSelection::UI_SILENT_COUNT;
}

static int GetSchematicFramePositionIndex(ElementType type, int index) {
    switch (type) {
        case ElementType::TRANSMITTER:
            return IsUiTransmitterIndex(index) ? kUiTransmitterLayoutPositions[static_cast<size_t>(index)] : -1;
        case ElementType::RECEIVER:
            return IsUiReceiverIndex(index) ? (kUiReceiverLayoutStartPosition + index) : -1;
        default:
            return -1;
    }
}

static bool IsSelectionElementSelected(const SchematicSelection& selection, ElementType type, int index) {
    switch (type) {
        case ElementType::TRANSMITTER:
            return IsUiTransmitterIndex(index) && static_cast<size_t>(index) < selection.transmitters.size()
                ? selection.transmitters[static_cast<size_t>(index)]
                : false;
        case ElementType::RECEIVER:
            return IsUiReceiverIndex(index) && static_cast<size_t>(index) < selection.receivers.size()
                ? selection.receivers[static_cast<size_t>(index)]
                : false;
        case ElementType::SILENT_TX:
            return IsUiSilentIndex(index) && static_cast<size_t>(index) < selection.silentTx.size()
                ? selection.silentTx[static_cast<size_t>(index)]
                : false;
        default:
            return false;
    }
}

static void RestoreSelectedTransmittersFromCurves(const ChartConfiguration& existingConfig,
                                                  SchematicSelection& selection) {
    for (const auto& curve : existingConfig.curves) {
        if (curve.sourceElement == ElementType::TRANSMITTER) {
            if (IsUiTransmitterIndex(curve.elementIndex) &&
                static_cast<size_t>(curve.elementIndex) < selection.transmitters.size()) {
                selection.transmitters[curve.elementIndex] = true;
            }
        }
    }
}

static void SelectAllUiTransmitters(SchematicSelection& selection) {
    for (size_t i = 0; i < selection.transmitters.size(); ++i) {
        selection.transmitters[i] = true;
    }
}

static void RestoreSelectedSilentTransmittersFromCurves(const ChartConfiguration& existingConfig,
                                                        SchematicSelection& selection) {
    for (const auto& curve : existingConfig.curves) {
        if (curve.sourceElement == ElementType::SILENT_TX) {
            if (IsUiSilentIndex(curve.elementIndex)) {
                selection.silentTx[curve.elementIndex] = true;
            }
        }
    }
}

// Определение цветов для элементов схемы
const COLORREF SchematicModule::TRANSMITTER_COLOR = RGB(180, 100, 100);  // Приглушенный красный
const COLORREF SchematicModule::RECEIVER_COLOR = RGB(100, 180, 100);     // Приглушенный зеленый  
const COLORREF SchematicModule::SILENT_TX_COLOR = RGB(128, 128, 128);    // Серый
const COLORREF SchematicModule::SELECTION_COLOR = RGB(255, 255, 0);      // Желтый для выделения

// Константы для рамки основных элементов
const COLORREF FRAME_BACKGROUND = RGB(245, 245, 245);  // Светло-серый фон
const COLORREF FRAME_BORDER = RGB(100, 100, 100);      // Тёмно-серая граница (сделано темнее)
const int FRAME_PADDING = 3;                           // Отступ внутри рамки (максимальная компактность: 15→10→6→4→3)
const int FRAME_BORDER_WIDTH = 2;                      // Толщина границы (увеличено с 1 до 2)
const int FRAME_CORNER_RADIUS = 5;                     // Радиус скругления углов (уменьшено с 8→6→5)

// Константы для градиентов элементов
// Передатчики (Transmitters)
const COLORREF TX_GRADIENT_START = RGB(200, 120, 120);
const COLORREF TX_GRADIENT_END = RGB(170, 90, 90);
const COLORREF TX_ACTIVE_GRADIENT_START = RGB(230, 130, 130);
const COLORREF TX_ACTIVE_GRADIENT_END = RGB(200, 100, 100);
const COLORREF TX_HOVER_GRADIENT_START = RGB(215, 125, 125);
const COLORREF TX_HOVER_GRADIENT_END = RGB(185, 95, 95);

// Приемники (Receivers)
const COLORREF RX_GRADIENT_START = RGB(120, 200, 120);
const COLORREF RX_GRADIENT_END = RGB(90, 170, 90);
const COLORREF RX_ACTIVE_GRADIENT_START = RGB(130, 230, 130);
const COLORREF RX_ACTIVE_GRADIENT_END = RGB(100, 200, 100);
const COLORREF RX_HOVER_GRADIENT_START = RGB(125, 215, 125);
const COLORREF RX_HOVER_GRADIENT_END = RGB(95, 185, 95);

// Молчащие передатчики (Silent)
const COLORREF SILENT_GRADIENT_START = RGB(160, 160, 160);
const COLORREF SILENT_GRADIENT_END = RGB(130, 130, 130);
const COLORREF SILENT_ACTIVE_GRADIENT_START = RGB(180, 180, 180);
const COLORREF SILENT_ACTIVE_GRADIENT_END = RGB(150, 150, 150);
const COLORREF SILENT_HOVER_GRADIENT_START = RGB(170, 170, 170);
const COLORREF SILENT_HOVER_GRADIENT_END = RGB(140, 140, 140);

// Константы для тени
const int SHADOW_OFFSET_X = 2;
const int SHADOW_OFFSET_Y = 2;
const COLORREF SHADOW_COLOR = RGB(200, 200, 200);  // Светло-серая тень

// Реализация CurveDefinition
CurveDefinition::CurveDefinition() 
    : yAxisGroup(0), enabled(false), color(RGB(0, 0, 0)), lineStyle(PS_SOLID),
      sourceElement(ElementType::TRANSMITTER), elementIndex(0), 
      dataType(DataType::AMPLITUDE_RX1), frequency(FrequencyBand::FREQ_400_KHZ),
      smtMode(SmtComputationMode::FROM_SONDE) {
}

CurveDefinition::CurveDefinition(const std::string& id, const std::string& name, 
                               const std::string& units, ElementType element, 
                               int index, DataType type, FrequencyBand freq)
    : id(id), displayName(name), units(units), yAxisGroup(0), enabled(true),
      color(RGB(0, 0, 255)), lineStyle(PS_SOLID), sourceElement(element),
      elementIndex(index), dataType(type), frequency(freq),
      smtMode(SmtComputationMode::FROM_SONDE) {
}

// Реализация SchematicSelection
SchematicSelection::SchematicSelection()
    : frequency(FrequencyBand::FREQ_400_KHZ)
    , dataFilter(DataTypeFilter::ALL)
    , rhoSmtMode(SmtComputationMode::FROM_SONDE)
    , phaseSmtMode(SmtComputationMode::FROM_SONDE) {
    transmitters.resize(UI_TRANSMITTER_COUNT, false);  // T1-T5
    receivers.resize(UI_RECEIVER_COUNT, false);     // R1-R2
    silentTx.resize(UI_SILENT_COUNT, false);      // Молчащие для R1 и R2
}

void SchematicSelection::Clear() {
    std::fill(transmitters.begin(), transmitters.end(), false);
    std::fill(receivers.begin(), receivers.end(), false);
    std::fill(silentTx.begin(), silentTx.end(), false);
    frequency = FrequencyBand::FREQ_400_KHZ;
    dataFilter = DataTypeFilter::ALL;
    rhoSmtMode = SmtComputationMode::FROM_SONDE;
    phaseSmtMode = SmtComputationMode::FROM_SONDE;
}

bool SchematicSelection::IsValid() const {
    return GetValidationError().empty();
}

int SchematicSelection::GetSelectedReceiversCount() const {
    return static_cast<int>(std::count(receivers.begin(), receivers.end(), true));
}

int SchematicSelection::GetSelectedTransmittersCount() const {
    return static_cast<int>(std::count(transmitters.begin(), transmitters.end(), true));
}

int SchematicSelection::GetSelectedSilentCount() const {
    return static_cast<int>(std::count(silentTx.begin(), silentTx.end(), true));
}

std::string SchematicSelection::GetValidationError() const {
    int receivers = GetSelectedReceiversCount();
    int transmitters = GetSelectedTransmittersCount();
    int silent = GetSelectedSilentCount();
    
    // Для пресетов (dataFilter != ALL) валидация всегда проходит
    // Кривые генерируются на основе фильтра, а не выбранных элементов
    if (dataFilter != DataTypeFilter::ALL) {
        return ""; // Пресет всегда валиден
    }
    
    // Для ручного выбора (dataFilter == ALL) проверяем выбранные элементы
    // Должен быть выбран хотя бы один элемент
    if (receivers == 0 && transmitters == 0 && silent == 0) {
        // "Выберите элементы для построения графика"
        return "\xD0\x92\xD1\x8B\xD0\xB1\xD0\xB5\xD1\x80\xD0\xB8\xD1\x82\xD0\xB5 \xD1\x8D\xD0\xBB\xD0\xB5\xD0\xBC\xD0\xB5\xD0\xBD\xD1\x82\xD1\x8B \xD0\xB4\xD0\xBB\xD1\x8F \xD0\xBF\xD0\xBE\xD1\x81\xD1\x82\xD1\x80\xD0\xBE\xD0\xB5\xD0\xBD\xD0\xB8\xD1\x8F \xD0\xB3\xD1\x80\xD0\xB0\xD1\x84\xD0\xB8\xD0\xBA\xD0\xB0";
    }
    
    // Нельзя одновременно выбирать передатчики и приемники
    if (transmitters > 0 && receivers > 0) {
        // "Нельзя выбирать передатчики и приёмники одновременно"
        return "\xD0\x9D\xD0\xB5\xD0\xBB\xD1\x8C\xD0\xB7\xD1\x8F \xD0\xB2\xD1\x8B\xD0\xB1\xD0\xB8\xD1\x80\xD0\xB0\xD1\x82\xD1\x8C \xD0\xBF\xD0\xB5\xD1\x80\xD0\xB5\xD0\xB4\xD0\xB0\xD1\x82\xD1\x87\xD0\xB8\xD0\xBA\xD0\xB8 \xD0\xB8 \xD0\xBF\xD1\x80\xD0\xB8\xD1\x91\xD0\xBC\xD0\xBD\xD0\xB8\xD0\xBA\xD0\xB8 \xD0\xBE\xD0\xB4\xD0\xBD\xD0\xBE\xD0\xB2\xD1\x80\xD0\xB5\xD0\xBC\xD0\xB5\xD0\xBD\xD0\xBD\xD0\xBE";
    }
    
    // ИСПРАВЛЕНО: Разрешаем комбинации молчащих передатчиков с активными элементами
    // Запрещена только комбинация приемников R1+R2 с молчащими передатчиками
    if (silent > 0 && receivers == 2) {
        // "Нельзя выбирать оба приёмника и молчащие передатчики одновременно"
        return "\xD0\x9D\xD0\xB5\xD0\xBB\xD1\x8C\xD0\xB7\xD1\x8F \xD0\xB2\xD1\x8B\xD0\xB1\xD0\xB8\xD1\x80\xD0\xB0\xD1\x82\xD1\x8C \xD0\xBE\xD0\xB1\xD0\xB0 \xD0\xBF\xD1\x80\xD0\xB8\xD1\x91\xD0\xBC\xD0\xBD\xD0\xB8\xD0\xBA\xD0\xB0 \xD0\xB8 \xD0\xBC\xD0\xBE\xD0\xBB\xD1\x87\xD0\xB0\xD1\x89\xD0\xB8\xD0\xB5 \xD0\xBF\xD0\xB5\xD1\x80\xD0\xB5\xD0\xB4\xD0\xB0\xD1\x82\xD1\x87\xD0\xB8\xD0\xBA\xD0\xB8";
    }
    
    return ""; // Выбор корректен
}

// Реализация ChartConfiguration
ChartConfiguration::ChartConfiguration() 
    : hasDualAxis(false), xAxisType(XAxisType::FRAME_NUMBER), 
      activePreset(PresetType::NONE), presetDisplayName(L"") {
}

void ChartConfiguration::Clear() {
    curves.clear();
    hasDualAxis = false;
    leftAxisUnits.clear();
    rightAxisUnits.clear();
    xAxisType = XAxisType::FRAME_NUMBER;
    activePreset = PresetType::NONE;
    presetDisplayName.clear();
}

bool ChartConfiguration::IsEmpty() const {
    return curves.empty();
}

ChartConfiguration ChartConfiguration::Copy() const {
    ChartConfiguration copy;
    copy.curves = this->curves;
    copy.hasDualAxis = this->hasDualAxis;
    copy.leftAxisUnits = this->leftAxisUnits;
    copy.rightAxisUnits = this->rightAxisUnits;
    copy.xAxisType = this->xAxisType;
    copy.activePreset = this->activePreset;
    copy.presetDisplayName = this->presetDisplayName;
    return copy;
}

void ChartConfiguration::AddCurve(const CurveDefinition& curve) {
    curves.push_back(curve);
    UpdateAxisConfiguration();
}

void ChartConfiguration::RemoveCurve(const std::string& curveId) {
    curves.erase(std::remove_if(curves.begin(), curves.end(),
        [&curveId](const CurveDefinition& curve) { return curve.id == curveId; }),
        curves.end());
    UpdateAxisConfiguration();
}

void ChartConfiguration::UpdateAxisConfiguration() {
    // Определяем, используются ли обе оси
    bool hasLeftAxis = false, hasRightAxis = false;
    
    for (const auto& curve : curves) {
        if (!curve.enabled) continue;
        
        if (curve.yAxisGroup == 0) {
            hasLeftAxis = true;
            if (leftAxisUnits.empty()) {
                leftAxisUnits = curve.units;
            }
        } else if (curve.yAxisGroup == 1) {
            hasRightAxis = true;
            if (rightAxisUnits.empty()) {
                rightAxisUnits = curve.units;
            }
        }
    }
    
    hasDualAxis = hasLeftAxis && hasRightAxis;
    
    // Очищаем единицы измерения неиспользуемых осей
    if (!hasLeftAxis) leftAxisUnits.clear();
    if (!hasRightAxis) rightAxisUnits.clear();
}

// Реализация SchematicModule
SchematicModule::SchematicModule() 
    : curveSelectionModule(nullptr), dataModule(nullptr), metrologyModule(nullptr), targetChartNumber(1), hoveredElement(nullptr),
      hSchematicDialog(nullptr), hParentWindow(nullptr),
      currentPreset(PresetType::NONE), currentPresetDisplayName(L"") {
}

SchematicModule::~SchematicModule() {
    // Закрываем немодальное окно при уничтожении объекта
    CloseSchematicDialog();
}

void SchematicModule::ShowSchematicDialog(HWND parent, int chartNumber) {
    targetChartNumber = chartNumber;
    hParentWindow = parent;
    
    // Если окно уже существует, показываем и активируем его
    if (hSchematicDialog && IsWindow(hSchematicDialog)) {
        // Обновляем заголовок окна в соответствии с текущим номером графика
        std::wstring title = L"\x0421\x0445\x0435\x043C\x0430 \x043F\x0440\x0438\x0431\x043E\x0440\x0430 - \x0413\x0440\x0430\x0444\x0438\x043A " + std::to_wstring(targetChartNumber);
        SetWindowText(hSchematicDialog, title.c_str());

        // Показываем окно (если оно было скрыто)
        ShowWindow(hSchematicDialog, SW_SHOW);
        
        // Если окно свернуто, разворачиваем
        if (IsIconic(hSchematicDialog)) {
            ShowWindow(hSchematicDialog, SW_RESTORE);
        }
        
        // Активируем и выводим на передний план
        SetForegroundWindow(hSchematicDialog);
        BringWindowToTop(hSchematicDialog);
        return;
    }
    
    // Создаем новое немодальное окно
    schematicElements.clear();
    deviceFrameRect = {0, 0, 0, 0};
    hoveredElement = nullptr;
    
    HINSTANCE hInst = GetModuleHandle(NULL);
    
    // Создаем немодальное диалоговое окно
    hSchematicDialog = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_SCHEMATIC_DIALOG), 
                                        parent, SchematicDialogProc, (LPARAM)this);
    
    if (hSchematicDialog) {
        // Показываем окно
        ShowWindow(hSchematicDialog, SW_SHOW);
        UpdateWindow(hSchematicDialog);
    } else {
    }
}

void SchematicModule::HideSchematicDialog() {
    if (hSchematicDialog && IsWindow(hSchematicDialog)) {
        ShowWindow(hSchematicDialog, SW_HIDE);
    }
}

void SchematicModule::CloseSchematicDialog() {
    if (hSchematicDialog && IsWindow(hSchematicDialog)) {
        DestroyWindow(hSchematicDialog);
        hSchematicDialog = nullptr;
    }
}

bool SchematicModule::IsSchematicDialogVisible() const {
    return hSchematicDialog && IsWindow(hSchematicDialog) && IsWindowVisible(hSchematicDialog);
}

HWND SchematicModule::GetSchematicDialogHandle() const {
    return hSchematicDialog;
}

void SchematicModule::SetOnConfigurationAppliedCallback(std::function<void(int)> callback) {
    onConfigurationApplied = callback;
}

void SchematicModule::DrawElementShadow(HDC hdc, const RECT& rect) {
    // Создаём смещённый прямоугольник для тени
    RECT shadowRect = {
        rect.left + SHADOW_OFFSET_X,
        rect.top + SHADOW_OFFSET_Y,
        rect.right + SHADOW_OFFSET_X,
        rect.bottom + SHADOW_OFFSET_Y
    };
    
    // Рисуем полупрозрачную тень
    HBRUSH shadowBrush = CreateSolidBrush(SHADOW_COLOR);
    HPEN shadowPen = CreatePen(PS_SOLID, 1, SHADOW_COLOR);
    
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, shadowBrush);
    HPEN oldPen = (HPEN)SelectObject(hdc, shadowPen);
    
    // Рисуем скруглённый прямоугольник для тени
    RoundRect(hdc, shadowRect.left, shadowRect.top,
              shadowRect.right, shadowRect.bottom, 8, 8);
    
    // Восстанавливаем объекты GDI
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(shadowBrush);
    DeleteObject(shadowPen);
}

void SchematicModule::DrawGradientRect(HDC hdc, const RECT& rect, COLORREF colorStart, COLORREF colorEnd) {
    // Простая эмуляция вертикального градиента через GDI
    // Рисуем множество горизонтальных линий с плавным переходом цвета
    
    int height = rect.bottom - rect.top;
    if (height <= 0) return;
    
    // Извлекаем компоненты цветов
    int r1 = GetRValue(colorStart), g1 = GetGValue(colorStart), b1 = GetBValue(colorStart);
    int r2 = GetRValue(colorEnd), g2 = GetGValue(colorEnd), b2 = GetBValue(colorEnd);
    
    // Рисуем градиент построчно
    for (int y = 0; y < height; y++) {
        // Вычисляем промежуточный цвет
        float ratio = (float)y / (float)height;
        int r = r1 + (int)((r2 - r1) * ratio);
        int g = g1 + (int)((g2 - g1) * ratio);
        int b = b1 + (int)((b2 - b1) * ratio);
        
        COLORREF color = RGB(r, g, b);
        HPEN pen = CreatePen(PS_SOLID, 1, color);
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        
        // Рисуем горизонтальную линию
        MoveToEx(hdc, rect.left, rect.top + y, NULL);
        LineTo(hdc, rect.right, rect.top + y);
        
        SelectObject(hdc, oldPen);
        DeleteObject(pen);
    }
}

void SchematicModule::RenderSchematic(HDC hdc, const RECT& rect) {
    // Используем двойную буферизацию для плавной отрисовки без мерцания
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    
    // Создаём буфер в памяти
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP oldBitmap = (HBITMAP)SelectObject(memDC, memBitmap);
    
    // Смещаем координаты для рисования в буфере
    SetViewportOrgEx(memDC, -rect.left, -rect.top, NULL);
    
    // Очищаем фон в буфере
    HBRUSH bgBrush = CreateSolidBrush(RGB(240, 240, 240));
    FillRect(memDC, &rect, bgBrush);
    DeleteObject(bgBrush);
    
    // Рассчитываем компоновку элементов (если элементы пустые ИЛИ рамка не инициализирована)
    if (schematicElements.empty() || 
        (deviceFrameRect.left == 0 && deviceFrameRect.top == 0 && 
         deviceFrameRect.right == 0 && deviceFrameRect.bottom == 0)) {
        CalculateSchematicLayout(rect);
    }
    
    // ОТЛАДКА: Выводим координаты рамки и элементов
    
    // Выводим координаты первых 3 элементов для отладки
    if (schematicElements.size() >= 3) {
        for (size_t i = 0; i < 3; i++) {
        }
    }
    
    // Отрисовываем рамку для основных элементов (T1-T5, R1-R2)
    if (deviceFrameRect.right > deviceFrameRect.left && 
        deviceFrameRect.bottom > deviceFrameRect.top) {
        DrawDeviceFrame(memDC, deviceFrameRect);
    } else {
    }
    
    // Отрисовываем все элементы схемы
    for (const auto& element : schematicElements) {
        std::wstring label = GetElementLabel(element.type, element.index);
        DrawElement(memDC, element.rect, element.type, element.index, element.selected, element.hovered, label);
    }
    
    // Отрисовываем соединительные линии между элементами
    DrawSchematicConnections(memDC, rect);
    
    // Добавляем подписи групп элементов
    DrawGroupLabels(memDC, rect);
    
    // Копируем буфер на экран
    BitBlt(hdc, rect.left, rect.top, width, height, memDC, rect.left, rect.top, SRCCOPY);
    
    // Очищаем ресурсы
    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
}

void SchematicModule::DrawElement(HDC hdc, const RECT& elementRect, ElementType type, 
                                 int index, bool selected, bool hovered, const std::wstring& label) {
    // === Корпоративный стиль: полная заливка цветом категории ===
    // Плоская заливка (без градиента/3D), аккуратный hover/selected, читаемый текст.

    auto Blend = [](COLORREF a, COLORREF b, double t) -> COLORREF {
        // a*(1-t) + b*t
        int ar = GetRValue(a), ag = GetGValue(a), ab = GetBValue(a);
        int br = GetRValue(b), bg = GetGValue(b), bb = GetBValue(b);
        int r = (int)std::lround(ar + (br - ar) * t);
        int g = (int)std::lround(ag + (bg - ag) * t);
        int bl = (int)std::lround(ab + (bb - ab) * t);
        r = (r < 0) ? 0 : (r > 255 ? 255 : r);
        g = (g < 0) ? 0 : (g > 255 ? 255 : g);
        bl = (bl < 0) ? 0 : (bl > 255 ? 255 : bl);
        return RGB(r, g, bl);
    };

    const COLORREF kWhite = RGB(255, 255, 255);
    const COLORREF kText = RGB(25, 25, 25);

    const bool isSupportedElement = (type != ElementType::TRANSMITTER) || SupportsTransmitterIndex(index);
    if (!isSupportedElement) {
        selected = false;
        hovered = false;
    }

    COLORREF marker = RGB(120, 120, 120);
    switch (type) {
        case ElementType::TRANSMITTER: marker = isSupportedElement ? TRANSMITTER_COLOR : RGB(170, 170, 170); break;
        case ElementType::RECEIVER:    marker = RECEIVER_COLOR; break;
        case ElementType::SILENT_TX:   marker = SILENT_TX_COLOR; break;
        default:                       marker = RGB(120, 120, 120); break;
    }

    // Полная заливка: базово цвет категории, hover — чуть светлее, selected — чуть темнее.
    COLORREF bg = marker;
    if (hovered) {
        bg = Blend(bg, kWhite, 0.14);     // подсветка
    }
    if (selected) {
        bg = Blend(bg, RGB(0, 0, 0), 0.12); // усиление выбранного
    }

    // Рамка: темнее заливки, в selected — толще и контрастнее.
    COLORREF borderColor = Blend(marker, RGB(0, 0, 0), selected ? 0.38 : 0.28);
    int borderWidth = selected ? 2 : 1;

    // Скругление: минимальное, "системное".
    const int cornerRadius = 6;

    // 1) Полная заливка + рамка одним проходом (без SelectClipRgn — чтобы не было "полос")
    {
        HBRUSH fillBrush = CreateSolidBrush(bg);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, fillBrush);

        HPEN pen = CreatePen(PS_SOLID, borderWidth, borderColor);
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);

        RoundRect(hdc,
                  elementRect.left, elementRect.top,
                  elementRect.right, elementRect.bottom,
                  cornerRadius, cornerRadius);

        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(pen);
        DeleteObject(fillBrush);
    }

    // 3) Дополнительная внутренняя рамка для selected (более "премиально" и читаемо)
    if (selected) {
        RECT inner = elementRect;
        InflateRect(&inner, -2, -2);
        COLORREF innerBorder = Blend(kWhite, marker, 0.30);

        HPEN pen = CreatePen(PS_SOLID, 1, innerBorder);
        HPEN oldPen = (HPEN)SelectObject(hdc, pen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));

        RoundRect(hdc, inner.left, inner.top, inner.right, inner.bottom, cornerRadius - 1, cornerRadius - 1);

        SelectObject(hdc, oldBrush);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);
    }

    // 4) Текст (системный шрифт).
    // Важно: для наших "приглушённых" фирменных цветов чёрный читается лучше и стабильнее.
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, isSupportedElement ? kText : RGB(95, 95, 95));

    // Жирный шрифт на базе системного Segoe UI (чтобы метки T1–T5/R1–R2 выделялись).
    static HFONT s_boldGuiFont = nullptr;
    if (!s_boldGuiFont) {
        LOGFONT lf = {};
        NONCLIENTMETRICS ncm = {};
        ncm.cbSize = sizeof(ncm);
        bool gotFont = false;
        if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0)) {
            lf = ncm.lfMessageFont;
            gotFont = true;
        }
        if (!gotFont) {
            HFONT base = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            if (base) GetObject(base, sizeof(lf), &lf);
        }
        lf.lfWeight = FW_BOLD;
        s_boldGuiFont = CreateFontIndirect(&lf);
        if (!s_boldGuiFont) {
            s_boldGuiFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
        }
    }

    HFONT oldFont = (HFONT)SelectObject(hdc, s_boldGuiFont);

    RECT textRect = elementRect;
    // Небольшой padding, чтобы текст не упирался в рамку
    textRect.left += 4;
    textRect.right -= 4;
    textRect.top += 2;
    textRect.bottom -= 2;

    if (type == ElementType::SILENT_TX) {
        // Для "Молч." используем 2 строки и центрируем блок текста по вертикали вручную.
        RECT calc = textRect;
        DrawText(hdc, label.c_str(), -1, &calc, DT_CENTER | DT_WORDBREAK | DT_CALCRECT);
        int textH = calc.bottom - calc.top;
        int rectH = textRect.bottom - textRect.top;
        if (rectH > textH) {
            int dy = (rectH - textH) / 2;
            textRect.top += dy;
        }
        DrawText(hdc, label.c_str(), -1, &textRect, DT_CENTER | DT_WORDBREAK);
    } else {
        DrawText(hdc, label.c_str(), -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
    }

    SelectObject(hdc, oldFont);
}

bool SchematicModule::HandleElementClick(int x, int y) {
    ElementRect* element = FindElementAt(x, y);
    if (!element) {
        return false;
    }

    if (element->type == ElementType::TRANSMITTER && !SupportsTransmitterIndex(element->index)) {
        return false;
    }
    
    // ОТЛАДКА: Выводим информацию о клике
    
    // Проверяем логику выбора элементов
    bool newState = !element->selected;
    
    // ИСПРАВЛЕНО: Применяем правила взаимоисключения ТОЛЬКО если выбираем новый элемент
    // И ТОЛЬКО если есть реальная несовместимость
    if (newState && !IsSelectionCompatible(element->type, element->index)) {
        
        // Очищаем несовместимые выборы
        ClearIncompatibleSelections(element->type);
    }
    
    // Обновляем состояние элемента
    element->selected = newState;
    UpdateSelection(element->type, element->index, newState);
    
    // При ручном выборе элементов сбрасываем фильтр на ALL
    currentSelection.dataFilter = DataTypeFilter::ALL;
    
    // v2.15.0: При ручном выборе элементов сбрасываем информацию о пресете
    currentPreset = PresetType::NONE;
    currentPresetDisplayName.clear();
    
    // Обновляем визуальное состояние всех элементов
    SynchronizeElementStates();
    
    // ОТЛАДКА: Выводим итоговое состояние
    for (int i = 0; i < SchematicSelection::UI_TRANSMITTER_COUNT; i++) {
    }
    for (int i = 0; i < SchematicSelection::UI_RECEIVER_COUNT; i++) {
    }
    
    return true;
}

void SchematicModule::UpdateSelection(ElementType type, int index, bool selected) {
    switch (type) {
        case ElementType::TRANSMITTER:
            if (SupportsTransmitterIndex(index) &&
                static_cast<size_t>(index) < currentSelection.transmitters.size()) {
                currentSelection.transmitters[index] = selected;
            }
            break;
        case ElementType::RECEIVER:
            if (IsUiReceiverIndex(index)) {
                currentSelection.receivers[index] = selected;
            }
            break;
        case ElementType::SILENT_TX:
            if (IsUiSilentIndex(index)) {
                currentSelection.silentTx[index] = selected;
            }
            break;
    }
}

void SchematicModule::UpdateHoverState(int x, int y) {
    // Находим элемент под курсором
    ElementRect* newHovered = FindElementAt(x, y);
    
    // Если hover состояние изменилось
    if (newHovered != hoveredElement) {
        // Сбрасываем предыдущий hover
        if (hoveredElement) {
            hoveredElement->hovered = false;
        }
        
        // Устанавливаем новый hover
        if (newHovered) {
            newHovered->hovered = true;
        }
        
        hoveredElement = newHovered;
        
        // Перерисовка будет вызвана из обработчика WM_MOUSEMOVE
    }
}

bool SchematicModule::ValidateCurrentSelection() {
    return currentSelection.IsValid();
}

SchematicSelection SchematicModule::GetCurrentSelection() const {
    return currentSelection;
}

void SchematicModule::SetCurrentSelection(const SchematicSelection& selection) {
    currentSelection = selection;
    NormalizeSelectionToAvailableTransmitters();
    
    // Обновляем состояние элементов схемы
    for (auto& element : schematicElements) {
        element.selected = element.type == ElementType::TRANSMITTER && !SupportsTransmitterIndex(element.index)
            ? false
            : IsSelectionElementSelected(currentSelection, element.type, element.index);
    }
}

void SchematicModule::SetCurveSelectionModule(CurveSelectionModule* module) {
    curveSelectionModule = module;
}

void SchematicModule::SetDataModule(DataModule* module) {
    dataModule = module;
}

void SchematicModule::SetMetrologyModule(MetrologyModule* module) {
    metrologyModule = module;
}

bool SchematicModule::IsRxPositionReversed() const {
    if (!metrologyModule) return false;
    if (!metrologyModule->HasData()) return false;
    return metrologyModule->Get().Rx_Position == 1;
}

int SchematicModule::GetAvailableTransmitterCount() const {
    if (!dataModule || !dataModule->HasData()) {
        return SchematicSelection::UI_TRANSMITTER_COUNT;
    }

    const int transmitterCount = dataModule->GetDataView(0).GetTransmitterCount();
    if (transmitterCount <= 0) {
        return SchematicSelection::UI_TRANSMITTER_COUNT;
    }

    return std::min(transmitterCount, SchematicSelection::UI_TRANSMITTER_COUNT);
}

bool SchematicModule::SupportsTransmitterIndex(int index) const {
    return index >= 0 && index < GetAvailableTransmitterCount();
}

void SchematicModule::NormalizeSelectionToAvailableTransmitters() {
    const size_t transmitterCount = static_cast<size_t>(GetAvailableTransmitterCount());
    currentSelection.transmitters.resize(transmitterCount, false);
}

void SchematicModule::InitializeSchematicElements(const RECT& schemaRect) {
    schematicElements.clear();
    
    // TODO: Реализовать инициализацию элементов схемы
    // Пока что создаем заглушки для элементов
    
    // Добавляем передатчики T1-T5
    for (int i = 0; i < SchematicSelection::UI_TRANSMITTER_COUNT; i++) {
        RECT rect = CalculateElementRect(schemaRect, ElementType::TRANSMITTER, i);
        schematicElements.emplace_back(rect, ElementType::TRANSMITTER, i);
    }
    
    // Добавляем приемники R1-R2
    for (int i = 0; i < SchematicSelection::UI_RECEIVER_COUNT; i++) {
        RECT rect = CalculateElementRect(schemaRect, ElementType::RECEIVER, i);
        schematicElements.emplace_back(rect, ElementType::RECEIVER, i);
    }
    
    // Добавляем молчащие передатчики
    for (int i = 0; i < SchematicSelection::UI_SILENT_COUNT; i++) {
        RECT rect = CalculateElementRect(schemaRect, ElementType::SILENT_TX, i);
        schematicElements.emplace_back(rect, ElementType::SILENT_TX, i);
    }
}

SchematicModule::ElementRect* SchematicModule::FindElementAt(int x, int y) {
    POINT pt = {x, y};
    for (auto& element : schematicElements) {
        if (PtInRect(&element.rect, pt)) {
            return &element;
        }
    }
    return nullptr;
}

std::wstring SchematicModule::GetElementLabel(ElementType type, int index) const {
    if (IsRxPositionReversed()) {
        if (type == ElementType::RECEIVER) {
            return BuildElementLabel(type, 1 - index);
        }
        if (type == ElementType::SILENT_TX) {
            return BuildElementLabel(type, 1 - index);
        }
    }
    return BuildElementLabel(type, index);
}

std::wstring SchematicModule::GetElementTooltip(ElementType type, int index) const {
    if (IsRxPositionReversed()) {
        if (type == ElementType::RECEIVER) {
            return BuildElementTooltip(type, 1 - index);
        }
        if (type == ElementType::SILENT_TX) {
            return BuildElementTooltip(type, 1 - index);
        }
    }
    return BuildElementTooltip(type, index);
}

void SchematicModule::DrawTransmitter(HDC hdc, const RECT& rect, int index, bool selected) {
    std::wstring label = BuildElementLabel(ElementType::TRANSMITTER, index);
    DrawElement(hdc, rect, ElementType::TRANSMITTER, index, selected, false, label);
}

void SchematicModule::DrawReceiver(HDC hdc, const RECT& rect, int index, bool selected) {
    std::wstring label = GetElementLabel(ElementType::RECEIVER, index);
    DrawElement(hdc, rect, ElementType::RECEIVER, index, selected, false, label);
}

void SchematicModule::DrawSilentTransmitter(HDC hdc, const RECT& rect, int index, bool selected) {
    std::wstring label = GetElementLabel(ElementType::SILENT_TX, index);
    DrawElement(hdc, rect, ElementType::SILENT_TX, index, selected, false, label);
}

void SchematicModule::DrawSelectionHighlight(HDC hdc, const RECT& rect) {
    // Создаем анимированную рамку выделения
    HPEN highlightPen = CreatePen(PS_SOLID, 4, SELECTION_COLOR);
    HPEN oldPen = (HPEN)SelectObject(hdc, highlightPen);
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    
    // Рисуем внешнюю рамку выделения
    int offset = 3;
    RoundRect(hdc, rect.left - offset, rect.top - offset, 
              rect.right + offset, rect.bottom + offset, 12, 12);
    
    // Добавляем внутреннюю рамку для большей заметности
    HPEN innerPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255)); // Белая внутренняя рамка
    SelectObject(hdc, innerPen);
    
    offset = 1;
    RoundRect(hdc, rect.left - offset, rect.top - offset, 
              rect.right + offset, rect.bottom + offset, 10, 10);
    
    // Восстанавливаем объекты GDI
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(highlightPen);
    DeleteObject(innerPen);
}

RECT SchematicModule::CalculateElementRect(const RECT& schemaRect, ElementType type, int index) {
    RECT rect;
    const int elementWidth = kSchematicElementWidth;
    const int elementHeight = kSchematicElementHeight;
    const int spacing = kSchematicElementSpacing;
    const int frameToSilentSpacing = kFrameToSilentSpacing;
    
    // Рассчитываем ширину рамки (7 элементов: T1-T5, R1-R2)
    const int frameElementsCount = kSchematicFrameElementsCount;
    const int frameWidth = frameElementsCount * elementWidth + (frameElementsCount - 1) * spacing + 2 * FRAME_PADDING;
    
    // Рассчитываем общую ширину (рамка + отступ + 2 молчащих)
    const int totalWidth = frameWidth + frameToSilentSpacing + 2 * elementWidth + spacing;
    
    // Центрируем всю компоновку
    const int startX = schemaRect.left + (schemaRect.right - schemaRect.left - totalWidth) / 2;
    const int centerY = schemaRect.top + (schemaRect.bottom - schemaRect.top - elementHeight) / 2;
    
    // Определяем позицию элемента в горизонтальной линии
    // ПРАВИЛЬНЫЙ порядок: T5(0), T3(1), T1(2), R1(3), R2(4), T2(5), T4(6), Silent R1(7), Silent R2(8)
    int positionIndex = -1;
    int currentX = startX + FRAME_PADDING;  // Начинаем внутри рамки
    
    switch (type) {
        case ElementType::TRANSMITTER:
        case ElementType::RECEIVER:
            positionIndex = GetSchematicFramePositionIndex(type, index);
            if (positionIndex >= 0) {
                currentX += positionIndex * (elementWidth + spacing);
            }
            break;
            
        case ElementType::SILENT_TX:
            {
                // Молчащие передатчики вне рамки (позиции 7-8)
                positionIndex = frameElementsCount + index;
                // Начинаем после рамки
                currentX = startX + frameWidth + frameToSilentSpacing + index * (elementWidth + spacing);
            }
            break;
    }
    
    // Вычисляем координаты
    if (positionIndex >= 0) {
        rect.left = currentX;
        rect.top = centerY;
        rect.right = rect.left + elementWidth;
        rect.bottom = rect.top + elementHeight;
    } else {
        // Fallback на случай ошибки
        rect.left = schemaRect.left + 20;
        rect.top = centerY;
        rect.right = rect.left + elementWidth;
        rect.bottom = rect.top + elementHeight;
    }
    
    return rect;
}

void SchematicModule::CalculateSchematicLayout(const RECT& schemaRect) {
    // ОТЛАДКА
    
    InitializeSchematicElements(schemaRect);
    
    // Рассчитываем координаты рамки для основных элементов (T1-T5, R1-R2)
    // Рамка должна охватывать элементы с позициями 0-6 (T5, T3, T1, R1, R2, T2, T4)
    
    if (schematicElements.size() >= 7) {
        
        // Находим минимальные и максимальные координаты для первых 7 элементов
        int minX = INT_MAX, minY = INT_MAX;
        int maxX = INT_MIN, maxY = INT_MIN;
        
        for (size_t i = 0; i < 7; i++) {
            const RECT& rect = schematicElements[i].rect;
            
            minX = (minX < (int)rect.left) ? minX : (int)rect.left;
            minY = (minY < (int)rect.top) ? minY : (int)rect.top;
            maxX = (maxX > (int)rect.right) ? maxX : (int)rect.right;
            maxY = (maxY > (int)rect.bottom) ? maxY : (int)rect.bottom;
        }
        
        // Добавляем отступы для рамки
        deviceFrameRect.left = minX - FRAME_PADDING;
        deviceFrameRect.top = minY - FRAME_PADDING;
        deviceFrameRect.right = maxX + FRAME_PADDING;
        deviceFrameRect.bottom = maxY + FRAME_PADDING;
    } else {
        // Если элементов меньше 7, рамка не отрисовывается
        deviceFrameRect = {0, 0, 0, 0};
    }
    
    // ИСПРАВЛЕНИЕ: Синхронизируем состояние элементов с currentSelection
    // После создания элементов нужно восстановить их состояние selected
    for (auto& element : schematicElements) {
        element.selected = element.type == ElementType::TRANSMITTER && !SupportsTransmitterIndex(element.index)
            ? false
            : IsSelectionElementSelected(currentSelection, element.type, element.index);
        if (element.selected) {
        }
    }
}

// Методы для работы с диалоговым окном
void SchematicModule::InitializeDialogControls(HWND hDlg) {
    NormalizeSelectionToAvailableTransmitters();

    // Пытаемся восстановить текущую конфигурацию графика
    bool hasExistingConfig = false;
    ChartConfiguration existingConfig; // Объявляем вне блока для использования позже
    
    if (curveSelectionModule && curveSelectionModule->HasConfiguration(targetChartNumber)) {
        // Получаем текущую конфигурацию графика
        existingConfig = curveSelectionModule->GetChartConfiguration(targetChartNumber);
        
        if (!existingConfig.IsEmpty()) {
            hasExistingConfig = true;
            
            // Очищаем текущий выбор
            currentSelection.Clear();
            NormalizeSelectionToAvailableTransmitters();
            
            // Восстанавливаем частоту из первой кривой
            if (!existingConfig.curves.empty()) {
                currentSelection.frequency = existingConfig.curves[0].frequency;
            }
            
            // Определяем фильтр типов данных по существующим кривым
            currentSelection.dataFilter = DetermineDataFilterFromCurves(existingConfig.curves);

            for (const auto& curve : existingConfig.curves) {
                if (curve.dataType == DataType::RHO_SMT) {
                    currentSelection.rhoSmtMode = curve.smtMode;
                } else if (curve.dataType == DataType::PHASE_SMT) {
                    currentSelection.phaseSmtMode = curve.smtMode;
                }
            }
            
            // ВАЖНО: Восстанавливаем элементы схемы ТОЛЬКО для режима ручного выбора (dataFilter == ALL)
            // Для пресетов (dataFilter != ALL) элементы схемы должны оставаться неактивными
            if (currentSelection.dataFilter == DataTypeFilter::ALL) {
                // Режим ручного выбора - восстанавливаем элементы схемы из кривых
                
                // ОТЛАДКА: Выводим информацию о восстанавливаемых кривых
                
                // ИСПРАВЛЕНИЕ: Сначала анализируем типы данных кривых для определения выбора
                // Ключевое отличие между выбором приёмников и передатчиков:
                // - Если выбран приёмник R1/R2: кривые для ВСЕХ 5 передатчиков (T1-T5)
                // - Если выбраны передатчики: кривые только для ВЫБРАННЫХ передатчиков
                
                bool hasRX1Types = false, hasRX2Types = false;
                bool hasRatioOrDiff = false;
                bool hasTransmitterSource = false;
                bool hasSilentSource = false;
                
                // Подсчитываем уникальные передатчики в кривых
                std::set<int> uniqueTransmitters;
                
                for (const auto& curve : existingConfig.curves) {
                    // Анализируем тип данных
                    if (curve.dataType == DataType::AMPLITUDE_RX1) {
                        hasRX1Types = true;
                    }
                    if (curve.dataType == DataType::AMPLITUDE_RX2) {
                        hasRX2Types = true;
                    }
                    if (curve.dataType == DataType::AMPLITUDE_RATIO || curve.dataType == DataType::PHASE_DIFFERENCE) {
                        hasRatioOrDiff = true;
                    }
                    
                    // Анализируем источник
                    if (curve.sourceElement == ElementType::TRANSMITTER) {
                        hasTransmitterSource = true;
                        uniqueTransmitters.insert(curve.elementIndex);
                    }
                    if (curve.sourceElement == ElementType::SILENT_TX) {
                        hasSilentSource = true;
                    }
                }
                
                // Определяем, что было выбрано на схеме
                if (hasRatioOrDiff) {
                    // Выбраны оба приемника R1+R2
                    SelectAllUiReceivers(currentSelection);
                }
                else if (hasRX1Types && !hasRX2Types && hasTransmitterSource &&
                         HasAllUiTransmittersSelected(uniqueTransmitters, currentSelection.transmitters.size())) {
                    // Выбран только приемник R1 (кривые для ВСЕХ 5 передатчиков с R1)
                    SelectUiReceiver(currentSelection, 0);
                }
                else if (hasRX2Types && !hasRX1Types && hasTransmitterSource &&
                         HasAllUiTransmittersSelected(uniqueTransmitters, currentSelection.transmitters.size())) {
                    // Выбран только приемник R2 (кривые для ВСЕХ 5 передатчиков с R2)
                    SelectUiReceiver(currentSelection, 1);
                }
                else if (hasRX1Types && hasRX2Types && hasTransmitterSource &&
                         !HasAllUiTransmittersSelected(uniqueTransmitters, currentSelection.transmitters.size())) {
                    // Выбран один или несколько передатчиков (кривые для R1 и R2, но НЕ для всех 5 передатчиков)
                    // Восстанавливаем передатчики из sourceElement
                    RestoreSelectedTransmittersFromCurves(existingConfig, currentSelection);
                }
                else if (hasRX1Types && hasRX2Types && hasTransmitterSource &&
                         HasAllUiTransmittersSelected(uniqueTransmitters, currentSelection.transmitters.size())) {
                    // Особый случай: есть кривые для всех 5 передатчиков с обоими приёмниками
                    // Это может быть либо выбор всех 4 передатчиков, либо какая-то другая комбинация
                    // Проверяем количество кривых: если их 16 (4 передатчика × 2 приёмника × 2 типа данных), 
                    // то скорее всего выбраны все передатчики
                    if (existingConfig.curves.size() >=
                        (currentSelection.transmitters.size() *
                         static_cast<size_t>(SchematicSelection::UI_RECEIVER_COUNT) * 2)) {
                        // Восстанавливаем все передатчики
                        SelectAllUiTransmitters(currentSelection);
                    } else {
                        // Восстанавливаем только те передатчики, которые есть в кривых
                        RestoreSelectedTransmittersFromCurves(existingConfig, currentSelection);
                    }
                }
                
                // Восстанавливаем молчащие передатчики
                if (hasSilentSource) {
                    RestoreSelectedSilentTransmittersFromCurves(existingConfig, currentSelection);
                }
                
                // ОТЛАДКА: Итоговое состояние выбора
                for (int i = 0; i < SchematicSelection::UI_TRANSMITTER_COUNT; i++) {
                }
                for (int i = 0; i < SchematicSelection::UI_RECEIVER_COUNT; i++) {
                }
            }
            else {
                // Для пресетов (dataFilter != ALL) элементы схемы НЕ подсвечиваются,
                // НО нужно восстановить информацию о приемниках для генерации кривых
                // Определяем приемники по типу данных в кривых
                for (const auto& curve : existingConfig.curves) {
                    switch (curve.dataType) {
                        case DataType::AMPLITUDE_RX1:
                            SelectUiReceiver(currentSelection, 0);
                            break;
                        case DataType::AMPLITUDE_RX2:
                            SelectUiReceiver(currentSelection, 1);
                            break;
                        case DataType::AMPLITUDE_RATIO:
                        case DataType::PHASE_DIFFERENCE:
                            // Для отношений и разностей нужны оба приемника
                            SelectAllUiReceivers(currentSelection);
                            break;
                        default:
                            break;
                    }
                }
            }
            
            // Сохраняем кривые с их состоянием enabled
            currentAvailableCurves = existingConfig.curves;
        }
    }
    
    InitializeSmtModeCheckboxes(hDlg);

    // Инициализируем комбобокс частоты
    HWND hFreqCombo = GetDlgItem(hDlg, IDC_FREQUENCY_COMBO);
    if (hFreqCombo) {
        // Локализованные подписи частот (кГц)
        SendMessage(hFreqCombo, CB_ADDSTRING, 0, (LPARAM)L"400 кГц");
        SendMessage(hFreqCombo, CB_ADDSTRING, 0, (LPARAM)L"2000 кГц");
        
        // Устанавливаем частоту из восстановленной конфигурации или по умолчанию
        int freqIndex = (currentSelection.frequency == FrequencyBand::FREQ_400_KHZ) ? 0 : 1;
        SendMessage(hFreqCombo, CB_SETCURSEL, freqIndex, 0);
    }
    
    // Инициализируем комбобокс типа оси X (v2.10.0)
    HWND hXAxisCombo = GetDlgItem(hDlg, IDC_XAXIS_TYPE_COMBO);
    if (hXAxisCombo) {
        // "Номер кадра" в Unicode
        SendMessage(hXAxisCombo, CB_ADDSTRING, 0, (LPARAM)L"\x041D\x043E\x043C\x0435\x0440 \x043A\x0430\x0434\x0440\x0430");
        // "Температура" в Unicode
        SendMessage(hXAxisCombo, CB_ADDSTRING, 0, (LPARAM)L"\x0422\x0435\x043C\x043F\x0435\x0440\x0430\x0442\x0443\x0440\x0430");
        
        // Устанавливаем тип оси X из восстановленной конфигурации или по умолчанию
        XAxisType xAxisType = XAxisType::FRAME_NUMBER;
        if (hasExistingConfig && !existingConfig.IsEmpty()) {
            xAxisType = existingConfig.xAxisType;
        }
        int xAxisIndex = (xAxisType == XAxisType::FRAME_NUMBER) ? 0 : 1;
        SendMessage(hXAxisCombo, CB_SETCURSEL, xAxisIndex, 0);
    }
    
    // Инициализируем область схемы с программной подгонкой размеров
    HWND hSchematicArea = GetDlgItem(hDlg, IDC_SCHEMATIC_AREA);
    if (hSchematicArea) {
        // Шаг 1: вычисляем точный размер контента в пикселях из констант
        const int contentW = kSchematicFrameElementsCount * kSchematicElementWidth
            + (kSchematicFrameElementsCount - 1) * kSchematicElementSpacing
            + 2 * FRAME_PADDING
            + kFrameToSilentSpacing
            + 2 * kSchematicElementWidth + kSchematicElementSpacing;
        const int contentH = kSchematicElementHeight + 2 * (FRAME_PADDING + FRAME_BORDER_WIDTH);

        // Шаг 2: параметры компактного layout
        const int areaPad = 6;            // padding внутри area вокруг контента
        const int gbTop = 2;              // верхний отступ groupbox от края диалога
        const int gbTitleH = 16;          // высота для заголовка groupbox + линия рамки
        const int gbMarginX = 4;          // боковой отступ groupbox от area
        const int gbMarginBottom = 4;     // нижний отступ внутри groupbox под area
        const int newAreaW = contentW + 2 * areaPad;
        const int newAreaH = contentH + 2 * areaPad;

        // Шаг 3: сохраняем оригинальное положение groupbox ДО ресайза
        HWND hGroupBox = GetDlgItem(hDlg, IDC_SCHEMATIC_GROUPBOX);
        int origGbBottom = 0;
        if (hGroupBox) {
            RECT origGbRect;
            GetWindowRect(hGroupBox, &origGbRect);
            MapWindowPoints(HWND_DESKTOP, hDlg, (POINT*)&origGbRect, 2);
            origGbBottom = origGbRect.bottom;
        }

        // Шаг 4: позиционируем groupbox и area с минимальными отступами
        RECT dlgClient;
        GetClientRect(hDlg, &dlgClient);
        const int newAreaX = (dlgClient.right - newAreaW) / 2;
        const int newAreaY = gbTop + gbTitleH;

        MoveWindow(hSchematicArea, newAreaX, newAreaY, newAreaW, newAreaH, TRUE);

        int newGbBottom = 0;
        if (hGroupBox) {
            const int gbH = gbTitleH + newAreaH + gbMarginBottom;
            MoveWindow(hGroupBox,
                newAreaX - gbMarginX,
                gbTop,
                newAreaW + 2 * gbMarginX,
                gbH,
                TRUE);
            newGbBottom = gbTop + gbH;
        }

        // Шаг 5: сдвигаем ВСЕ контролы ниже оригинального groupbox вверх
        if (origGbBottom > 0 && newGbBottom > 0 && newGbBottom < origGbBottom) {
            const int shiftUp = origGbBottom - newGbBottom;

            HWND hChild = GetWindow(hDlg, GW_CHILD);
            while (hChild) {
                if (hChild != hSchematicArea && hChild != hGroupBox) {
                    RECT cr;
                    GetWindowRect(hChild, &cr);
                    MapWindowPoints(HWND_DESKTOP, hDlg, (POINT*)&cr, 2);
                    if (cr.top >= origGbBottom) {
                        MoveWindow(hChild, cr.left, cr.top - shiftUp,
                            cr.right - cr.left, cr.bottom - cr.top, TRUE);
                    }
                }
                hChild = GetWindow(hChild, GW_HWNDNEXT);
            }

            // Уменьшаем высоту диалога
            RECT dlgRect;
            GetWindowRect(hDlg, &dlgRect);
            SetWindowPos(hDlg, NULL, 0, 0,
                dlgRect.right - dlgRect.left,
                dlgRect.bottom - dlgRect.top - shiftUp,
                SWP_NOMOVE | SWP_NOZORDER);
        }

        // Шаг 6: переинициализируем элементы с новым размером area
        RECT newSchematicRect;
        GetClientRect(hSchematicArea, &newSchematicRect);
        InitializeSchematicElements(newSchematicRect);
        
        // Синхронизируем визуальное состояние элементов с восстановленным выбором
        if (hasExistingConfig) {
            // Для пресетов (dataFilter != ALL) элементы схемы НЕ подсвечиваются
            if (currentSelection.dataFilter == DataTypeFilter::ALL) {
                // Режим ручного выбора - синхронизируем элементы схемы
                SetCurrentSelection(currentSelection);
            }
            else {
                // Режим пресета - все элементы схемы неактивны
                for (auto& element : schematicElements) {
                    element.selected = false;
                }
            }
        }
    }
    
    // Обновляем список доступных кривых
    UpdateAvailableCurves(hDlg);
}

void SchematicModule::UpdateAvailableCurves(HWND hDlg) {
    HWND hCurveList = GetDlgItem(hDlg, IDC_CURVE_LIST);
    if (!hCurveList) return;

    // Сбрасываем сохранённые кривые и состояние кнопки "Применить" заранее,
    // чтобы не оставлять "залипшее" состояние после ранних return.
    currentAvailableCurves.clear();
    if (HWND hApplyBtn = GetDlgItem(hDlg, IDC_APPLY_SETTINGS)) {
        EnableWindow(hApplyBtn, FALSE);
    }
    
    // Очищаем ListView (удаляем все элементы и колонки)
    ListView_DeleteAllItems(hCurveList);
    
    // Удаляем все существующие колонки
    int columnCount = Header_GetItemCount(ListView_GetHeader(hCurveList));
    for (int i = columnCount - 1; i >= 0; i--) {
        ListView_DeleteColumn(hCurveList, i);
    }
    
    if (!currentSelection.IsValid()) {
        // Показываем сообщение об ошибке выбора
        std::string error = currentSelection.GetValidationError();
        
        // Преобразуем UTF-8 строку в wstring
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, error.c_str(), (int)error.size(), NULL, 0);
        std::wstring werror(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, error.c_str(), (int)error.size(), &werror[0], size_needed);
        
        // Создаём одну колонку для сообщения об ошибке
        // "Сообщение" в Unicode
        LVCOLUMN column = {};
        column.mask = LVCF_TEXT | LVCF_WIDTH;
        column.cx = 350;
        column.pszText = const_cast<LPWSTR>(L"\x0421\x043E\x043E\x0431\x0449\x0435\x043D\x0438\x0435");
        ListView_InsertColumn(hCurveList, 0, &column);
        
        LVITEM item = {};
        item.mask = LVIF_TEXT;
        item.iItem = 0;
        item.iSubItem = 0;
        item.pszText = const_cast<LPWSTR>(werror.c_str());
        ListView_InsertItem(hCurveList, &item);

        // Обновляем заголовок окна (добавляем/оставляем признак ошибки)
        ValidateCurrentCurveConfiguration(hDlg);
        return;
    }
    
    if (!curveSelectionModule) {
        // Создаём одну колонку для сообщения об ошибке
        // "Сообщение" в Unicode
        LVCOLUMN column = {};
        column.mask = LVCF_TEXT | LVCF_WIDTH;
        column.cx = 350;
        column.pszText = const_cast<LPWSTR>(L"\x0421\x043E\x043E\x0431\x0449\x0435\x043D\x0438\x0435");
        ListView_InsertColumn(hCurveList, 0, &column);
        
        LVITEM item = {};
        item.mask = LVIF_TEXT;
        item.iItem = 0;
        item.iSubItem = 0;
        // "Модуль выбора кривых не инициализирован" в Unicode
        item.pszText = const_cast<LPWSTR>(L"\x041C\x043E\x0434\x0443\x043B\x044C \x0432\x044B\x0431\x043E\x0440\x0430 \x043A\x0440\x0438\x0432\x044B\x0445 \x043D\x0435 \x0438\x043D\x0438\x0446\x0438\x0430\x043B\x0438\x0437\x0438\x0440\x043E\x0432\x0430\x043D");
        ListView_InsertItem(hCurveList, &item);

        ValidateCurrentCurveConfiguration(hDlg);
        return;
    }
    
    // Получаем доступные кривые на основе текущего выбора
    std::vector<CurveDefinition> availableCurves = curveSelectionModule->GetAvailableCurves(currentSelection);
    
    // Если у нас уже есть сохранённые кривые (из существующей конфигурации),
    // синхронизируем состояние enabled с новыми доступными кривыми
    if (!currentAvailableCurves.empty()) {
        // Создаём карту ID -> enabled для быстрого поиска
        std::map<std::string, bool> savedEnabledStates;
        for (const auto& savedCurve : currentAvailableCurves) {
            savedEnabledStates[savedCurve.id] = savedCurve.enabled;
        }
        
        // Применяем сохранённые состояния к новым кривым
        for (auto& curve : availableCurves) {
            auto it = savedEnabledStates.find(curve.id);
            if (it != savedEnabledStates.end()) {
                curve.enabled = it->second;
            }
        }
    }
    
    if (availableCurves.empty()) {
        // Создаём одну колонку для сообщения
        // "Сообщение" в Unicode
        LVCOLUMN column = {};
        column.mask = LVCF_TEXT | LVCF_WIDTH;
        column.cx = 350;
        column.pszText = const_cast<LPWSTR>(L"\x0421\x043E\x043E\x0431\x0449\x0435\x043D\x0438\x0435");
        ListView_InsertColumn(hCurveList, 0, &column);
        
        LVITEM item = {};
        item.mask = LVIF_TEXT;
        item.iItem = 0;
        item.iSubItem = 0;
        item.pszText = const_cast<LPWSTR>(L"\x041D\x0435\x0442 \x0434\x043E\x0441\x0442\x0443\x043F\x043D\x044B\x0445 \x043A\x0440\x0438\x0432\x044B\x0445 \x0434\x043B\x044F \x0442\x0435\x043A\x0443\x0449\x0435\x0433\x043E \x0432\x044B\x0431\x043E\x0440\x0430");
        ListView_InsertItem(hCurveList, &item);

        ValidateCurrentCurveConfiguration(hDlg);
        return;
    }
    
    // Настраиваем ListView для чекбоксов и группировки
    ListView_SetExtendedListViewStyle(hCurveList, LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
    
    // Включаем группировку в ListView
    ListView_EnableGroupView(hCurveList, TRUE);
    
    // Группируем кривые по элементам (T1-T5, R1-R2)
    std::map<int, std::vector<size_t>> curvesByElement; // Ключ: elementIndex, Значение: индексы кривых
    std::vector<int> elementOrder; // Порядок элементов
    
    for (size_t i = 0; i < availableCurves.size(); i++) {
        const auto& curve = availableCurves[i];
        int elementKey = curve.elementIndex;
        
        if (curvesByElement.find(elementKey) == curvesByElement.end()) {
            elementOrder.push_back(elementKey);
        }
        
        curvesByElement[elementKey].push_back(i);
    }
    
    int numElements = static_cast<int>(elementOrder.size());
    int selectedReceivers = currentSelection.GetSelectedReceiversCount();
    
    // Создаём одну колонку для всех кривых
    LVCOLUMN column = {};
    column.mask = LVCF_TEXT | LVCF_WIDTH;
    column.cx = 400;
    column.pszText = const_cast<LPWSTR>(L"\x0414\x043E\x0441\x0442\x0443\x043F\x043D\x044B\x0435 \x043A\x0440\x0438\x0432\x044B\x0435");
    ListView_InsertColumn(hCurveList, 0, &column);
    
    // ИЗМЕНЕНО v2.6.2: Для одного приёмника (R1 или R2) отображаем кривые в плоском списке
    // без группировки, чтобы сохранить порядок: сначала все фазы, потом все амплитуды
    if (selectedReceivers == 1) {
        // Отключаем группировку для этого случая
        ListView_EnableGroupView(hCurveList, FALSE);
        
        // Добавляем все кривые без группировки в том порядке, в котором они пришли
        for (size_t i = 0; i < availableCurves.size(); i++) {
            const auto& curve = availableCurves[i];
            
            std::wstring displayText = Utf8ToWide(curve.displayName);
            displayText += L" (";
            displayText += Utf8ToWide(curve.units);
            displayText += L")";
            
            if (curve.yAxisGroup == 0) {
                displayText += L" [Левая ось]";
            } else {
                displayText += L" [Правая ось]";
            }
            
            LVITEM item = {};
            item.mask = LVIF_TEXT | LVIF_PARAM;
            item.iItem = static_cast<int>(i);
            item.iSubItem = 0;
            item.pszText = const_cast<LPWSTR>(displayText.c_str());
            item.lParam = static_cast<LPARAM>(i);
            
            int itemIndex = ListView_InsertItem(hCurveList, &item);
            ListView_SetCheckState(hCurveList, itemIndex, curve.enabled ? TRUE : FALSE);
        }
    }
    // Специальный случай: R1 + R2 (Ratios & Differences) - без группировки
    else if (selectedReceivers == 2 && numElements > 1) {
        // Отключаем группировку для этого случая
        ListView_EnableGroupView(hCurveList, FALSE);
        
        // Добавляем все кривые без группировки
        for (size_t i = 0; i < availableCurves.size(); i++) {
            const auto& curve = availableCurves[i];
            
            std::wstring displayText = Utf8ToWide(curve.displayName);
            displayText += L" (";
            displayText += Utf8ToWide(curve.units);
            displayText += L")";
            
            if (curve.yAxisGroup == 0) {
                displayText += L" [Левая ось]";
            } else {
                displayText += L" [Правая ось]";
            }
            
            LVITEM item = {};
            item.mask = LVIF_TEXT | LVIF_PARAM;
            item.iItem = static_cast<int>(i);
            item.iSubItem = 0;
            item.pszText = const_cast<LPWSTR>(displayText.c_str());
            item.lParam = static_cast<LPARAM>(i);
            
            int itemIndex = ListView_InsertItem(hCurveList, &item);
            ListView_SetCheckState(hCurveList, itemIndex, curve.enabled ? TRUE : FALSE);
        }
    }
    // Один элемент - без группировки
    else if (numElements == 1) {
        // Отключаем группировку
        ListView_EnableGroupView(hCurveList, FALSE);
        
        int elementIndex = elementOrder[0];
        
        for (size_t curveIdx : curvesByElement[elementIndex]) {
            const auto& curve = availableCurves[curveIdx];
            
            std::wstring displayText = Utf8ToWide(curve.displayName);
            displayText += L" (";
            displayText += Utf8ToWide(curve.units);
            displayText += L")";
            
            if (curve.yAxisGroup == 0) {
                displayText += L" [Левая ось]";
            } else {
                displayText += L" [Правая ось]";
            }
            
            LVITEM item = {};
            item.mask = LVIF_TEXT | LVIF_PARAM;
            item.iItem = static_cast<int>(curveIdx);
            item.iSubItem = 0;
            item.pszText = const_cast<LPWSTR>(displayText.c_str());
            item.lParam = static_cast<LPARAM>(curveIdx);
            
            int itemIndex = ListView_InsertItem(hCurveList, &item);
            ListView_SetCheckState(hCurveList, itemIndex, curve.enabled ? TRUE : FALSE);
        }
    }
    // Несколько элементов - используем группировку
    else {
        // Создаём группы для каждого элемента
        for (int groupIndex = 0; groupIndex < numElements; groupIndex++) {
            int elementIndex = elementOrder[groupIndex];
            size_t firstCurveIdx = curvesByElement[elementIndex][0];
            ElementType elementType = availableCurves[firstCurveIdx].sourceElement;
            
            // Определяем заголовок группы
            std::wstring groupHeader;
            switch (elementType) {
                case ElementType::TRANSMITTER:
                    groupHeader = GetElementLabel(ElementType::TRANSMITTER, elementIndex);
                    break;
                case ElementType::RECEIVER:
                    groupHeader = GetElementLabel(ElementType::RECEIVER, elementIndex);
                    break;
                case ElementType::SILENT_TX:
                    groupHeader = GetElementLabel(ElementType::SILENT_TX, elementIndex);
                    break;
                default:
                    // "Группа" в Unicode
                    groupHeader = L"\x0413\x0440\x0443\x043F\x043F\x0430 " + std::to_wstring(groupIndex + 1);
            }
            
            // Создаём группу
            LVGROUP group = {};
            group.cbSize = sizeof(LVGROUP);
            group.mask = LVGF_HEADER | LVGF_GROUPID | LVGF_STATE;
            group.pszHeader = const_cast<LPWSTR>(groupHeader.c_str());
            group.iGroupId = groupIndex;
            group.state = LVGS_NORMAL | LVGS_COLLAPSIBLE;
            group.stateMask = LVGS_NORMAL | LVGS_COLLAPSIBLE;
            
            ListView_InsertGroup(hCurveList, groupIndex, &group);
        }
        
        // Добавляем кривые в соответствующие группы
        int globalItemIndex = 0;
        for (int groupIndex = 0; groupIndex < numElements; groupIndex++) {
            int elementIndex = elementOrder[groupIndex];
            const auto& curvesInGroup = curvesByElement[elementIndex];
            
            for (size_t curveIdx : curvesInGroup) {
                const auto& curve = availableCurves[curveIdx];
                
                std::wstring displayText = Utf8ToWide(curve.displayName);
                displayText += L" (";
                displayText += Utf8ToWide(curve.units);
                displayText += L")";
                
                if (curve.yAxisGroup == 0) {
                    displayText += L" [Левая ось]";
                } else {
                    displayText += L" [Правая ось]";
                }
                
                LVITEM item = {};
                item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_GROUPID;
                item.iItem = globalItemIndex;
                item.iSubItem = 0;
                item.pszText = const_cast<LPWSTR>(displayText.c_str());
                item.lParam = static_cast<LPARAM>(curveIdx);
                item.iGroupId = groupIndex;
                
                int itemIndex = ListView_InsertItem(hCurveList, &item);
                ListView_SetCheckState(hCurveList, itemIndex, curve.enabled ? TRUE : FALSE);
                
                globalItemIndex++;
            }
        }
    }
    
    // Сохраняем доступные кривые для последующего использования
    currentAvailableCurves = availableCurves;

    // ВАЖНО: после обновления списка кривых обязательно пересчитываем валидность конфигурации,
    // иначе кнопка "Применить" может остаться выключенной после предыдущего невалидного выбора
    // (например, после снятия молчащих передатчиков).
    ValidateCurrentCurveConfiguration(hDlg);
}

void SchematicModule::HandleFrequencyChange(HWND hDlg) {
    HWND hFreqCombo = GetDlgItem(hDlg, IDC_FREQUENCY_COMBO);
    if (hFreqCombo) {
        int selection = (int)SendMessage(hFreqCombo, CB_GETCURSEL, 0, 0);
        currentSelection.frequency = (selection == 0) ? FrequencyBand::FREQ_400_KHZ : FrequencyBand::FREQ_2000_KHZ;
        
        // При изменении частоты сохраняем текущий фильтр (не сбрасываем)
        // Это позволяет пользователю менять частоту без потери выбранного пресета
        
        // Обновляем список доступных кривых
        UpdateAvailableCurves(hDlg);
    }
}

void SchematicModule::HandleXAxisTypeChange(HWND hDlg) {
    // v2.10.0: Обработка изменения типа оси X
    // Тип оси X сохраняется и применяется при нажатии Apply
    // Здесь ничего не делаем, просто фиксируем изменение
}

void SchematicModule::HandlePresetButton(HWND hDlg, int presetId) {
    // Сохраняем текущую частоту из комбобокса перед Clear()
    FrequencyBand savedFrequency = currentSelection.frequency;
    HWND hFreqCombo = GetDlgItem(hDlg, IDC_FREQUENCY_COMBO);
    if (hFreqCombo) {
        int selection = (int)SendMessage(hFreqCombo, CB_GETCURSEL, 0, 0);
        savedFrequency = (selection == 0) ? FrequencyBand::FREQ_400_KHZ : FrequencyBand::FREQ_2000_KHZ;
    }

    // Очищаем текущий выбор (включая старые кривые)
    currentSelection.Clear();
    currentAvailableCurves.clear();

    // Восстанавливаем частоту из комбобокса (не сбрасываем на 400 кГц)
    currentSelection.frequency = savedFrequency;
    
    // v2.15.0: Сохраняем информацию о пресете для отображения в легенде
    switch (presetId) {
        case IDC_PRESET_ALL_AMP_R1:
            currentSelection.receivers[0] = true; // R1
            ApplyBasicPresetState(currentSelection,
                                  DataTypeFilter::AMPLITUDE_ONLY,
                                  PresetType::ALL_AMPLITUDES_R1,
                                  L"Амплитуда RX1",
                                  currentPreset,
                                  currentPresetDisplayName);
            break;
        case IDC_PRESET_ALL_AMP_R2:
            currentSelection.receivers[1] = true; // R2
            ApplyBasicPresetState(currentSelection,
                                  DataTypeFilter::AMPLITUDE_ONLY,
                                  PresetType::ALL_AMPLITUDES_R2,
                                  L"Амплитуда RX2",
                                  currentPreset,
                                  currentPresetDisplayName);
            break;
        case IDC_PRESET_RATIOS:
            currentSelection.receivers[0] = true; // R1
            currentSelection.receivers[1] = true; // R2
            ApplyBasicPresetState(currentSelection,
                                  DataTypeFilter::RATIO_ONLY,
                                  PresetType::AMPLITUDE_RATIOS,
                                  L"Отношение амплитуд",
                                  currentPreset,
                                  currentPresetDisplayName);
            break;
        case IDC_PRESET_DIFFERENCES:
            currentSelection.receivers[0] = true; // R1
            currentSelection.receivers[1] = true; // R2
            ApplyBasicPresetState(currentSelection,
                                  DataTypeFilter::DIFFERENCE_ONLY,
                                  PresetType::PHASE_DIFFERENCES,
                                  L"Разность фаз",
                                  currentPreset,
                                  currentPresetDisplayName);
            break;
        case IDC_PRESET_TEMPERATURE:
            // Температура не требует выбора приёмников или передатчиков
            ApplyBasicPresetState(currentSelection,
                                  DataTypeFilter::TEMPERATURE_ONLY,
                                  PresetType::TEMPERATURE,
                                  L"Температура",
                                  currentPreset,
                                  currentPresetDisplayName);
            break;
        case IDC_PRESET_RHO_SMT:
        case IDC_PRESET_PHASE_SMT:
            if (const SmtUiBinding* binding = FindSmtBindingByPresetId(presetId)) {
                ApplySmtPresetState(hDlg, *binding, currentSelection, currentPreset, currentPresetDisplayName);
            }
            break;
        case IDC_PRESET_SILENT_AMPL:
            // Амплитуды молчащих передатчиков: две кривые (R1 и R2) из ZERO_AM_RX_1/2
            currentSelection.silentTx[0] = true; // Молч. R1
            currentSelection.silentTx[1] = true; // Молч. R2
            ApplyBasicPresetState(currentSelection,
                                  DataTypeFilter::AMPLITUDE_ONLY,
                                  PresetType::SILENT_AMPLITUDES,
                                  L"Ампл. молч.",
                                  currentPreset,
                                  currentPresetDisplayName);
            break;
        case IDC_PRESET_SILENT_DPH:
            // Разность фаз молчащих передатчиков: одна кривая (R2−R1) из ZERO_dPH с нормализацией и переводом в mdeg
            currentSelection.silentTx[0] = true;
            currentSelection.silentTx[1] = true;
            ApplyBasicPresetState(currentSelection,
                                  DataTypeFilter::DIFFERENCE_ONLY,
                                  PresetType::SILENT_PHASE_DIFFERENCE,
                                  L"Разность фаз молч.",
                                  currentPreset,
                                  currentPresetDisplayName);
            break;
    }
    
    // Для пресетов НЕ синхронизируем элементы схемы (они должны оставаться неактивными)
    // Просто очищаем выбор всех элементов схемы
    for (auto& element : schematicElements) {
        element.selected = false;
    }
    
    // Перерисовываем схему
    HWND hSchematicArea = GetDlgItem(hDlg, IDC_SCHEMATIC_AREA);
    if (hSchematicArea) {
        InvalidateRect(hSchematicArea, NULL, TRUE);
    }
    
    // Обновляем список кривых
    UpdateAvailableCurves(hDlg);
}

void SchematicModule::InitializeSmtModeCheckboxes(HWND hDlg) {
    // По умолчанию все чекбоксы пустые.
    // Галочки восстанавливаем только если пресет УЭС/Симм. фазы уже был активен
    // (т.е. dataFilter соответствует одному из этих пресетов).
    if (const SmtUiBinding* rhoBinding = FindSmtBindingByPresetId(IDC_PRESET_RHO_SMT)) {
        SyncSmtModeCheckboxes(hDlg, *rhoBinding, currentSelection);
    }

    if (const SmtUiBinding* phaseBinding = FindSmtBindingByPresetId(IDC_PRESET_PHASE_SMT)) {
        SyncSmtModeCheckboxes(hDlg, *phaseBinding, currentSelection);
    }
}

void SchematicModule::HandleSmtModeCheckbox(HWND hDlg, int checkboxId) {
    const SmtUiBinding* binding = FindSmtBindingByCheckboxId(checkboxId);
    if (!binding) {
        return;
    }

    const SmtComputationMode mode =
        (checkboxId == binding->fromComputerCheckboxId)
        ? SmtComputationMode::FROM_COMPUTER
        : SmtComputationMode::FROM_SONDE;

    ApplySmtModeCheckboxSelection(hDlg, *binding, currentSelection, mode);

    // Активируем соответствующий пресет (GROUPBOX не шлёт BN_CLICKED,
    // поэтому чекбоксы — основной триггер пресета УЭС / Симм. фазы).
    HandlePresetButton(hDlg, binding->presetId);
}

void SchematicModule::HandleMouseHover(HWND hDlg, int x, int y) {
    // Обновляем hover состояние элементов
    UpdateHoverState(x, y);
    
    ElementRect* element = FindElementAt(x, y);
    
    if (element) {
        // Устанавливаем курсор "рука" для интерактивных элементов
        SetCursor(LoadCursor(NULL, IDC_HAND));
        
        // Можно добавить показ подсказки с информацией об элементе
        std::wstring tooltip = GetElementTooltip(element->type, element->index);
        // TODO: Показать tooltip (можно использовать SetWindowText для статусной строки)
        
        // Перерисовываем область схемы для отображения hover эффекта
        HWND hSchematicArea = GetDlgItem(hDlg, IDC_SCHEMATIC_AREA);
        if (hSchematicArea) {
            RECT schematicRect;
            GetClientRect(hSchematicArea, &schematicRect);
            MapWindowPoints(hSchematicArea, hDlg, (POINT*)&schematicRect, 2);
            InvalidateRect(hDlg, &schematicRect, FALSE);
        }
    } else {
        // Обычный курсор
        SetCursor(LoadCursor(NULL, IDC_ARROW));
    }
}

void SchematicModule::HandleCurveCheckboxChange(HWND hDlg, int itemIndex) {
    HWND hCurveList = GetDlgItem(hDlg, IDC_CURVE_LIST);
    if (!hCurveList) return;
    
    LVITEM item = {};
    item.mask = LVIF_PARAM;
    item.iItem = itemIndex;
    
    if (!ListView_GetItem(hCurveList, &item)) {
        return; // Не удалось получить элемент
    }
    
    int curveIndex = static_cast<int>(item.lParam);
    
    // Проверяем границы массива
    if (curveIndex < 0 || curveIndex >= static_cast<int>(currentAvailableCurves.size())) {
        return;
    }
    
    // Получаем новое состояние чекбокса
    BOOL isChecked = ListView_GetCheckState(hCurveList, itemIndex);
    

    currentAvailableCurves[curveIndex].enabled = (isChecked == TRUE);
    

    
    ValidateCurrentCurveConfiguration(hDlg);
}

void SchematicModule::ValidateCurrentCurveConfiguration(HWND hDlg) {
    if (!curveSelectionModule) return;
    
    // Создаем временную конфигурацию для валидации
    ChartConfiguration tempConfig;
    tempConfig.curves = currentAvailableCurves;
    tempConfig.UpdateAxisConfiguration();
    
    // Проверяем конфигурацию
    std::string error = curveSelectionModule->GetConfigurationError(tempConfig);
    
    // Обновляем состояние кнопки "Применить"
    HWND hApplyBtn = GetDlgItem(hDlg, IDC_APPLY_SETTINGS);
    if (hApplyBtn) {
        EnableWindow(hApplyBtn, error.empty() ? TRUE : FALSE);
    }
    
    // Обновляем заголовок окна ВСЕГДА: иначе "[Configuration Error]" может "залипнуть",
    // даже когда конфигурация снова стала валидной (типичный сценарий после снятия silent).
    std::wstring title = L"\x0421\x0445\x0435\x043C\x0430 \x043F\x0440\x0438\x0431\x043E\x0440\x0430 - \x0413\x0440\x0430\x0444\x0438\x043A " + std::to_wstring(targetChartNumber);
    if (!error.empty()) {
        title += L" [Configuration Error]";
    }
    SetWindowText(hDlg, title.c_str());
}

bool SchematicModule::ApplyCurrentCurveConfiguration() {
    if (!curveSelectionModule) return false;
    if (!dataModule || !dataModule->HasData()) {
        HWND hMessageOwner = hSchematicDialog ? hSchematicDialog : hParentWindow;
        MessageBox(hMessageOwner,
                   L"Невозможно вывести кривые на график: данные отсутствуют.\nОткройте файл данных или подключите прибор к COM-порту.",
                   L"Нет данных для отображения",
                   MB_OK | MB_ICONINFORMATION);
        return false;
    }
    
    // ОТЛАДКА: Выводим информацию о применяемой конфигурации
    
    int enabledCount = 0;
    for (size_t i = 0; i < currentAvailableCurves.size(); i++) {
        const auto& curve = currentAvailableCurves[i];
        if (curve.enabled) enabledCount++;
    }
    
    // Создаем конфигурацию из текущих кривых
    ChartConfiguration config;
    config.curves = currentAvailableCurves;

    ApplySmtModesToConfigurationCurves(config, currentSelection);

    config.UpdateAxisConfiguration();
    
    // v2.15.0: Передаём информацию о пресете
    config.activePreset = currentPreset;
    config.presetDisplayName = BuildAppliedPresetDisplayName(currentPreset,
                                                             currentPresetDisplayName,
                                                             currentSelection);
    
    // v2.10.0: Получаем тип оси X из combobox
    HWND hDlg = GetParent(GetDlgItem(hSchematicDialog, IDC_XAXIS_TYPE_COMBO));
    HWND hXAxisCombo = GetDlgItem(hDlg, IDC_XAXIS_TYPE_COMBO);
    if (hXAxisCombo) {
        int selection = (int)SendMessage(hXAxisCombo, CB_GETCURSEL, 0, 0);
        config.xAxisType = (selection == 0) ? XAxisType::FRAME_NUMBER : XAxisType::TEMPERATURE;
    }
    
    // Валидируем конфигурацию
    if (!curveSelectionModule->ValidateConfiguration(config)) {
        return false;
    }
    
    // Применяем конфигурацию к соответствующему графику
    curveSelectionModule->ApplyCurveConfiguration(targetChartNumber, config);
    
    // Вызываем callback для обновления графика (v2.9.4)
    if (onConfigurationApplied) {
        onConfigurationApplied(targetChartNumber);
    }
    
    return true;
}

DataTypeFilter SchematicModule::DetermineDataFilterFromCurves(const std::vector<CurveDefinition>& curves) const {
    if (curves.empty()) {
        return DataTypeFilter::ALL;
    }
    
    // ОТЛАДКА: Выводим информацию о кривых
    
    // Подсчитываем типы данных в кривых
    bool hasAmplitudes = false;
    bool hasPhases = false;
    bool hasRatios = false;
    bool hasDifferences = false;
    bool hasSilentAmplitudes = false;
    bool hasSilentDifferences = false;
    bool hasRhoSmt = false;         // v2.16.0
    bool hasPhaseSmt = false;       // v2.16.0
    bool hasTemperature = false;
    
    for (const auto& curve : curves) {
        
        switch (curve.dataType) {
            case DataType::AMPLITUDE_RX1:
            case DataType::AMPLITUDE_RX2:
                hasAmplitudes = true;
                break;
            case DataType::SILENT_AMPLITUDE_RX1:
            case DataType::SILENT_AMPLITUDE_RX2:
                hasSilentAmplitudes = true;
                break;
            case DataType::SILENT_PHASE_DIFFERENCE:
                hasSilentDifferences = true;
                break;
            case DataType::PHASE_SMT:
                hasPhaseSmt = true;
                break;
            case DataType::AMPLITUDE_RATIO:
                hasRatios = true;
                break;
            case DataType::PHASE_DIFFERENCE:
                hasDifferences = true;
                break;
            case DataType::RHO_SMT:
                hasRhoSmt = true;
                break;
            case DataType::TEMPERATURE:
                hasTemperature = true;
                break;
            default:
                break;
        }
    }
    
    // Определяем фильтр на основе найденных типов
    // ИСПРАВЛЕНО: Учитываем молчащие передатчики
    
    DataTypeFilter result = DataTypeFilter::ALL;
    
    // v2.16.0: Если есть только УЭС
    if (hasRhoSmt && !hasPhaseSmt && !hasDifferences && !hasRatios && !hasAmplitudes && !hasPhases && !hasSilentAmplitudes && !hasTemperature) {
        result = DataTypeFilter::RHO_SMT_ONLY;
    }
    // v2.16.0: Если есть только симметризованные фазы
    else if (hasPhaseSmt && !hasRhoSmt && !hasDifferences && !hasRatios && !hasAmplitudes && !hasPhases && !hasSilentAmplitudes && !hasTemperature) {
        result = DataTypeFilter::PHASE_SMT_ONLY;
    }
    // Если есть только отношения амплитуд
    else if (hasRatios && !hasDifferences && !hasAmplitudes && !hasPhases && !hasSilentAmplitudes && !hasRhoSmt && !hasPhaseSmt && !hasTemperature) {
        result = DataTypeFilter::RATIO_ONLY;
    }
    // Если есть только разности фаз
    else if (hasDifferences && !hasRatios && !hasAmplitudes && !hasPhases && !hasSilentAmplitudes && !hasRhoSmt && !hasPhaseSmt && !hasTemperature) {
        result = DataTypeFilter::DIFFERENCE_ONLY;
    }
    // Если есть только разность фаз молчащих передатчиков
    else if (hasSilentDifferences && !hasRatios && !hasAmplitudes && !hasPhases && !hasSilentAmplitudes && !hasDifferences && !hasRhoSmt && !hasPhaseSmt && !hasTemperature) {
        result = DataTypeFilter::DIFFERENCE_ONLY;
    }
    // Если есть только обычные амплитуды (без отношений и молчащих)
    else if (hasAmplitudes && !hasPhases && !hasRatios && !hasDifferences && !hasSilentAmplitudes && !hasRhoSmt && !hasPhaseSmt && !hasTemperature) {
        result = DataTypeFilter::AMPLITUDE_ONLY;
    }
    // Если есть только фазы (без разностей)
    else if (hasPhases && !hasAmplitudes && !hasRatios && !hasDifferences && !hasSilentAmplitudes && !hasRhoSmt && !hasPhaseSmt && !hasTemperature) {
        result = DataTypeFilter::PHASE_ONLY;
    }
    // Если есть только температура
    else if (hasTemperature && !hasAmplitudes && !hasPhases && !hasRatios && !hasDifferences && !hasSilentAmplitudes && !hasRhoSmt && !hasPhaseSmt) {
        result = DataTypeFilter::TEMPERATURE_ONLY;
    }
    // ИСПРАВЛЕНО: Если есть только молчащие передатчики или смешанные типы - ALL
    else {
        result = DataTypeFilter::ALL;
    }
    
    return result;
}

// Дополнительные методы отрисовки схемы
void SchematicModule::DrawDeviceFrame(HDC hdc, const RECT& frameRect) {
    // Рисуем фон рамки
    HBRUSH bgBrush = CreateSolidBrush(FRAME_BACKGROUND);
    HPEN borderPen = CreatePen(PS_SOLID, FRAME_BORDER_WIDTH, FRAME_BORDER);
    
    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, bgBrush);
    HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
    
    // Рисуем скруглённый прямоугольник для рамки
    RoundRect(hdc, frameRect.left, frameRect.top, 
              frameRect.right, frameRect.bottom,
              FRAME_CORNER_RADIUS, FRAME_CORNER_RADIUS);
    
    // Восстанавливаем объекты GDI
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(bgBrush);
    DeleteObject(borderPen);
}

void SchematicModule::DrawSchematicConnections(HDC hdc, const RECT& rect) {
    // В горизонтальной схеме соединительные линии не нужны
    // Все элементы расположены в логическом порядке слева направо
    // Разделители убраны для чистого современного вида
}


void SchematicModule::DrawGroupLabels(HDC hdc, const RECT& rect) {
    // Устанавливаем единый шрифт для подписей
    LOGFONT lfLabel = {};
    NONCLIENTMETRICS ncmLabel = {};
    ncmLabel.cbSize = sizeof(ncmLabel);
    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncmLabel), &ncmLabel, 0)) {
        lfLabel = ncmLabel.lfMessageFont;
    } else {
        wcscpy_s(lfLabel.lfFaceName, L"Segoe UI");
        lfLabel.lfHeight = 12;
        lfLabel.lfQuality = CLEARTYPE_QUALITY;
    }
    HFONT labelFont = CreateFontIndirect(&lfLabel);
    HFONT oldFont = (HFONT)SelectObject(hdc, labelFont);
    
    SetTextColor(hdc, RGB(80, 80, 80));
    SetBkMode(hdc, TRANSPARENT);
    
    // Поскольку все элементы теперь в одной линии, добавляем подписи под каждым элементом
    // Получаем позиции элементов для подписей (используем те же константы, что и CalculateElementRect)
    const int elW = kSchematicElementWidth;
    const int elSp = kSchematicElementSpacing;
    const int frameW = kSchematicFrameElementsCount * elW + (kSchematicFrameElementsCount - 1) * elSp + 2 * FRAME_PADDING;
    const int totalW = frameW + kFrameToSilentSpacing + 2 * elW + elSp;
    int startX = rect.left + (rect.right - rect.left - totalW) / 2;
    int centerY = rect.top + (rect.bottom - rect.top - kSchematicElementHeight) / 2;
    int labelY = centerY + kSchematicElementHeight + 5; // Под элементами
    
    // Подписи типов элементов убраны для более чистого вида
    // Пользователи ИАТК понимают обозначения T1-T5, R1-R2, Silent R1-R2 без дополнительных пояснений
    
    SelectObject(hdc, oldFont);
    DeleteObject(labelFont);
}

void SchematicModule::DrawConnectionArrows(HDC hdc, const std::vector<RECT>& transmitterRects, 
                                          const RECT& r1Rect, const RECT& r2Rect) {
    // В горизонтальной схеме стрелки не нужны, так как логика понятна из расположения
    // Можно добавить небольшие индикаторы направления сигнала, если необходимо
    
    // Создаем перо для индикаторов
    HPEN indicatorPen = CreatePen(PS_SOLID, 1, RGB(120, 120, 120));
    HPEN oldPen = (HPEN)SelectObject(hdc, indicatorPen);
    
    // Добавляем небольшие стрелки над приемниками для указания входящего сигнала
    int arrowSize = 4;
    
    // Стрелка над R1
    int r1CenterX = r1Rect.left + (r1Rect.right - r1Rect.left) / 2;
    int r1TopY = r1Rect.top - 8;
    
    MoveToEx(hdc, r1CenterX - arrowSize, r1TopY - arrowSize, NULL);
    LineTo(hdc, r1CenterX, r1TopY);
    LineTo(hdc, r1CenterX + arrowSize, r1TopY - arrowSize);
    
    // Стрелка над R2
    int r2CenterX = r2Rect.left + (r2Rect.right - r2Rect.left) / 2;
    int r2TopY = r2Rect.top - 8;
    
    MoveToEx(hdc, r2CenterX - arrowSize, r2TopY - arrowSize, NULL);
    LineTo(hdc, r2CenterX, r2TopY);
    LineTo(hdc, r2CenterX + arrowSize, r2TopY - arrowSize);
    
    SelectObject(hdc, oldPen);
    DeleteObject(indicatorPen);
}

// Методы для интеллектуальной обработки выбора элементов
bool SchematicModule::IsSelectionCompatible(ElementType type, int index) const {
    // ИСПРАВЛЕНО: Обновленные правила совместимости выбора элементов
    
    // Правило 1: Нельзя одновременно выбирать передатчики и приемники
    if (type == ElementType::TRANSMITTER) {
        if (!SupportsTransmitterIndex(index)) {
            return false;
        }
        return currentSelection.GetSelectedReceiversCount() == 0;
    }
    
    if (type == ElementType::RECEIVER) {
        return currentSelection.GetSelectedTransmittersCount() == 0;
    }
    
    // ИСПРАВЛЕНО: Молчащие передатчики МОЖНО комбинировать с активными элементами
    // Запрещена только комбинация: оба приемника (R1+R2) + молчащие
    if (type == ElementType::SILENT_TX) {
        // Разрешаем комбинации с передатчиками и одним приемником
        // Запрещаем только комбинацию с двумя приемниками
        return currentSelection.GetSelectedReceiversCount() < 2;
    }
    
    return true;
}

void SchematicModule::ClearIncompatibleSelections(ElementType newType) {
    // ИСПРАВЛЕНО: Минимальная очистка - только реально несовместимые элементы
    
    if (newType == ElementType::TRANSMITTER) {
        // Передатчики несовместимы только с приемниками
        if (currentSelection.GetSelectedReceiversCount() > 0) {
            std::fill(currentSelection.receivers.begin(), currentSelection.receivers.end(), false);
        }
        // Молчащие передатчики СОВМЕСТИМЫ с активными передатчиками - НЕ очищаем
    }
    else if (newType == ElementType::RECEIVER) {
        // Приемники несовместимы только с передатчиками
        if (currentSelection.GetSelectedTransmittersCount() > 0) {
            std::fill(currentSelection.transmitters.begin(), currentSelection.transmitters.end(), false);
        }
        
        // Если выбираем второй приемник, очищаем молчащие (R1+R2+Silent запрещено)
        int selectedReceivers = currentSelection.GetSelectedReceiversCount();
        if (selectedReceivers >= 1 && currentSelection.GetSelectedSilentCount() > 0) {
            std::fill(currentSelection.silentTx.begin(), currentSelection.silentTx.end(), false);
        }
    }
    else if (newType == ElementType::SILENT_TX) {
        // Молчащие передатчики несовместимы только с R1+R2 одновременно
        if (currentSelection.GetSelectedReceiversCount() == 2) {
            std::fill(currentSelection.receivers.begin(), currentSelection.receivers.end(), false);
        }
        // С передатчиками и одним приемником молчащие СОВМЕСТИМЫ - НЕ очищаем
    }
}

void SchematicModule::SynchronizeElementStates() {
    // Синхронизируем визуальное состояние элементов с логическим состоянием
    for (auto& element : schematicElements) {
        element.selected = element.type == ElementType::TRANSMITTER && !SupportsTransmitterIndex(element.index)
            ? false
            : IsSelectionElementSelected(currentSelection, element.type, element.index);
    }
}





// Процедура диалогового окна схемы прибора (v2.9.4: немодальное окно)
INT_PTR CALLBACK SchematicDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    // Для немодального окна используем GWLP_USERDATA для хранения указателя
    SchematicModule* pSchematic = reinterpret_cast<SchematicModule*>(GetWindowLongPtr(hDlg, GWLP_USERDATA));
    
    switch (message) {
        case WM_INITDIALOG:
            {
                // Получаем указатель на объект SchematicModule
                pSchematic = reinterpret_cast<SchematicModule*>(lParam);
                SetWindowLongPtr(hDlg, GWLP_USERDATA, lParam);
                
                // Устанавливаем заголовок окна
                // "Схема прибора - График N" в Unicode
                std::wstring title = L"\x0421\x0445\x0435\x043C\x0430 \x043F\x0440\x0438\x0431\x043E\x0440\x0430 - \x0413\x0440\x0430\x0444\x0438\x043A " + std::to_wstring(pSchematic->targetChartNumber);
                SetWindowText(hDlg, title.c_str());

                // Наследуем иконки от главного окна приложения.
                HWND hParent = GetParent(hDlg);
                if (hParent) {
                    HICON hIconBig = (HICON)SendMessage(hParent, WM_GETICON, ICON_BIG, 0);
                    if (!hIconBig) hIconBig = (HICON)GetClassLongPtr(hParent, GCLP_HICON);
                    if (hIconBig) SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);

                    HICON hIconSmall = (HICON)SendMessage(hParent, WM_GETICON, ICON_SMALL, 0);
                    if (!hIconSmall) hIconSmall = (HICON)GetClassLongPtr(hParent, GCLP_HICONSM);
                    if (hIconSmall) SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);
                }
                
                // Инициализируем элементы управления
                pSchematic->InitializeDialogControls(hDlg);
                
                // Центрируем окно
                RECT parentRect, dialogRect;
                GetWindowRect(hParent, &parentRect);
                GetWindowRect(hDlg, &dialogRect);
                
                int x = parentRect.left + (parentRect.right - parentRect.left - 
                       (dialogRect.right - dialogRect.left)) / 2;
                int y = parentRect.top + (parentRect.bottom - parentRect.top - 
                       (dialogRect.bottom - dialogRect.top)) / 2;
                
                SetWindowPos(hDlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
                
                return (INT_PTR)TRUE;
            }
            
        case WM_CLOSE:
            {
                // Для немодального окна скрываем вместо закрытия
                if (pSchematic) {
                    pSchematic->HideSchematicDialog();
                }
                return (INT_PTR)TRUE;
            }
            
        case WM_DESTROY:
            {
                // Очищаем указатель при уничтожении окна
                SetWindowLongPtr(hDlg, GWLP_USERDATA, 0);
                return (INT_PTR)TRUE;
            }
            
        case WM_PAINT:
            {
                if (pSchematic) {
                    PAINTSTRUCT ps;
                    HDC hdc = BeginPaint(hDlg, &ps);
                    
                    // Получаем область для отрисовки схемы
                    HWND hSchematicArea = GetDlgItem(hDlg, IDC_SCHEMATIC_AREA);
                    if (hSchematicArea) {
                        RECT schematicRect;
                        GetClientRect(hSchematicArea, &schematicRect);
                        MapWindowPoints(hSchematicArea, hDlg, (POINT*)&schematicRect, 2);
                        
                        // Проверяем, пересекается ли область схемы с областью перерисовки
                        RECT intersectRect;
                        if (IntersectRect(&intersectRect, &schematicRect, &ps.rcPaint)) {
                            // Отрисовываем схему прибора только если нужно
                            pSchematic->RenderSchematic(hdc, schematicRect);
                        }
                    }
                    
                    EndPaint(hDlg, &ps);
                }
                return (INT_PTR)TRUE;
            }
            
        case WM_LBUTTONDOWN:
            {
                if (pSchematic) {
                    int x = LOWORD(lParam);
                    int y = HIWORD(lParam);
                    
                    // Проверяем, попал ли клик в область схемы
                    HWND hSchematicArea = GetDlgItem(hDlg, IDC_SCHEMATIC_AREA);
                    if (hSchematicArea) {
                        RECT schematicRect;
                        GetClientRect(hSchematicArea, &schematicRect);
                        MapWindowPoints(hSchematicArea, hDlg, (POINT*)&schematicRect, 2);
                        
                        POINT pt = {x, y};
                        if (PtInRect(&schematicRect, pt)) {
                            // ИСПРАВЛЕНО: Координаты клика уже в системе координат диалога,
                            // а элементы в schematicElements хранят координаты относительно schematicRect
                            // Поэтому НЕ нужно вычитать смещение - передаем координаты как есть
                            
                            if (pSchematic->HandleElementClick(x, y)) {
                                // Обновляем отображение схемы
                                InvalidateRect(hDlg, &schematicRect, TRUE);
                                
                                // Обновляем список доступных кривых
                                pSchematic->UpdateAvailableCurves(hDlg);
                            }
                        }
                    }
                }
                return (INT_PTR)TRUE;
            }
            
        case WM_MOUSEMOVE:
            {
                if (pSchematic) {
                    int x = LOWORD(lParam);
                    int y = HIWORD(lParam);
                    
                    // Проверяем, находится ли курсор над элементом схемы
                    HWND hSchematicArea = GetDlgItem(hDlg, IDC_SCHEMATIC_AREA);
                    if (hSchematicArea) {
                        RECT schematicRect;
                        GetClientRect(hSchematicArea, &schematicRect);
                        MapWindowPoints(hSchematicArea, hDlg, (POINT*)&schematicRect, 2);
                        
                        POINT pt = {x, y};
                        if (PtInRect(&schematicRect, pt)) {
                            // ИСПРАВЛЕНО: Координаты курсора уже в системе координат диалога,
                            // а элементы в schematicElements хранят координаты относительно schematicRect
                            // Поэтому НЕ нужно вычитать смещение - передаем координаты как есть
                            
                            // Обновляем курсор в зависимости от того, над каким элементом находимся
                            pSchematic->HandleMouseHover(hDlg, x, y);
                        }
                    }
                }
                return (INT_PTR)TRUE;
            }
            
        case WM_NOTIFY:
            {
                if (pSchematic) {
                    LPNMHDR pnmh = (LPNMHDR)lParam;
                    if (pnmh->idFrom == IDC_CURVE_LIST && pnmh->code == LVN_ITEMCHANGED) {
                        LPNMLISTVIEW pnmlv = (LPNMLISTVIEW)lParam;
                        
                        // Проверяем, изменилось ли состояние чекбокса
                        if ((pnmlv->uChanged & LVIF_STATE) && 
                            ((pnmlv->uNewState & LVIS_STATEIMAGEMASK) != (pnmlv->uOldState & LVIS_STATEIMAGEMASK))) {
                            
                            pSchematic->HandleCurveCheckboxChange(hDlg, pnmlv->iItem);
                        }
                    }
                }
                return (INT_PTR)TRUE;
            }
            
        case WM_COMMAND:
            {
                int wmId = LOWORD(wParam);
                
                switch (wmId) {
                    case IDC_APPLY_SETTINGS:
                    case IDOK:
                        if (pSchematic && pSchematic->ValidateCurrentSelection()) {
                            // Применяем конфигурацию кривых
                            if (pSchematic->ApplyCurrentCurveConfiguration()) {
                                // v2.9.4: НЕ закрываем окно после Apply - оставляем открытым
                                // Можно показать сообщение об успехе (опционально)
                                // MessageBox(hDlg, L"Settings applied successfully", L"Success", MB_OK | MB_ICONINFORMATION);
                            } else {
                                MessageBox(hDlg, L"Error applying curve configuration", L"Error", MB_OK | MB_ICONERROR);
                            }
                        } else {
                            // Показываем сообщение об ошибке
                            if (pSchematic) {
                                std::string error = pSchematic->GetCurrentSelection().GetValidationError();
                                std::wstring werror(error.begin(), error.end());
                                MessageBox(hDlg, werror.c_str(), L"Selection Error", MB_OK | MB_ICONWARNING);
                            }
                        }
                        return (INT_PTR)TRUE;
                        
                    case IDCANCEL:
                        // v2.9.4: Для немодального окна скрываем вместо закрытия
                        if (pSchematic) {
                            pSchematic->HideSchematicDialog();
                        }
                        return (INT_PTR)TRUE;
                        
                    case IDC_FREQUENCY_COMBO:
                        if (HIWORD(wParam) == CBN_SELCHANGE && pSchematic) {
                            pSchematic->HandleFrequencyChange(hDlg);
                        }
                        return (INT_PTR)TRUE;
                        
                    case IDC_XAXIS_TYPE_COMBO:
                        if (HIWORD(wParam) == CBN_SELCHANGE && pSchematic) {
                            pSchematic->HandleXAxisTypeChange(hDlg);
                        }
                        return (INT_PTR)TRUE;

                    case IDC_RHO_SMT_FROM_SONDE:
                    case IDC_RHO_SMT_FROM_COMPUTER:
                    case IDC_PHASE_SMT_FROM_SONDE:
                    case IDC_PHASE_SMT_FROM_COMPUTER:
                        if (pSchematic && HIWORD(wParam) == BN_CLICKED) {
                            pSchematic->HandleSmtModeCheckbox(hDlg, wmId);
                        }
                        return (INT_PTR)TRUE;
                        
                    // Обработка быстрых пресетов
                    case IDC_PRESET_ALL_AMP_R1:
                    case IDC_PRESET_ALL_AMP_R2:
                    case IDC_PRESET_RATIOS:
                    case IDC_PRESET_DIFFERENCES:
                    case IDC_PRESET_TEMPERATURE:
                    case IDC_PRESET_RHO_SMT:
                    case IDC_PRESET_PHASE_SMT:
                    case IDC_PRESET_SILENT_AMPL:
                    case IDC_PRESET_SILENT_DPH:
                        if (pSchematic) {
                            pSchematic->HandlePresetButton(hDlg, wmId);
                        }
                        return (INT_PTR)TRUE;
                }
                break;
            }
    }
    return (INT_PTR)FALSE;
}

