#pragma once
#include <cstddef>
#include <cstring>
#include <queue>
#include "FreeRTOS.h"

struct QueueDef {
    std::queue<std::vector<uint8_t>> q;
    size_t item_size;
    size_t max_len;
};
using QueueHandle_t = QueueDef*;

inline QueueHandle_t xQueueCreate(int len, size_t item_size) {
    auto* q = new QueueDef();
    q->item_size = item_size;
    q->max_len = len;
    return q;
}

inline BaseType_t xQueueSend(QueueHandle_t q, const void* item, TickType_t) {
    if (!q || q->q.size() >= q->max_len) return pdFALSE;
    std::vector<uint8_t> buf(q->item_size);
    std::memcpy(buf.data(), item, q->item_size);
    q->q.push(buf);
    return pdTRUE;
}

inline BaseType_t xQueueReceive(QueueHandle_t q, void* item, TickType_t) {
    if (!q || q->q.empty()) return pdFALSE;
    std::memcpy(item, q->q.front().data(), q->item_size);
    q->q.pop();
    return pdTRUE;
}

inline void xQueueReset(QueueHandle_t q) {
    if (!q) return;
    while (!q->q.empty()) q->q.pop();
}

inline size_t uxQueueMessagesWaiting(QueueHandle_t q) {
    return q ? q->q.size() : 0;
}
