#include <jni.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <locale.h>
#include <atomic>
#include <vector>
#include <limits.h>

#include <mpv/client.h>

#include <pthread.h>

extern "C" {
    #include <libavcodec/jni.h>
}

#include "log.h"
#include "jni_utils.h"
#include "event.h"

#define ARRAYLEN(a) (sizeof(a)/sizeof(a[0]))

extern "C" {
    jni_func(void, create, jobject appctx, jstring log_lvl);
    jni_func(void, init);
    jni_func(void, destroy);

    jni_func(void, command, jobjectArray jarray);
};

JavaVM *g_vm;
mpv_handle *g_mpv;
std::atomic<bool> g_event_thread_request_exit(false);

static pthread_t event_thread_id;

// Unicode crosses JNI as UTF-16, never JNI's modified UTF-8. Keep this callback
// independent of the event observer lock; rendering may run on a decoder thread.
static int filter_subtitle_text(void *, const uint32_t *input, int length, uint32_t *output) {
    if (length < 0 || length > INT_MAX / 2)
        return -1;
    JNIEnv *env = nullptr;
    bool attached = g_vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) == JNI_EDETACHED;
    if (!acquire_jni_env(g_vm, &env))
        return -1;
    int count = -1;
    if (env->PushLocalFrame(4) == JNI_OK) {
        std::vector<jchar> text;
        text.reserve(length * 2);
        for (int i = 0; i < length; i++) {
            uint32_t cp = input[i];
            if (cp > 0xFFFF) {
                text.push_back(0xD800 + ((cp - 0x10000) >> 10));
                text.push_back(0xDC00 + ((cp - 0x10000) & 1023));
            } else {
                text.push_back(cp);
            }
        }
        jstring original = env->NewString(text.data(), text.size());
        jstring filtered = original ? static_cast<jstring>(env->CallStaticObjectMethod(
            mpv_MPVLib, mpv_MPVLib_filterSubtitleText, original)) : nullptr;
        if (!env->ExceptionCheck() && filtered) {
            int size = env->GetStringLength(filtered);
            const jchar *chars = env->GetStringChars(filtered, nullptr);
            if (chars) {
                count = 0;
                for (int i = 0; i < size; i++) {
                    uint32_t cp = chars[i];
                    if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < size &&
                        chars[i + 1] >= 0xDC00 && chars[i + 1] <= 0xDFFF) {
                        cp = 0x10000 + ((cp - 0xD800) << 10) + chars[++i] - 0xDC00;
                    }
                    if (count == length) { count = -1; break; }
                    output[count++] = cp;
                }
                env->ReleaseStringChars(filtered, chars);
            }
        }
        env->PopLocalFrame(nullptr);
    }
    if (env->ExceptionCheck()) {
        env->ExceptionClear();
        count = -1; // A filter failure must not break playback.
    }
    if (attached)
        g_vm->DetachCurrentThread();
    return count;
}

static void prepare_environment(JNIEnv *env, jobject appctx) {
    setlocale(LC_NUMERIC, "C");

    if (!env->GetJavaVM(&g_vm) && g_vm)
        av_jni_set_java_vm(g_vm, NULL);
    init_methods_cache(env);
}

jni_func(void, create, jobject appctx, jstring log_lvl) {
    prepare_environment(env, appctx);

    if (g_mpv) {
        die("mpv is already initialized");
        return;
    }

    g_mpv = mpv_create();
    if (!g_mpv) {
        die("context init failed");
        return;
    }

    mpv_set_subtitle_text_filter(g_mpv, filter_subtitle_text, nullptr);

    const char *log_lvl_string = env->GetStringUTFChars(log_lvl, 0);
    mpv_request_log_messages(g_mpv, log_lvl_string);
    env->ReleaseStringUTFChars(log_lvl, log_lvl_string);
}

jni_func(void, init) {
    if (!g_mpv) {
        die("mpv is not created");
        return;
    }

    if (mpv_initialize(g_mpv) < 0) {
        die("mpv init failed");
        return;
    }

#ifdef __aarch64__
    ALOGV("You're using the 64-bit build of mpv!");
#endif

    g_event_thread_request_exit = false;
    pthread_create(&event_thread_id, NULL, event_thread, NULL);
}

jni_func(void, destroy) {
    if (!g_mpv) {
        die("mpv destroy called but it's already destroyed");
        return;
    }

    // poke event thread and wait for it to exit
    g_event_thread_request_exit = true;
    mpv_wakeup(g_mpv);
    pthread_join(event_thread_id, NULL);

    mpv_terminate_destroy(g_mpv);
    g_mpv = NULL;
    jfieldID filter = env->GetStaticFieldID(mpv_MPVLib, "subtitleTextFilter", "Lis/xyz/mpv/MPVLib$SubtitleTextFilter;");
    if (filter)
        env->SetStaticObjectField(mpv_MPVLib, filter, nullptr);
}

jni_func(void, command, jobjectArray jarray) {
    const char *arguments[128] = { 0 };
    int len = env->GetArrayLength(jarray);
    if (!g_mpv) {
        die("Cannot run command: mpv is not initialized");
        return;
    }
    if (len >= ARRAYLEN(arguments)) {
        die("Cannot run command: too many arguments");
        return;
    }

    for (int i = 0; i < len; ++i)
        arguments[i] = env->GetStringUTFChars((jstring)env->GetObjectArrayElement(jarray, i), NULL);

    mpv_command(g_mpv, arguments);

    for (int i = 0; i < len; ++i)
        env->ReleaseStringUTFChars((jstring)env->GetObjectArrayElement(jarray, i), arguments[i]);
}
