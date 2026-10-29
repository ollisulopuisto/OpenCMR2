#ifndef OPENCMR2_GPU_STARTUP_OPTIONS_H
#define OPENCMR2_GPU_STARTUP_OPTIONS_H
#include "port/sys.h"
// Must use the override-aware reader: per-launch --effect never writes the INI.
struct GPUStartupOptions {
 bool spatialAA,metalFXSpatial;
 static GPUStartupOptions Read(){return {Sys_GetOption("video.spatial_aa",0)==1,Sys_GetOption("video.metalfx_spatial",0)==1};}
};
#endif
