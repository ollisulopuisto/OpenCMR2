#!/usr/bin/env python3
"""Colour grade S-curve: the shader source carries exactly the curve this test evaluates, and the curve keeps its stated properties.
Not a shader compile or a pixel test: the GLSL itself is only checked textually."""
import pathlib, sys
r = pathlib.Path(__file__).resolve().parents[1]
s = (r / "src/render/shaders/colorGrading.frag").read_text()
ok = True
def need(c, m):
    global ok
    if not c: print("FAIL", m); ok = False
need("c = clamp(c, 0.0, 1.0);" in s, "clamp before the curve")
need("float k = clamp((contrast - 1.0) * 2.0, -1.0, 1.0);" in s, "k from contrast")
need("c = mix(c, c * c * (3.0 - 2.0 * c), k);" in s, "mix with smoothstep polynomial")
need("(c - 0.5) * contrast" not in s, "old linear contrast removed")
def curve(x, contrast):
    k = max(-1.0, min(1.0, (contrast - 1.0) * 2.0)); return x + (x * x * (3 - 2 * x) - x) * k
xs = [i / 1000 for i in range(1001)]
for c in (0.0, 0.5, 0.8, 1.0, 1.05, 1.2, 1.5, 2.0):
    ys = [curve(x, c) for x in xs]
    need(all(b >= a - 1e-12 for a, b in zip(ys, ys[1:])), f"monotonic at contrast {c}")
    need(abs(ys[0]) < 1e-12 and abs(ys[-1] - 1) < 1e-12, f"endpoints at contrast {c}")
    k = max(-1.0, min(1.0, (c - 1) * 2))
    need(abs((curve(.5 + 1e-4, c) - curve(.5 - 1e-4, c)) / 2e-4 - (1 + .5 * k)) < 1e-5, f"mid slope 1+k/2 at contrast {c}")
    need(abs((curve(1.0, c) - curve(1 - 1e-6, c)) / 1e-6 - (1 - k)) < 1e-4 and abs((curve(1e-6, c) - curve(0.0, c)) / 1e-6 - (1 - k)) < 1e-4, f"end slope 1-k at contrast {c}")
need(all(curve(x, 1.0) == x for x in xs), "contrast 1 is identity")
need(abs(curve(.9, 1.05) - .9072) < 1e-4 and abs(curve(.95, 1.05) - .9543) < 1e-4, "default values")
sys.exit(0 if ok else 1)
