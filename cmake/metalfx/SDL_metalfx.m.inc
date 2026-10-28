/* OpenCMR2 extension for SDL release-3.4.18. Kept inside the Metal backend so
 * opaque SDL resources are never cast by application code. Spatial only. */
static bool OPENCMR2_MetalFXBlit(SDL_GPUCommandBuffer *commandBuffer, const SDL_GPUBlitInfo *info)
{
    @autoreleasepool {
        if (@available(macOS 13.0, *)) {
            MetalCommandBuffer *cmd = (MetalCommandBuffer *)commandBuffer;
            MetalRenderer *renderer = cmd->renderer;
            MetalTextureContainer *source = (MetalTextureContainer *)info->source.texture;
            if (cmd->renderEncoder || cmd->blitEncoder || cmd->computeEncoder ||
                info->source.x || info->source.y || info->source.mip_level || info->source.layer_or_depth_plane ||
                info->source.w != source->header.info.width || info->source.h != source->header.info.height ||
                info->flip_mode != SDL_FLIP_NONE ||
                info->destination.w < info->source.w || info->destination.h < info->source.h ||
                (info->destination.w == info->source.w && info->destination.h == info->source.h) ||
                source->header.info.format != SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM)
                return false;
            if (!renderer->opencmr2Scaler || renderer->opencmr2InputW != info->source.w ||
                renderer->opencmr2InputH != info->source.h || renderer->opencmr2OutputW != info->destination.w ||
                renderer->opencmr2OutputH != info->destination.h) {
                MTLFXSpatialScalerDescriptor *desc = [MTLFXSpatialScalerDescriptor new];
                desc.inputWidth = info->source.w; desc.inputHeight = info->source.h;
                desc.outputWidth = info->destination.w; desc.outputHeight = info->destination.h;
                desc.colorTextureFormat = MTLPixelFormatBGRA8Unorm;
                desc.outputTextureFormat = MTLPixelFormatBGRA8Unorm;
                desc.colorProcessingMode = MTLFXSpatialScalerColorProcessingModePerceptual;
                id<MTLFXSpatialScaler> scaler = [desc newSpatialScalerWithDevice:renderer->device];
                if (!scaler) return false;
                SDL_GPUTextureCreateInfo create = {0};
                create.type = SDL_GPU_TEXTURETYPE_2D;
                create.format = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM;
                create.usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER;
                create.width = info->destination.w; create.height = info->destination.h;
                create.layer_count_or_depth = 1; create.num_levels = 1; create.sample_count = SDL_GPU_SAMPLECOUNT_1;
                SDL_GPUTexture *output = METAL_CreateTexture((SDL_GPURenderer *)renderer, &create);
                if (!output) return false;
                if (renderer->opencmr2Output)
                    METAL_ReleaseTexture((SDL_GPURenderer *)renderer, renderer->opencmr2Output);
                renderer->opencmr2Output = output;
                renderer->opencmr2Scaler = scaler;
                renderer->opencmr2InputW = info->source.w; renderer->opencmr2InputH = info->source.h;
                renderer->opencmr2OutputW = info->destination.w; renderer->opencmr2OutputH = info->destination.h;
            }
            MetalTextureContainer *container = (MetalTextureContainer *)renderer->opencmr2Output;
            MetalTexture *output = METAL_INTERNAL_PrepareTextureForWrite(renderer, container, true);
            MetalTexture *input = source->activeTexture;
            id<MTLFXSpatialScaler> scaler = renderer->opencmr2Scaler;
            if ((input->handle.usage & scaler.colorTextureUsage) != scaler.colorTextureUsage ||
                (output->handle.usage & scaler.outputTextureUsage) != scaler.outputTextureUsage)
                return false;
            scaler.colorTexture = input->handle; scaler.outputTexture = output->handle;
            scaler.inputContentWidth = info->source.w; scaler.inputContentHeight = info->source.h;
            METAL_INTERNAL_TrackTexture(cmd, input);
            METAL_INTERNAL_TrackTexture(cmd, output);
            [scaler encodeToCommandBuffer:cmd->handle];
            SDL_GPUBlitInfo final = *info;
            final.source.texture = renderer->opencmr2Output;
            final.source.w = info->destination.w; final.source.h = info->destination.h;
            final.filter = SDL_GPU_FILTER_NEAREST; // same-size placement, preserves letterbox/clear
            METAL_Blit(commandBuffer, &final);
            return true;
        }
        return false;
    }
}
