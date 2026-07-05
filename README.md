# OpenMW-VR for Meta Quest (native APK)

Self-contained build of OpenMW-VR as a standalone Quest APK (OpenXR + GLES via gl4es).
Everything needed lives in this repo:

```
android/
  buildscripts/   cross-compiles the engine + all native deps for arm64-v8a
  app/            minimal VR APK that loads libopenmw.so straight into VR
```

Based on https://gitlab.com/madsbuvi/openmw/-/tree/openmw-51 and https://github.com/xyzz/openmw-android

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


## Debugging

`build.sh` keeps unstripped libs in `buildscripts/symbols/arm64-v8a/` for symbolizing
tombstones. Watch startup with:

```sh
adb logcat -s OpenMW SDL openxr VrApi DEBUG
```
