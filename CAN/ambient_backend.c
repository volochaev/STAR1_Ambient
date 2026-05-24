#include "ambient_backend.h"

#include "ambient_source_arbiter.h"
#include "ambient_state_store.h"

/* Initialize backend facade and shared state store. */
void ambient_backend_init(void)
{
    ambient_state_store_init();
    ambient_source_arbiter_init();
}

/* Forward unified CAN state into source arbiter. */
void ambient_backend_on_can_state(const can_state_t *can_state, uint32_t now_ms)
{
    ambient_source_arbiter_apply(can_state, now_ms);
}
