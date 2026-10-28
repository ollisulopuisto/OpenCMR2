#ifndef OPENCMR2_POSTPROCESS_PASS_H
#define OPENCMR2_POSTPROCESS_PASS_H
#include <SDL3/SDL.h>
// Full-screen pipelines have no depth target. A nonnull descriptor with a
// null texture is NOT equivalent to no attachment (Metal dereferences it).
inline SDL_GPURenderPass *Gfx_BeginPostProcessPass(SDL_GPUCommandBuffer *cmd,
    SDL_GPUGraphicsPipeline *pipeline,SDL_GPUTexture *target,
    const SDL_GPUTextureSamplerBinding *bindings,int count) {
    if(!cmd || !pipeline || !target || !bindings || count<=0 || count>7)return nullptr;
    for(int i=0;i<count;++i)
        if(!bindings[i].texture || !bindings[i].sampler || bindings[i].texture==target)return nullptr;
    SDL_GPUColorTargetInfo colour={};colour.texture=target;
    colour.load_op=SDL_GPU_LOADOP_CLEAR;colour.store_op=SDL_GPU_STOREOP_STORE;
    colour.clear_color={0,0,0,1};
    return SDL_BeginGPURenderPass(cmd,&colour,1,nullptr);
}
#endif
