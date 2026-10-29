// Included inside the GL Executor. Deliberately GL-only: no game data or shared
// options, no changes to SDL_GPU, and no extra third-party GL API requirement.
void ReleaseEffectTargets()
{
    gl.DeleteTextures(4, effectTex); gl.DeleteFramebuffers(4, effectFbo);
    gl.DeleteTextures(1, &historyTex);
    memset(effectTex, 0, sizeof(effectTex)); memset(effectFbo, 0, sizeof(effectFbo));
    historyTex = 0; historyValid = false;
}
void ReleaseShadow()
{
    gl.DeleteTextures(1, &shadowMap); gl.DeleteRenderbuffers(1, &shadowDepth);
    gl.DeleteFramebuffers(1, &shadowFbo);
    shadowMap = shadowDepth = shadowFbo = 0; shadowReady = false;
}
void DestroyEffects()
{
    ReleaseEffectTargets(); ReleaseShadow();
    for (GLuint &p : effectPrograms) { gl.DeleteProgram(p); p = 0; }
    gl.DeleteProgram(shadowProgram); gl.DeleteProgram(weatherProgram);
    gl.DeleteBuffers(1, &effectUbo); gl.DeleteBuffers(1, &weatherBuffer);
    gl.DeleteVertexArrays(1, &effectVao); gl.DeleteVertexArrays(1, &weatherVao);
}
// Raw post-process operations intentionally bypass the ordinary draw cache.
// Forget it completely before returning to normal geometry; never leave cached
// depth, viewport, program or attribute assumptions from a full-screen pass.
void ForgetEffectState()
{
    bound = Bound();
    gl.BindVertexArray(vertexArray);
    gl.Enable(GL_SCISSOR_TEST);
}
void EffectState(GLuint fbo, int width, int height)
{
    gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
    gl.Viewport(0, 0, width, height); gl.DepthRange(0, 1);
    gl.Disable(GL_SCISSOR_TEST); gl.Disable(GL_BLEND); gl.Disable(GL_CULL_FACE);
    gl.Disable(GL_DEPTH_TEST); gl.DepthMask(GL_FALSE);
    gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}
GLuint EffectTexture(int width, int height)
{
    GLuint t = 0; gl.GenTextures(1, &t); gl.BindTexture(GL_TEXTURE_2D, t);
    gl.TexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    return t;
}
bool EffectTargets()
{
    if (effectTex[0]) return true;
    gl.ActiveTexture(GL_TEXTURE0);
    gl.GenFramebuffers(4, effectFbo);
    for (int i = 0; i < 4; ++i) {
        effectTex[i] = EffectTexture(backWidth, backHeight);
        gl.BindFramebuffer(GL_FRAMEBUFFER, effectFbo[i]);
        gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, effectTex[i], 0);
        if (gl.CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            Sys_Log("gfx: GL effect framebuffer incomplete; presenting original scene");
            ReleaseEffectTargets(); ForgetEffectState(); return false;
        }
    }
    historyTex = EffectTexture(backWidth, backHeight);
    if (!effectUbo) gl.GenBuffers(1, &effectUbo);
    if (!effectVao) gl.GenVertexArrays(1, &effectVao);
    ForgetEffectState(); return true;
}
GLuint EffectProgram(int effect)
{
    if (effectPrograms[effect]) return effectPrograms[effect];
    static const unsigned char *sources[] = {g_brightpass_frag_glsl, g_blur_frag_glsl,
        g_composite_frag_glsl, g_motionblur_frag_glsl, g_lens_grime_frag_glsl,
        g_dof_frag_glsl, g_godRays_frag_glsl, g_colorGrading_frag_glsl, g_spatial_aa_frag_glsl};
    static const char *blocks[] = {"BrightPassUniforms", "BlurUniforms", "CompositeUniforms",
        "MotionBlurUniforms", "LensGrimeUniforms", "DoFUniforms", "GodRaysUniforms", "ColorGradingUniforms", "SpatialAAUniforms"};
    static const char *samplers0[] = {"scene", "source", "scene", "current", "scene", "sceneTex", "scene", "scene", "scene"};
    static const char *samplers1[] = {NULL, NULL, "bloom", "previous", NULL, "depthTex", "depthTex", NULL, NULL};
    GLuint p = Link(glEffectsVertex, sources[effect]);
    if (p) {
        GLuint block = gl.GetUniformBlockIndex(p, blocks[effect]);
        if (block != GL_INVALID_INDEX) gl.UniformBlockBinding(p, block, FRAGMENT_BLOCK);
        gl.UseProgram(p);
        gl.Uniform1i(gl.GetUniformLocation(p, samplers0[effect]), 0);
        if (samplers1[effect]) gl.Uniform1i(gl.GetUniformLocation(p, samplers1[effect]), 1);
    }
    effectPrograms[effect] = p;
    return p;
}
bool EffectPass(int effect, int target, GLuint source, GLuint second, const void *data, size_t bytes)
{
    // Prevent undefined read/write feedback even when effects are stacked.
    if (effectTex[target] == source || (second && effectTex[target] == second)) return false;
    GLuint p = EffectProgram(effect);
    if (!p) { ForgetEffectState(); return false; }
    EffectState(effectFbo[target], backWidth, backHeight);
    gl.UseProgram(p); gl.BindVertexArray(effectVao);
    GfxStageState linear; linear.minFilter = linear.magFilter = GFX_FILTER_LINEAR; linear.address = GFX_ADDRESS_CLAMP;
    GfxStageState nearest; nearest.address = GFX_ADDRESS_CLAMP;
    for (int i = 0; i < 2; ++i) {
        gl.ActiveTexture(GL_TEXTURE0 + i);
        gl.BindTexture(GL_TEXTURE_2D, i ? (second ? second : white2D) : source);
        gl.BindSampler(i, Sampler(i == 1 && second == backDepth ? nearest : linear));
    }
    // std140 blocks are padded, including shaders whose public argument is one float.
    unsigned char block[64] = {};
    if (bytes > sizeof(block)) { ForgetEffectState(); return false; }
    memcpy(block, data, bytes);
    gl.BindBuffer(GL_UNIFORM_BUFFER, effectUbo);
    gl.BufferData(GL_UNIFORM_BUFFER, sizeof(block), block, GL_STREAM_DRAW);
    gl.BindBufferRange(GL_UNIFORM_BUFFER, FRAGMENT_BLOCK, effectUbo, 0, sizeof(block));
    gl.DrawArrays(GL_TRIANGLES, 0, 3);
    GLenum error = gl.GetError();
    ForgetEffectState();
    if (error != GL_NO_ERROR) { Sys_Log("gfx: GL effect %d failed (%#x); retaining previous result", effect, (unsigned)error); return false; }
    return true;
}
GLuint ApplyEffects()
{
    bool active = spatialAA || bloomOn && bloomAmount > 0 || motionOn && motionMix > 0 ||
        dofOn && dofRange > 0 && dofBlur > 0 || raysOn && raysAmount > 0 && sunVisible ||
        grade[0] || grade[1] || grade[2] != 1 || grade[3] != 1 || grime[0] || grime[1] || grime[2];
    if (!active) { historyValid = false; return backFramebuffer; }
    if (!EffectTargets()) return backFramebuffer;
    GLuint source = backColor, sourceFbo = backFramebuffer;
    auto run = [&](int effect, GLuint second, const void *data, size_t bytes) {
        int target = source == effectTex[0] ? 1 : 0;
        if (EffectPass(effect, target, source, second, data, bytes)) {
            source = effectTex[target]; sourceFbo = effectFbo[target];
            static const unsigned bits[]={0,0,2,4,0,8,16,32,1};
            encodedEffects |= bits[effect];
            return true;
        }
        return false;
    };
    if(spatialAA){
        float size[4]={1.0f/backWidth,1.0f/backHeight,0,0};
        run(8,0,size,sizeof(size));
    }
    if (bloomOn && bloomAmount > 0) {
        float bright[4] = {0.7f, 0.1f, 0, 0};
        bool ok = EffectPass(0, 2, source, 0, bright, sizeof(bright));
        // Match the existing finite Gaussian, with enough std140 storage.
        struct Blur { float x,y,scale; int samples; float pad[4]; };
        Blur horizontal = {1,0,1,3,{}}, vertical = {0,1,1,3,{}};
        for (int i = 0; ok && i < 2; ++i) {
            ok = EffectPass(1, 3, effectTex[2], 0, &horizontal, sizeof(horizontal));
            ok = ok && EffectPass(1, 2, effectTex[3], 0, &vertical, sizeof(vertical));
        }
        float amount[4] = {bloomAmount / 255.0f, 0, 0, 0};
        if (ok) run(2, effectTex[2], amount, sizeof(amount));
    }
    if (motionOn && motionMix > 0) {
        if (historyValid) {
            float mix[4] = {motionMix / 255.0f, 0, 0, 0};
            run(3, historyTex, mix, sizeof(mix));
        }
        // Seed history on the FIRST frame without blending uninitialized pixels.
        gl.BindFramebuffer(GL_FRAMEBUFFER, sourceFbo);
        gl.ActiveTexture(GL_TEXTURE0); gl.BindTexture(GL_TEXTURE_2D, historyTex);
        gl.CopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, backWidth, backHeight);
        historyValid = gl.GetError() == GL_NO_ERROR;
        ForgetEffectState();
    } else historyValid = false;
    if (grime[0] || grime[1] || grime[2]) run(4, 0, grime, sizeof(grime));
    if (dofOn && dofRange > 0 && dofBlur > 0) {
        float u[4] = {dofFocus / 100.0f, dofRange / 100.0f, dofBlur * 0.04f, 0};
        run(5, backDepth, u, sizeof(u));
    }
    if (raysOn && raysAmount > 0 && sunVisible) {
        float u[4] = {raysAmount / 255.0f, sunX, sunY, 0};
        run(6, backDepth, u, sizeof(u));
    }
    if (grade[0] || grade[1] || grade[2] != 1 || grade[3] != 1) run(7, 0, grade, sizeof(grade));
    return sourceFbo;
}
void PrepareShadow()
{
    shadowReady = false;
    if (!shadowType) return;
    if (!shadowProgram) shadowProgram = Link(g_shadow_vert_glsl, g_shadow_frag_glsl);
    if (!shadowProgram) { ForgetEffectState(); return; }
    if (!shadowFbo) {
        gl.ActiveTexture(GL_TEXTURE0);
        gl.GenFramebuffers(1, &shadowFbo); gl.BindFramebuffer(GL_FRAMEBUFFER, shadowFbo);
        gl.GenTextures(1, &shadowMap); gl.BindTexture(GL_TEXTURE_2D, shadowMap);
        gl.TexImage2D(GL_TEXTURE_2D, 0, GL_R32F, shadowSize, shadowSize, 0, GL_RED, GL_FLOAT, NULL);
        gl.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
        gl.FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadowMap, 0);
        gl.GenRenderbuffers(1, &shadowDepth); gl.BindRenderbuffer(GL_RENDERBUFFER, shadowDepth);
        gl.RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, shadowSize, shadowSize);
        gl.FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, shadowDepth);
        if (gl.CheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            Sys_Log("gfx: GL shadow framebuffer incomplete; original shadows retained");
            ReleaseShadow(); ForgetEffectState(); return;
        }
    }
    shadowReady = true; ForgetEffectState();
}
void RenderShadow(const GfxFrame &frame)
{
    if (!shadowReady) return;
    EffectState(shadowFbo, shadowSize, shadowSize);
    gl.Enable(GL_DEPTH_TEST); gl.DepthMask(GL_TRUE); gl.DepthFunc(GL_LEQUAL);
    gl.ClearColor(1,1,1,1); gl.ClearDepth(1); gl.Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    gl.UseProgram(shadowProgram); gl.BindVertexArray(vertexArray);
    GfxTexture *target = NULL;
    for (size_t i = 0; i < frame.commands.size(); ++i) {
        const GfxCommand &c = frame.commands[i];
        if (c.type == GFX_CMD_TARGET) { target = c.target; continue; }
        if (c.type != GFX_CMD_DRAW || c.transformed || target || c.primitive != GFX_TRIANGLELIST) continue;
        const GfxDrawState &state = frame.states[c.state];
        // Preserve SDL_GPU exclusions: no opaque billboard shadows or weather.
        if (!state.depthTest || !state.depthWrite || state.alphaBlend || state.alphaTest) continue;
        gl.BindBufferRange(GL_UNIFORM_BUFFER, VERTEX_BLOCK, uniformBuffer,
            drawUniforms[i].vertexOffset, sizeof(VertexUniforms));
        BindVertices(c.buffer ? ((GlVertexBuffer *)c.buffer->gpu)->buffer : vertexBuffer, false);
        GLint first = (GLint)(c.firstVertexByte / sizeof(GfxVertex));
        if (c.firstIndex != ~0u)
            gl.DrawElementsBaseVertex(PrimitiveMode(c.primitive), c.indexCount, GL_UNSIGNED_SHORT,
                (const void *)(uintptr_t)(c.firstIndex * 2), first);
        else gl.DrawArrays(PrimitiveMode(c.primitive), first, c.vertexCount);
    }
    if (gl.GetError() != GL_NO_ERROR) { shadowReady = false; Sys_Log("gfx: GL shadow pass failed"); }
    if(shadowReady) encodedEffects |= 64;
    ForgetEffectState();
}
float WeatherRandom()
{
    weatherSeed = weatherSeed * 1664525u + 1013904223u;
    return (weatherSeed & 0x7fffffff) / float(0x7fffffff);
}
void RenderWeather()
{
    Uint64 now = SDL_GetTicks();
    float dt = weatherTicks ? std::min(0.05f, float(now - weatherTicks) / 1000.0f) : 0;
    weatherTicks = now;
    if (!weatherType || !weatherAmount) { weather.clear(); weatherCarry = 0; return; }
    if (!weatherProgram) weatherProgram = Link(glWeatherVertex, glWeatherFragment);
    if (!weatherProgram) { ForgetEffectState(); return; }
    weatherCarry += (weatherType == 1 ? 400.0f : 120.0f) * weatherAmount / 255.0f * dt;
    int count = int(weatherCarry); weatherCarry -= count;
    for (int i = 0; i < count && weather.size() < 2048; ++i) {
        float a = WeatherRandom(), b = WeatherRandom();
        weather.push_back(weatherType == 1 ? WeatherParticle{a,-0.05f-b*0.1f,0,0.9f+a*1.2f,0.0015f,0.018f,1.5f} :
            WeatherParticle{a,-0.05f-b*0.1f,(b-0.5f)*0.15f,0.15f+a*0.25f,0.008f,0.008f,6});
    }
    struct Vertex { float x,y,r,g,b,a; };
    std::vector<Vertex> vertices;
    for (auto &p : weather) {
        p.x += p.vx * dt; p.y += p.vy * dt; p.life -= dt;
        if (p.life <= 0 || p.y > 1.05f || p.x < -0.05f || p.x > 1.05f) continue;
        static const int corner[6][2] = {{-1,-1},{1,-1},{-1,1},{-1,1},{1,-1},{1,1}};
        for (auto &c : corner) vertices.push_back({p.x+c[0]*p.sx,p.y+c[1]*p.sy,
            weatherType == 1 ? 0.7f : 0.95f, weatherType == 1 ? 0.8f : 0.98f,1,weatherType == 1 ? 0.35f : 0.5f});
    }
    weather.erase(std::remove_if(weather.begin(), weather.end(), [](const WeatherParticle &p) {
        return p.life <= 0 || p.y > 1.05f || p.x < -0.05f || p.x > 1.05f;
    }), weather.end());
    if (vertices.empty()) { ForgetEffectState(); return; }
    if (!weatherVao) gl.GenVertexArrays(1, &weatherVao);
    if (!weatherBuffer) gl.GenBuffers(1, &weatherBuffer);
    EffectState(backFramebuffer, backWidth, backHeight);
    gl.Enable(GL_BLEND); gl.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    gl.UseProgram(weatherProgram); gl.BindVertexArray(weatherVao);
    gl.BindBuffer(GL_ARRAY_BUFFER, weatherBuffer);
    gl.BufferData(GL_ARRAY_BUFFER, vertices.size()*sizeof(Vertex), vertices.data(), GL_STREAM_DRAW);
    gl.EnableVertexAttribArray(0); gl.VertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,sizeof(Vertex),0);
    gl.EnableVertexAttribArray(1); gl.VertexAttribPointer(1,4,GL_FLOAT,GL_FALSE,sizeof(Vertex),(const void *)(2*sizeof(float)));
    gl.DrawArrays(GL_TRIANGLES,0,vertices.size());
    if(gl.GetError()==GL_NO_ERROR) encodedEffects |= 128;
    ForgetEffectState();
}
