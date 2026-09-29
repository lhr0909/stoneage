# Native VER25 port: protocol milestone

**The native protocol target works; the graphical game is not ported yet.**
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
support movement/battle controls. `build/native25/stoneage25-probe` is an ARM64
Mac executable on Apple Silicon; Docker runs only the servers.

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
3. Port the original graphics/sprite loaders to explicit disk layouts and the
   downloaded 2.5 asset filenames (`real_15`, `adrn_15`, `spr_4`, `spradrn_5`).
   VER25 still defaults to newer filenames; the source's custom UI may need
   graphics not present in a generic 2.5 pack. Resolve that by checking actual
   referenced IDs, not by renaming asset files.
4. Adapt and verify gameplay-status fields and feature flags against the older
   server. Successful authentication does not establish map/battle/UI parity.
5. Verify movement, NPC interaction, battle, save/restart, and native app packaging.

The earlier mobile-source build experiment is intentionally excluded from this
implementation. No downloaded Windows executables are required or executed.
