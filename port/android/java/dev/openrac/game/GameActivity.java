// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (c) 2026 the OpenRAC contributors

package dev.openrac.game;

import android.content.Intent;
import android.content.pm.ActivityInfo;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Process;
import android.system.ErrnoException;
import android.system.Os;
import android.util.Log;
import android.view.Display;
import android.view.InputDevice;
import android.view.KeyEvent;
import android.view.View;
import android.view.Window;
import android.view.WindowInsets;
import android.view.WindowInsetsController;
import android.view.WindowManager;

import org.libsdl.app.SDLActivity;

/**
 * The game on Android: SDL's activity around libopenrac-&lt;id&gt;.so, whose SDL_main is the
 * game's main (port/game/common/main.cpp). The launcher starts it with the library to load, the
 * command line it would give the program on a desktop and the environment to run it in
 * (docs/port/ANDROID.md); it runs in a process of its own (":game" in the manifest), which ends
 * with the game.
 */
public class GameActivity extends SDLActivity {
    /** The game's library, without lib and .so: "openrac-rac1-pal". */
    public static final String EXTRA_LIBRARY = "dev.openrac.game.LIBRARY";
    /** The program's arguments, as on a desktop: --data, --window, --levels... */
    public static final String EXTRA_ARGUMENTS = "dev.openrac.game.ARGUMENTS";
    /** NAME=VALUE pairs set before the game starts: HOME, the GPU driver's (platform/gpu_driver.h). */
    public static final String EXTRA_ENVIRONMENT = "dev.openrac.game.ENVIRONMENT";

    private static final String TAG = "OpenRAC";
    /** How long a game asked to quit has to end its process (onDestroy). */
    private static final long QUIT_WAIT_MS = 3000;

    private String library = "openrac-rac1-pal";
    private String[] arguments = new String[0];

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Intent intent = getIntent();
        String named = intent.getStringExtra(EXTRA_LIBRARY);
        if (named != null && named.matches("openrac-[a-z0-9-]+")) {
            library = named;
        }
        String[] given = intent.getStringArrayExtra(EXTRA_ARGUMENTS);
        if (given != null) {
            arguments = given;
        }
        String[] environment = intent.getStringArrayExtra(EXTRA_ENVIRONMENT);
        if (environment != null) {
            for (String pair : environment) {
                int at = pair.indexOf('=');
                if (at <= 0) {
                    continue;
                }
                try {
                    Os.setenv(pair.substring(0, at), pair.substring(at + 1), true);
                } catch (ErrnoException e) {
                    Log.w(TAG, "cannot set " + pair.substring(0, at) + ": " + e);
                }
            }
        }
        Log.i(TAG, "starting lib" + library + ".so with " + String.join(" ", arguments));
        coverCutout();
        super.onCreate(savedInstanceState);
        preferFastestRefresh();
        coverScreen();
    }

    /**
     * The game across the camera's notch or hole as well, the safe area keeping the on-screen pad
     * and the drawer out of it (SDL_GetWindowSafeArea). SDL sets the mode in the window's
     * attributes without handing them back to the window, so it only took when something else
     * changed them later, and a phone could start with a black band where the camera is.
     */
    private void coverCutout() {
        WindowManager.LayoutParams attributes = getWindow().getAttributes();
        attributes.layoutInDisplayCutoutMode = Build.VERSION.SDK_INT >= Build.VERSION_CODES.R
                ? WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_ALWAYS
                : WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
        getWindow().setAttributes(attributes);
    }

    /**
     * The game over the whole screen, without the status and navigation bars (a swipe from the edge
     * shows them for a moment). SDL shows the bars when its activity starts and hides them only
     * once the game's fullscreen window exists, with the old system UI flags, which the system
     * drops when the window is not in front at that moment (the launcher's still closing, a
     * dialog): some phones then kept the status bar, a black band over the top of the game and its
     * frame counter. Applied again whenever SDL changes the style and the game comes back to the
     * front; the game has no windowed mode on Android (window.cpp, set_display_mode).
     */
    @SuppressWarnings("deprecation")
    private void coverScreen() {
        Window window = getWindow();
        window.clearFlags(WindowManager.LayoutParams.FLAG_FORCE_NOT_FULLSCREEN);
        window.addFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN);
        View decor = window.getDecorView();
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            window.setDecorFitsSystemWindows(false);
            WindowInsetsController controller = decor.getWindowInsetsController();
            if (controller != null) {
                controller.setSystemBarsBehavior(WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
                controller.hide(WindowInsets.Type.systemBars());
            }
        } else {
            decor.setSystemUiVisibility(View.SYSTEM_UI_FLAG_FULLSCREEN | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                    | View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                    | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION | View.SYSTEM_UI_FLAG_LAYOUT_STABLE);
        }
        // SDL's own re-hiding after a swipe (onSystemUiVisibilityChange) follows this.
        mFullscreenModeActive = true;
    }

    @Override
    protected boolean sendCommand(int command, Object data) {
        boolean sent = super.sendCommand(command, data);
        if (command == COMMAND_CHANGE_WINDOW_STYLE) {
            // After SDL's own handling of it, which runs on the same thread's queue.
            new Handler(Looper.getMainLooper()).post(this::coverScreen);
        }
        return sent;
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        super.onWindowFocusChanged(hasFocus);
        if (hasFocus) {
            coverScreen();
        }
    }

    /**
     * Landscape only, either way up. The manifest says so (sensorLandscape), but SDL asks again
     * when it opens the window and, for a resizable one, asked for any orientation the player's
     * auto-rotate allows: a phone turned upright turned the game with it, and nothing in the game
     * is laid out for a portrait screen.
     */
    @Override
    public void setOrientationBis(int w, int h, boolean resizable, String hint) {
        setRequestedOrientation(ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE);
    }

    /**
     * The display's fastest refresh, asked for while the game is in front. Without it a display
     * that changes its rate (Odin's 120 Hz panel) follows what the game draws: a while at a cap of
     * 60 frames a second sets it to 60 Hz, and then the display's rate (the "Display" frame rate,
     * the swap's wait) is 60 for good. The game's own frame cap paces it below the display's rate.
     */
    private void preferFastestRefresh() {
        @SuppressWarnings("deprecation")
        Display display = Build.VERSION.SDK_INT >= Build.VERSION_CODES.R ? getDisplay()
                                                                          : getWindowManager().getDefaultDisplay();
        if (display == null) {
            return;
        }
        Display.Mode current = display.getMode();
        Display.Mode fastest = current;
        for (Display.Mode mode : display.getSupportedModes()) {
            if (mode.getPhysicalWidth() == current.getPhysicalWidth()
                    && mode.getPhysicalHeight() == current.getPhysicalHeight()
                    && mode.getRefreshRate() > fastest.getRefreshRate()) {
                fastest = mode;
            }
        }
        WindowManager.LayoutParams attributes = getWindow().getAttributes();
        attributes.preferredDisplayModeId = fastest.getModeId();
        getWindow().setAttributes(attributes);
        Log.i(TAG, "display mode " + fastest.getPhysicalWidth() + "x" + fastest.getPhysicalHeight() + " at "
                + fastest.getRefreshRate() + " Hz");
    }

    @Override
    protected String[] getLibraries() {
        return new String[] {"SDL3", library};
    }

    @Override
    protected String[] getArguments() {
        return arguments;
    }

    /**
     * A handheld's or a controller's Back key (Odin's, a Shield's) as the system's Back, which opens
     * the game's drawer (debug_overlay.h). SDL counts Back from a gamepad as the pad's Select, the
     * same button as KEYCODE_BUTTON_SELECT, and the game then opened its map; the pad's own Select
     * stays Select.
     */
    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        final int source = event.getSource();
        final boolean pad = (source & InputDevice.SOURCE_GAMEPAD) == InputDevice.SOURCE_GAMEPAD
                || (source & InputDevice.SOURCE_JOYSTICK) == InputDevice.SOURCE_JOYSTICK;
        if (event.getKeyCode() == KeyEvent.KEYCODE_BACK && pad) {
            if (event.getAction() == KeyEvent.ACTION_DOWN && event.getRepeatCount() == 0) {
                onNativeKeyDown(KeyEvent.KEYCODE_BACK);
            } else if (event.getAction() == KeyEvent.ACTION_UP) {
                onNativeKeyUp(KeyEvent.KEYCODE_BACK);
            }
            return true;
        }
        return super.dispatchKeyEvent(event);
    }

    @Override
    protected void onDestroy() {
        // Closed from Recents or with the launcher's Stop: the game is asked to quit and ends its
        // process itself (main.cpp, leave). SDL's own onDestroy waits a second for that and then
        // frees the state the game's thread may still be using, and the exit below ran the
        // library's destructors beside the game's: a close that came out as a crash report. The
        // game gets longer here, and one that has not ended by then is ended without either.
        if (isFinishing() && mSDLThread != null && mSDLThread.isAlive()) {
            nativeSendQuit();
            try {
                mSDLThread.join(QUIT_WAIT_MS);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
            if (mSDLThread != null && mSDLThread.isAlive()) {
                Log.w(TAG, "the game did not quit within " + QUIT_WAIT_MS + " ms: ending its process");
                Process.killProcess(Process.myPid());
            }
        }
        super.onDestroy();
        // The game's state lives in its library's globals: a second play in the same process
        // would start from the first's. The process is the game's alone, so it ends with it.
        if (isFinishing()) {
            System.exit(0);
        }
    }
}
