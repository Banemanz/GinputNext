# Validation tools

`python tests/run_host_regressions.py` compiles actual Config, ModernControlsHook,
GTAAdapter and InputArbitration code with g++ against minimal Linux fixtures.
It executes each game variant, checks guarded patch rollback, fire/alias edges,
combat codes, controls locks, native staging, vehicle/frontend contexts,
ownership, finite config parsing and failed saves. The fixtures are not a game,
a Windows ABI test or a real SDL/controller backend test.

`python tests/cross_compile.py --compiler /path/to/i686-w64-mingw32-clang++
--sdk /path/to/plugin-sdk --sdl /path/to/SDL2-2.32.10 --output /path/to/objects`
(compose as one shell command) checks all 40 production translation units for
Windows x86 against the exact SDK pin. Used llvm-mingw 20260908 UCRT and the
official SDL2 VC development headers. Only COFF objects are generated. This is
not an MSVC build and does not link against the SDK Release libraries.

The check-only cross_compat headers provide Windows.h case matching, unsigned
char char_traits for libc++, and a pointer cast for III SDK overloaded metadata
that MSVC accepts as an extension. The pinned SDK checkout is unmodified.
Production vcxproj builds do not include these compatibility headers.

`python tests/audit_idb_calls.py --help` describes dump audit arguments. It
checks exact CALL instruction bytes and destinations for all Modern sites.

Existing *_smoke.py scripts and cross_game_signature_audit.py are inexpensive
source/math checks retained from earlier versions. Their success alone is not
behavioral validation. modern_controls_smoke.py now checks project manifests;
compiled regressions supersede its old assertions enforcing v24's faulty
blank-field policy.

See new chat/VALIDATION_RUN.txt for actual results and HANDOFF.md for the
remaining Windows build and in-game acceptance gates.
