# v4 build fix

Observed MSVC error:

```text
SDL2Dyn.cpp(228,75): error C2440:
'return': cannot convert from 'int' to 'SDL_bool'
```

Exact bad wrapper in v3:

```cpp
SDL_bool JoystickIsHaptic(SDL_Joystick* j) {
    return p_SDL_JoystickIsHaptic(j);
}
```

Correct SDL2 2.32.10 declaration:

```cpp
int SDL_JoystickIsHaptic(SDL_Joystick* joystick);
```

Correct v4 wrapper:

```cpp
int JoystickIsHaptic(SDL_Joystick* j) {
    return p_SDL_JoystickIsHaptic(j);
}
```

No cast is used. The type itself is corrected.

`SDLSignatureAudit.h` now checks this and several adjacent SDL wrapper return
types at compile time.


# v5 SDL DLL/runtime fix

Observed runtime:

```text
SDL2 export missing: SDL_GameControllerGetType
Could not locate GInputNext.SDL2.dll/SDL2.dll ...
```

The ASI found DLL candidates but rejected them because the loaded SDL2 did not
export the expected modern controller API.

v5:
- downloads official SDL2 2.32.10 VC dev release from GitHub,
- selects `lib\x86\SDL2.dll`,
- verifies PE machine `0x014C`,
- verifies exports with VS2022 x86 `dumpbin`,
- copies it as `GInputNext.SDL2.dll`,
- verifies the final packaged DLL is x86,
- makes nonessential newer SDL exports optional at runtime.


# v8 build control-flow fix

Root causes of the "downloads deps then closes / no dist folder" failure:

- `FETCH_SDL2.ps1` used `exit 0` when dependency validation succeeded. Since
  `BUILD_PROJECT.ps1` invokes that script in-process with `&`, `exit` could end
  the entire build PowerShell host before MSBuild.
- `BUILD_PROJECT.ps1` used `$LASTEXITCODE` after PowerShell script calls. That
  variable is only trustworthy for native executables.

Fixes:
- dependency scripts return normally;
- dependency calls rely on terminating errors, not `$LASTEXITCODE`;
- MSBuild is launched by `Start-Process -Wait -PassThru` and its real ExitCode
  is checked;
- BAT wrappers pause when run directly;
- per-game transcript logs are written to `build_logs`;
- BUILD_ALL prepares dependencies once and builds all three targets sequentially.


# v9 illegal path fix

Observed:

```text
Test-Path : Illegal characters in path.
FETCH_SDL2.ps1:17
```

Cause: PREPARE_DEPS.bat passed `-Root "%~dp0"`. `%~dp0` ends in `\`, and the
quoted trailing slash can produce a malformed argument containing a literal
quote when crossing cmd.exe -> powershell.exe.

Fix:
- PREPARE_DEPS.bat no longer passes -Root.
- PREPARE_SDL2_X86.bat no longer passes -Root.
- fetch scripts derive root from `$MyInvocation.MyCommand.Path`.
- explicit Root values are trimmed/canonicalized defensively.
- canonical paths are printed before Test-Path.
