#pragma once

#include "SchematicModule.h"
#include <string>

struct ElementPresentationDescriptor {
    ElementType elementType = ElementType::TRANSMITTER;
    int index = 0;
    std::wstring shortLabel;
    std::wstring tooltip;
    std::string displayToken;
};

inline ElementPresentationDescriptor GetElementPresentationDescriptor(ElementType elementType, int index) {
    ElementPresentationDescriptor descriptor = {};
    descriptor.elementType = elementType;
    descriptor.index = index;

    switch (elementType) {
        case ElementType::TRANSMITTER:
            descriptor.shortLabel = L"T" + std::to_wstring(index + 1);
            descriptor.tooltip = L"Передатчик T" + std::to_wstring(index + 1) + L" - нажмите для выбора";
            descriptor.displayToken = "T" + std::to_string(index + 1);
            break;
        case ElementType::RECEIVER:
            descriptor.shortLabel = L"R" + std::to_wstring(index + 1);
            descriptor.tooltip = L"Приёмник R" + std::to_wstring(index + 1) + L" - нажмите для выбора";
            descriptor.displayToken = "R" + std::to_string(index + 1);
            break;
        case ElementType::SILENT_TX:
            descriptor.shortLabel = L"Молч.\nR" + std::to_wstring(index + 1);
            descriptor.tooltip = L"Молчащий передатчик для R" + std::to_wstring(index + 1) + L" - нажмите для выбора";
            descriptor.displayToken = "Молчащий R" + std::to_string(index + 1);
            break;
        default:
            descriptor.shortLabel = L"Unknown";
            descriptor.tooltip = L"Unknown element";
            descriptor.displayToken = "Unknown";
            break;
    }

    return descriptor;
}

inline std::wstring GetElementShortLabel(ElementType elementType, int index) {
    return GetElementPresentationDescriptor(elementType, index).shortLabel;
}

inline std::wstring GetElementTooltipText(ElementType elementType, int index) {
    return GetElementPresentationDescriptor(elementType, index).tooltip;
}

inline std::string GetElementDisplayToken(ElementType elementType, int index, bool silentPhaseDifference = false) {
    if (elementType == ElementType::SILENT_TX && silentPhaseDifference) {
        return "Молчащие";
    }
    return GetElementPresentationDescriptor(elementType, index).displayToken;
}
