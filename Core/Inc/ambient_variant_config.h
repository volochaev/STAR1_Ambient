#pragma once

#include "ambient_backend.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Active backend kind selected by firmware variant.
 */
#ifndef AMB_VARIANT_BACKEND_KIND
#define AMB_VARIANT_BACKEND_KIND AMBIENT_BACKEND_MODERN
#endif

/**
 * @brief Return backend kind selected at compile time.
 */
static inline ambient_backend_kind_t ambient_variant_backend_kind(void)
{
    return (ambient_backend_kind_t)AMB_VARIANT_BACKEND_KIND;
}

#ifdef __cplusplus
}
#endif
