Run `./tests/Run-UIWorkspaceSmoke.ps1` from PowerShell with Visual Studio's v143 C++ toolset and Windows SDK installed. Use `-SkipBuild` after building Debug x64 yourself.

The checks exercise the real Classic and CyberBiology3 interfaces using SDL software rendering into memory. They do not create a desktop window, capture the screen, send system input, or use the graphics card.

Coverage includes initial seeding through the header button, pause/run, a single step, organism selection at the new field origin and after zooming, brush tool selection, world save/load, opening the existing tool windows, and resizing from 640×480 to 1920×1080. Header and footer overflow is checked on every frame.

PNG previews and isolated save fixtures are written under `x64/Debug/ui-smoke-classic/` and `x64/Debug/ui-smoke-cb3/`. The tests do not overwrite the project's saved worlds or `imgui.ini`.

Brain viewer regressions cover neuron selection, switching current/initial brains without stale pointers, dragging the window with its diagram, closing using its title-bar cross, and keeping the viewer inside a resized desktop. Input is injected into ImGui in memory, not into Windows. Undo/redo checks click the real randomize and clear buttons, compare exact neuron snapshots across both brain versions, preserve live neuron memory, discard redo after branching, ignore no-op edits, and reset history on owner changes, deselection, closing and reopening.

CB3 brain regressions check output activation thresholds, output biases, and large positive/negative sums through `Process` and `GetOutput`. Real bot ticks cover all eight desired headings and the zero vector, including rotation energy cost.

Shared brain genomes (both engines): a child without mutation shares its parent's genome, while mutation, global mutation and editing through the brain window give the bot its own copy and leave relatives unchanged; a child of an edited bot is rebuilt from the initial brain, and neuron memory stays per bot.

Pass `-ApplicationTargetName VirtualLifeSimulator1-UI` to build a separate executable when the normal application is open.

The slim, single-row header is checked for overflow. All four display-mode buttons are clicked in wide and compact footers, including returning from hidden-field mode. Feeding descriptions are checked against real bot state for unknown, photosynthetic, predatory, organic, mixed, and two-thirds boundary cases in both engines.

Chart checks cover all four ecosystem modes and periods at wide and compact sizes, exact snapshots of feeding/energy/age, an empty population, worker-tick births and deaths versus manual editing, per-100-tick rates, replacing a paused snapshot, and 25,000 observations without losing the earliest time or extrema. Chart previews are stored alongside the workspace PNGs. All worlds and synthetic history fixtures remain inside the isolated test directories.
