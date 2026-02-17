Real Strategy Game – Standalone Build
====================================

Contents
--------
release/
├── macos/
│   ├── RealStrategyGame/
│   │   ├── cin          ⟵ prebuilt macOS binary
│   │   └── assets/      ⟵ textures, models, shaders, audio, etc.
│   └── Run.command      ⟵ helper script that cds here and runs `cin`
└── windows/
    ├── RealStrategyGame/ (placeholder)
    └── README.txt       ⟵ how to drop in the Windows build

macOS Requirements
------------------
- 64-bit macOS 12+ with OpenGL 4.1 capable GPU.
- Gatekeeper may need you to approve the unsigned binary the first time.

macOS Running
-------------
Double-click `Run.command` (or run it from Terminal) so Terminal handles the
launch:
    cd /path/to/release/macos
    ./Run.command

macOS protects Desktop/Documents/Downloads.  If you keep this folder on one of
those locations, grant Terminal/`cin` access via System Settings → Privacy &
Security → Files & Folders, or move the folder somewhere else (e.g.
`/Applications/RealStrategyGame`, `/Users/Shared`).  The executable looks for
the `assets` directory beside it, so keep that structure intact.

Windows Support
---------------
The repo now searches for assets relative to the executable, so any Windows
build (created via Visual Studio/CMake) can be copied directly into
`release/windows/RealStrategyGame` beside the provided `assets/` folder.  See
`release/windows/README.txt` for build + usage details.  Once the Windows
binary is placed there (e.g., `RealStrategyGame.exe`), the package becomes a
fully portable zip for Windows users as well.
