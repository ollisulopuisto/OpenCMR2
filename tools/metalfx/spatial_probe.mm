// Standalone native MetalFX smoke probe. No SDL private layouts or game assets.
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalFX/MetalFX.h>
#include <cstdio>
#include <vector>

int main(int argc, const char **argv)
{
    @autoreleasepool {
        if (@available(macOS 13.0, *)) {
            id<MTLDevice> device = MTLCreateSystemDefaultDevice();
            if (!device) { std::fprintf(stderr, "No Metal device\n"); return 2; }
            MTLFXSpatialScalerDescriptor *descriptor = [MTLFXSpatialScalerDescriptor new];
            descriptor.inputWidth = 320; descriptor.inputHeight = 200;
            descriptor.outputWidth = 640; descriptor.outputHeight = 400;
            descriptor.colorTextureFormat = MTLPixelFormatBGRA8Unorm;
            descriptor.outputTextureFormat = MTLPixelFormatBGRA8Unorm;
            descriptor.colorProcessingMode = MTLFXSpatialScalerColorProcessingModePerceptual;
            id<MTLFXSpatialScaler> scaler = [descriptor newSpatialScalerWithDevice:device];
            if (!scaler) { std::fprintf(stderr, "MetalFX spatial scaler unavailable\n"); return 2; }
            MTLTextureDescriptor *inputDesc = [MTLTextureDescriptor
                texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:320 height:200 mipmapped:NO];
            inputDesc.storageMode = MTLStorageModeShared;
            inputDesc.usage = scaler.colorTextureUsage;
            id<MTLTexture> input = [device newTextureWithDescriptor:inputDesc];
            MTLTextureDescriptor *outputDesc = [MTLTextureDescriptor
                texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:640 height:400 mipmapped:NO];
            outputDesc.storageMode = MTLStorageModePrivate;
            outputDesc.usage = scaler.outputTextureUsage;
            id<MTLTexture> output = [device newTextureWithDescriptor:outputDesc];
            id<MTLCommandQueue> queue = [device newCommandQueue];
            id<MTLBuffer> readback = [device newBufferWithLength:640 * 400 * 4 options:MTLResourceStorageModeShared];
            if (!input || !output || !queue || !readback) { std::fprintf(stderr, "Resource allocation failed\n"); return 1; }
            std::vector<unsigned char> pixels(320 * 200 * 4);
            for (int y = 0; y < 200; ++y) for (int x = 0; x < 320; ++x) {
                auto *p = &pixels[(y * 320 + x) * 4];
                p[0] = (unsigned char)(x * 255 / 319);
                p[1] = (unsigned char)(y * 255 / 199);
                p[2] = (x > y ? 220 : 30); p[3] = 255;
            }
            [input replaceRegion:MTLRegionMake2D(0, 0, 320, 200) mipmapLevel:0
                withBytes:pixels.data() bytesPerRow:320 * 4];
            scaler.colorTexture = input; scaler.outputTexture = output;
            scaler.inputContentWidth = 320; scaler.inputContentHeight = 200;
            id<MTLCommandBuffer> command = [queue commandBuffer];
            [scaler encodeToCommandBuffer:command];
            id<MTLBlitCommandEncoder> blit = [command blitCommandEncoder];
            [blit copyFromTexture:output sourceSlice:0 sourceLevel:0 sourceOrigin:MTLOriginMake(0, 0, 0)
                sourceSize:MTLSizeMake(640, 400, 1) toBuffer:readback destinationOffset:0
                destinationBytesPerRow:640 * 4 destinationBytesPerImage:640 * 400 * 4];
            [blit endEncoding]; [command commit]; [command waitUntilCompleted];
            if (command.status != MTLCommandBufferStatusCompleted) {
                std::fprintf(stderr, "GPU failure: %s\n", command.error.localizedDescription.UTF8String); return 1;
            }
            const auto *result = static_cast<const unsigned char *>(readback.contents);
            // A flat/empty result means the smoke probe did not produce the test pattern.
            if (result[0] == result[(640 * 399 + 639) * 4] &&
                result[1] == result[(640 * 399 + 639) * 4 + 1]) {
                std::fprintf(stderr, "Unexpected flat output\n"); return 1;
            }
            if (argc > 1) {
                FILE *file = std::fopen(argv[1], "wb");
                if (!file) { std::perror("output"); return 1; }
                std::fprintf(file, "P6\n640 400\n255\n");
                for (int i = 0; i < 640 * 400; ++i) {
                    unsigned char rgb[3] = {result[i * 4 + 2], result[i * 4 + 1], result[i * 4]};
                    if (std::fwrite(rgb, 1, 3, file) != 3) { std::fclose(file); return 1; }
                }
                if (std::fclose(file)) return 1;
            }
            std::printf("MetalFX spatial encode completed on %s. Synthetic smoke test only, not game integration or an FPS benchmark.\n",
                        device.name.UTF8String);
            return 0;
        }
        std::fprintf(stderr, "Requires macOS 13 or newer\n"); return 2;
    }
}
