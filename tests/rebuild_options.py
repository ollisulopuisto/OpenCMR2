#!/usr/bin/env python3
"""rebuild.sh asset options: validation and dry run only (no build, no write). Run from anywhere."""
import os, subprocess, sys, tempfile
root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
fails = 0
def run(*args, env=None):
    p = subprocess.run(["bash", os.path.join(root, "rebuild.sh"), *args], capture_output=True, text=True, env=env)
    return p.returncode, p.stdout + p.stderr
def need(cond, msg):
    global fails
    if not cond: print("FAIL", msg); fails += 1
with tempfile.TemporaryDirectory() as d:
    rc, out = run("--help"); need(rc == 0 and "--trees" in out and "--dry-run" in out, "help lists the asset options")
    rc, out = run("--trees", "crossed", "--dry-run", env={**os.environ, "CMR2_DATA": ""}); need(rc == 2 and "--data" in out, "asset option without data stops")
    rc, out = run("--data", d + "/nope", "--trees", "crossed", "--dry-run"); need(rc == 2 and "not found" in out, "missing data folder is reported")
    rc, out = run("--data", d, "--trees", "crossed", "--dry-run"); need(rc == 2 and "frahit.bfl" in out and "found 0" in out, "missing archives are reported, not skipped")
    rc, out = run("--data", d, "--textures", "france1", "--dry-run"); need(rc == 0 and "--prepare-only" in out and not os.path.exists(d + "/enhancements"), "texture dry run needs no scenery archives and writes nothing")
    for b in ("frahit", "fralot", "finhit", "finlot"): open(f"{d}/{b}.bfl", "w").close()
    rc, out = run("--data", d, "--trees", "crossed", "--spectators", "animated", "--dry-run")
    need(rc == 0 and "--trees crossed --spectators animated --prepare-only" in out and "Dry run" in out, "full dry run prints the plan")
    need(sorted(os.listdir(d)) == ["finhit.bfl", "finlot.bfl", "fralot.bfl", "frahit.bfl"] or len(os.listdir(d)) == 4, "dry run leaves the data folder untouched")
    rc, out = run("--trees", "bogus"); need(rc == 2 and "crossed or shell" in out, "bad value rejected")
    rc, out = run("--bogus"); need(rc == 2 and "unknown option" in out, "unknown option rejected")
    rc, out = run("--install-enhancement-deps"); need(rc == 2, "deps flag alone rejected")
    rc, out = run("--assets-only"); need(rc == 2, "assets-only alone rejected")
print("FAILED" if fails else "rebuild options ok")
sys.exit(1 if fails else 0)
