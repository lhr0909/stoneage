# Native VER25 port: protocol and rendering milestones

**Native protocol integration and an SDL2 asset preview work; the graphical game
is not playable yet.**
It compiles `newproto/autil.cpp` directly from this repository's newer client,
with `_SA_VERSION_25` and `_SA_VERSION_SPECIAL`. It does not use the mobile
client as its gameplay baseline.

## Build and check on macOS

Requires Xcode Command Line Tools and CMake. Start the 2.5 Docker stack from
`lhr0909/stone-age` first, then run from this repository:

```sh
./script/check_native25.sh player stoneage
```

For a character lifecycle check:

```sh
./script/check_native25.sh player stoneage MacPlayer
```

The optional character argument creates a character only when the account has no
characters, enters the world, then saves/logs out. It refuses to replace different
existing characters. Omitting all arguments runs only local codec tests. The
network probe is restricted to `127.0.0.1:9065` and has response timeouts.

This is a command-line integration check. It does not open a game window or
support movement/battle controls. The separate asset preview below is offline. `build/native25/stoneage25-probe` is an ARM64
Mac executable on Apple Silicon; Docker runs only the servers.

## Native SDL2 asset preview

With SDL2 2.0.18+ installed (`brew install sdl2` on macOS):

```sh
./script/preview_native25.sh /path/to/stoneage2.5
```

This opens an animated character preview using the actual 2.5 files. Left/right
select a sprite; up/down select its animation; Space pauses; Escape exits.
It is a renderer development tool, with no server connection or gameplay UI.

`assets25.cpp` ports the indexed RD decoding from `system/unpack.cpp`, palette
loading from `system/directdraw.cpp`, and the layouts in `loadrealbin.h` and
`loadsprbin.h`. It reads explicit little-endian fields, preserving 80-byte graphic
records, 12-byte sprite indices/animation headers, and 10-byte frames on ARM64.
It uses `real_15.bin`, `adrn_15.bin`, `spr_4.bin`, `spradrn_5.bin`, and
`data/pal/Palet_1.sap` from the 2.5 pack. Duplicate graphic and sprite IDs keep the last record,
as in the original loaders; `0xffffffff` sprite frames have no image. Index zero
is transparent; SAP BGR entries occupy slots 16–239. No game assets are committed.

The preview uploads RGBA pixels to [SDL2 textures](https://wiki.libsdl.org/SDL2/SDL_CreateTextureFromSurface)
and draws them using [SDL_RenderCopy](https://wiki.libsdl.org/SDL2/SDL_RenderCopy),
with the original graphic/frame offsets. macOS Cocoa rendering was exercised and
captured successfully. Linux/Windows execution has not been tested; SDL is the
portable graphics layer, while the current network probe still uses POSIX sockets.

The complete scan of the downloaded `SA2.5-20260823` pack examined 285,143 unique
graphics and 975 unique sprites (539,374 frames after duplicate IDs are replaced).
285,061 graphics decoded successfully; **82 records were rejected** for invalid
dimensions or compressed runs. The scan returned nonzero as intended, without
sanitizer memory errors. These results apply to `real_15.bin` SHA-256
`57b47c00d42017538716a5b9da0ea0a284194d95fad47e3c15eac36735ed70d1`.

The downloaded archive contains malformed graphics; the bounded decoder reports
these instead of trusting dimensions and writing beyond the output buffer.
The raw-image path also handles the original encoder's pointer-valued `RD.size`
field by checking the actual ADRN record length, as required by the original
decoder. Selecting an affected animation stops the preview with the bitmap ID and error.
Repair/compatibility handling is still needed before general gameplay. To scan
all unique graphic records (returns nonzero for rejected records):

```sh
build/native25/stoneage25-assets-test /path/to/stoneage2.5
# Automated native rendering smoke check, then exit:
build/native25/stoneage25-assets /path/to/stoneage2.5 --smoke /tmp/stoneage25.bmp
```

## What VER25 actually changes

The Visual Studio `VER25_DEBUG` configuration defines `_SA_VERSION_25` and
`_SA_VERSION_SPECIAL`, alongside Win32 flags. The version header then selects
2.5 features, but global settings still enable the newer private server's packet
format, extended login, Lua integration, VMProtect, and Windows APIs.

The unmodified configuration is not wire-compatible with the separate 2.5 server:

| Setting | Upstream VER25 | Local 2.5 server profile |
| --- | --- | --- |
| Greeting | `N` | `L` |
| Initial/running key | `shiqi` / `shiqi.hk` | `cary` / `cary` |
| Packet encoding | `_NEWNET_` TEA path | Existing legacy codec |
| Message IDs | Send +13, receive -23 | Unshifted |
| Login fields | Account, password, machine ID, server selection, IP | Account and password |

`native/local25_profile.h` makes those differences explicit without changing
normal Windows builds. The local packet envelope also uses the server's `&;`
prefix. The original login call now has its missing two-field fallback when
extended login is disabled. `STONEAGE_PROTOCOL_ONLY` isolates the codec from UI
headers for this native target; it is not a claim that the rest of the game builds.

## Verified behavior

The probe checks the server greeting and response checksums. Tests cover codec
roundtrips, integer boundaries, malformed termination, and empty input under
AddressSanitizer/UndefinedBehaviorSanitizer. Integration checks cover successful
login, wrong-password rejection, character listing, creation, entry into the
world, save/logout, and relogin.

## Remaining work before a playable Mac client

1. Replace the original VER25 DirectDraw rendering and surface access with SDL2,
   preserving the original gameplay/UI code. The existing mobile SDL port can
   inform the platform implementation, but is not the baseline.
2. Replace Win32 entry/event loop, input/IME, font/audio, file paths, and Winsock
   integration. Build the required Lua functionality from portable source and
   remove the local target's proprietary launcher dependencies.
3. Integrate the new 2.5 asset reader into the original gameplay renderer and
   resolve malformed records in the downloaded pack. The original VER25 entrypoint
   still defaults to newer filenames; the source's custom UI may need
   graphics not present in a generic 2.5 pack. Resolve that by checking actual
   referenced IDs, not by renaming asset files.
4. Adapt and verify gameplay-status fields and feature flags against the older
   server. Successful authentication does not establish map/battle/UI parity.
5. Verify movement, NPC interaction, battle, save/restart, and native app packaging.

The earlier mobile-source build experiment is intentionally excluded from this
implementation. No downloaded Windows executables are required or executed.
