#pragma once

#include <cstddef>
#include <cstdint>

/// Если payload — 240 B кадра данных и сигнатура известна адаптерам, добавляет в лог
/// многострочный разбор полей (вариант A). Иначе ничего не делает.
void DeviceLogTryAppendVariantAForDataPayload240(const uint8_t* payload, size_t payloadLen);
