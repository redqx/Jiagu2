package com.example.myapplication2;

import android.content.Context;
import android.util.Log;

import dalvik.system.InMemoryDexClassLoader;

public class myApplication extends android.app.Application {

    @Override
    public void onCreate() {
        super.onCreate();
        //InMemoryDexClassLoader;
        Log.d("myApplication", "onCreate: ");
    }
    @Override
    protected void attachBaseContext(Context base) {
        super.attachBaseContext(base);
        Log.d("myApplication", "attachBaseContext: ");
    }
}
