package com.tzd;

/**
 * Low-level JNI interface to the TzdLanguage runtime library (tzd.dll).
 */
public final class TzdLanguage {
    private static volatile boolean loaded = false;

    static {
        loadNativeLibrary();
    }

    public static synchronized void loadNativeLibrary() {
        if (!loaded) {
            try {
                System.loadLibrary("tzd");
                loaded = true;
            } catch (UnsatisfiedLinkError e) {
                // Try current working directory or x64/Release fallback
                String currentDir = System.getProperty("user.dir");
                String[] paths = {
                    currentDir + "\\tzd.dll",
                    currentDir + "\\x64\\Release\\tzd.dll",
                    currentDir + "\\..\\x64\\Release\\tzd.dll"
                };
                boolean found = false;
                for (String p : paths) {
                    try {
                        System.load(new java.io.File(p).getAbsolutePath());
                        loaded = true;
                        found = true;
                        break;
                    } catch (Throwable ignored) {}
                }
                if (!found) {
                    throw e;
                }
            }
        }
    }

    private TzdLanguage() {}

    // Global Lifecycle & Info
    public static native String getVersion();
    public static native int init();
    public static native void shutdown();

    // Global Config
    public static native void setJitEnabled(boolean enable);
    public static native boolean isJitEnabled();
    public static native void setOptLevel(int level);
    public static native int getOptLevel();
    public static native void setSilentMode(boolean silent);
    public static native void addGlobalIncludePath(String path);

    // One-shot Execution
    public static native int runFile(String filepath);
    public static native int runString(String code);

    // Context Lifecycle
    public static native long createContext();
    public static native void destroyContext(long ctxHandle);
    public static native void resetContext(long ctxHandle);
    public static native void contextAddIncludePath(long ctxHandle, String path);

    // Context Execution & Variable Access
    public static native String eval(long ctxHandle, String code);
    public static native void loadFile(long ctxHandle, String filepath);
    public static native String callFunction(long ctxHandle, String funcName, String[] args);

    public static native void setDouble(long ctxHandle, String name, double value);
    public static native double getDouble(long ctxHandle, String name);
    public static native void setLong(long ctxHandle, String name, long value);
    public static native long getLong(long ctxHandle, String name);
    public static native void setString(long ctxHandle, String name, String value);
    public static native String getString(long ctxHandle, String name);
    public static native boolean hasGlobal(long ctxHandle, String name);

    // Error Message
    public static native String getLastError();
}
