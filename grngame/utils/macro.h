#pragma once

#include "grngame/utils/c_cpp.h"

#include "grngame/math/types.h"
#include "kvec.h"
#include "stdbool.h"

BEGIN_DECLARATIONS

// to avoid macro conflit
typedef kvec_t(float64) float64_vec_t;
typedef kvec_t(float32) float32_vec_t;
typedef kvec_t(int32) int32_vec_t;
typedef kvec_t(bool) bool_vec_t;
typedef kvec_t(char *) string_vec_t;
typedef kvec_t(const char *) string_const_vec_t;

END_DECLARATIONS