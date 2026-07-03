package org.openmw.vr;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.system.Os;
import android.util.Log;

import org.libsdl.app.SDLActivity;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Minimal Quest VR entry point. Extends SDL's stock activity, loads libopenmw.so, and on first
 * run deploys the engine's bundled resources/config so OpenMW can boot straight into VR.
 *
 * The full openmw-android launcher (mod manager, on-screen controls, data-import wizard) is
 * intentionally omitted — on a headset, input is OpenXR. Game data is read from either a shared
 * /sdcard/Morrowind folder or the app's external data dir (see android/README.md).
 */
public class OpenMWActivity extends SDLActivity {

    private static final String TAG = "OpenMW";
    private static final int REQ_STORAGE = 42;

    // App-scoped external dir: /sdcard/Android/data/org.openmw.vr/files  (no storage permission needed).
    private String userStorage;   // user config + data live here
    private String globalRoot;    // = filesDir.getParent(); engine reads <global>/files/config

    private native void getPathToJni(String pathGlobal, String pathUser);

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // Ask for shared-storage read so we can use /sdcard/Morrowind. Non-blocking: super.onCreate
        // must still run now; onResume regenerates the config once permission is granted.
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M
                && checkSelfPermission(Manifest.permission.READ_EXTERNAL_STORAGE) != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(new String[] { Manifest.permission.READ_EXTERNAL_STORAGE,
                    Manifest.permission.WRITE_EXTERNAL_STORAGE }, REQ_STORAGE);
        }

        userStorage = getExternalFilesDir(null).getAbsolutePath();
        globalRoot = getFilesDir().getParent();

        setup();

        super.onCreate(savedInstanceState); // loads native libs (see getLibraries), starts SDL
        getPathToJni(globalRoot, userStorage);
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        // Permission may have just been granted — regenerate so /sdcard/Morrowind is picked up
        // (takes effect on the next launch; the SDL thread has already read the config this run).
        if (requestCode == REQ_STORAGE)
            setup();
    }

    private void setup() {
        try {
            deployAssetsIfNeeded();
            writeConfig();
        } catch (Exception e) {
            Log.e(TAG, "Failed to set up OpenMW config", e);
        }
    }

    @Override
    protected String[] getLibraries() {
        // Order matters: dependencies before libopenmw.so; last entry is the "main" object.
        return new String[] { "c++_shared", "openal", "SDL2", "GL", "openmw" };
    }

    @Override
    protected String[] getArguments() {
        return new String[] {}; // everything comes from openmw.cfg
    }

    @Override
    public void loadLibraries() {
        // Done before the libs load so native getenv() sees it.
        try {
            // gl4es is a GL->GLES2 translator; LIBGL_ES=3 breaks its context creation. Keep GLES2.
            // (True GLES3 would require a native GLES3 OSG/OpenMW build without gl4es.)
            Os.setenv("LIBGL_ES", "2", true);
            Os.setenv("OPENMW_GLES_VERSION", "2", true);
            Os.setenv("LIBGL_LOGSHADERERROR", "1", true); // gl4es: dump source+error on shader fail

            // Quest disables the Khronos OpenXR runtime broker (com.oculus.systemdriver/
            // .DisabledRuntimeBroker), so the loader can't auto-discover the runtime. Point it
            // straight at the system's active-runtime manifest (Meta's libopenxr_forwardloader.so).
            for (String p : new String[] {
                    "/odm/etc/openxr/1/active_runtime.aarch64.json",
                    "/vendor/etc/openxr/1/active_runtime.aarch64.json",
                    "/system/etc/openxr/1/active_runtime.aarch64.json" }) {
                if (new File(p).exists()) {
                    Os.setenv("XR_RUNTIME_JSON", p, true);
                    Log.i(TAG, "XR_RUNTIME_JSON=" + p);
                    break;
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "setenv failed", e);
        }
        super.loadLibraries();
    }

    private File globalConfigDir() { return new File(getFilesDir(), "config"); }
    private File resourcesDir()    { return new File(getFilesDir(), "resources"); }

    /** Copy the bundled engine files out of the APK on first run (or after an update). */
    private void deployAssetsIfNeeded() throws Exception {
        File stamp = new File(getFilesDir(), "deployed-" + appVersion());
        if (stamp.exists())
            return;

        copyAssetDir("libopenmw/resources", resourcesDir());
        File cfgDir = globalConfigDir();
        cfgDir.mkdirs();
        // These go in the global config dir (paths.front()):
        //  - overrides.bin: VR settings overrides; the engine THROWS at startup without it in VR mode.
        //  - openxrinteractionprofiles.xml: VR controller bindings (configureVRInputProfiles looks here).
        for (String f : new String[] { "openmw.base.cfg", "defaults.bin", "gamecontrollerdb.txt",
                "overrides.bin", "openxrinteractionprofiles.xml" })
            copyAssetFile("libopenmw/openmw/" + f, new File(cfgDir, f));

        stamp.getParentFile().mkdirs();
        new FileOutputStream(stamp).close();
    }

    /**
     * Candidate data folders, in priority order (only the ones that exist are used):
     *   1. /sdcard/Morrowind/Data Files  — a full Morrowind install dropped on shared storage
     *   2. /sdcard/Morrowind             — fallback if data was extracted there directly
     *   3. <app external files>/data     — app-scoped, always writable, no permission needed
     * 1 & 2 need the storage permission.
     */
    private java.util.List<File> dataDirs() {
        File ext = Environment.getExternalStorageDirectory();
        java.util.ArrayList<File> dirs = new java.util.ArrayList<>();
        dirs.add(new File(ext, "Morrowind/Data Files"));
        dirs.add(new File(ext, "Morrowind"));
        dirs.add(new File(userStorage, "data"));
        return dirs;
    }

    /** Generate openmw.cfg each launch: global points at resources, user lists data dirs + content. */
    private void writeConfig() throws Exception {
        // Global config: base settings + where the engine finds its resources. Drop any
        // resources= line from the base cfg first — it's single-valued, so our absolute
        // Android path must be the only one (the bundled base cfg ships a relative ./resources).
        File baseCfg = new File(globalConfigDir(), "openmw.base.cfg");
        StringBuilder global = new StringBuilder();
        if (baseCfg.exists()) {
            for (String line : readFile(baseCfg).split("\n", -1)) {
                if (line.replace(" ", "").startsWith("resources="))
                    continue;
                global.append(line).append('\n');
            }
        }
        global.append("resources=\"").append(resourcesDir().getAbsolutePath()).append("\"\n");
        writeFile(new File(globalConfigDir(), "openmw.cfg"), global.toString());

        // User config: every existing data dir + the game files found across them.
        File userCfgDir = new File(userStorage, "config");
        userCfgDir.mkdirs();
        new File(userStorage, "data").mkdirs(); // ensure the app-scoped fallback exists

        StringBuilder sb = new StringBuilder();
        java.util.List<File> present = new java.util.ArrayList<>();
        for (File d : dataDirs()) {
            if (d.isDirectory()) {
                sb.append("data=\"").append(d.getAbsolutePath()).append("\"\n");
                present.add(d);
            }
        }
        // BSA archives hold Morrowind's meshes/textures/UI assets; without fallback-archive=
        // entries the world renders black. Register every .bsa found (base game first).
        for (String bsa : detect(present, new String[] { ".bsa" },
                new String[] { "Morrowind.bsa", "Tribunal.bsa", "Bloodmoon.bsa" }))
            sb.append("fallback-archive=").append(bsa).append('\n');
        for (String esm : detect(present, new String[] { ".esm", ".omwgame", ".esp", ".omwaddon" },
                new String[] { "Morrowind.esm", "Tribunal.esm", "Bloodmoon.esm" }))
            sb.append("content=").append(esm).append('\n');
        writeFile(new File(userCfgDir, "openmw.cfg"), sb.toString());
        Log.i(TAG, "openmw.cfg data dirs: " + present);

        // Seed fog defaults once: radial fog (no fog swimming when turning the head) + sky
        // blending (distant geometry fades into the sky instead of popping at the clip plane).
        // Linear fog, not exponential: exponential hazes the near range and bleaches the scene.
        // Only when the user file has no [Fog] section yet, so in-game changes stick.
        File userSettings = new File(userCfgDir, "settings.cfg");
        String existing = userSettings.exists() ? readFile(userSettings) : "";
        if (!existing.contains("[Fog]"))
            writeFile(userSettings,
                    existing + "\n[Fog]\nradial fog = true\nsky blending = true\n");
    }

    /** Files with any of the given extensions across all data dirs, deduped, known names first. */
    private static String[] detect(java.util.List<File> dataDirs, String[] exts, String[] knownOrder) {
        java.util.ArrayList<String> out = new java.util.ArrayList<>();
        for (String known : knownOrder) {
            for (File d : dataDirs)
                if (new File(d, known).exists() && !out.contains(known)) out.add(known);
        }
        for (File d : dataDirs) {
            File[] files = d.listFiles();
            if (files == null) continue;
            java.util.Arrays.sort(files);
            for (File f : files) {
                String n = f.getName();
                for (String ext : exts) {
                    if (n.toLowerCase().endsWith(ext) && !out.contains(n)) { out.add(n); break; }
                }
            }
        }
        return out.toArray(new String[0]);
    }

    private String appVersion() {
        try {
            return getPackageManager().getPackageInfo(getPackageName(), 0).versionName;
        } catch (Exception e) {
            return "0";
        }
    }

    // --- tiny IO helpers ---
    private void copyAssetDir(String assetPath, File dst) throws Exception {
        String[] children = getAssets().list(assetPath);
        if (children == null || children.length == 0) { // it's a file
            copyAssetFile(assetPath, dst);
            return;
        }
        dst.mkdirs();
        for (String c : children)
            copyAssetDir(assetPath + "/" + c, new File(dst, c));
    }

    private void copyAssetFile(String assetPath, File dst) throws Exception {
        dst.getParentFile().mkdirs();
        try (InputStream in = getAssets().open(assetPath); OutputStream out = new FileOutputStream(dst)) {
            byte[] buf = new byte[65536];
            int n;
            while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
        }
    }

    private static String readFile(File f) throws Exception {
        byte[] b = new byte[(int) f.length()];
        try (java.io.FileInputStream in = new java.io.FileInputStream(f)) { in.read(b); }
        return new String(b);
    }

    private static void writeFile(File f, String s) throws Exception {
        f.getParentFile().mkdirs();
        try (FileOutputStream out = new FileOutputStream(f)) { out.write(s.getBytes()); }
    }
}
