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

    // Dynamic Class Retransformation & Object Introspection
    public static native boolean retransformClass(long ctxHandle, String className, String classSource);
    public static native long getObjectSize(long objectHandle);
    public static native long getVariableSize(long ctxHandle, String varName);

    // Dynamic Compression (动态压缩技术)
    public static class CompressionStats {
        public long totalObjectsScanned;
        public long instancesCompressed;
        public long stringsDeduped;
        public long containersShrunk;
        public long originalBytes;
        public long compressedBytes;
        public long bytesSaved;
        public double compressionRatio;

        @Override
        public String toString() {
            return String.format("CompressionStats[scanned=%d, compressed=%d, saved=%d bytes (%.1f%%)]",
                totalObjectsScanned, instancesCompressed, bytesSaved, compressionRatio);
        }
    }

    public enum CompressionEventType {
        START(1),
        FINISH(2),
        OBJECT_COMPRESSED(3),
        OBJECT_DECOMPRESSED(4);

        public final int id;
        CompressionEventType(int id) { this.id = id; }
        public static CompressionEventType fromId(int id) {
            for (CompressionEventType t : values()) {
                if (t.id == id) return t;
            }
            return START;
        }
    }

    public static class CompressionEvent {
        public CompressionEventType type;
        public long instanceAddress;
        public String className;
        public long originalBytes;
        public long compressedBytes;
        public long bytesSaved;
        public long nonDefaultFields;
        public CompressionStats stats;

        @Override
        public String toString() {
            return String.format("CompressionEvent[type=%s, class=%s, saved=%d bytes]",
                type, className != null ? className : "Object", bytesSaved);
        }
    }

    public interface CompressionEventListener {
        void onCompressionEvent(CompressionEvent event);
    }

    public static native void setCompressionEventListener(CompressionEventListener listener);

    public static native CompressionStats dynamicCompress(long ctxHandle);
    public static native void setDynamicCompression(boolean enabled);
    public static native boolean isDynamicCompressionEnabled();

    // Hardware & High-Resolution Timing APIs
    public static class HardwareInfo {
        public int logicalCores;
        public int physicalCores;
        public String cpuBrand;
        public boolean hasAVX2;
        public boolean hasAVX512;
        public boolean hasFMA;
        public long totalMemoryBytes;
        public long availableMemoryBytes;
        public long processMemoryBytes;

        @Override
        public String toString() {
            return String.format("HardwareInfo[cpu='%s', cores=%d/%d, avx2=%b, mem=%.1f/%.1f GB]",
                cpuBrand != null ? cpuBrand.trim() : "Unknown", physicalCores, logicalCores, hasAVX2,
                processMemoryBytes / (1024.0 * 1024.0 * 1024.0),
                totalMemoryBytes / (1024.0 * 1024.0 * 1024.0));
        }
    }

    public static native long nanoTime();
    public static native HardwareInfo getHardwareInfo();

    // Error Message
    public static native String getLastError();
}
