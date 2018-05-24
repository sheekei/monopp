#pragma once

#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string>

#define MONO_ZERO_LEN_ARRAY 1

template <typename T>
using non_owning_ptr = T*;
