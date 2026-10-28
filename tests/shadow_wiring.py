#!/usr/bin/env python3
"""Guard shadow draw bindings. Source checks complement shader pixel tests."""
from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'src/render/gfx_gpu.cpp').read_text()
a=s[s.index('SDL_GPUGraphicsPipeline *ShadowPipeline()'):s.index('// Full-screen lens grime')]
assert 'vb.pitch = sizeof(GfxVertex)' in a and 'num_vertex_attributes = 1' in a
b=s[s.index('GfxTexture *shadowTarget = NULL;'):s.index('for (size_t i = 0; i < frame.commands.size(); i++)')]
for required in ['sc.transformed', 'shadowTarget != NULL', 'ss.alphaTest', 'ss.alphaBlend',
                 'SDL_BindGPUVertexBuffers(spass', 'SDL_BindGPUIndexBuffer(spass']:
    assert required in b, required
assert 'Multiply(lightView, lightProj, lightViewProj)' in s
assert 'SDL_GPU_SHADERSTAGE_FRAGMENT, 8, 1' in s
assert 'bindings[8] =' not in s # no undeclared depth sampler/read-write attachment feedback
assert 'shadowMap && shadowDepth && shadowPipeline && fade' in s
print('shadow wiring invariants pass; native draw submission still needs acceptance')
