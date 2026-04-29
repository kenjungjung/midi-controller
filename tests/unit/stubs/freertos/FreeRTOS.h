#pragma once
#include <cstdint>

using BaseType_t = int;
using UBaseType_t = unsigned int;
using TickType_t = uint32_t;

#define pdTRUE  1
#define pdFALSE 0
#define portMAX_DELAY (~(TickType_t)0)

constexpr TickType_t pdMS_TO_TICKS(TickType_t ms) { return ms; }
