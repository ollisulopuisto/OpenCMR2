from pathlib import Path
s=(Path(__file__).resolve().parents[1]/'src/render/gfx_gpu.cpp').read_text()
p=s[s.index('SDL_GPUGraphicsPipeline *ParticlePipeline()'):s.index('SDL_GPUGraphicsPipeline *ShadowPipeline()')]
assert 'has_depth_stencil_target = false' in p
r=s[s.index('bool RenderParticles('):s.index('SDL_GPUSampler *Sampler(')]
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

# Binding contract supplements the executable recorder's command-order test.
game=(root/'game/Game.cpp').read_text()
view=game[game.index('int Game_DrawSceneViewport(SceneNode *pRoot, SceneNode *pCamera, void *pRect, int bit, BYTE flag)\n{'):game.index('void CGame::SetSectorDrawState(')]
assert view.index('CGraphics::SetProjection(0, 0, CGraphics::m_farPlaneFixed, 0)') < view.index('Particle_DrawAll(')
assert view.index('Particle_DrawAll(') < view.index('Game_DrawCulledSectorMeshes(')
assert view.index('Scene_DrawShadowBatches(bit)') < view.index('Gfx_FlushDustVolume()') < view.index('Flare_SampleVisibility(') < view.index('Sprite_DrawLayer(3)')
