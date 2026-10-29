import re,sys
fx=open('game/FixedPoint.cpp').read(); rd=open('game/RallyData.cpp').read(); tp=open('game/TuningPanel.h').read()
ok=True
def need(c,m):
    global ok
    if not c: print("FAIL",m); ok=False
need('"video.fov_preview"' in tp,'panel row exists')
need('Sys_GetOption("video.fov_preview", 0)' in fx and 'FovPreview_Effective(preview, 70)' in fx,'projection reads the preview live')
need('if (g_previewFov || shown != g_appliedFovSetting * 65536)' in fx and 'X87S_DIV(0x275c2 * 70.0 * 65536.0, (double)shown)' in fx,'preview scale uses the original formula on the eased fov, uncached')
need('View_GetFovPreview' in rd and 'FOV PREVIEW' in rd and 'Speed FOV %% below is separate' in rd,'overlay labels preview as absolute and separate from Speed FOV %')
need('strcmp(rows[i].key, "video.fov_preview")' in rd,'key 7 never saves the preview')
eh=open('src/platform/enhancements.h').read()
need('--fov-preview' in eh and 'video.fov_preview' in eh and '{"video.fov_preview"' not in eh,'CLI flag exists, not part of any preset')
need('Measured from the projection matrix' in rd and 'View_GetMeasuredFov' in fx,'overlay shows measured angles')
need('deg (0 off)' not in tp,'row is not labelled as degrees')
sys.exit(0 if ok else 1)
