#pragma once

#include "type.h"

#include <cstdint>
#include <string>

struct Symbol {
    std::string id;
    FundType type;

    std::uint32_t level;
};