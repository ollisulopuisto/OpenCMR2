import re,sys
fx=open('game/FixedPoint.cpp').read(); rd=open('game/RallyData.cpp').read(); tp=open('game/TuningPanel.h').read()
ok=True
def need(c,m):
    global ok
    if not c: print("FAIL",m); ok=False
need('"video.fov_preview"' in tp,'panel row exists')
need('Sys_GetOption("video.fov_preview", 0)' in fx and 'FovPreview_Effective(preview, 70)' in fx,'projection reads the preview live')
need(re.search(r'if \(g_previewFov\) return FTOI\(int, X87S_DIV\(0x275c2 \* 70\.0, g_previewFov\)\)',fx) is not None,'preview scale uses the original formula, uncached')
need('View_GetFovPreview' in rd and 'FOV PREVIEW' in rd and 'Speed FOV %% below is separate' in rd,'overlay labels preview as absolute and separate from Speed FOV %')
need('strcmp(rows[i].key, "video.fov_preview")' in rd,'key 7 never saves the preview')
need('video.fov_preview' not in open('src/platform/enhancements.h').read(),'preview is not a CLI preset or default')
sys.exit(0 if ok else 1)
