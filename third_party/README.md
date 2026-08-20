# Third-party dependencies

## SDL2 2.32.10

The build downloads the official Visual C++ development archive:

`SDL2-devel-2.32.10-VC.zip`

v3 uses the SDL headers but deliberately does not import-link `SDL2.lib`.

The official x86 `SDL2.dll` is copied into each output bundle under the private
name:

`GInputNext.SDL2.dll`

The ASI loads that exact file from its own directory at runtime.

## Plugin-SDK

User-supplied checkout/build:

`62fd0ef66f704cf7e649607b57cc6e8097ed6e58`

Required Release libs:

- `plugin_iii.lib`
- `plugin_vc.lib`
- `plugin.lib`
