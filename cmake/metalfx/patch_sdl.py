#!/usr/bin/env python3
"""Apply the isolated, idempotent backend extension to pinned SDL sources."""
import pathlib
import sys

root = pathlib.Path(sys.argv[1])
path = root / 'src/gpu/metal/SDL_gpu_metal.m'
s = path.read_text()
marker = '/* OPENCMR2_METALFX_EXTENSION */'
if marker in s:
    sys.exit(0)
if 'VERSION "3.4.18"' not in (root / 'CMakeLists.txt').read_text():
    raise SystemExit('MetalFX extension requires pinned SDL3.4.18')

def replace(old, new):
    global s
    if s.count(old) != 1:
        raise SystemExit('SDL backend anchor changed: ' + old[:60])
    s = s.replace(old, new, 1)

replace('#include <Metal/Metal.h>', '#include <Metal/Metal.h>\n#if defined(SDL_PLATFORM_MACOS) && __has_include(<MetalFX/MetalFX.h>)\n#import <MetalFX/MetalFX.h>\n#define OPENCMR2_METALFX 1\n#endif\n' + marker)
replace('    id<MTLCommandQueue> queue;', '''    id<MTLCommandQueue> queue;
#ifdef OPENCMR2_METALFX
    id<MTLFXSpatialScaler> opencmr2Scaler;
    SDL_GPUTexture *opencmr2Output;
    Uint32 opencmr2InputW, opencmr2InputH, opencmr2OutputW, opencmr2OutputH;
#endif''')
replace('    // Flush any remaining GPU work...', '''#ifdef OPENCMR2_METALFX
    if (renderer->opencmr2Output) {
        METAL_ReleaseTexture(device->driverData, renderer->opencmr2Output);
        renderer->opencmr2Output = NULL;
    }
    renderer->opencmr2Scaler = nil;
#endif
    // Flush any remaining GPU work...''')
# DestroyDevice precedes ReleaseTexture definition.
replace('static bool METAL_Wait(SDL_GPURenderer *driverData);', 'static bool METAL_Wait(SDL_GPURenderer *driverData);\nstatic void METAL_ReleaseTexture(SDL_GPURenderer *driverData, SDL_GPUTexture *texture);')
code = pathlib.Path(__file__).with_name('SDL_metalfx.m.inc').read_text()
replace('// Compute State\n', '#ifdef OPENCMR2_METALFX\n' + code + '\n#endif\n\n// Compute State\n')
replace('        renderer->sdlGPUDevice = result;', '''        renderer->sdlGPUDevice = result;
#ifdef OPENCMR2_METALFX
        SDL_SetPointerProperty(renderer->props, "OpenCMR2.MetalFXSpatialBlit", (void *)OPENCMR2_MetalFXBlit);
#endif''')
path.write_text(s)
