#include "ambient_backend.h"

#include "ambient_backend_modern.h"
#include "ambient_state_store.h"

static volatile ambient_backend_kind_t g_backend_kind = AMBIENT_BACKEND_MODERN;

/* Initialize backend facade and shared state store. */
void ambient_backend_init(void)
{
    ambient_state_store_init();
    g_backend_kind = AMBIENT_BACKEND_MODERN;
}

/* Legacy/modern selector is intentionally pinned to modern backend. */
void ambient_backend_set_kind(ambient_backend_kind_t kind)
{
    (void)kind;
    g_backend_kind = AMBIENT_BACKEND_MODERN;
}

/* Return active backend kind for diagnostics. */
ambient_backend_kind_t ambient_backend_get_kind(void)
{
    return g_backend_kind;
}

/* Forward unified CAN state into active backend implementation. */
void ambient_backend_on_can_state(const can_state_t *can_state, uint32_t now_ms)
{
    if (!can_state) return;
    ambient_backend_modern_on_can_state(can_state, now_ms);
}
