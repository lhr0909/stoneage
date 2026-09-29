# macOS feasibility notes

Inspected source revision: `1997fc20456dbda36d181b9680ae10bed2e9cdf9`.

This is not a ready-to-run Mac distribution. The promising route is a 32-bit
Linux server and a Windows client running through a compatibility layer.
Neither a complete server startup nor client gameplay has been verified.

## Server

The source is packaged in `石器时代服务器端最新完整源代码/Serv.zip`.
`说明.txt` explicitly requests **32-bit CentOS** for compilation.

On an Apple Silicon Mac with OrbStack, this command successfully returned
`i686`:

```sh
docker run --rm --platform linux/386 i386/debian:bookworm uname -m
```

An experimental build in that image, with GCC 12, Make, MariaDB client
development libraries and zlib development headers, found these blockers:

1. Both server makefiles contain generated dependencies on obsolete absolute
   CentOS paths. Stock builds fail on `/usr/include/sys/cdefs.h`. Remove the
   generated sections after `# DO NOT DELETE THIS LINE` in a disposable build
   copy, then regenerate dependencies for the chosen build environment.
2. After that adjustment, `gmsv` stops because `char/ls2data.h` is absent from
   the archive. The listed generator `~/bin/convertStringtographicnumber.perl`
   is not included either. A correct matching generated header or generator
   is needed; an empty placeholder would not establish correctness.
3. The account server's `longzoro.sh` produces an invalid multiline C string
   under Debian's `/bin/sh` because `echo` interprets `\n`. Use portable
   `printf` output when adapting the build.
4. The archive has no `gmsv/data/` world-data directory. The included Lua scripts,
   configs, and SQL schema do not replace maps, NPC data, or other world data.

These are the first observed failures, not an exhaustive porting checklist.
The newer server also includes Linux `epoll` headers and LuaJIT 2.0.3. Native
macOS/ARM64 compilation will require platform and pointer-width work beyond
fixing makefiles.

## Client

The Visual Studio project in `石器时代8.5客户端最新源代码` targets Win32, mostly
with toolset `v141`; some configurations use `v120_xp`. The repository note
suggests Visual Studio 2015/2017.

The client depends on Win32, DirectDraw, DirectSound, DirectInput, WinSock,
Windows IME and binary import libraries. Its source expects files including
`data/real_136.bin`, `adrn_136.bin`, `spr_115.bin`, and `spradrn_115.bin`; these
game assets are not included. VMProtect runtime calls are present, but the
corresponding DLL is absent. `system/gamemain.cpp` installs a timer whose
callback checks for a debugger or virtual machine and exits if detected.

For a Mac experiment, first obtain a complete compatible Windows client with
its assets and dependencies. [CrossOver](https://www.codeweavers.com/support/docs/crossover-mac/index)
can run Windows programs on Mac, but this client has **not** been tested in it.
A Windows VM is another experiment, subject to the checks described above.
Building the current client source calls for a compatible Windows/MSVC build
environment; it is not an Xcode project.

## Relationship to the older server

[lhr0909/stone-age](https://github.com/lhr0909/stone-age) is a different,
older 2.5 server tree that includes server-side world data. It is a smaller
starting point for a local-server experiment. The client's `VER25` build
configurations do not prove that its protocol, encryption keys, assets, and
feature flags match that older server. Verify the handshake and gameplay
before combining the repositories. Do not blindly import this repo's
`CSA.sql` into the older server's database.
