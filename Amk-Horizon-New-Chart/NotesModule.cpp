#include "framework.h"
#include "NotesModule.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <random>
#include <unordered_map>

namespace {
    bool StartsWith(const std::string& s, const char* prefix) {
        const size_t n = strlen(prefix);
        return s.size() >= n && s.compare(0, n, prefix) == 0;
    }

    void SkipSpaces(const std::string& s, size_t& i) {
        while (i < s.size() && (s[i] == ' ' || s[i] == '\t' || s[i] == '\r' || s[i] == '\n')) i++;
    }

    // Parse JSON string literal starting at s[i] == '"'. Returns false on failure.
    bool ParseJsonString(const std::string& s, size_t& i, std::string& out) {
        if (i >= s.size() || s[i] != '"') return false;
        i++; // skip "
        std::string r;
        r.reserve(64);
        while (i < s.size()) {
            char c = s[i++];
            if (c == '"') {
                out = std::move(r);
                return true;
            }
            if (c == '\\') {
                if (i >= s.size()) return false;
                char e = s[i++];
                switch (e) {
                    case '\\': r.push_back('\\'); break;
                    case '"': r.push_back('"'); break;
                    case 'n': r.push_back('\n'); break;
                    case 'r': r.push_back('\r'); break;
                    case 't': r.push_back('\t'); break;
                    default:
                        // unknown escape, keep as-is
                        r.push_back(e);
                        break;
                }
            } else {
                r.push_back(c);
            }
        }
        return false;
    }

    bool FindJsonKey(const std::string& line, const char* key, size_t& valuePos) {
        // very small/lenient: find "key" then colon
        std::string pattern = "\"";
        pattern += key;
        pattern += "\"";
        size_t p = line.find(pattern);
        if (p == std::string::npos) return false;
        p = line.find(':', p + pattern.size());
        if (p == std::string::npos) return false;
        p++; // after ':'
        valuePos = p;
        return true;
    }

    bool ExtractJsonStringField(const std::string& line, const char* key, std::string& out) {
        size_t p = 0;
        if (!FindJsonKey(line, key, p)) return false;
        SkipSpaces(line, p);
        return ParseJsonString(line, p, out);
    }

    bool ExtractJsonUInt64Field(const std::string& line, const char* key, uint64_t& out) {
        size_t p = 0;
        if (!FindJsonKey(line, key, p)) return false;
        SkipSpaces(line, p);
        const char* start = line.c_str() + p;
        char* end = nullptr;
        unsigned long long v = strtoull(start, &end, 10);
        if (end == start) return false;
        out = static_cast<uint64_t>(v);
        return true;
    }

    bool ExtractJsonIntField(const std::string& line, const char* key, int& out) {
        size_t p = 0;
        if (!FindJsonKey(line, key, p)) return false;
        SkipSpaces(line, p);
        const char* start = line.c_str() + p;
        char* end = nullptr;
        long v = strtol(start, &end, 10);
        if (end == start) return false;
        out = static_cast<int>(v);
        return true;
    }

    bool ExtractJsonDoubleField(const std::string& line, const char* key, double& out) {
        size_t p = 0;
        if (!FindJsonKey(line, key, p)) return false;
        SkipSpaces(line, p);
        const char* start = line.c_str() + p;
        char* end = nullptr;
        double v = strtod(start, &end);
        if (end == start) return false;
        out = v;
        return true;
    }

    bool ExtractJsonBoolField(const std::string& line, const char* key, bool& out) {
        size_t p = 0;
        if (!FindJsonKey(line, key, p)) return false;
        SkipSpaces(line, p);
        if (StartsWith(line.substr(p), "true")) { out = true; return true; }
        if (StartsWith(line.substr(p), "false")) { out = false; return true; }
        return false;
    }

    std::string EscapeJsonString(const std::string& s) {
        std::string r;
        r.reserve(s.size() + 16);
        for (char c : s) {
            switch (c) {
                case '\\': r += "\\\\"; break;
                case '"': r += "\\\""; break;
                case '\n': r += "\\n"; break;
                case '\r': r += "\\r"; break;
                case '\t': r += "\\t"; break;
                default: r.push_back(c); break;
            }
        }
        return r;
    }
} // namespace

void NotesModule::SetNotesPath(const std::wstring& path) {
    if (notesPath == path) return;
    notesPath = path;
    loaded = false;
    notes.clear();
    renderedMarkers.clear();
}

void NotesModule::Clear() {
    notes.clear();
    renderedMarkers.clear();
    loaded = false;
    notesPath.clear();
}

bool NotesModule::LoadFromDisk() {
    notes.clear();
    renderedMarkers.clear();
    loaded = true;

    if (notesPath.empty()) {
        return true; // nothing to load
    }

    std::ifstream in(notesPath, std::ios::binary);
    if (!in.is_open()) {
        // File may legitimately not exist yet
        return true;
    }

    std::unordered_map<std::string, ChartNote> noteMap; // noteId -> note (последняя запись побеждает)

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;

        std::string op;
        if (ExtractJsonStringField(line, "op", op)) {
            if (op == "delete") {
                std::string id;
                if (ExtractJsonStringField(line, "noteId", id) && !id.empty()) {
                    noteMap.erase(id);
                }
                continue;
            }

            if (op == "update") {
                std::string id;
                std::string textUtf8;
                if (ExtractJsonStringField(line, "noteId", id) &&
                    ExtractJsonStringField(line, "text", textUtf8) &&
                    !id.empty()) {
                    auto it = noteMap.find(id);
                    if (it != noteMap.end()) {
                        it->second.text = WidenUtf8(textUtf8);
                    }
                }
                continue;
            }
        }

        ChartNote n;
        (void)ExtractJsonStringField(line, "noteId", n.noteId);
        (void)ExtractJsonUInt64Field(line, "createdUtcMs", n.createdUtcMs);
        (void)ExtractJsonUInt64Field(line, "configKey", n.configKey);
        (void)ExtractJsonStringField(line, "configSnapshot", n.configSnapshot);

        std::string presetUtf8;
        if (ExtractJsonStringField(line, "presetName", presetUtf8)) {
            n.presetName = WidenUtf8(presetUtf8);
        }

        std::string xAxisTypeStr;
        if (ExtractJsonStringField(line, "xAxisType", xAxisTypeStr)) {
            if (xAxisTypeStr == "TEMPERATURE") n.xAxisType = XAxisType::TEMPERATURE;
            else n.xAxisType = XAxisType::FRAME_NUMBER;
        }

        (void)ExtractJsonIntField(line, "xIndex", n.xIndex);
        (void)ExtractJsonDoubleField(line, "yLeft", n.yLeft);
        (void)ExtractJsonDoubleField(line, "yRight", n.yRight);
        (void)ExtractJsonBoolField(line, "hasYRight", n.hasYRight);

        std::string textUtf8;
        if (ExtractJsonStringField(line, "text", textUtf8)) {
            n.text = WidenUtf8(textUtf8);
        }

        if (!n.noteId.empty() && n.configKey != 0) {
            noteMap[n.noteId] = std::move(n);
        }
    }

    notes.reserve(noteMap.size());
    for (auto& kv : noteMap) {
        notes.push_back(std::move(kv.second));
    }
    std::sort(notes.begin(), notes.end(), [](const ChartNote& a, const ChartNote& b) {
        return a.createdUtcMs < b.createdUtcMs;
    });

    return true;
}

std::vector<size_t> NotesModule::GetNotesForConfig(uint64_t cfgKey) const {
    std::vector<size_t> idx;
    for (size_t i = 0; i < notes.size(); i++) {
        if (notes[i].configKey == cfgKey) idx.push_back(i);
    }
    return idx;
}

bool NotesModule::AppendNote(const ChartNote& note) {
    if (notesPath.empty()) return false;

    std::ofstream out(notesPath, std::ios::binary | std::ios::app);
    if (!out.is_open()) return false;

    std::string presetUtf8 = NarrowUtf8(note.presetName);
    std::string textUtf8 = NarrowUtf8(note.text);

    std::ostringstream ss;
    ss.setf(std::ios::fixed);
    ss.precision(8);

    ss << "{";
    ss << "\"noteId\":\"" << EscapeJsonString(note.noteId) << "\",";
    ss << "\"createdUtcMs\":" << note.createdUtcMs << ",";
    ss << "\"configKey\":" << note.configKey << ",";
    ss << "\"configSnapshot\":\"" << EscapeJsonString(note.configSnapshot) << "\",";
    ss << "\"presetName\":\"" << EscapeJsonString(presetUtf8) << "\",";
    ss << "\"xAxisType\":\"" << (note.xAxisType == XAxisType::TEMPERATURE ? "TEMPERATURE" : "FRAME_NUMBER") << "\",";
    ss << "\"xIndex\":" << note.xIndex << ",";
    ss << "\"yLeft\":" << note.yLeft << ",";
    ss << "\"hasYRight\":" << (note.hasYRight ? "true" : "false") << ",";
    ss << "\"yRight\":" << note.yRight << ",";
    ss << "\"text\":\"" << EscapeJsonString(textUtf8) << "\"";
    ss << "}\n";

    const std::string line = ss.str();
    out.write(line.data(), static_cast<std::streamsize>(line.size()));
    out.close();

    notes.push_back(note);
    return true;
}

bool NotesModule::DeleteNoteById(const std::string& noteId) {
    if (noteId.empty()) return false;
    if (notesPath.empty()) return false;

    std::ofstream out(notesPath, std::ios::binary | std::ios::app);
    if (!out.is_open()) return false;

    std::ostringstream ss;
    ss << "{";
    ss << "\"op\":\"delete\",";
    ss << "\"noteId\":\"" << EscapeJsonString(noteId) << "\",";
    ss << "\"deletedUtcMs\":" << NowUtcMs();
    ss << "}\n";

    const std::string line = ss.str();
    out.write(line.data(), static_cast<std::streamsize>(line.size()));
    out.close();

    // Удаляем из памяти
    auto it = std::find_if(notes.begin(), notes.end(), [&](const ChartNote& n) {
        return n.noteId == noteId;
    });
    if (it != notes.end()) {
        notes.erase(it);
    }

    // Маркеры пересчитаются при следующей отрисовке
    renderedMarkers.clear();
    return true;
}

bool NotesModule::UpdateNoteTextById(const std::string& noteId, const std::wstring& newText) {
    if (noteId.empty()) return false;
    if (notesPath.empty()) return false;

    auto it = std::find_if(notes.begin(), notes.end(), [&](const ChartNote& n) {
        return n.noteId == noteId;
    });
    if (it == notes.end()) return false;

    std::ofstream out(notesPath, std::ios::binary | std::ios::app);
    if (!out.is_open()) return false;

    std::string textUtf8 = NarrowUtf8(newText);

    std::ostringstream ss;
    ss << "{";
    ss << "\"op\":\"update\",";
    ss << "\"noteId\":\"" << EscapeJsonString(noteId) << "\",";
    ss << "\"updatedUtcMs\":" << NowUtcMs() << ",";
    ss << "\"text\":\"" << EscapeJsonString(textUtf8) << "\"";
    ss << "}\n";

    const std::string line = ss.str();
    out.write(line.data(), static_cast<std::streamsize>(line.size()));
    out.close();

    it->text = newText;
    return true;
}

void NotesModule::SetRenderedMarkers(std::vector<RenderedMarker>&& markers) {
    renderedMarkers = std::move(markers);
}

const ChartNote* NotesModule::HitTestRenderedMarkers(int chartNumber, int x, int y, int radiusPx) const {
    const int r2 = radiusPx * radiusPx;
    for (const auto& m : renderedMarkers) {
        if (m.chartNumber != chartNumber) continue;
        int dx = x - m.screenX;
        int dy = y - m.screenY;
        if ((dx * dx + dy * dy) <= r2) {
            if (m.noteIndex < notes.size()) return &notes[m.noteIndex];
        }
    }
    return nullptr;
}

uint64_t NotesModule::ComputeConfigKey(const ChartConfiguration& config) {
    const std::string snapshot = BuildConfigSnapshot(config);
    return Fnv1a64(reinterpret_cast<const uint8_t*>(snapshot.data()), snapshot.size());
}

std::string NotesModule::BuildConfigSnapshot(const ChartConfiguration& config) {
    std::vector<std::pair<std::string, int>> enabled;
    enabled.reserve(config.curves.size());
    for (const auto& c : config.curves) {
        if (!c.enabled) continue;
        enabled.emplace_back(c.id, c.yAxisGroup);
    }
    std::sort(enabled.begin(), enabled.end(), [](const auto& a, const auto& b) {
        if (a.first != b.first) return a.first < b.first;
        return a.second < b.second;
    });

    std::ostringstream ss;
    ss << "xAxisType=" << (config.xAxisType == XAxisType::TEMPERATURE ? "TEMPERATURE" : "FRAME_NUMBER");
    ss << ";activePreset=" << static_cast<int>(config.activePreset);
    ss << ";curves=";
    for (size_t i = 0; i < enabled.size(); i++) {
        if (i) ss << ",";
        ss << enabled[i].first << "@" << enabled[i].second;
    }
    return ss.str();
}

bool NotesModule::ParseConfigSnapshot(const std::string& snapshot,
                                      XAxisType& outXAxisType,
                                      PresetType& outPreset,
                                      std::vector<SnapshotCurveItem>& outCurves) {
    outXAxisType = XAxisType::FRAME_NUMBER;
    outPreset = PresetType::NONE;
    outCurves.clear();

    if (snapshot.empty()) return false;

    size_t xAxisPos = snapshot.find("xAxisType=");
    if (xAxisPos == std::string::npos) return false;
    size_t xAxisValueStart = xAxisPos + strlen("xAxisType=");
    size_t xAxisValueEnd = snapshot.find(';', xAxisValueStart);
    std::string xAxisValue = snapshot.substr(xAxisValueStart, xAxisValueEnd - xAxisValueStart);
    if (xAxisValue == "TEMPERATURE") outXAxisType = XAxisType::TEMPERATURE;
    else outXAxisType = XAxisType::FRAME_NUMBER;

    size_t presetPos = snapshot.find("activePreset=");
    if (presetPos != std::string::npos) {
        size_t presetStart = presetPos + strlen("activePreset=");
        size_t presetEnd = snapshot.find(';', presetStart);
        std::string presetValue = snapshot.substr(presetStart, presetEnd - presetStart);

        char* end = nullptr;
        long presetInt = strtol(presetValue.c_str(), &end, 10);
        if (end != presetValue.c_str()) {
            if (presetInt >= static_cast<long>(PresetType::NONE) &&
                presetInt <= static_cast<long>(PresetType::CUSTOM)) {
                outPreset = static_cast<PresetType>(presetInt);
            }
        }
    }

    size_t curvesPos = snapshot.find("curves=");
    if (curvesPos == std::string::npos) return true;

    size_t curvesStart = curvesPos + strlen("curves=");
    std::string curvesChunk = snapshot.substr(curvesStart);
    if (curvesChunk.empty()) return true;

    size_t tokenStart = 0;
    while (tokenStart <= curvesChunk.size()) {
        size_t tokenEnd = curvesChunk.find(',', tokenStart);
        std::string token = (tokenEnd == std::string::npos)
            ? curvesChunk.substr(tokenStart)
            : curvesChunk.substr(tokenStart, tokenEnd - tokenStart);

        if (!token.empty()) {
            size_t atPos = token.rfind('@');
            if (atPos != std::string::npos && atPos > 0 && (atPos + 1) < token.size()) {
                SnapshotCurveItem item;
                item.curveId = token.substr(0, atPos);

                char* end = nullptr;
                long axisGroup = strtol(token.c_str() + atPos + 1, &end, 10);
                if (end != (token.c_str() + atPos + 1)) {
                    item.yAxisGroup = (axisGroup == 1) ? 1 : 0;
                    outCurves.push_back(std::move(item));
                }
            }
        }

        if (tokenEnd == std::string::npos) break;
        tokenStart = tokenEnd + 1;
    }

    return true;
}

std::string NotesModule::NarrowUtf8(const std::wstring& w) {
    if (w.empty()) return {};
    int len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    if (len <= 0) return {};
    std::string out;
    out.resize((size_t)len);
    // C++14: std::string::data() возвращает const char*, поэтому пишем через &out[0]
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &out[0], len, nullptr, nullptr);
    return out;
}

std::wstring NotesModule::WidenUtf8(const std::string& s) {
    if (s.empty()) return {};
    int len = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    if (len <= 0) return {};
    std::wstring out;
    out.resize((size_t)len);
    // C++14: std::wstring::data() возвращает const wchar_t*, поэтому пишем через &out[0]
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &out[0], len);
    return out;
}

std::string NotesModule::GenerateNoteId() {
    // timestamp + random suffix
    uint64_t t = NowUtcMs();
    std::random_device rd;
    uint32_t r = (uint32_t)rd();
    std::ostringstream ss;
    ss << t << "_" << std::hex << r;
    return ss.str();
}

uint64_t NotesModule::NowUtcMs() {
    using namespace std::chrono;
    return (uint64_t)duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

uint64_t NotesModule::Fnv1a64(const uint8_t* data, size_t len) {
    const uint64_t FNV_OFFSET = 14695981039346656037ULL;
    const uint64_t FNV_PRIME = 1099511628211ULL;
    uint64_t h = FNV_OFFSET;
    for (size_t i = 0; i < len; i++) {
        h ^= (uint64_t)data[i];
        h *= FNV_PRIME;
    }
    return h;
}

