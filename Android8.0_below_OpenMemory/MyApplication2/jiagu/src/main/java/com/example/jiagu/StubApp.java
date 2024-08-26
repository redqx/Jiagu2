package com.example.jiagu;

import android.app.Application;
import android.content.Context;
import android.content.pm.PackageManager;

import com.example.jiagu.util.ApplicationHook;
import com.example.jiagu.util.AssetsUtil;
import com.example.jiagu.util.FileUtils;
import com.example.jiagu.util.ZipUtil;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;


public class StubApp extends Application
{
    /**
     * so的版本，格式为: v + 数字
     */
    public static final String VERSION = "v1";

    static
    {
        System.loadLibrary("jiagu");
    }

    public native static void attach(StubApp base);

    @Override
    protected void attachBaseContext(Context context)
    {
        super.attachBaseContext(context);
        // System.load(AssetsUtil.copyJiagu(context));
        attach(this);
    }

    @Override
    public void onCreate()
    {
        super.onCreate();
        ApplicationHook.replaceApplicationContext(this);
    }

    @Override
    public String getPackageName()
    {
        return "JIAGU"; // 如果有ContentProvider，修改getPackageName后会重走createPackageContext
    }

    @Override
    public Context createPackageContext(
            String packageName,
            int flags
    ) throws PackageManager.NameNotFoundException
    {
        return ApplicationHook.replaceContentProvider(this);
    }

    //在native层进行一个调用
    public byte[] invoke1(String s)
    {
        return ZipUtil.getDexData(s);
    }

    //在native层进行一个调用
    public void invoke2(
            Application application,
            String org_appname
    )
    {
        ApplicationHook.hook(application, org_appname);
    }

    //实在不想native层写了,直接native调用
    public String getAppName(Context base)
    {
        return FileUtils.readAppName(base);
    }

    public byte[][] readDex(Context base) throws IOException
    {
        List<byte[]> dexBytesList = new ArrayList<>();
        InputStream inputStream = base.getAssets().open("i11111i111.zip");
        ZipInputStream zipInputStream = new ZipInputStream(inputStream);
        ZipEntry zipEntry;
        while ((zipEntry = zipInputStream.getNextEntry()) != null)
        {
            if (!zipEntry.isDirectory() && zipEntry.getName().endsWith(".dex"))
            {
                // 处理Dex文件
                try
                {
                    ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
                    byte[] buffer = new byte[4096];
                    int bytesRead;
                    while ((bytesRead = zipInputStream.read(buffer)) != -1)
                    {
                        outputStream.write(buffer, 0, bytesRead);
                    }
                    dexBytesList.add(outputStream.toByteArray());
                } catch (IOException e)
                {
                    // 处理异常
                    e.printStackTrace();
                }
            }
        }
        return dexBytesList.toArray(new byte[dexBytesList.size()][]);
    }

}
