# OpenMW-VR for Meta Quest (native APK)

Self-contained build of OpenMW-VR as a standalone Quest APK (OpenXR + GLES via gl4es).
Everything needed lives in this repo:

```
android/
  buildscripts/   cross-compiles the engine + all native deps for arm64-v8a
  app/            minimal VR APK that loads libopenmw.so straight into VR
```

No 2D launcher, mod manager, or on-screen controls — on a headset, input is OpenXR and
you push your Morrowind data to the device.

## Install & run on the headset (one command)

Once the Quest is reachable over adb (USB, or `adb connect <ip>:5555`):

```sh
android/run-on-device.sh ~/Morrowind   # pushes data, installs, grants perms, launches, tails log
android/run-on-device.sh               # if data is already on the device
```

It captures the startup log to `android/omw-device.log` (filtered to OpenMW/SDL/OpenXR/crash tags).

## Build

```sh
cd android/buildscripts
./build.sh            # downloads the NDK, builds deps + libopenmw.so, deploys into app/
cd ..
ANDROID_HOME=$HOME/Android/Sdk ./gradlew :app:assembleDebug
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

`build.sh` (arm64 only; ~1–2 h first run, incremental after) produces and copies into `app/`:
- `app/src/main/jniLibs/arm64-v8a/`: libopenmw, libSDL2, libopenal, libGL (gl4es),
  libc++_shared, libopenxr_loader
- `app/src/main/java/org/libsdl/app/`: stock SDL2 Java glue (from the SDL we build)
- `app/src/main/assets/libopenmw/`: engine `resources/` + base config

## Game data

The APK ships no Morrowind data. Put your Morrowind install in **either** location:

```sh
# Option A — full Morrowind folder on shared storage (contains "Data Files/"):
adb push Morrowind/ /sdcard/Morrowind/
# Option B — app-scoped folder, just the Data Files contents (no permission needed):
adb push "Morrowind/Data Files/." /sdcard/Android/data/org.openmw.vr/files/data/
```

`OpenMWActivity` regenerates `…/files/config/openmw.cfg` on each launch, scanning
`/sdcard/Morrowind/Data Files`, `/sdcard/Morrowind`, and the app-scoped `data/` folder — it adds a
`data=` line for each that exists and a `content=` entry per game file found (Morrowind/Tribunal/
Bloodmoon ordered first). For Option A, grant the storage permission when prompted, or pre-grant:

```sh
adb shell pm grant org.openmw.vr android.permission.READ_EXTERNAL_STORAGE
```

## How it fits together

- The engine builds as `libopenmw.so` with `BUILD_OPENMW_VR=ON` and (on Android)
  `XR_USE_PLATFORM_ANDROID` + `XR_USE_GRAPHICS_API_OPENGL_ES` (see `components/CMakeLists.txt`,
  `components/xr/`).
- `OpenMWActivity` (extends SDL's stock `SDLActivity`) sets the gl4es env, loads the libs,
  hands the config/data paths to the engine via the `getPathToJni` native
  (`components/files/androidpath.cpp`), and boots into VR.
- The OpenXR loader is built from source by the engine's own `extern/` (no external SDK).

## Debugging

`build.sh` keeps unstripped libs in `buildscripts/symbols/arm64-v8a/` for symbolizing
tombstones. Watch startup with:

```sh
adb logcat -s OpenMW SDL openxr VrApi DEBUG
```
