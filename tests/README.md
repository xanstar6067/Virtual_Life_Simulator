Run `./tests/Run-UIWorkspaceSmoke.ps1` from PowerShell with Visual Studio's v143 C++ toolset and Windows SDK installed. Use `-SkipBuild` after building Debug x64 yourself.

The checks exercise the real Classic and CyberBiology3 interfaces using SDL software rendering into memory. They do not create a desktop window, capture the screen, send system input, or use the graphics card.

Coverage includes initial seeding through the header button, pause/run, a single step, organism selection at the new field origin and after zooming, brush tool selection, world save/load, opening the existing tool windows, and resizing from 640×480 to 1920×1080. Header and footer overflow is checked on every frame.

PNG previews and isolated save fixtures are written under `x64/Debug/ui-smoke-classic/` and `x64/Debug/ui-smoke-cb3/`. The tests do not overwrite the project's saved worlds or `imgui.ini`.

Brain viewer regressions cover neuron selection, switching current/initial brains without stale pointers, dragging the window with its diagram, closing using its title-bar cross, and keeping the viewer inside a resized desktop. Input is injected into ImGui in memory, not into Windows.

Pass `-ApplicationTargetName VirtualLifeSimulator1-UI` to build a separate executable when the normal application is open.
