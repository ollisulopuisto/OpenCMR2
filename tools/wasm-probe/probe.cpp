#include <SDL3/SDL.h>
#define GL_GLEXT_PROTOTYPES
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include <emscripten.h>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <string>
#include "shaders_spirv.h"

static std::string report;
static void say(const char* fmt, ...) {
    char buf[1024];
    va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof buf, fmt, ap); va_end(ap);
    report += buf; report += "\n";
    printf("%s\n", buf);
}
static void publish() {
    EM_ASM({ document.title = "PROBE_DONE"; var pre = document.createElement('pre'); pre.id = 'probe'; pre.textContent = UTF8ToString($0); document.body.appendChild(pre); }, report.c_str());
}
static void checkPtr(const char* name) {
    void* p = (void*)SDL_GL_GetProcAddress(name);
    say("proc %-28s %s", name, p ? "OK" : "MISSING");
}
int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) { say("SDL_Init video FAILED: %s", SDL_GetError()); publish(); return 1; }
    say("SDL_Init video OK (driver: %s)", SDL_GetCurrentVideoDriver() ? SDL_GetCurrentVideoDriver() : "?");
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_Window* w = SDL_CreateWindow("probe", 320, 240, SDL_WINDOW_OPENGL);
    if (!w) { say("SDL_CreateWindow FAILED: %s", SDL_GetError()); publish(); return 1; }
    SDL_GLContext ctx = SDL_GL_CreateContext(w);
    if (!ctx) { say("SDL_GL_CreateContext FAILED: %s", SDL_GetError()); publish(); return 1; }
    say("GL context created");
    const char* ver = (const char*)glGetString(GL_VERSION);
    const char* slv = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    const char* ren = (const char*)glGetString(GL_RENDERER);
    say("GL_VERSION: %s", ver ? ver : "null");
    say("GLSL_VERSION: %s", slv ? slv : "null");
    say("GL_RENDERER: %s", ren ? ren : "null");
    checkPtr("glGetTexImage");
    checkPtr("glClearDepth");
    checkPtr("glDrawElementsBaseVertex");
    checkPtr("glGenSamplers");
    checkPtr("glMapBufferRange");
    checkPtr("glBlitFramebuffer");
    checkPtr("glCompressedTexImage2D");
    checkPtr("glGenVertexArrays");
    checkPtr("glCopyTexSubImage2D");
    // compile the game's real generated ES300 shaders
    std::string vsrc((const char*)g_fixed3d_vert_glsl, sizeof g_fixed3d_vert_glsl);
    std::string fsrc((const char*)g_fixed_frag_glsl, sizeof g_fixed_frag_glsl);
    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    const GLchar* vp = vsrc.c_str(); glShaderSource(vs, 1, &vp, nullptr); glCompileShader(vs);
    GLint ok = 0; glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
    char logb[2048] = {0}; glGetShaderInfoLog(vs, sizeof logb, nullptr, logb);
    say("fixed3d.vert compile: %s %s", ok ? "OK" : "FAILED", logb);
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    const GLchar* fp = fsrc.c_str(); glShaderSource(fs, 1, &fp, nullptr); glCompileShader(fs);
    ok = 0; glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
    memset(logb, 0, sizeof logb); glGetShaderInfoLog(fs, sizeof logb, nullptr, logb);
    say("fixed.frag compile: %s %s", ok ? "OK" : "FAILED", logb);
    GLuint prog = glCreateProgram(); glAttachShader(prog, vs); glAttachShader(prog, fs); glLinkProgram(prog);
    ok = 0; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    memset(logb, 0, sizeof logb); glGetProgramInfoLog(prog, sizeof logb, nullptr, logb);
    say("program link: %s %s", ok ? "OK" : "FAILED", logb);
    publish();
    return 0;
}
