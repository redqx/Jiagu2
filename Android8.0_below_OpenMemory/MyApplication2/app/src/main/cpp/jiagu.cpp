//
// Created by Ming on 2021/9/7.
//

#include <jni.h>
#include <string>
#include <vector>
#include<sys/mman.h>
#include<sys/types.h>
#include<fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "utils/plog.h"
#include "utils/invoke_java.h"
#include "utils/aes.h"
#include "utils/dlopen.h"
#include "memload.h"

#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))

#if defined(__aarch64__)
#define LIB_ART_PATH "/system/lib64/libart.so"
#elif defined(__arm__)
#define LIB_ART_PATH "/system/lib/libart.so"
#else
#define LIB_ART_PATH "/system/lib/libart.so"
#endif

static jobject g_context;
static int g_sdk_version;
static const char *g_jiagu_path;
static int chooseAndroid8 = 0;

// vm dex
unsigned char VMDEX[400] = {
        0x64, 0x65, 0x78, 0x0A, 0x30, 0x33, 0x35, 0x00, 0xAD, 0x2A, 0x11, 0x26, 0x1C, 0x47, 0xDE, 0xC9,
        0x5F, 0x16, 0x03, 0x2F, 0xD2, 0xCC, 0x7A, 0xCB, 0x92, 0x13, 0xB3, 0x2A, 0xD0, 0x7A, 0x9B, 0xF5,
        0x90, 0x01, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x78, 0x56, 0x34, 0x12, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x14, 0x01, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00,
        0x03, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x8C, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x98, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0xA8, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x00, 0x00,
        0xE0, 0x00, 0x00, 0x00, 0xE8, 0x00, 0x00, 0x00, 0xF2, 0x00, 0x00, 0x00, 0x06, 0x01, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00,
        0x09, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x70, 0x10, 0x01, 0x00, 0x00, 0x00, 0x0E, 0x00,
        0x06, 0x3C, 0x69, 0x6E, 0x69, 0x74, 0x3E, 0x00, 0x08, 0x4C, 0x63, 0x6F, 0x6D, 0x2F, 0x56, 0x6D,
        0x3B, 0x00, 0x12, 0x4C, 0x6A, 0x61, 0x76, 0x61, 0x2F, 0x6C, 0x61, 0x6E, 0x67, 0x2F, 0x4F, 0x62,
        0x6A, 0x65, 0x63, 0x74, 0x3B, 0x00, 0x01, 0x56, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x81, 0x80,
        0x04, 0xC8, 0x01, 0x00, 0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x70, 0x00, 0x00, 0x00,
        0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00,
        0x01, 0x00, 0x00, 0x00, 0x8C, 0x00, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00,
        0x98, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xA8, 0x00, 0x00, 0x00,
        0x01, 0x20, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xC8, 0x00, 0x00, 0x00, 0x02, 0x20, 0x00, 0x00,
        0x04, 0x00, 0x00, 0x00, 0xE0, 0x00, 0x00, 0x00, 0x00, 0x20, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
        0x09, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x14, 0x01, 0x00, 0x00
};

static void write_vm_dex(const char *vmdex)
{
    if (access(vmdex, F_OK) == -1) {
        LOGD("write_vm_dex");
        FILE *file = fopen(vmdex, "wb");
        fwrite(VMDEX, 400, 1, file);
        fclose(file);
    }

}

static int byte2Int(char *b, int index) {
    return ((b[index] & 0xff) << 24) + ((b[index + 1] & 0xff) << 16) + ((b[index + 2] & 0xff) << 8) +(b[index + 3] & 0xff);
}

//-------------------------

static void init(JNIEnv *env, jobject cur_application)
{
    SetEnv(env);//全局变量赋值,g_envPtr = env;
    ndk_init(env);//创建一块区域,写入了机器码码..估计是为了后序的hook, 这是大于Android 8.0的情况

    jobject ctx = CallObjectMethod(cur_application, "getBaseContext", "()Landroid/content/Context;").l;//getBaseContext是ContextWrapper的方法
    g_context = env->NewGlobalRef(ctx);
    g_sdk_version = GetStaticField("android/os/Build$VERSION", "SDK_INT", "I").i;
}

static jbyteArray getDex(JNIEnv *env, jobject application)
{
    //获取apk安装路径
    jobject appInfo = CallObjectMethod(application, "getApplicationInfo", "()Landroid/content/pm/ApplicationInfo;").l;
    jstring sourceDir = static_cast<jstring>(GetField(appInfo, "sourceDir","Ljava/lang/String;").l);

    // 调用源文件的java层代码, 获取dex文件数据
    jbyteArray dexArray = static_cast<jbyteArray>(CallObjectMethod(application, "invoke1","(Ljava/lang/String;)[B",sourceDir).l);

    //不太明白的,还要手动删除ref??
    env->DeleteLocalRef(appInfo);
    env->DeleteLocalRef(sourceDir);

    return dexArray;
}

static jobject openmemory_load_dex(JNIEnv *env, void *art_handle, char *base, int dex_size)
{
    int zero = open("/dev/zero", PROT_WRITE);
    void *g_decrypt_base = mmap(0, dex_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, zero, 0);
    close(zero);
    if (g_decrypt_base == MAP_FAILED)
    {
        LOGE("[-]ANONYMOUS mmap failed:%s", strerror(errno));
        exit(-1);
    }
    memcpy(g_decrypt_base, base, dex_size);

    char dex_path[128];
    char odex_path[128];
    sprintf(dex_path, "%s/vm.dex", g_jiagu_path);
    sprintf(odex_path, "%s/vm.odex", g_jiagu_path);
    jstring dex = env->NewStringUTF(dex_path);
    jstring odex = env->NewStringUTF(odex_path);
    LOGD("replace %s", dex_path);
    jobject dexFile = CallStaticMethod("dalvik/system/DexFile", "loadDex",
                                       "(Ljava/lang/String;Ljava/lang/String;I)Ldalvik/system/DexFile;",
                                       dex, odex, 0).l;

    jfieldID cookie_field;
    jclass DexFileClass = env->FindClass("dalvik/system/DexFile");
    if (g_sdk_version < 23) {
        std::unique_ptr<std::vector<const void *>> dex_files(new std::vector<const void *>());
        dex_files.get()->push_back(load(g_sdk_version, art_handle, (char *)g_decrypt_base, (dex_size)));

        cookie_field = env->GetFieldID(DexFileClass, "mCookie", "J");
        jlong mCookie = static_cast<jlong>(reinterpret_cast<uintptr_t>(dex_files.release()));
        env->SetLongField(dexFile, cookie_field, mCookie);
    } else {
        std::vector<std::unique_ptr<const void *>> dex_files;
        dex_files.push_back(std::move(load23(art_handle, (char *)g_decrypt_base, (dex_size))));

        cookie_field = env->GetFieldID(DexFileClass, "mCookie", "Ljava/lang/Object;");
        jobject mCookie = env->GetObjectField(dexFile, cookie_field);

        jlongArray long_array = env->NewLongArray(1 + dex_files.size());
        jboolean is_long_data_copied;
        jlong* mix_element = env->GetLongArrayElements(long_array, &is_long_data_copied);

        mix_element[0] = NULL;
        for (size_t i = 0; i < dex_files.size(); ++i) {
            if (g_sdk_version == 23) {
                mix_element[i] = reinterpret_cast<uintptr_t>(dex_files[i].get());
            } else {
                mix_element[1 + i] = reinterpret_cast<uintptr_t>(dex_files[i].get());
            }
        }

        // 更新mCookie
        env->ReleaseLongArrayElements((jlongArray)mCookie, mix_element, 0);
        if (env->ExceptionCheck())
        {
            LOGE("[-]g_sdk_int Update cookie failed");
            return NULL;
        }

        for (auto& dex_file : dex_files) {
            dex_file.release();
        }

        env->SetObjectField(dexFile, cookie_field, mCookie);
    }

    env->DeleteLocalRef(dex);
    env->DeleteLocalRef(odex);

    return dexFile;
}

static void make_dex_elements(JNIEnv *env, jobject classLoader, std::vector<jobject> dexFileobjs)
{
    jclass PathClassLoader = env->GetObjectClass(classLoader);
    jclass BaseDexClassLoader = env->GetSuperclass(PathClassLoader);
    // get pathList fieldid
    jfieldID pathListid = env->GetFieldID(BaseDexClassLoader, "pathList", "Ldalvik/system/DexPathList;");
    jobject pathList = env->GetObjectField(classLoader, pathListid);

    // get DexPathList Class
    jclass DexPathListClass = env->GetObjectClass(pathList);
    // get dexElements fieldid
    jfieldID dexElementsid = env->GetFieldID(DexPathListClass, "dexElements", "[Ldalvik/system/DexPathList$Element;");
    jobjectArray dexElement = static_cast<jobjectArray>(env->GetObjectField(pathList, dexElementsid));

    jint len = env->GetArrayLength(dexElement);

    LOGD("[+]Elements size:%d, dex File size: %d", len, dexFileobjs.size());

    // Get dexElement all values and add  add each value to the new array
    jclass ElementClass = env->FindClass("dalvik/system/DexPathList$Element"); // dalvik/system/DexPathList$Element
    jobjectArray new_dexElement = env->NewObjectArray(len + dexFileobjs.size(), ElementClass, NULL);
    for (int i = 0; i < len; i++)
    {
        env->SetObjectArrayElement(new_dexElement, i, env->GetObjectArrayElement(dexElement, i));
    }


    for (int i = 0; i < dexFileobjs.size(); i++)
    {
        env->SetObjectArrayElement(new_dexElement, len + i, dexFileobjs[i]);
    }

    env->SetObjectField(pathList, dexElementsid, new_dexElement);

    env->DeleteLocalRef(ElementClass);
    env->DeleteLocalRef(dexElement);
    env->DeleteLocalRef(DexPathListClass);
    env->DeleteLocalRef(pathList);
    env->DeleteLocalRef(BaseDexClassLoader);
    env->DeleteLocalRef(PathClassLoader);
}

static void hook_application(jobject app, jstring name)
{
    CallObjectMethod(app,  "invoke2", "(Landroid/app/Application;Ljava/lang/String;)V", app, name);
}

//jobjectArray makePathElements(JNIEnv* env,const char *pathChs) {
//    jstring path = env->NewStringUTF(pathChs);
//    java_io_File file(env,path);
//
//    java_util_ArrayList files(env);
//    files.add(file.getInstance());
//    java_util_ArrayList suppressedExceptions(env);
//
//    clock_t cl = clock();
//    jobjectArray elements;
//    if(android_get_device_api_level() >= __ANDROID_API_M__) {
//        elements = dalvik_system_DexPathList::makePathElements(env,
//                                                               files.getInstance(),
//                                                               nullptr,
//                                                               suppressedExceptions.getInstance());
//    }
//    else {
//        elements = dalvik_system_DexPathList::makeDexElements(env,
//                                                              files.getInstance(),
//                                                              nullptr,
//                                                              suppressedExceptions.getInstance());
//    }
//    printTime("makePathElements success,took = ",cl);
//    return elements;
//}

/**
 * Dex加密格式
 * 壳dex + (1字节application名长度 + app的application名 + 4字节源dex大小 + 源dex) + 4字节源dex2大小 + [源dex2] + 4字节源dex3大小 + [源dex3] + ... + 4字节壳dex大小
 * 小括号的数据前512字节进行AES加密
 * 中括号的数据的dex头(也就是前112位进行异或)
 */
static void loadDex(JNIEnv *env, jobject cur_application)
{
    // 调用源文件的java层代码, 获取application的原始类名, 也就是读取文本文件, asset/app_name
    jstring appname_java= static_cast<jstring>(CallObjectMethod(
            cur_application,
            "getAppName",
            "(Landroid/content/Context;)Ljava/lang/String;", cur_application).l);

    char *app_name = const_cast<char *>(env->GetStringUTFChars(appname_java, nullptr));
    LOGD("app name: %s", app_name);


    // 调用源文件的java层代码, 读取目录asset/i11111i111.zip, 它是dex的压缩包
    jobjectArray dexList= static_cast<jobjectArray>(CallObjectMethod(
            cur_application,
            "readDex",
            "(Landroid/content/Context;)[[B", cur_application).l);
    jsize dex_cnt = env->GetArrayLength(dexList);//原始dex个数


    void *art_handle;
    // 加载dex
    jobject classLoader = CallObjectMethod(g_context, "getClassLoader", "()Ljava/lang/ClassLoader;").l;
    std::vector<jobject> dexobjs;
    std::vector<jobject> dexBuffers;


    for(int i=0; i < dex_cnt; i++)
    {
        jbyteArray innerArray = static_cast<jbyteArray>(env->GetObjectArrayElement(dexList, i));
        jbyte* dex_data = env->GetByteArrayElements(innerArray, nullptr);
        jsize innerLength = env->GetArrayLength(innerArray);

        //把每个dex的内容放进 vector dexBuffers
        jobject dexBuffer = env->NewDirectByteBuffer(reinterpret_cast<char *>(dex_data), innerLength);
        dexBuffers.push_back(dexBuffer);

    }

    jstring appname = env->NewStringUTF(app_name);

    jclass ElementClass = env->FindClass("java/nio/ByteBuffer");
    jobjectArray dexBufferArr = env->NewObjectArray(dexBuffers.size(), ElementClass, NULL);
    for (int i = 0; i < dexBuffers.size(); i++)
    {
        //转存dex内容? 放进 jobjectArray dexBufferArr
        env->SetObjectArrayElement(dexBufferArr, i, dexBuffers[i]);
    }

    jclass InMemoryDexClassLoaderClass = env->FindClass("dalvik/system/InMemoryDexClassLoader");
    jmethodID InMemoryDexClassLoaderInit = env->GetMethodID(
            InMemoryDexClassLoaderClass,
            "<init>",
            "([Ljava/nio/ByteBuffer;Ljava/lang/ClassLoader;)V");

    //调用构造函数 public InMemoryDexClassLoader (ByteBuffer[] dexBuffers, ClassLoader parent)
    // 这里已经完成了dex的自动加载....疑惑的是它并没有指定so的加载路径
    jobject mm_InMemoryDexClassLoader = env->NewObject(
            InMemoryDexClassLoaderClass,
            InMemoryDexClassLoaderInit,
            dexBufferArr,//成员得是ByteBuffer[],类似于byte[][]
            classLoader);//并没有用确定so的加载路径

//    jobject pathListObj = GetField(
//            mm_InMemoryDexClassLoader,
//            "pathList",
//            "Ldalvik/system/DexPathList;").l;
//    //在BaseDexClassLoader的构造函数中,会生成一个DexPathList

    jobjectArray dexElements;
    //替换DexPathList列表...

    jclass list_jcs = env->FindClass("java/util/ArrayList");
    jmethodID list_init = env->GetMethodID(list_jcs, "<init>", "()V");
    jobject list_obj = env->NewObject(list_jcs, list_init);
    dexElements = static_cast<jobjectArray>(CallStaticMethod(
            "dalvik/system/DexPathList",
            "makeInMemoryDexElements",
            "([Ljava/nio/ByteBuffer;Ljava/util/List;)[Ldalvik/system/DexPathList$Element;",
            dexBufferArr,
            list_obj).l);
    //public static Element[] makeInMemoryDexElements(..
    /*
     * makeInMemoryDexElements做的工作类似于 makeDexElements
     * 在DexPathList的初始化中,就会调用makeDexElements. 也可以这么说InMemoryDexClassLoaderInit的调用已经生成了一个一模一样的Elements[]
     * 所有这里又生成了一遍????大概DexPathList.dexElements[]是私有的,同时makeDexElements也是私有的.但是makeInMemoryDexElements是公开的
     * */


    for (int i = 0; i < env->GetArrayLength(dexElements); i++)
    {
        dexobjs.push_back(env->GetObjectArrayElement(dexElements, i));//感觉有点多此一举,dexElements本身就算数组了
    }

    env->DeleteLocalRef(ElementClass);
    env->DeleteLocalRef(InMemoryDexClassLoaderClass);
    env->DeleteLocalRef(mm_InMemoryDexClassLoader);
    //env->DeleteLocalRef(pathListObj);


    make_dex_elements(env, classLoader, dexobjs);//把自己的dexobjs添加到已经的dexElements里面
    hook_application(cur_application, appname);

   // free(decryptdex_buff);
    for (int i = 0; i < dexobjs.size(); i++)
    {
        env->DeleteLocalRef(dexobjs[i]);
    }
    for (int i = 0; i < dexBuffers.size(); i++)
    {
        env->DeleteLocalRef(dexBuffers[i]);
    }
    env->DeleteLocalRef(classLoader);
    env->DeleteLocalRef(appname);
    //env->ReleaseByteArrayElements(dexArray, dexData, 0);
}

void uninit(JNIEnv *env) {
    env->DeleteGlobalRef(g_context);
}


//void native_attach(JNIEnv *env, jclass clazz, jobject application);

//// Dalvik VM type signatures
//static const JNINativeMethod gMethods[] =
//{
//    {
//        "attach",
//        "(Lcom/frezrik/jiagu/StubApp;)V",
//        (void *)native_attach
//        },
//};


// ----------------------------------------------------------------------------
const char* const kClassPathName = "com/frezrik/jiagu/StubApp";
//int register_natives(JNIEnv *env)
//{
//    jclass javaClass = env->FindClass(kClassPathName);
//    if (javaClass == NULL)
//    {
//        return JNI_ERR;
//    }
//    return env->RegisterNatives(javaClass, gMethods, NELEM(gMethods));
//}

jint JNI_OnLoad(JavaVM* vm, void* reserved __unused)
{

    JNIEnv* env = NULL;
    jint result = -1;

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK)
    {
        LOGE("ERROR: GetEnv failed\n");
        goto bail;
    }

//    if (register_natives(env) < 0)
//    {
//        LOGE("ERROR: jiagu native registration failed\n");
//        goto bail;
//    }

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

    bail:
    return result;
}

//void native_attach(JNIEnv *env, jclass clazz, jobject application)
//{
//    init(env, application);
//
//    // 从/data/app/xxx/base.apk获取dex
//    LOGD("[-]getDex");
//    jbyteArray dexArray = getDex(env, application);
//
//    // 内存加载dex
//    LOGD("[-]loadDex");
//    loadDex(env, application, dexArray);
//
//    uninit(env);
//}


extern "C"
JNIEXPORT void JNICALL
Java_com_example_jiagu_StubApp_attach(JNIEnv *env, jclass clazz, jobject application)
{
    //JNIEnv *env, jclass clazz, jobject application
    // TODO: implement attach()
    init(env, application);

    // 从/data/app/xxx/base.apk获取dex
    LOGD("[-]getDex");
    //jbyteArray dexArray = getDex(env, application); //修改了dex加载的方式,从asset的i11111i111.zip读取dex

    // 内存加载dex
    LOGD("[-]loadDex");
    loadDex(env, application);

    uninit(env);
}