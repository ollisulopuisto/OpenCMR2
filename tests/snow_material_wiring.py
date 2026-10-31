# Snow material (video.snow_material) wiring: the lift flows through the
# shared vertex-diffuse channel, so both backends must shade the SAME
# fixed3d shader source and Mesh_RefreshVertices must keep its verbatim
# 0x30-stride copy into the locked vertex buffer.
import pathlib, sys

root = pathlib.Path(__file__).resolve().parents[1]

def rd(p): return (root / p).read_text()

gl = rd("src/render/gfx_gl.cpp")
gpu = rd("src/render/gfx_gpu.cpp")
vert = rd("src/render/shaders/fixed3d.vert")
mesh = rd("game/Mesh.cpp")
core = rd("src/render/gfx_core.cpp")
hook = rd("game/StageObjects.cpp")
race = rd("game/Race.cpp")
osd = rd("game/RallyData.cpp")
enh = rd("src/platform/enhancements.h")

def need(cond, msg):
    if not cond:
        print("snow_material_wiring:", msg)
        sys.exit(1)

need("g_fixed3d_vert_glsl" in gl, "GL backend must build the shared fixed3d vertex shader")
need("SHADER_CODE(fixed3d_vert)" in gpu, "SDL_GPU backend must build the shared fixed3d vertex shader")
need("vec4 diffuse = color1" in vert and "inDiffuse" in vert,
     "fixed3d must shade the vertex diffuse channel (both backends use this source)")
need("Mesh_RefreshVertices" in mesh and "pMesh->field_0x10 * 0x30" in mesh,
     "Mesh_RefreshVertices must keep the verbatim 0x30-stride copy")
need("Gfx_LockVertexBuffer" in core and "buffer->vertices.data()" in core,
     "locked vertex bytes must be the shared executor-facing store")
need("SnowMaterial_UpdateFrame();" in hook, "per-frame update hook (view 0)")
need("SnowMaterial_SetStageScene(root)" in race, "stage scene capture hook")
need("SnowMaterial_GetStats" in osd, "debug OSD line")
need('"--snow-material"' in enh and '"video.snow_material"' in enh, "flag plumbing")
print("snow_material_wiring: ok")
