Real Strategy Game – Windows Packaging
======================================

Goal
----
Provide a drop-in Windows build so players without development tools can run
the game by double-clicking an `.exe`.

What’s Already Here
-------------------
release/windows/RealStrategyGame/
├── assets/            ⟵ identical to the macOS build
└── (place your .exe here)

How to Produce the Windows Binary
---------------------------------
1. On a Windows machine install:
   - Visual Studio 2022 with Desktop C++ workload (includes MSVC + CMake).
   - Git (optional) to clone the repo.
2. Clone or copy this repository to Windows.
3. In “x64 Native Tools Command Prompt for VS 2022” run:
       cmake -S . -B build_win -DCMAKE_BUILD_TYPE=Release
       cmake --build build_win --config Release
   This generates `build_win/Release/cin.exe`.

Packaging Steps
---------------
1. Copy `build_win/Release/cin.exe` into
       release/windows/RealStrategyGame/
   (Rename if desired, e.g., `RealStrategyGame.exe`.)
2. Verify the folder contains:
       RealStrategyGame.exe
       assets/...
3. Zip `release/windows/RealStrategyGame` and share the archive with players.

Running on Windows
------------------
Players simply extract the zip anywhere and run the `.exe`.  The executable
automatically locates the adjacent `assets/` directory (thanks to the new
runtime asset path resolver), so no extra configuration is required.
