from pathlib import Path
root=Path(__file__).resolve().parents[1]
graphics=(root/'game/Graphics.cpp').read_text()
menu=(root/'game/FrontendScreens.cpp').read_text()
start=graphics[graphics.index('BOOL CGraphics::CreateSelectedDisplayAndRenderDevice'):]
assert 'if(!Gfx_IsOpenGL())EffectSettings::Apply();' not in start
assert 'EffectSettings::Apply();' in start
enter=menu[menu.index('void FrontendMenu_EnterAdvancedGraphicsOptions'):menu.index('void FrontendMenu_UpdateAdvancedGraphicsOptions')]
apply=menu[menu.index('void FrontendMenu_ApplyAdvancedGraphicsOptions'):menu.index('void FrontendMenu_ApplyGraphicsOptions')]
assert 'Gfx_IsOpenGL' not in enter+apply
assert 'EffectSettings::Apply();' in apply
assert 'Sys_SetOption' not in enter
print('GL startup and menu share bounded effect reader, no forced-off or enter-time setting writes')
