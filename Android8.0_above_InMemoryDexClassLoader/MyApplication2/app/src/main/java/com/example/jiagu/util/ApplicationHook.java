package com.example.jiagu.util;

import android.app.Application;
import android.content.ContentProvider;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.text.TextUtils;
import android.util.Log;

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Map;
import java.util.Set;

public class ApplicationHook
{

    private static Application org_Application;

    /**
     * jni层回调创建真实Application
     */
    public static void hook(
            Application cur_application,
            String org_ApplicationName
    )
    {
        if (TextUtils.isEmpty(org_ApplicationName) || "com.frezrik.jiagu.StubApp".equals(org_ApplicationName))
        {
            org_Application = cur_application;
            return;
        }

        Log.w("NDK_JIAGU", "hook");
        try
        {
            // 先获取到ContextImpl对象
            Context contextImpl = cur_application.getBaseContext();//这里我感觉应该用ContextImpl tmp_contextImpl = cur_application.getBaseContext();
            // 创建插件中真实的Application且，执行生命周期
            ClassLoader classLoader = cur_application.getClassLoader();
            Class<?> cls_org_application = classLoader.loadClass(org_ApplicationName);
            org_Application = (Application) cls_org_application.newInstance();
            //新创建的org_Application.mbase为空,同时该类型一般为ContextImpl extends Context
            // 走DelegateApplication的生命周期
            Reflect.invokeMethod(
                    Application.class,
                    org_Application,
                    new Object[]{contextImpl},
                    "attach",
                    Context.class);
        } catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    /**
     * hook 替换ApplicationContext
     *
     * @param cur_application
     */
    public static void replaceApplicationContext(Application cur_application)
    {
        if (org_Application == null || "com.frezrik.jiagu.StubApp".equals(org_Application.getClass().getName()))
        {
            return;
        }
        Log.w("NDK_JIAGU", "replaceApplicationContext");

        try
        {
            // 先获取到ContextImpl对象
            Context contextImpl = cur_application.getBaseContext();//ContextWrapper.mbase

            // 替换ContextImpl的代理Application
            Reflect.invokeMethod(
                    contextImpl.getClass(),//class
                    contextImpl,//obj
                    new Object[]{org_Application},//实际参数
                    "setOuterContext",
                    Context.class //参数类型
            );

            // 替换ActivityThread的代理Application
            Object mMainThread = Reflect.getFieldValue(contextImpl.getClass(), contextImpl, "mMainThread");
            Reflect.setFieldValue("android.app.ActivityThread", mMainThread, "mInitialApplication", org_Application);

            // 替换ActivityThread的mAllApplications
            ArrayList<Application> mAllApplications = (ArrayList<Application>) Reflect.getFieldValue("android.app.ActivityThread", mMainThread, "mAllApplications");
            mAllApplications.add(org_Application);//添加新的
            mAllApplications.remove(cur_application);//删除原始的

            // 替换LoadedApk的代理Application
            Object loadedApk = Reflect.getFieldValue(contextImpl.getClass(), contextImpl, "mPackageInfo");
            Reflect.setFieldValue("android.app.LoadedApk", loadedApk, "mApplication", org_Application);

            // 替换LoadedApk中的mApplicationInfo中name
            ApplicationInfo applicationInfo = (ApplicationInfo) Reflect.getFieldValue("android.app.LoadedApk", loadedApk, "mApplicationInfo");
            applicationInfo.className = org_Application.getClass().getName();

            //相比较于其它的项目,,此项目做的处理不是很大,,,或许做多了无用,也或许是作者没有考虑那么多

            org_Application.onCreate();

        } catch (Exception e)
        {
            e.printStackTrace();
        }
    }

    /**
     * 修改已经存在ContentProvider中application
     *
     * @param application
     * @return
     */
    public static Application replaceContentProvider(Application application)
    {
        if (org_Application == null || "com.frezrik.jiagu.StubApp".equals(org_Application.getClass().getName()))
        {
            return application;
        }
        Log.w("NDK_JIAGU", "replaceContentProvider");
        try
        {
            // 替换LoadedApk的代理Application
            Context contextImpl = application.getBaseContext();
            Object loadedApk = Reflect.getFieldValue(contextImpl.getClass(), contextImpl,
                    "mPackageInfo");
            Reflect.setFieldValue("android.app.LoadedApk", loadedApk, "mApplication",
                    org_Application);

            Object activityThread = currentActivityThread();
            Map<Object, Object> mProviderMap =
                    (Map<Object, Object>) Reflect.getFieldValue(activityThread.getClass(),
                            activityThread, "mProviderMap");
            Set<Map.Entry<Object, Object>> entrySet = mProviderMap.entrySet();
            for (Map.Entry<Object, Object> entry : entrySet)
            {
                // 取出ContentProvider
                ContentProvider contentProvider =
                        (ContentProvider) Reflect.getFieldValue(entry.getValue().getClass(),
                                entry.getValue(), "mLocalProvider");

                if (contentProvider != null)
                {
                    // 修改ContentProvider中的context
                    Reflect.setFieldValue("android.content.ContentProvider", contentProvider,
                            "mContext", org_Application);
                }
            }
        } catch (Exception e)
        {
            e.printStackTrace();
        }
        return org_Application;
    }

    private static Object currentActivityThread()
    {
        try
        {
            Class<?> cls = Class.forName("android.app.ActivityThread");
            Method declaredMethod = cls.getDeclaredMethod("currentActivityThread",
                    new Class[0]);
            declaredMethod.setAccessible(true);
            return declaredMethod.invoke(null, new Object[0]);
        } catch (Exception e)
        {
        }
        return null;
    }

}
