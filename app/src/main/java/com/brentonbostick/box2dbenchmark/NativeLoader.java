package com.brentonbostick.box2dbenchmark;

import android.app.NativeActivity;
import android.os.Bundle;

import java.io.File;

public class NativeLoader extends NativeActivity {

    static {
        System.loadLibrary("box2dbenchmark-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        File filesDir = getFilesDir();

        int res = setFilesDir(filesDir.getAbsolutePath());
        if (res != 0) {
            throw new RuntimeException("setFilesDir failed");
        }
    }

    private native int setFilesDir(String filesDir);
}







