#pragma once

#include "framework.h"
#include "ToolDataTypes.h"
#include <cstdint>
#include <string>
#include <vector>

namespace FileConstructor {

struct SegmentBoundary {
    size_t startIndex = 0;
    size_t recordCount = 0;
    size_t sourceStartIndex = 0;
    std::wstring sourcePath;
    std::wstring label;
};

std::wstring BuildSegmentSidecarPath(const std::wstring& dataFilePath);

bool LoadSegmentLayout(const std::wstring& dataFilePath, std::vector<SegmentBoundary>& outSegments);
bool DeleteSegmentLayout(const std::wstring& dataFilePath);

bool TryGetHomogeneousSignature(const std::vector<GP_TOOL_DATA>& records, uint32_t& outSignature);

bool AppendRecordsToExistingFile(const std::wstring& targetFilePath,
                                 const std::vector<GP_TOOL_DATA>& records,
                                 const std::wstring& segmentSourcePath,
                                 size_t segmentSourceStartIndex,
                                 const std::wstring& segmentLabel,
                                 std::wstring& outError);

bool SaveLayoutExcludingRange(const std::wstring& sourceDataFilePath,
                              const std::wstring& targetDataFilePath,
                              size_t startIndex,
                              size_t endIndex,
                              size_t originalRecordCount,
                              std::wstring& outError);

bool ShowFileConstructorDialog(HWND parent, std::wstring& outSavedFilePath);

} // namespace FileConstructor
