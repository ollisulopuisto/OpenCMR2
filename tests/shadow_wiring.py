#!/usr/bin/env python3
"""Guard shadow draw bindings. Source checks complement shader pixel tests."""
from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'src/render/gfx_gpu.cpp').read_text()
a=s[s.index('SDL_GPUGraphicsPipeline *ShadowPipeline()'):s.index('// Full-screen lens grime')]
assert 'vb.pitch = sizeof(GfxVertex)' in a and 'num_vertex_attributes = 3' in a
b=s[s.index('GfxTexture *shadowTarget = NULL;'):s.index('size_t worldEnd=frame.commands.size();')]
for required in ['sc.transformed', 'shadowTarget != NULL', 'Gfx_ShadowCaster(ss)', 'Gfx_CutoutShadow(ss)', 'SDL_BindGPUFragmentSamplers(spass',
                 'SDL_BindGPUVertexBuffers(spass', 'SDL_BindGPUIndexBuffer(spass']:
    assert required in b, required
assert 'Multiply(lightView, lightProj, lightViewProj)' in s
assert 'SDL_GPU_SHADERSTAGE_FRAGMENT, 8, 1' in s
assert 'bindings[8] =' not in s # no undeclared depth sampler/read-write attachment feedback
assert 'shadowMap && shadowDepth && shadowPipeline && fade' in s
print('shadow wiring invariants pass; native draw submission still needs acceptance')

coverage=(Path(__file__).resolve().parents[1]/'src/render/shadow_caster.h').read_text()
for required in ('s.depthTest && s.depthWrite','!s.alphaBlend&&!s.alphaTest','Gfx_CutoutShadow(s)','a.texCoordIndex==0','a.textureTransform==GFX_TTFF_DISABLE','s.stages[1].colorOp==GFX_TOP_DISABLE'):
    assert required in coverage,required
fragment=(Path(__file__).resolve().parents[1]/'src/render/shaders/shadow.frag').read_text()
assert 'texture(coverage,shadowUV).a' in fragment and 'discard' in fragment
