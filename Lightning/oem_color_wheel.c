#include "oem_color_wheel.h"

static const scene_rgb8_t k_oem_wheel[OEM_WHEEL_COUNT] = {
    {255u, 20u, 20u},   /* red */
    {255u, 122u, 26u},  /* orange */
    {255u, 218u, 26u},  /* yellow */
    {196u, 238u, 44u},  /* light yellow (green-yellow) */
    {54u, 220u, 54u},   /* green */
    {88u, 244u, 166u},  /* light green */
    {116u, 212u, 255u}, /* sky blue / ice blue */
    {52u, 142u, 255u},  /* blue */
    {20u, 62u, 196u},   /* deep blue */
    {148u, 66u, 232u},  /* purple */
    {255u, 112u, 206u}, /* pink */
    {232u, 244u, 255u}, /* white (cold) */
};

const scene_rgb8_t *oem_color_wheel_get(void)
{
    return k_oem_wheel;
}

uint8_t oem_color_wheel_count(void)
{
    return (uint8_t)OEM_WHEEL_COUNT;
}

scene_rgb8_t oem_color_wheel_at(uint8_t idx)
{
    return k_oem_wheel[idx % (uint8_t)OEM_WHEEL_COUNT];
}
