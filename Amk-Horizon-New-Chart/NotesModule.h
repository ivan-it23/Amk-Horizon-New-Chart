#pragma once

#include "framework.h"
#include "SchematicModule.h" // ChartConfiguration, XAxisType, CurveDefinition

#include <cstdint>
#include <string>
#include <vector>

struct ChartNote {
    std::string noteId;              // уникальный ID (ascii)
    uint64_t createdUtcMs = 0;       // время создания (UTC, мс)

    uint64_t configKey = 0;          // ключ конфигурации кривых
    std::string configSnapshot;      // нормализованный снапшот конфигурации (utf-8)
    std::wstring presetName;         // имя пресета для отображения

    XAxisType xAxisType = XAxisType::FRAME_NUMBER;
    int xIndex = 0;                  // индекс кадра (всегда индекс, даже если XAxisType::TEMPERATURE)

    double yLeft = 0.0;
    double yRight = 0.0;
    bool hasYRight = false;

    std::wstring text;               // текст заметки
};

class NotesModule {
public:
    struct SnapshotCurveItem {
        std::string curveId;
        int yAxisGroup = 0;
    };

    struct RenderedMarker {
        size_t noteIndex = 0;        // индекс в notes
        int chartNumber = 0;         // 1 или 2
        int screenX = 0;
        int screenY = 0;
    };

    void SetNotesPath(const std::wstring& path);
    const std::wstring& GetNotesPath() const { return notesPath; }

    void Clear();
    bool LoadFromDisk();
    bool IsLoaded() const { return loaded; }

    const std::vector<ChartNote>& GetAllNotes() const { return notes; }
    std::vector<size_t> GetNotesForConfig(uint64_t configKey) const;

    bool AppendNote(const ChartNote& note);
    bool DeleteNoteById(const std::string& noteId);

    void SetRenderedMarkers(std::vector<RenderedMarker>&& markers);
    const ChartNote* HitTestRenderedMarkers(int chartNumber, int x, int y, int radiusPx) const;

    static uint64_t ComputeConfigKey(const ChartConfiguration& config);
    static std::string BuildConfigSnapshot(const ChartConfiguration& config);
    static bool ParseConfigSnapshot(const std::string& snapshot,
                                    XAxisType& outXAxisType,
                                    PresetType& outPreset,
                                    std::vector<SnapshotCurveItem>& outCurves);
    static std::string NarrowUtf8(const std::wstring& w);
    static std::wstring WidenUtf8(const std::string& s);

    static std::string GenerateNoteId();
    static uint64_t NowUtcMs();

private:
    std::vector<ChartNote> notes;
    std::wstring notesPath;
    bool loaded = false;

    std::vector<RenderedMarker> renderedMarkers;

    static uint64_t Fnv1a64(const uint8_t* data, size_t len);
};

