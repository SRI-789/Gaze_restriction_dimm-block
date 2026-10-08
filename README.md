# Eye Attention Qt

This is the Qt Quick / C++ version of the Eye Attention desktop demo.

- `qml/Main.qml` contains the visible controls, totals, and desktop privacy overlay.
- `src/AttentionController.*` contains calibration, gaze classification, session timers,
  and overlay state exposed as a `QObject` to QML and Qt WebChannel.
- `tracker.html` runs the existing MediaPipe face/iris landmark model in a hidden
  Qt WebEngine view and sends normalized gaze features to the C++ controller.

The UI and attention/session logic are native QML and C++. MediaPipe inference remains
JavaScript/WebAssembly because a native Windows MediaPipe C++ build would require a
separate, substantially heavier toolchain. Webcam frames are processed locally; the
MediaPipe runtime and model are downloaded from public CDNs on first use. No camera
frames are uploaded.

## Build on Windows

Install Qt 6.8 or newer with the **Qt Quick**, **Qt WebEngine**, and **Qt WebChannel**
modules, plus CMake and a supported C++20 compiler (for example, MSVC matching the Qt kit).
Then from this directory, in a Qt-enabled developer terminal:

```powershell
cmake -S . -B build
cmake --build build --config Release
.\build\Release\eye-attention-qt.exe
```

## Download a Windows build from GitHub

The `Build Windows app` GitHub Actions workflow builds the project and packages the
executable with its Qt runtime files. Push the project to GitHub, open the repository's
**Actions** tab, select a successful **Build Windows app** run, and download the
`eye-attention-qt-windows-x64` artifact. Extract the ZIP and run
`eye-attention-qt.exe`. The app still needs internet access to download MediaPipe
components when tracking starts.

The desktop overlay is a visual privacy aid, not a secure Windows lock. Use `Win+L`
for actual access protection.
