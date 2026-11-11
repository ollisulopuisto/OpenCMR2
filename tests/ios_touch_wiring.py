"""Integration wiring contracts; not an Apple/device run."""
import pathlib
import unittest
R = pathlib.Path(__file__).resolve().parents[1]
class TouchWiring(unittest.TestCase):
    def test_every_presentation_updates_bounds_before_race_gate(self):
        source=(R/'src/render/gfx_core.cpp').read_text().split('static void Present(BOOL vsync, bool stretch)',1)[1]
        self.assertLess(source.index('Input_SetTouchPicture(s_width, s_height, stretch)'),source.index('if (s_raceWorld)'))
        self.assertNotIn('Input_SetTouchPicture', (R/'src/render/touch_overlay.cpp').read_text())
    def test_options_route_exact_selected_slot_before_sampling(self):
        source=(R/'game/GameInfo.cpp').read_text().split('void OptionMenu_UpdateActivePage(void)\n{',1)[1].split('// FUNCTION:',1)[0]
        self.assertLess(source.index('input = 1'),source.index('Input_SetTouchMenuPlayer(input)'))
        self.assertLess(source.index('Input_SetTouchMenuPlayer(input)'),source.index('CInput::UpdateAllAvailableDevices()'))
        player=(R/'src/input/input_sdl.cpp').read_text()
        self.assertIn('index == s_touchMenuPlayer && IosTouchNavigation()',player)
        self.assertIn('s_touchMenuPlayer = 0; // selection applies to this sample only',player)
    def test_pause_uses_owner_slot(self):
        source=(R/'game/GameInfo.cpp').read_text()
        self.assertIn('Input_SetTouchMenuPlayer(g_unk0x0052af58[0]);\n    CInput::UpdateAllAvailableDevices();',source)
    def test_replay_and_movies_preserve_nonmenu_context(self):
        source=(R/'game/GameInfo.cpp').read_text()
        replay=source.split('if (CGameInfo::GetConfiguredGameMode() == 4) {',1)[1].split('return;',1)[0]
        self.assertIn('Input_SetTouchContext(INPUT_TOUCH_DRIVING)',replay)
        self.assertIn('s_moviePreviousTouchContext = Input_GetTouchContext()',source)
        self.assertIn('Input_SetTouchContext(s_moviePreviousTouchContext)',source)
        pause=source.split('void InRaceMenu_Open(BYTE param1)',1)[1].split('// FUNCTION:',1)[0]
        self.assertIn('CGameInfo::SetInputAndGamePaused(1)',pause)
if __name__=='__main__': unittest.main()
