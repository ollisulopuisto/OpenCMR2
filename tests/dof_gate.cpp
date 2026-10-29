#include "render/dof_gate.h"
#include <cstdio>
#include <cstdlib>
#define CHECK(x) do{if(!(x)){std::printf("FAIL %s:%d %s\n",__FILE__,__LINE__,#x);std::exit(1);}}while(0)
int main()
{
    // DoF on, blur 0, tunnel driven by speed: the pass runs for tunnel vision only (the reported native case).
    DofGate g = Dof_Gate(true, 30, 0, 0.8f, 0.5f, true);
    CHECK(g.runPass && g.tunnel && !g.depthBlur);
    // DoF off, tunnel active: still runs, with no depth blur.
    g = Dof_Gate(false, 30, 100, 0.8f, 0.0f, true);
    CHECK(g.runPass && g.tunnel && !g.depthBlur);
    // tunnel 0 (target 0, smooth settled): nothing to do with blur 0, status 3; DoF off: status 1.
    g = Dof_Gate(true, 30, 0, 0.0f, 0.0f, true);   CHECK(!g.runPass && g.reason == 3);
    g = Dof_Gate(false, 30, 100, 0.0f, 0.0f, true); CHECK(!g.runPass && g.reason == 1);
    // plain DoF, tunnel 0: unchanged behaviour.
    g = Dof_Gate(true, 30, 30, 0.0f, 0.0f, true);  CHECK(g.runPass && g.depthBlur && !g.tunnel);
    // smoothing tail keeps the pass alive until the tunnel has faded out.
    g = Dof_Gate(true, 30, 30, 0.0f, 0.2f, true);  CHECK(g.runPass && g.tunnel);
    // no sampled depth: no pass.
    g = Dof_Gate(true, 30, 30, 1.0f, 1.0f, false); CHECK(!g.runPass && g.reason == 2);
    // Not a race frame (setup/repair screen car preview, menus): never runs, whatever the settings or the last speed.
    g = Dof_Gate(true, 30, 100, 1.0f, 1.0f, true, false); CHECK(!g.runPass && !g.depthBlur && !g.tunnel && g.reason == 6);
    g = Dof_Gate(false, 30, 0, 1.0f, 1.0f, true, false);  CHECK(!g.runPass && g.reason == 6);
    g = Dof_Gate(true, 30, 100, 1.0f, 1.0f, true, true);  CHECK(g.runPass);   // a race frame is unchanged
    std::puts("dof_gate ok");
}
