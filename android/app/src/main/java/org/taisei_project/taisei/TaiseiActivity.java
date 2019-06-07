package org.taisei_project.taisei;

import android.content.res.AssetManager;

import org.libsdl.app.SDLActivity;

import java.io.IOException;
import java.util.Objects;

public final class TaiseiActivity extends SDLActivity {
    private static final String TAG = "TaiseiActivity";

    @Override
    protected String[] getLibraries() {
        return new String[] { "taisei" };
    }

    public static final int ASSET_INVALID = 0;
    public static final int ASSET_FILE = 1;
    public static final int ASSET_DIR = 2;

    public int queryAssetPath(String path) {
        AssetManager am = getAssets();

        try {
            am.open(path).close();
            return ASSET_FILE;
        } catch(IOException e) {
            try {
                boolean haveChildren = Objects.requireNonNull(am.list(path)).length > 0;

                if(!haveChildren) {
                    // or maybe it's just FUCKING EMPTY HUH?
                    return ASSET_INVALID;
                }

                return ASSET_DIR;
            } catch(IOException | NullPointerException e2) {
                return ASSET_INVALID;
            }
        }
    }
}
