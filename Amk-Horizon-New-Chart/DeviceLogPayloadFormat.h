#pragma once

#include <cstddef>
#include <cstdint>

/// Если payload — 240 B кадра данных и сигнатура известна адаптерам, добавляет в лог
void DeviceLogTryAppendVariantAForDataPayload240(const uint8_t* payload, size_t payloadLen);
