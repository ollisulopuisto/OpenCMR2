#!/usr/bin/env python3
"""Optional real-pixel post-effect regression. Same dependencies as spatial_aa_shader.py."""
import argparse
import pathlib
import subprocess
import tempfile

import moderngl
import numpy as np
from PIL import Image, ImageDraw


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=pathlib.Path)
    parser.add_argument('--shader-root', type=pathlib.Path)
    args = parser.parse_args()
    root = args.shader_root or pathlib.Path(__file__).resolve().parents[1] / 'src/render/shaders'
    ctx = moderngl.create_standalone_context(require=330, backend='egl')
    w, h = 320, 200
    programs = {}
    vertex = '''#version 330
    out vec2 inTex;
    void main() { vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(p * 2.0 - 1.0, 0, 1); inTex = p; }'''
    for name in ('dof', 'godRays', 'colorGrading'):
        with tempfile.TemporaryDirectory() as scratch:
            spv = str(pathlib.Path(scratch) / 'effect.spv')
            subprocess.run(['glslangValidator', '-V', str(root / (name + '.frag')), '-o', spv], check=True)
            fragment = subprocess.check_output(['spirv-cross', spv, '--version', '330', '--no-es'], text=True)
        programs[name] = ctx.program(vertex_shader=vertex, fragment_shader=fragment)

    def render(name, pixels, depth, uniforms):
        program = programs[name]
        block = {'dof': 'DoFUniforms', 'godRays': 'GodRaysUniforms', 'colorGrading': 'ColorGradingUniforms'}[name]
        program[block].binding = 0
        ubo = ctx.buffer(np.array(uniforms, dtype='f4').tobytes()); ubo.bind_to_uniform_block(0)
        scene = ctx.texture((w, h), 4, pixels.tobytes())
        scene.filter = (moderngl.LINEAR, moderngl.LINEAR); scene.repeat_x = scene.repeat_y = False
        scene.use(0); program['sceneTex' if name == 'dof' else 'scene'].value = 0
        d = ctx.texture((w, h), 1, depth.astype('f4').tobytes(), dtype='f4')
        d.filter = (moderngl.NEAREST, moderngl.NEAREST); d.repeat_x = d.repeat_y = False
        if name != 'colorGrading': d.use(1); program['depthTex'].value = 1
        target = ctx.texture((w, h), 4)
        fbo = ctx.framebuffer([target]); fbo.use()
        vao = ctx.vertex_array(program, []); vao.render(vertices=3)
        result = np.frombuffer(fbo.read(components=4), dtype='u1').reshape(h, w, 4).copy()
        vao.release(); fbo.release(); target.release(); scene.release(); d.release(); ubo.release()
        return result

    image = Image.new('RGBA', (w, h), '#97badbff')
    draw = ImageDraw.Draw(image)
    draw.ellipse((240, 12, 285, 55), fill='#ffffdfff')
    draw.polygon([(0, 130), (100, 60), (180, 115), (319, 75), (319, 199), (0, 199)], fill='#305137ff')
    draw.polygon([(70, 199), (160, 104), (175, 104), (280, 199)], fill='#80817aff')
    for x in range(0, w, 12): draw.line((x, 105, x + 22, 199), fill='#9b926aff', width=1)
    draw.rectangle((24, 60, 40, 160), fill='#202a25ff')
    draw.text((8, 178), 'SYNTHETIC - NOT GAME FOOTAGE', fill='white')
    pixels = np.asarray(image)
    depth = np.full((h, w), 0.9, dtype='f4'); depth[:70] = 1.0; depth[60:160, 24:41] = 0.3
    neutral = render('colorGrading', pixels, depth, [0, 0, 1, 1])
    assert np.max(np.abs(neutral.astype(int) - pixels.astype(int))) <= 1, 'neutral grade must preserve original colors'
    grade = render('colorGrading', pixels, depth, [0.15, 0, 1.05, 1.03])
    focused = render('dof', pixels, np.full((h, w), 0.5), [0.5, 0.3, 4, 0])
    assert np.array_equal(focused, pixels), 'in-focus pixels changed'
    dof = render('dof', pixels, depth, [0.3, 0.25, 4, 0])
    assert np.array_equal(dof[80:150, 28:37], pixels[80:150, 28:37]), 'focused foreground blurred'
    assert np.any(dof != pixels), 'out-of-focus blur absent'
    constant = np.full((h, w, 4), [61, 93, 132, 113], dtype='u1')
    flat = render('dof', constant, np.full((h, w), 0.9), [0.3, 0.25, 4, 0])
    assert np.max(np.abs(flat.astype(int) - constant.astype(int))) <= 1, 'defocus must not change flat color/contrast'
    rays = render('godRays', pixels, depth, [0.65, 0.85, 0.15, 0])
    blocked = render('godRays', pixels, np.zeros((h, w)), [0.65, 0.85, 0.15, 0])
    assert np.max(np.abs(blocked.astype(int) - pixels.astype(int))) <= 1, 'fully occluded frame acquired light'
    off = render('godRays', pixels, depth, [0, 0.85, 0.15, 0])
    assert np.array_equal(off, pixels), 'zero rays changed frame'
    assert np.any(rays != pixels), 'visible sky produced no shafts'
    for output in (neutral, grade, dof, rays): assert np.array_equal(output[:, :, 3], pixels[:, :, 3])
    if args.output:
        canvas = Image.new('RGB', (w * 2, (h + 24) * 2), 'black'); labels = ImageDraw.Draw(canvas)
        for i, (label, arr) in enumerate([('Synthetic input', pixels), ('Subtle warm grade', grade),
                                         ('Depth-aware disk blur', dof), ('Occluded light shafts', rays)]):
            x, y = (i % 2) * w, (i // 2) * (h + 24)
            canvas.paste(Image.fromarray(arr).convert('RGB'), (x, y + 24)); labels.text((x + 5, y + 5), label, fill='white')
        canvas.resize((w * 4, (h + 24) * 4)).save(args.output)
    print('PASS: neutral grade, focused preservation, real defocus, flat color, occluded/zero rays, alpha;', ctx.info['GL_RENDERER'])


if __name__ == '__main__':
    main()
