#include <SDL3/SDL.h>
#define GL_GLEXT_PROTOTYPES
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include <emscripten.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "shaders_spirv.h"

static std::string report;
static void say(const char* fmt, ...) {
    char buf[1200]; va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
    report += buf; report += "\n"; printf("%s\n", buf);
}
struct Sh { const char* name; const unsigned char* p; size_t n; GLenum type; };
int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) { say("SDL_Init FAILED: %s", SDL_GetError()); return 1; }
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_Window* w = SDL_CreateWindow("probe2", 320, 240, SDL_WINDOW_OPENGL);
    SDL_GLContext ctx = SDL_GL_CreateContext(w);
    if (!ctx) { say("GL ctx FAILED: %s", SDL_GetError()); return 1; }
    say("GL_VERSION: %s", (const char*)glGetString(GL_VERSION));
    std::vector<Sh> all = {
#define VS(n) {#n, n, sizeof n, GL_VERTEX_SHADER}
#define FS(n) {#n, n, sizeof n, GL_FRAGMENT_SHADER}
        VS(g_fixed3d_vert_glsl), VS(g_fixedtl_vert_glsl), VS(g_motionblur_vert_glsl), VS(g_particle_vert_glsl), VS(g_shadow_vert_glsl),
        FS(g_fixed_frag_glsl), FS(g_blur_frag_glsl), FS(g_brightpass_frag_glsl), FS(g_clouds_frag_glsl), FS(g_composite_frag_glsl),
        FS(g_dof_frag_glsl), FS(g_dust_composite_frag_glsl), FS(g_dust_volume_frag_glsl), FS(g_fog_frag_glsl),
        FS(g_lens_grime_frag_glsl), FS(g_motionblur_frag_glsl), FS(g_particle_frag_glsl), FS(g_shadow_frag_glsl),
        FS(g_spatial_aa_frag_glsl), FS(g_speedstreak_frag_glsl), FS(g_sun_glare_frag_glsl), FS(g_temporal_frag_glsl)
    };
    int pass = 0, fail = 0;
    for (auto& s : all) {
        std::string src((const char*)s.p, s.n);
        GLuint sh = glCreateShader(s.type);
        const GLchar* sp = src.c_str();
        glShaderSource(sh, 1, &sp, nullptr);
        glCompileShader(sh);
        GLint ok = 0; glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
        if (ok) { pass++; }
        else { fail++; char lb[800] = {0}; glGetShaderInfoLog(sh, sizeof lb, nullptr, lb); say("SHADER FAIL %s: %.400s", s.name, lb); }
        glDeleteShader(sh);
    }
    say("shader compile: %d OK, %d FAILED of %d", pass, fail, (int)all.size());
    // direct-symbol check: ES3 core functions the game loads via GetProcAddress
    GLuint vao = 0, vbo = 0, ebo = 0;
    glGenVertexArrays(1, &vao); glBindVertexArray(vao);
    glGenBuffers(1, &vbo); glBindBuffer(GL_ARRAY_BUFFER, vbo);
    float tri[9] = {0,0,0, 1,0,0, 0,1,0};
    glBufferData(GL_ARRAY_BUFFER, sizeof tri, tri, GL_STATIC_DRAW);
    glGenBuffers(1, &ebo); glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    unsigned short idx[3] = {0,1,2};
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof idx, idx, GL_STATIC_DRAW);
    const char* exts = (const char*)glGetString(GL_EXTENSIONS);
    say("ext OES_draw_elements_base_vertex: %s", exts && strstr(exts, "OES_draw_elements_base_vertex") ? "PRESENT" : "absent");
    void* p1 = (void*)SDL_GL_GetProcAddress("glDrawElementsBaseVertexOES");
    void* p2 = (void*)SDL_GL_GetProcAddress("glDrawElementsBaseVertexEXT");
    say("glDrawElementsBaseVertexOES proc: %s, EXT proc: %s", p1 ? "OK" : "MISSING", p2 ? "OK" : "MISSING");
    if (p1) {
        typedef void (*FP)(GLenum, GLsizei, GLenum, const void*, GLint);
        ((FP)p1)(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, nullptr, 0);
        say("glDrawElementsBaseVertexOES call: no crash, glError=%d", (int)glGetError());
    }
    glClearDepthf(1.0f);
    say("glClearDepthf direct call: OK");
    glClearColor(0.2f, 0.4f, 0.6f, 1.0f); glClear(GL_COLOR_BUFFER_BIT);
    unsigned char px[4] = {0}; glReadPixels(0, 0, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
    say("clear+readback: %d,%d,%d,%d (FBO readback works as GetTexImage replacement path)", px[0], px[1], px[2], px[3]);
    EM_ASM({ document.title = "PROBE_DONE"; var pre = document.createElement('pre'); pre.id = 'probe'; pre.textContent = UTF8ToString($0); document.body.appendChild(pre); }, report.c_str());
    return 0;
}
