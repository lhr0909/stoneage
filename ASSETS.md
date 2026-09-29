# Downloaded client asset checks

Checked on 2026-09-29 against client source revision
`1997fc20456dbda36d181b9680ae10bed2e9cdf9`.

## Result

Both community-hosted archives contain readable Stone Age graphics and animation
data. The **8.0 pack is the better match for this client source**: it includes
all six default asset filenames and the truecolor pair. This is static asset
validation, not proof of executable safety, client startup, login, or gameplay.
No downloaded executables were run.

| Item | 2.5 client | 8.0 client |
| --- | --- | --- |
| Archive bytes | 553,320,338 | 1,214,843,601 |
| Graphics/index pair | `real_15.bin`, `adrn_15.bin` | `real_136.bin`, `adrn_136.bin` |
| Sprite/index pair | `spr_4.bin`, `spradrn_5.bin` | `spr_115.bin`, `spradrn_115.bin` |
| Graphics index records | 285,290 | 439,403 |
| Sprite index records | 981 | 1,789 |
| Animation frame records | 540,826 | 764,834 |
| Truecolor pair | Absent | `realtrue_13.bin`, `adrntrue_5.bin` (861 index records) |
| Client map files | 2,041 | 2,344 |
| Palette files | 16 | 16 |

Map counts exclude the bundled helper tools' separate map directories.
Graphics record counts include duplicate IDs: 147 repeated records in 2.5 and
92 in 8.0. The source indexes graphics by ID and lets later entries overwrite
earlier entries; counts therefore do not equal distinct graphics IDs.

## Downloads and checksums

Source page: <https://99ds.com/>. These are community-hosted packages, not
verified official distributions. The filenames at the stable download URLs
can change over time; the hashes below identify the copies inspected.

- [2.5 RAR](https://99ds.com/downloads/windows/Stoneage2.5-Windows.rar), served as `SA2.5-20260823.rar`.
  SHA-256: `0e157c0fcfe2bcf91826907d580d4403138d86fab9488aa39cd9494af17449ae`
- [8.0 ZIP](https://99ds.com/downloads/windows/Stoneage8.0-Windows.zip), served as `SA8.0-20260804.zip`.
  SHA-256: `48bef440851d361d699358ab70ffbd38f6b92c6263fb243863987fb54998475e`

The RAR extracted successfully through macOS libarchive without reported errors.
Every ZIP entry was fully extracted through Python's CRC-checking ZIP reader.
Chinese ZIP filenames were decoded with GB18030 metadata support.

## Checks performed

The checker used the Win32 layouts from `systeminc/loadrealbin.h` and
`systeminc/loadsprbin.h`: 80-byte graphics index records, 40-byte truecolor
index records, 12-byte sprite index/animation headers, and 10-byte frames.

- Every standard graphics record points inside its paired data file; every
  referenced header starts with `RD`, and header/index dimensions agree.
- Every sprite animation and frame sequence stays inside its paired sprite
  file. Sprite IDs fit the source's configured sprite array.
- The 8.0 truecolor records also stay in bounds and point to `RD` data after
  their palettes, using the read layout in `Read16BMP`.
- Sixteen representative sprite frames from **each** pack were decoded with
  the source's RLE algorithm and palettes into PNG contact sheets and visually
  inspected. The sprites are recognizable and correctly colored. This is
  sampled pixel decoding, not exhaustive decoding of every bitmap.

For both packs, the only animation bitmap value absent from the graphics index
is `0xFFFFFFFF` (3,492 references in 2.5; 4,760 in 8.0). It appears to be an
empty-frame sentinel; its full runtime handling was not verified. No other
missing bitmap IDs were found.

## Integration implications

Use the **8.0 pack's complete `data/` and `map/` directories** for the first
experiment with this repository's client. Its six default filenames match
`system/main.cpp`, including the high-color assets. Preserve files as downloaded
while testing; renaming the 2.5 pack does not add missing newer graphics or
truecolor data. The 2.5 pack remains useful for a separate legacy-client baseline.

Both packs include Windows executables and helper tools, which were inventoried
but not executed. Neither pack supplies `VMProtectSDK32.dll` or the `msjh.ttf`
font referenced by this source. Windows build dependencies, protocol/key
compatibility, and the previously observed account-server crash remain separate
work. Asset availability does not resolve those issues.
