#!/bin/sh -e
# Install + launch OpenMW VR on a connected Quest and capture the startup log.
# Run this once your headset is reachable over adb (USB, or `adb connect <ip>:5555`).
#
# Usage: android/run-on-device.sh [path-to-Morrowind-install]
#   - with no arg, assumes data is already on the device
#   - with a Morrowind/ dir, pushes it to /sdcard/Morrowind first

PKG=org.openmw.vr
ACT=$PKG/.OpenMWActivity
DIR="$(cd "$(dirname "$0")" && pwd)"
APK="$DIR/app/build/outputs/apk/debug/app-debug.apk"
LOG="$DIR/omw-device.log"

if [ -z "$(adb get-state 2>/dev/null)" ]; then
    echo "No device on adb. Connect the Quest (USB or 'adb connect <ip>:5555') and retry." >&2
    echo "Tip: enable Developer Mode + USB/Wireless debugging in the Meta Quest app." >&2
    exit 1
fi
echo ">> device: $(adb shell getprop ro.product.model | tr -d '\r')"

[ -f "$APK" ] || { echo "APK not built. Run buildscripts/build.sh then gradlew assembleDebug." >&2; exit 1; }

if [ -n "$1" ]; then
    echo ">> pushing Morrowind data from $1 to /sdcard/Morrowind/ (this can take a while)"
    adb push "$1/." /sdcard/Morrowind/
fi

echo ">> installing"
adb install -r "$APK"
adb shell pm grant $PKG android.permission.READ_EXTERNAL_STORAGE 2>/dev/null || true
adb shell pm grant $PKG android.permission.WRITE_EXTERNAL_STORAGE 2>/dev/null || true

echo ">> launching $ACT"
adb logcat -c
adb shell am start -n "$ACT"

echo ">> capturing log to $LOG (Ctrl-C to stop)"
adb logcat -v time OpenMW:V SDL:V openxr:V VrApi:I DEBUG:V libc:V AndroidRuntime:E '*:S' | tee "$LOG"
