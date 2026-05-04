#pragma once

#include <cassert>

#define KE_ASSERT(expr)        assert(expr)
#define KE_ASSERT_MSG(expr, m) assert((expr) && (m))
#define KE_CHECK(expr)         do { if(!(expr)) { KE_LOG_ERROR("Check failed: {}", #expr); return false; } } while(0)
