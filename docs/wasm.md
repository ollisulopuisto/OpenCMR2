# WebAssembly / browser port feasibility

Status: feasibility probe, not a runnable port. The full game compiles and
links to WebAssembly today with the build-system deltas on the
`wasm-feasibility` branch; the game's generated ES 3.00 shaders compile and
link in a real WebGL2 context. No game code changes are needed for the build.

## Build

Requires emsdk (probe used emcc 6.0.13) and two **host** build tools on PATH:

- glslang 16.6.0 release binary (`glslang`), from
  https://github.com/KhronosGroup/glslang/releases
- spirv-cross vulkan-sdk-1.4.363.0, built natively from
  https://github.com/KhronosGroup/SPIRV-Cross (the in-tree FetchContent build
  produces a wasm binary that cannot run as a build tool)

Then:

    emcmake cmake -S . -B build-wasm -DOPENCMR2_VENDORED_SDL=ON -DCMAKE_BUILD_TYPE=Release
    cmake --build build-wasm -j

Produces `build-wasm/src/opencmr2.js` + `opencmr2.wasm` (~4 MB).

## Build-system deltas on this branch (all CMake, no game code)

1. Vendored SDL3: static instead of shared under Emscripten.
2. Skip the post-build SDL soname copy under Emscripten (no shared lib).
3. `-msimd128` under Emscripten; `-msse2 -mfpmath=sse` only on real x86
   (`CMAKE_SYSTEM_PROCESSOR` reflects the host when cross-compiling).
4. Skip `tests/` under Emscripten (x86 MS-asm reference), same pattern as Android.
5. Shader translation targets GLSL ES 3.00 (`--es --version 300`) instead of
   GLSL 330 under Emscripten.
6. Link flags: `-sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sFULL_ES3=1
   -sALLOW_MEMORY_GROWTH=1 -sSTACK_SIZE=4MB`.

## Browser probe (no game data needed)

`tools/wasm-probe/probe.cpp` is a minimal SDL3 + GL harness that initializes
video exactly like the game (GL 3.3 core request), then checks function
availability and compiles/links the game's real generated ES 3.00 shaders
(`fixed3d.vert` + `fixed.frag`). Build it against the vendored SDL3 and the
build's generated `shaders_spirv.h`, serve the directory, open in Chrome
(headless works with `--use-angle=swiftshader --enable-unsafe-swiftshader`).

Observed on headless Chrome (SwiftShader WebGL2), 2026-11-01:

- SDL3 video init OK (emscripten driver), GL context created:
  "OpenGL ES 3.0 (WebGL 2.0 (OpenGL ES 3.0 Chromium))".
- `fixed3d.vert` + `fixed.frag` compile OK, program links OK.
- Function gaps through `SDL_GL_GetProcAddress`:
  - `glGetTexImage` MISSING - no WebGL2 equivalent (desktop-only readback);
    needs a framebuffer-readback rewrite at its use sites.
  - `glClearDepth` MISSING - ES alias is `glClearDepthf`; trivial mapping.
  - `glDrawElementsBaseVertex` MISSING via GetProcAddress though it is core
    GLES3; the GL renderer loads every function through SDL_GL_GetProcAddress,
    so ES3 core entry points need a direct-symbol fallback in the loader.
  - OK: glGenSamplers, glMapBufferRange, glBlitFramebuffer,
    glCompressedTexImage2D, glGenVertexArrays, glCopyTexSubImage2D.

## Known remaining gaps before a runnable game

- `src/render/gl_effects_sources.h` holds hand-written `#version 330` shaders
  (effects, weather); they need ES 3.00 variants.
- The GL renderer's `glGetTexImage` use needs a WebGL2-compatible readback.
- Renderer choice: only the OpenGL renderer can work in a browser today.
  SDL_GPU has no released web backend: PR https://github.com/libsdl-org/SDL/pull/12046
  (WebGPU backend, closed unmerged March 2025) and draft PR
  https://github.com/libsdl-org/SDL/pull/16020 (experimental WebGPU SDL_GPU
  backend, open). Emscripten itself is a supported SDL3 platform.
- Game data: the game needs the user's own CMR2 assets, packaged with
  `--preload-file` or fetched at runtime. Assets are private and must never be
  committed or published.
