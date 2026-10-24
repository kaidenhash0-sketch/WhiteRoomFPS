WHITE ROOM FPS — Windows build package

This is a native Win32 C++ first-person arena shooter. It has a white room, first-person gun view, mouse aiming, WASD movement, shooting, enemy health, hit reactions, red hit particles, ammo/reload and waves.

IMPORTANT: This source package is not itself a compiled .exe. This chat's build environment has no Windows SDK / Windows cross-compiler, so the Windows binary must be compiled by the included GitHub Actions workflow.

Build/download the actual EXE:
1. Create a repository on GitHub and upload the contents of this folder (including the hidden .github folder).
2. Open the repository's Actions tab and run "Build Windows EXE" (or push the files to main).
3. Open the completed workflow run, scroll to Artifacts, and download WhiteRoomFPS-Windows-x64.
4. Extract the ZIP and double-click WhiteRoomFPS.exe on Windows.

Controls:
- WASD: move
- Mouse: aim
- Left mouse: shoot
- R: reload
- Enter: new wave after clearing the room
- Esc: quit

This is a lightweight stylized raycast-like shooter, not a photorealistic 3D engine. Enemies have simple movement and hit feedback; there is no gore simulation or advanced ragdoll physics.
