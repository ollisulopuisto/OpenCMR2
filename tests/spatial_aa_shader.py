#!/usr/bin/env python3
"""Optional real-pixel shader test. Requires moderngl, numpy, Pillow, glslangValidator,
spirv-cross and an OpenGL 3.3 context (EGL by default). No game data needed.
"""
import argparse
import pathlib
import shutil
import subprocess
import tempfile

import moderngl
import numpy as np
from PIL import Image, ImageDraw


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--output', type=pathlib.Path)
    parser.add_argument('--backend', default='egl')
    args = parser.parse_args()
    root = pathlib.Path(__file__).resolve().parents[1]
    with tempfile.TemporaryDirectory() as scratch:
        spv = pathlib.Path(scratch) / 'aa.spv'
        subprocess.run([shutil.which('glslangValidator'), '-V',
                        str(root / 'src/render/shaders/spatial_aa.frag'), '-o', str(spv)], check=True)
        frag = subprocess.check_output([shutil.which('spirv-cross'), str(spv),
                                        '--version', '330', '--no-es'], text=True)
    ctx = moderngl.create_standalone_context(require=330, backend=args.backend)
    program = ctx.program(vertex_shader='''#version 330
    void main() { vec2 p = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
        gl_Position = vec4(p * 2.0 - 1.0, 0, 1); }''', fragment_shader=frag)
    w, h = 320, 200
    program['SpatialAAUniforms'].binding = 0
    ctx.buffer(np.array([1/w, 1/h, 0, 0], dtype='f4').tobytes()).bind_to_uniform_block(0)
    program['scene'].value = 0
    vao = ctx.vertex_array(program, [])

    def render(pixels):
        texture = ctx.texture((w, h), 4, pixels.tobytes(), dtype='f1')
        texture.filter = (moderngl.LINEAR, moderngl.LINEAR)
        texture.repeat_x = texture.repeat_y = False
        target = ctx.texture((w, h), 4, dtype='f1')
        framebuffer = ctx.framebuffer([target])
        framebuffer.use()
        texture.use(0)
        vao.render(vertices=3)
        result = np.frombuffer(framebuffer.read(components=4), dtype=np.uint8).reshape(h, w, 4).copy()
        framebuffer.release(); target.release(); texture.release()
        return result

    constant = np.full((h, w, 4), [53, 111, 190, 73], dtype=np.uint8)
    assert np.max(np.abs(render(constant).astype(int) - constant.astype(int))) <= 1, 'flat colour or alpha changed'
    gradient = constant.copy()
    gradient[:, :, :3] = np.arange(w, dtype=np.uint8)[None, :, None] // 2
    gradient[:, :, 3] = 255
    assert np.max(np.abs(render(gradient).astype(int) - gradient.astype(int))) <= 1, 'low-contrast gradient changed'
    image = Image.new('RGBA', (w, h), '#18202aff')
    draw = ImageDraw.Draw(image)
    draw.polygon([(0, 180), (300, 0), (319, 0), (319, 199), (0, 199)], fill='#ddd5b8ff')
    draw.line([(10, 15), (290, 145)], fill='white', width=1)
    draw.text((12, 165), 'HUD  123  Stage 01', fill='white')
    pixels = np.asarray(image)
    filtered = render(pixels)
    assert np.any(filtered != pixels), 'edge smoothing did not run'
    assert np.array_equal(filtered[:, :, 3], pixels[:, :, 3]), 'alpha changed'
    # Clamp-to-edge must not leak a bright right border onto a dark left border.
    borders = constant.copy(); borders[:, -1, :3] = 255
    assert np.max(np.abs(render(borders)[:, 0].astype(int) - borders[:, 0].astype(int))) <= 1
    if args.output:
        comparison = Image.new('RGB', (w * 2, h + 24), 'black')
        comparison.paste(image.convert('RGB'), (0, 24))
        comparison.paste(Image.fromarray(filtered).convert('RGB'), (w, 24))
        labels = ImageDraw.Draw(comparison)
        labels.text((6, 6), 'Original synthetic input', fill='white')
        labels.text((w + 6, 6), 'Opt-in spatial AA (real shader output)', fill='white')
        comparison.resize((w * 4, (h + 24) * 2)).save(args.output)
    print('PASS: flat colour, alpha, gradient, diagonal edge, clamp-to-edge; renderer:', ctx.info['GL_RENDERER'])


if __name__ == '__main__':
    main()
