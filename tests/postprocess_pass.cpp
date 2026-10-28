#include "render/postprocess_pass.h"
#include <cstdio>
static int calls=0,fail=0;
extern "C" SDL_GPURenderPass *SDL_BeginGPURenderPass(SDL_GPUCommandBuffer*,const SDL_GPUColorTargetInfo *c,Uint32 n,const SDL_GPUDepthStencilTargetInfo *d){
 ++calls;if(d || n!=1 || !c || !c->texture || c->load_op!=SDL_GPU_LOADOP_CLEAR || c->store_op!=SDL_GPU_STOREOP_STORE)++fail;
 return reinterpret_cast<SDL_GPURenderPass*>(0x10);
}
int main(){
 auto cmd=reinterpret_cast<SDL_GPUCommandBuffer*>(1);auto pipe=reinterpret_cast<SDL_GPUGraphicsPipeline*>(2);
 auto target=reinterpret_cast<SDL_GPUTexture*>(3);auto source=reinterpret_cast<SDL_GPUTexture*>(4);auto sampler=reinterpret_cast<SDL_GPUSampler*>(5);
 SDL_GPUTextureSamplerBinding b={source,sampler};
 if(!Gfx_BeginPostProcessPass(cmd,pipe,target,&b,1)||calls!=1)++fail;
 if(Gfx_BeginPostProcessPass(cmd,nullptr,target,&b,1) || Gfx_BeginPostProcessPass(cmd,pipe,nullptr,&b,1) ||
    Gfx_BeginPostProcessPass(nullptr,pipe,target,&b,1) || Gfx_BeginPostProcessPass(cmd,pipe,target,nullptr,1) ||
    Gfx_BeginPostProcessPass(cmd,pipe,target,&b,0))++fail;
 b.texture=target;if(Gfx_BeginPostProcessPass(cmd,pipe,target,&b,1))++fail;
 b.texture=nullptr;if(Gfx_BeginPostProcessPass(cmd,pipe,target,&b,1))++fail;
 b.texture=source;b.sampler=nullptr;if(Gfx_BeginPostProcessPass(cmd,pipe,target,&b,1))++fail;
 if(calls!=1)++fail;
 printf("postprocess pass: %s, no dummy depth, failed resources rejected\n",fail?"FAIL":"pass");return fail?1:0;
}
