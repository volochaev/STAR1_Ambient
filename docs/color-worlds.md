# Color Worlds (M4.1)

## Purpose

`Color Worlds` is the animated ambient mode enabled by HU `Ambient Effect`.

- `effect = 0`: static OEM color (no world animation)
- `effect = 1`: world-first animated mode

This implementation uses a fixed world registry (not neighbor-based anchor cycling).

## OEM Color Wheel (Canonical)

Single source of truth for static mode and world selection reference:

| wheel index | OEM color |
|---|---|
| 0 | Red |
| 1 | Orange |
| 2 | Yellow |
| 3 | Light Yellow (Green-Yellow) |
| 4 | Green |
| 5 | Light Green |
| 6 | Sky Blue (Ice Blue) |
| 7 | Blue |
| 8 | Deep Blue |
| 9 | Purple |
| 10 | Pink |
| 11 | White (Cold White) |

## World Registry

Canonical 10 worlds + 3 generated coverage worlds:

| id | world | type | generated | dominant color |
|---|---|---|---|---|
| 0 | Ocean Blue | DUAL | no | blue-cyan |
| 1 | Red Moon | SPECTRAL | no | magenta-red |
| 2 | Miami Rose | DUAL | no | neon pink-blue |
| 3 | Malibu Sunset | SPECTRAL | no | orange-sunset |
| 4 | Fiery White | DUAL | no | warm white-orange |
| 5 | Purple Sky | SPECTRAL | no | violet-sky blue |
| 6 | Jungle Green | SPECTRAL | no | emerald-mint |
| 7 | Glacier Blue | DUAL | no | ice blue-deep blue |
| 8 | Sun Yellow | DUAL | no | yellow-amber |
| 9 | Silver Light | LUMA | no | white brightness-wave |
| 10 | Citrus Dawn | SPECTRAL | yes (`generated_v1`) | yellow-green sunrise |
| 11 | Mint Breeze | DUAL | yes (`generated_v1`) | mint-ice cyan |
| 12 | Midnight Tide | SPECTRAL | yes (`generated_v1`) | deep blue ultramarine |

## Selection Policy (`effect=1`)

`Ambient Color` acts as selector through fixed LUT only (runtime deterministic).

| HU color | fixed world |
|---|---|
| Red | Red Moon |
| Orange | Malibu Sunset |
| Yellow | Sun Yellow |
| Light Yellow | Citrus Dawn (`generated_v1`) |
| Green | Jungle Green |
| Light Green | Mint Breeze (`generated_v1`) |
| Sky Blue | Glacier Blue |
| Blue | Ocean Blue |
| Deep Blue | Midnight Tide (`generated_v1`) |
| Purple | Purple Sky |
| Pink | Miami Rose |
| White | Silver Light |

## HU Color -> Closest Worlds (Ranked)

Ranked metric mapping retained for tuning reference only (not used in runtime):

| HU color | #1 | #2 | #3 |
|---|---|---|---|
| Red | Red Moon | Malibu Sunset | Sun Yellow |
| Orange | Malibu Sunset | Sun Yellow | Fiery White |
| Yellow | Sun Yellow | Citrus Dawn | Malibu Sunset |
| Light Yellow | Citrus Dawn | Sun Yellow | Malibu Sunset |
| Green | Jungle Green | Mint Breeze | Citrus Dawn |
| Light Green | Mint Breeze | Jungle Green | Glacier Blue |
| Sky Blue | Glacier Blue | Ocean Blue | Mint Breeze |
| Blue | Ocean Blue | Glacier Blue | Purple Sky |
| Deep Blue | Ocean Blue | Glacier Blue | Purple Sky |
| Purple | Purple Sky | Miami Rose | Ocean Blue |
| Pink | Miami Rose | Red Moon | Fiery White |
| White | Silver Light | Glacier Blue | Ocean Blue |

This preserves HU-to-visual color affinity while keeping fixed world palettes.

## Animation Types

- `DUAL`: two-color wave with triangle blend.
- `SPECTRAL`: 3-stop/4-stop sequential gradient wave.
- `LUMA`: constant hue with brightness wave.

Runtime tuning updates (M4.1 pack):
- world-level brightness normalization (`luma_norm`) for perceived parity.
- `Silver Light` runs slowest period; spectral worlds are medium; dual worlds are faster.
- `Midnight Tide` upgraded to 3-stop deep-blue spectral progression.
- world-switch crossfade (`~420 ms`) to remove hard scene jumps.
- lounge preset applies extra saturation softening for high-energy worlds (`Miami Rose`, `Sun Yellow`).

## Zone Scope and Phase

World base is applied to all ambient zones:
- strip
- handle zone (RGB zone)
- storage
- footwell

Zones use phase offsets to avoid flat synchronization.
`Miami Rose` uses upper/lower counter-phase behavior.

## Preset Interaction

`classic/calm/sport/lounge` affect only dynamic behavior:
- temporal speed scale
- gain/contrast shaping

Preset does not change world palette composition.

## Diagnostics

Runtime diagnostics expose:
- `world_mode` (0/1)
- `world_id`
- `world_generated`
- `world_dominant_rgb`
- `world_selection_distance`

## Notes

- World palettes are fixed and curated.
- Missing coverage from canonical 10 worlds is solved by generated worlds, not by runtime anchor-neighbor logic.
