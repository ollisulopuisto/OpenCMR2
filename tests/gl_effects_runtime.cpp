// Actual GL Executor regression with synthetic pixels, no original game data.
// Linux/software GL execution is NOT native Mac OpenGL acceptance.
#include "../src/render/gfx_gl.cpp"
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
static SDL_Window *testWindow;
static int glareSetting=0;
static int snowGlintSetting=-1;
extern "C" int Sys_GetOption(const char*k,int fallback){return !strcmp(k,"video.sun_glare")?glareSetting:!strcmp(k,"video.snow_glint")&&snowGlintSetting>=0?snowGlintSetting:fallback;}
extern "C" void Gfx_SetEffectStatus(unsigned,unsigned,int,int){}
extern "C" void Gfx_SetDofStatus(GfxDofStatus){}
extern "C" void Gfx_SetRaysStatus(int){}
SDL_Window *Platform_GetWindow() { return testWindow; }
int Platform_GetSettingInt(const char *, int fallback) { return fallback; }
const char *Platform_GetSetting(const char *, const char *fallback) { return fallback; }
extern "C" void Sys_Log(const char *format, ...) { va_list a; va_start(a,format); vfprintf(stderr,format,a); fputc('\n',stderr); va_end(a); }
void Gfx_SetWindowMode(SDL_Window *, bool, int, int) {}
bool Gfx_FrameHashWanted() { return false; }
void Gfx_DecodeBlock(int, const unsigned char *, unsigned int *) {}
void Gfx_HashFrame(const unsigned char *, int, int) {}
size_t Gfx_LevelSize(int,int,int,LONG*) { return 0; }
int Gfx_BytesPerPixel(int) { return 4; }
static void require(bool ok, const char *what) { if (!ok) { fprintf(stderr,"FAIL: %s\n",what); exit(1); } }
int main(int argc, char **argv)
{
    require(SDL_Init(SDL_INIT_VIDEO), SDL_GetError());
    Gfx_PrepareGlWindow();
    testWindow=SDL_CreateWindow("GL executor synthetic regression",96,64,SDL_WINDOW_OPENGL|SDL_WINDOW_HIDDEN);
    require(testWindow!=NULL, SDL_GetError());
    {
        Executor e; require(e.SetVideoMode(96,64,false), "video mode");
        const int w=96,h=64;
        std::vector<unsigned char> scene(w*h*4);
        for(int y=0;y<h;y++) for(int x=0;x<w;x++) {
            int i=(y*w+x)*4;
            scene[i]=x*255/(w-1); scene[i+1]=y*255/(h-1); scene[i+2]=(x/4+y/4)%2?230:20; scene[i+3]=173;
        }
        auto upload=[&]() {gl.ActiveTexture(GL_TEXTURE0);gl.BindTexture(GL_TEXTURE_2D,e.backColor);gl.TexSubImage2D(GL_TEXTURE_2D,0,0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,scene.data());e.ForgetEffectState();};
        auto read=[&](GLuint fbo) {std::vector<unsigned char> out(scene.size());gl.BindFramebuffer(GL_FRAMEBUFFER,fbo);gl.ReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,out.data());e.ForgetEffectState();return out;};
        upload(); require(read(e.ApplyEffects())==scene,"disabled effects exact identity");
        e.spatialAA=true;e.encodedEffects=0;upload();auto aa=read(e.ApplyEffects());
        require(aa!=scene && (e.encodedEffects&1),"AA actual pixels and encoded status");
        for(size_t i=3;i<scene.size();i+=4)require(aa[i]==scene[i],"AA preserves alpha");
        auto saved=scene;for(size_t i=0;i<scene.size();i+=4){scene[i]=53;scene[i+1]=111;scene[i+2]=190;scene[i+3]=73;}
        upload();require(read(e.ApplyEffects())==scene,"AA flat color and alpha exact");
        for(int y=0;y<h;y++)for(int x=0;x<w;x++){size_t i=(y*w+x)*4;scene[i]=scene[i+1]=scene[i+2]=x;}
        upload();require(read(e.ApplyEffects())==scene,"AA low contrast gradient identity");
        for(int y=0;y<h;y++)for(int x=0;x<w;x++){size_t i=(y*w+x)*4;scene[i]=scene[i+1]=scene[i+2]=x==w-1?255:0;}
        upload();auto border=read(e.ApplyEffects());for(int y=0;y<h;y++)require(border[y*w*4]==0,"AA border clamps without wrap");
        e.spatialAA=false;scene=saved;upload();require(read(e.ApplyEffects())==scene,"AA toggle disabled identity");
        e.SetColorGrading(25,0,110,100); auto grade=read(e.ApplyEffects()); require(grade!=scene,"grade changes pixels"); require(e.encodedEffects&32,"grade status follows executed pass");
        require(grade[0]==0 && grade[(h-1)*w*4+1]>180,"whole-source UV/no vertical flip");
        e.SetColorGrading(0,0,100,100); e.SetMotionBlur(true,128); upload();e.encodedEffects=0;
        auto first=read(e.ApplyEffects());require(first==scene,"motion first frame seeded, not black");require(!(e.encodedEffects&4),"seed is not an encoded blend");
        std::vector<unsigned char> old=scene;
        for(size_t i=0;i<scene.size();i+=4) scene[i]=255-scene[i];
        upload();auto blend=read(e.ApplyEffects());require(blend!=scene&&blend!=old,"history blended");require(e.encodedEffects&4,"history blend status executed");
        e.SetMotionBlur(false,128); e.SetMotionBlur(true,128);upload();require(read(e.ApplyEffects())==scene,"toggle resets history");
        e.SetMotionBlur(false,0);e.SetBloom(true,128);upload();auto bloom=read(e.ApplyEffects());require(bloom!=scene,"bloom executes");
        for(size_t i=3;i<scene.size();i+=4) require(bloom[i]==scene[i],"bloom preserves alpha");
        e.SetBloom(false,0);e.SetDepthOfField(true,20,10,100);
        gl.BindFramebuffer(GL_FRAMEBUFFER,e.backFramebuffer);gl.DepthMask(GL_TRUE);gl.ClearDepth(.9);gl.Clear(GL_DEPTH_BUFFER_BIT);e.ForgetEffectState();
        upload();auto dof=read(e.ApplyEffects());require(dof!=scene,"DoF samples actual depth");
        // Atmosphere fog through the production fog.frag. The GL executor feeds the raw-depth
        // ramp (no near/far recovery, see fog_math.h): depth 0 is near, 0.990..1.0 the fog band.
        e.SetDepthOfField(false,20,10,100);e.SetSunScreenPosition(.5f,.2f,false);
        e.SetPostFog(false,100,60);upload();require(read(e.ApplyEffects())==scene,"fog off exact identity");
        e.SetPostFog(true,100,60);
        gl.BindFramebuffer(GL_FRAMEBUFFER,e.backFramebuffer);gl.DepthMask(GL_TRUE);gl.ClearDepth(0);gl.Clear(GL_DEPTH_BUFFER_BIT);e.ForgetEffectState();
        upload();require(read(e.ApplyEffects())==scene,"near geometry (cockpit/UI depth) unfogged identity");
        gl.BindFramebuffer(GL_FRAMEBUFFER,e.backFramebuffer);gl.DepthMask(GL_TRUE);gl.ClearDepth(.9995);gl.Clear(GL_DEPTH_BUFFER_BIT);e.ForgetEffectState();
        upload();e.encodedEffects=0;auto fogged=read(e.ApplyEffects());
        require(fogged!=scene,"distant geometry fogs");
        require(e.encodedEffects&512,"fog encoded status bit");
        {   // height gradient: bottom row fogs harder than the top row at equal depth
            int tr=((1*w+w/2)*4), br=(((h-2)*w+w/2)*4);
            int dTop=std::abs(int(fogged[tr])-int(scene[tr])), dBot=std::abs(int(fogged[br])-int(scene[br]));
            require(dBot>dTop,"ground mist gradient fogs the bottom more");
        }
        for(size_t i=3;i<scene.size();i+=4) require(fogged[i]==scene[i],"fog preserves alpha");
        gl.BindFramebuffer(GL_FRAMEBUFFER,e.backFramebuffer);gl.DepthMask(GL_TRUE);gl.ClearDepth(1);gl.Clear(GL_DEPTH_BUFFER_BIT);e.ForgetEffectState();
        upload();auto sky=read(e.ApplyEffects());
        {   // sky cap at the bottom row (height scale clamps factor to 1, cap holds it at 0.85):
            // keeps 15 percent of the original pixel (here: the blue checker channel)
            size_t i=(((h-2)*w)+w/2)*4+2; int orig=scene[i];
            int expected=int(orig*0.15+184*0.85+0.5); // fog blue 0.72*255=184 at the 0.85 cap
            require(std::abs(sky[i]-expected)<=3,"sky cap keeps 15 percent of the sky pixel");
            size_t top=((h/4)*w+w/2)*4+2; // height gradient: top row fogs well under the cap
            require(std::abs(sky[top]-int(scene[top]*0.4+184*0.6))<=8,"top of frame fogs less than the cap");
        }
        {   // sun glow: with the sun visible at top centre, pixels near it warm and brighten
            e.SetSunScreenPosition(.5f,.2f,true);e.SetPostFog(true,100,100);
            gl.BindFramebuffer(GL_FRAMEBUFFER,e.backFramebuffer);gl.DepthMask(GL_TRUE);gl.ClearDepth(.9995);gl.Clear(GL_DEPTH_BUFFER_BIT);e.ForgetEffectState();
            upload();auto glow=read(e.ApplyEffects());
            size_t nearSun=((int)(h*.2f)*w+w/2)*4, farSun=((int)(h*.2f)*w+4)*4;
            require(int(glow[nearSun])+int(glow[nearSun+2])>int(glow[farSun])+int(glow[farSun+2]),"sun-ward glow brightens toward the sun");
            e.SetSunScreenPosition(.5f,.2f,false);
        }
        {   // amount scaling: 200 fogs harder than 50 at equal depth
            e.SetPostFog(true,50,0);upload();auto f50=read(e.ApplyEffects());
            e.SetPostFog(true,200,0);upload();auto f200=read(e.ApplyEffects());
            size_t i=((h/4)*w+w/2)*4; // top row: height scale keeps the factor below the clamp
            require(std::abs(int(f200[i])-int(scene[i]))>std::abs(int(f50[i])-int(scene[i])),"fog amount scales the effect");
        }
        {   // near-field clear distance (video.fog_clear): depth 0.991 is about 1.5 world
            // units down the raw-depth ramp, inside a 100-unit clear distance, so the
            // pixels survive; the same depth fogs with the clear distance off.
            e.SetPostFog(true,100,0);e.SetPostFogClearDistance(100);
            gl.BindFramebuffer(GL_FRAMEBUFFER,e.backFramebuffer);gl.DepthMask(GL_TRUE);gl.ClearDepth(.991);gl.Clear(GL_DEPTH_BUFFER_BIT);e.ForgetEffectState();
            upload();require(read(e.ApplyEffects())==scene,"clear distance keeps near-field pixels unfogged");
            e.SetPostFogClearDistance(0);upload();require(read(e.ApplyEffects())!=scene,"same depth fogs with clear distance off");
        }
        e.SetPostFog(false,0,0);upload();require(read(e.ApplyEffects())==scene,"fog toggle off restores identity");
        e.SetPostFog(true,100,60);e.effectsRaceFrame=false;upload();require(read(e.ApplyEffects())==scene,"fog never touches menu/preview frames");e.effectsRaceFrame=true;e.SetPostFog(false,0,0);
        // Snow glint pixels (video.snow_glint): two deterministic flakes, one
        // inside a projected car light beam and one outside it. The lit flake
        // must read brighter, the sun must brighten the flake in front of it,
        // and snow_glint 0 must disable the response. Synthetic pixels only.
        {
            const int ww=320,wh=200;e.SetVideoMode(ww,wh,false);
            std::vector<unsigned char> dark(ww*wh*4,32);
            auto upload=[&](){gl.BindTexture(GL_TEXTURE_2D,e.backColor);gl.TexSubImage2D(GL_TEXTURE_2D,0,0,0,ww,wh,GL_RGBA,GL_UNSIGNED_BYTE,dark.data());};
            auto lum=[&](float ux,float uy){std::vector<unsigned char> pix(ww*wh*4);gl.BindFramebuffer(GL_FRAMEBUFFER,e.backFramebuffer);gl.ReadPixels(0,0,ww,wh,GL_RGBA,GL_UNSIGNED_BYTE,pix.data());int x=int(ux*ww),y=int((1.0f-uy)*wh);size_t i=(size_t)(y*ww+x)*4;return int(pix[i])+int(pix[i+1])+int(pix[i+2]);};
            GfxFogLight one={};one.x=0.25f;one.y=0.5f;one.viewZ=-1;one.intensity=1;one.r=1;one.g=0.95f;one.b=0.85f;
            e.SetParticles(2,255);e.SetSunScreenPosition(0.5f,0.2f,false);
            e.weather.clear();
            e.weather.push_back({0.25f,0.5f,0,0,0.03f,0.05f,60});
            // Same flake across renders: beam on vs off, then sun on vs off.
            e.SetFogLights(NULL,0);
            upload();e.RenderWeather();
            // Review frames for visual inspection: plain, beam-lit, sun-lit.
            auto snap=[&](const char *tag){if(argc>1){std::vector<unsigned char> pix(ww*wh*4);
                gl.BindFramebuffer(GL_FRAMEBUFFER,e.backFramebuffer);gl.ReadPixels(0,0,ww,wh,GL_RGBA,GL_UNSIGNED_BYTE,pix.data());
                FILE *f=fopen((std::string(argv[1])+"-weather-"+tag+".ppm").c_str(),"wb");
                if(f){fprintf(f,"P6\n%d %d\n255\n",ww,wh);
                    for(int y=wh-1;y>=0;--y)for(int x=0;x<ww;++x)fwrite(pix.data()+((size_t)y*ww+x)*4,1,3,f);fclose(f);}}};
            int plain=lum(0.25f,0.5f);snap("plain");
            e.SetFogLights(&one,1);
            upload();e.RenderWeather();
            require(lum(0.25f,0.5f)>plain+20,"beam brightens its flake");snap("beam");
            e.SetFogLights(NULL,0);e.SetSunScreenPosition(0.25f,0.5f,true);
            upload();e.RenderWeather();
            require(lum(0.25f,0.5f)>plain+20,"sun forward-scatter brightens the flake toward it");snap("sun");
            // Disabled: light on vs light off must read the same.
            snowGlintSetting=0;e.SetSunScreenPosition(0.5f,0.2f,false);
            upload();e.RenderWeather();
            int offUnlit=lum(0.25f,0.5f);
            e.SetFogLights(&one,1);
            upload();e.RenderWeather();
            require(std::abs(lum(0.25f,0.5f)-offUnlit)<=6,"snow_glint 0 disables the flake light response");
            snowGlintSetting=-1;
            e.SetFogLights(NULL,0);e.SetParticles(0,0);e.RenderWeather();e.SetVideoMode(w,h,false);fprintf(stderr,"snow glint pixels: beam, sun forward-scatter and disable all pass\n");
        }
        e.SetDepthOfField(false,20,10,100);e.SetGodRays(true,255);e.SetSunScreenPosition(.8,.2,false);upload();require(read(e.ApplyEffects())==scene,"invisible sun bypass");
        e.SetSunScreenPosition(.8,.2,true);gl.BindFramebuffer(GL_FRAMEBUFFER,e.backFramebuffer);gl.DepthMask(GL_TRUE);gl.ClearDepth(1);gl.Clear(GL_DEPTH_BUFFER_BIT);e.ForgetEffectState();
        upload();auto rays=read(e.ApplyEffects());require(rays!=scene,"visible sun rays execute");
        {   // Rays occlusion: a full-height occluder band around the sun's column blocks the
            // shaft for pixels below it, while a pixel whose march stays in open sky keeps it.
            e.SetSunScreenPosition(.5f,.1f,true);
            gl.BindFramebuffer(GL_FRAMEBUFFER,e.backFramebuffer);gl.DepthMask(GL_TRUE);
            gl.ClearDepth(1);gl.Clear(GL_DEPTH_BUFFER_BIT);
            gl.Enable(GL_SCISSOR_TEST);gl.Scissor(w*9/20,0,w/10,h);
            gl.ClearDepth(.5);gl.Clear(GL_DEPTH_BUFFER_BIT);
            gl.Disable(GL_SCISSOR_TEST);e.ForgetEffectState();
            upload();auto occ=read(e.ApplyEffects());
            size_t shadowed=((h*3/4)*w+w/2)*4, openPx=((h*3/4)*w+w*3/4)*4;
            int dShadow=std::abs(int(occ[shadowed])-int(scene[shadowed]));
            int dOpen=std::abs(int(occ[openPx])-int(scene[openPx]));
            require(dOpen>dShadow,"occluder band blocks the light shaft");
        }
        e.SetGodRays(false,0);e.SetShadows(1,5); e.PrepareShadow();require(e.shadowReady,"shadow targets ready");
        GfxFrame empty; e.RenderShadow(empty);require(e.shadowReady,"shadow clear succeeds");
        e.SetShadowQuality(512,0);e.PrepareShadow();require(e.shadowReady&&e.shadowSize==512,"quality reallocates shadow targets");
        // Actual recorded indexed geometry through shadow and ordinary draw paths.
        GfxFrame geometry; GfxDrawState state = {};
        auto identity=[](GfxMatrix &m) { memset(&m,0,sizeof(m)); m._11=m._22=m._33=m._44=1; };
        identity(state.world);identity(state.view);identity(state.projection);
        identity(state.texture[0]);identity(state.texture[1]);
        state.lighting=false;state.cull=GFX_CULL_NONE;state.viewport={0,0,w,h,0,1};
        state.stages[0].colorOp=GFX_TOP_SELECTARG2;state.stages[0].colorArg2=GFX_TA_DIFFUSE;
        state.stages[0].alphaOp=GFX_TOP_SELECTARG2;state.stages[0].alphaArg2=GFX_TA_DIFFUSE;
        state.stages[1].colorOp=GFX_TOP_DISABLE;geometry.states.push_back(state);
        GfxVertex triangle[3] = {};
        triangle[0].x=-.8f; triangle[0].y=-.8f; triangle[0].z=.3f;
        triangle[1].x=.8f; triangle[1].y=-.8f; triangle[1].z=.3f;
        triangle[2].x=0; triangle[2].y=.8f; triangle[2].z=.3f;
        for(auto &v:triangle) v.diffuse=0xff40a060;
        geometry.vertices.resize(sizeof(triangle));memcpy(geometry.vertices.data(),triangle,sizeof(triangle));
        geometry.indices={0,1,2};
        GfxCommand clear={};clear.type=GFX_CMD_CLEAR;clear.clearFlags=GFX_CLEAR_TARGET|GFX_CLEAR_ZBUFFER;
        clear.clearColor=0xff000000;clear.clearZ=1;clear.clearViewport={0,0,w,h,0,1};geometry.commands.push_back(clear);
        GfxCommand draw={};draw.type=GFX_CMD_DRAW;draw.primitive=GFX_TRIANGLELIST;draw.vertexCount=3;draw.firstIndex=0;draw.indexCount=3;
        geometry.commands.push_back(draw);
        memset(e.lightViewProj,0,sizeof(e.lightViewProj));e.lightViewProj[0]=e.lightViewProj[5]=e.lightViewProj[10]=e.lightViewProj[15]=1;
        GfxReadback rb;std::vector<GfxTexture*> textures;
        e.Execute(geometry,textures,rb,false,false,false);
        auto normal=read(e.backFramebuffer);require(normal[(h/2*w+w/2)*4+1]>100,"geometry state restored after effects");
        gl.BindFramebuffer(GL_FRAMEBUFFER,e.shadowFbo);float center=1;gl.ReadPixels(e.shadowSize/2,e.shadowSize/2,1,1,GL_RED,GL_FLOAT,&center);
        e.ForgetEffectState();require(center>.29f&&center<.31f,"shadow indexed caster writes real depth");
        geometry.states[0].alphaTest=true;e.Execute(geometry,textures,rb,false,false,false);
        gl.BindFramebuffer(GL_FRAMEBUFFER,e.shadowFbo);gl.ReadPixels(e.shadowSize/2,e.shadowSize/2,1,1,GL_RED,GL_FLOAT,&center);
        e.ForgetEffectState();require(center==1,"alpha-tested foliage excluded from opaque shadow pass");
        e.SetShadows(0,5);
        // All passes stacked must use distinct source/target, and leave alpha.
        e.spatialAA=true;e.SetBloom(true,100);e.SetMotionBlur(true,80);e.SetDepthOfField(true,20,10,80);
        e.SetGodRays(true,120);e.SetSunScreenPosition(.7f,.1f,true);e.SetColorGrading(10,-5,105,103);
        upload();auto stacked=read(e.ApplyEffects());require(stacked!=scene,"stacked effects execute");
        for(size_t i=3;i<scene.size();i+=4) require(stacked[i]==scene[i],"stacked effects preserve alpha");
        e.spatialAA=false;e.SetBloom(false,0);e.SetMotionBlur(false,0);e.SetDepthOfField(false,20,10,80);e.SetGodRays(false,0);e.SetColorGrading(0,0,100,100);

        // Drone render gate must clear already-eased tunnel/streak and reject
        // late physics updates, then restore regular camera behavior.
        e.SetSpeedFocus(1.3f);e.speedFocusSmooth=1.2f;e.SetSpeedStreak(1);e.speedStreakSmooth=.8f;
        e.SetSpeedEffectsSuppressed(true);e.SetSpeedFocus(1.3f);e.SetSpeedStreak(1);
        require(e.speedFocusTarget==0&&e.speedFocusSmooth==0&&e.speedStreakTarget==0&&e.speedStreakSmooth==0,"drone clears residual and late speed inputs");
        upload();require(read(e.ApplyEffects())==scene,"drone actual pixels have no tunnel or streak");
        e.SetSpeedEffectsSuppressed(false);e.SetSpeedFocus(1.3f);e.SetSpeedStreak(1);
        require(e.speedFocusTarget>1&&e.speedStreakTarget==1,"classic speed effects restore");e.ClearSpeedEffects();
        // Restrained glare: actual translated GLSL, depth occlusion, alpha, cap and zero identity.
        e.SetSunScreenPosition(.5f,.3f,true);glareSetting=100;e.sunGlare.response=1;
        gl.BindFramebuffer(GL_FRAMEBUFFER,e.backFramebuffer);gl.DepthMask(GL_TRUE);gl.ClearDepth(1);gl.Clear(GL_DEPTH_BUFFER_BIT);e.ForgetEffectState();
        upload();auto glare=read(e.ApplyEffects());require(glare!=scene,"sun glare changes actual pixels");
        int sumBefore=0,sumAfter=0;for(size_t i=0;i<scene.size();i++)if(i%4!=3){sumBefore+=scene[i];sumAfter+=glare[i];}
        require(sumAfter>sumBefore*.95,"sun response dims frame less than5percent");
        for(size_t i=0;i<scene.size();i++){if(i%4==3)require(glare[i]==scene[i],"glare alpha unchanged");else require(int(glare[i])-int(scene[i])<=9,"glare brightening capped8/255 plus rounding");}
        gl.BindFramebuffer(GL_FRAMEBUFFER,e.backFramebuffer);gl.DepthMask(GL_TRUE);gl.ClearDepth(.5);gl.Clear(GL_DEPTH_BUFFER_BIT);e.ForgetEffectState();
        upload();require(read(e.ApplyEffects())==scene,"sun hidden by geometry exact identity");
        e.effectsRaceFrame=false;upload();require(read(e.ApplyEffects())==scene,"glare never touches menu/preview");e.effectsRaceFrame=true;
        // Cloud transmission0 is also exact identity at otherwise visible sun.
        gl.BindFramebuffer(GL_FRAMEBUFFER,e.backFramebuffer);gl.DepthMask(GL_TRUE);gl.ClearDepth(1);gl.Clear(GL_DEPTH_BUFFER_BIT);e.ForgetEffectState();
        unsigned char dark[4]={255,255,255,0};gl.BindTexture(GL_TEXTURE_2D,e.white2D);gl.TexSubImage2D(GL_TEXTURE_2D,0,0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,dark);e.ForgetEffectState();
        upload();require(read(e.ApplyEffects())==scene,"cloud blocks sun glare");
        dark[3]=255;gl.BindTexture(GL_TEXTURE_2D,e.white2D);gl.TexSubImage2D(GL_TEXTURE_2D,0,0,0,1,1,GL_RGBA,GL_UNSIGNED_BYTE,dark);e.ForgetEffectState();
        glareSetting=0;upload();require(read(e.ApplyEffects())==scene,"sun glare0 bypass exact identity");
        if(argc>1){FILE*f=fopen((std::string(argv[1])+"-glare.ppm").c_str(),"wb");fprintf(f,"P6\n%d %d\n255\n",w,h*2);for(auto*arr:{&scene,&glare})for(int i=0;i<w*h;i++)fwrite(arr->data()+i*4,1,3,f);fclose(f);}
        if(argc>1){ // Larger synthetic driving frame, actual shader output for review.
            const int fw=640,fh=360;e.SetVideoMode(fw,fh,false);
            std::vector<unsigned char> road(fw*fh*4);std::vector<float> depths(fw*fh,1.f);
            for(int y=0;y<fh;y++)for(int x=0;x<fw;x++){
                size_t i=(y*fw+x)*4;int r=100,g=150,b=190;
                if(y>110){r=60;g=95;b=46;depths[y*fw+x]=.98f;float width=(y-105)*1.5f;
                    if(std::abs(x-fw/2)<width){r=g=b=72+(x+y)%7;if(std::abs(x-fw/2)<3&&y%30<16)r=g=b=220;}}
                // Blue car and glass stay protected foreground pixels.
                if(y>245&&y<345&&x>265&&x<375){r=28;g=58;b=148;if(y<280){r=70;g=110;b=140;}}
                float dx=x-fw*.51f,dy=y-fh*.24f;if(dx*dx+dy*dy<7*7)r=g=b=245;
                road[i]=r;road[i+1]=g;road[i+2]=b;road[i+3]=255;
            }
            gl.ActiveTexture(GL_TEXTURE0);gl.BindTexture(GL_TEXTURE_2D,e.backColor);gl.TexSubImage2D(GL_TEXTURE_2D,0,0,0,fw,fh,GL_RGBA,GL_UNSIGNED_BYTE,road.data());
            gl.BindTexture(GL_TEXTURE_2D,e.backDepth);gl.TexSubImage2D(GL_TEXTURE_2D,0,0,0,fw,fh,GL_DEPTH_COMPONENT,GL_FLOAT,depths.data());e.ForgetEffectState();
            e.SetSunScreenPosition(.51f,.24f,true);glareSetting=100;e.sunGlare.response=1;
            GLuint output=e.ApplyEffects();std::vector<unsigned char> result(road.size());gl.BindFramebuffer(GL_FRAMEBUFFER,output);gl.ReadPixels(0,0,fw,fh,GL_RGBA,GL_UNSIGNED_BYTE,result.data());e.ForgetEffectState();
            FILE*f=fopen((std::string(argv[1])+"-road.ppm").c_str(),"wb");fprintf(f,"P6\n%d %d\n255\n",fw,fh*2);for(auto*arr:{&road,&result})for(int i=0;i<fw*fh;i++)fwrite(arr->data()+i*4,1,3,f);fclose(f);
            glareSetting=0;e.SetVideoMode(w,h,false);
        }
        if(argc>1){ // Fog review frame: road depth ramps through the fog band, sun visible. off / 100 / 200 stacked.
            const int fw=640,fh=360;e.SetVideoMode(fw,fh,false);
            std::vector<unsigned char> road(fw*fh*4);std::vector<float> depths(fw*fh,1.f);
            for(int y=0;y<fh;y++)for(int x=0;x<fw;x++){
                size_t i=(y*fw+x)*4;int r=100,g=150,b=190;
                if(y>110){r=60;g=95;b=46;float t=(y-110.f)/(fh-110.f);depths[y*fw+x]=0.9998f-t*t*0.0085f;
                    float width=(y-105)*1.5f;
                    if(std::abs(x-fw/2)<width){r=g=b=72+(x+y)%7;if(std::abs(x-fw/2)<3&&y%30<16)r=g=b=220;}}
                // Blue car and glass stay protected foreground pixels (near depth: no fog).
                if(y>245&&y<345&&x>265&&x<375){r=28;g=58;b=148;depths[y*fw+x]=0.f;if(y<280){r=70;g=110;b=140;}}
                float dx=x-fw*.51f,dy=y-fh*.24f;if(dx*dx+dy*dy<7*7)r=g=b=245;
                road[i]=r;road[i+1]=g;road[i+2]=b;road[i+3]=255;
            }
            std::vector<std::vector<unsigned char>> frames;frames.push_back(road);
            for(int amount:{100,200}){
                gl.ActiveTexture(GL_TEXTURE0);gl.BindTexture(GL_TEXTURE_2D,e.backColor);gl.TexSubImage2D(GL_TEXTURE_2D,0,0,0,fw,fh,GL_RGBA,GL_UNSIGNED_BYTE,road.data());
                gl.BindTexture(GL_TEXTURE_2D,e.backDepth);gl.TexSubImage2D(GL_TEXTURE_2D,0,0,0,fw,fh,GL_DEPTH_COMPONENT,GL_FLOAT,depths.data());e.ForgetEffectState();
                e.SetSunScreenPosition(.51f,.24f,true);e.SetPostFog(true,amount,100);
                GLuint output=e.ApplyEffects();std::vector<unsigned char> result(road.size());
                gl.BindFramebuffer(GL_FRAMEBUFFER,output);gl.ReadPixels(0,0,fw,fh,GL_RGBA,GL_UNSIGNED_BYTE,result.data());e.ForgetEffectState();
                frames.push_back(result);
            }
            e.SetPostFog(false,0,0);
            // foreground car pixels are near depth: identical in all frames
            for(int f=1;f<3;f++)for(int y=250;y<340;y+=17)for(int x=270;x<370;x+=13){size_t i=(y*fw+x)*4;
                if(frames[f][i]!=road[i]||frames[f][i+1]!=road[i+1]||frames[f][i+2]!=road[i+2]){fprintf(stderr,"FAIL: fog touches foreground car pixels\n");exit(1);}}
            // horizon fogs more than the bottom of the road at 100
            {size_t hi=((112)*fw+fw/2)*4, li=((fh-20)*fw+fw/2)*4;
             int dH=std::abs(int(frames[1][hi])-int(road[hi])),dL=std::abs(int(frames[1][li])-int(road[li]));
             if(!(dH>dL)){fprintf(stderr,"FAIL: fog review frame not depth graded\n");exit(1);}}
            FILE*f=fopen((std::string(argv[1])+"-fog-road.ppm").c_str(),"wb");fprintf(f,"P6\n%d %d\n255\n",fw,fh*3);
            for(auto&fr:frames)for(int i=0;i<fw*fh;i++)fwrite(fr.data()+i*4,1,3,f);
            fclose(f);e.SetVideoMode(w,h,false);
        }
        e.SetParticles(1,255);e.RenderWeather();SDL_Delay(30);e.RenderWeather();require(!e.weather.empty(),"weather emits");
        e.SetParticles(0,0);e.RenderWeather();require(e.weather.empty(),"weather clears on disable");
        e.SetMotionBlur(true,128);e.ApplyEffects();require(e.SetVideoMode(80,60,false),"resize");require(!e.historyValid&&!e.effectTex[0],"resize invalidates targets/history");
        require(gl.GetError()==GL_NO_ERROR,"no GL errors");
        if(argc>1) {
            FILE *f=fopen(argv[1],"wb");fprintf(f,"P6\n%d %d\n255\n",w,h*7);
            for(auto *arr : {&old,&aa,&grade,&scene,&bloom,&dof,&stacked}) for(int i=0;i<w*h;i++) fwrite(arr->data()+i*4,1,3,f);
            fclose(f);
        }
    }
    SDL_DestroyWindow(testWindow);SDL_Quit();puts("PASS actual GL executor: spatial AA flat/gradient/border/alpha/toggle, identity, orientation, history, bloom, depth blur, sun visibility/rays, shadow caster/exclusion, stacked alpha, particles, resize");
}
