from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'src/render/gfx_gpu.cpp').read_text()
p=s[s.index('SDL_GPUGraphicsPipeline *ParticlePipeline()'):s.index('SDL_GPUGraphicsPipeline *ShadowPipeline()')]
assert 'has_depth_stencil_target = false' in p
r=s[s.index('void RenderParticles('):s.index('SDL_GPUSampler *Sampler(')]
assert 'Gfx_BeginColourOnlyPass(cmd,backColor,SDL_GPU_LOADOP_LOAD)' in r
assert 'SDL_GPUDepthStencilTargetInfo' not in r
# Remaining raw calls are actual geometry + shadow, both with validated depth.
assert s.count('SDL_BeginGPURenderPass(')==2
b=s[s.index('SDL_GPURenderPass *BeginPass('):s.index('void Execute(')]
assert 'if (!color.texture || !depth.texture) return nullptr' in b
sh=s[s.index('if (shadowType != 0 && shadowMap != NULL'):s.index('for (size_t i = 0; i < frame.commands.size(); i++)')]
assert 'depth.texture = shadowDepth' in sh
print('all render-pass attachment sites audited; native Metal acceptance remains open')

# Default changes must respect saved choices, not set/overwrite them.
root=Path(__file__).resolve().parents[1]
car=(root/'game/CarEffects.cpp').read_text()
graphics=(root/'game/Graphics.cpp').read_text()
assert 'Sys_GetOption("video.dust_plume", 1)' in car
assert 'Sys_GetOption("video.dust_volume",1)' in graphics
assert 'Sys_SetOption("video.dust_' not in car+graphics
