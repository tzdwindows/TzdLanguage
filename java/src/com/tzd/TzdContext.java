package com.tzd;

/**
 * Idiomatic Java AutoCloseable Script Context for TzdLanguage.
 *
 * Example usage:
 * <pre>{@code
 * try (TzdContext ctx = new TzdContext()) {
 *     ctx.eval("fun square(x) { return x * x; }");
 *     String result = ctx.call("square", "12");
 *     System.out.println("Result: " + result);
 * }
 * }</pre>
 */
public class TzdContext implements AutoCloseable {
    private long handle;

    public TzdContext() {
        this.handle = TzdLanguage.createContext();
        if (this.handle == 0) {
            throw new TzdException("Failed to allocate native Tzd context");
        }
    }

    public long getNativeHandle() {
        checkValid();
        return handle;
    }

    public void addIncludePath(String path) {
        checkValid();
        TzdLanguage.contextAddIncludePath(handle, path);
    }

    public void reset() {
        checkValid();
        TzdLanguage.resetContext(handle);
    }

    public String eval(String code) {
        checkValid();
        return TzdLanguage.eval(handle, code);
    }

    public void loadFile(String filepath) {
        checkValid();
        TzdLanguage.loadFile(handle, filepath);
    }

    public String call(String funcName, String... args) {
        checkValid();
        return TzdLanguage.callFunction(handle, funcName, args);
    }

    public void setDouble(String name, double val) {
        checkValid();
        TzdLanguage.setDouble(handle, name, val);
    }

    public double getDouble(String name) {
        checkValid();
        return TzdLanguage.getDouble(handle, name);
    }

    public void setLong(String name, long val) {
        checkValid();
        TzdLanguage.setLong(handle, name, val);
    }

    public long getLong(String name) {
        checkValid();
        return TzdLanguage.getLong(handle, name);
    }

    public void setString(String name, String val) {
        checkValid();
        TzdLanguage.setString(handle, name, val);
    }

    public String getString(String name) {
        checkValid();
        return TzdLanguage.getString(handle, name);
    }

    public boolean has(String name) {
        checkValid();
        return TzdLanguage.hasGlobal(handle, name);
    }

    public boolean retransformClass(String className, String classSource) {
        checkValid();
        return TzdLanguage.retransformClass(handle, className, classSource);
    }

    public long getObjectSize(long objectHandle) {
        checkValid();
        return TzdLanguage.getObjectSize(objectHandle);
    }

    public long getVariableSize(String varName) {
        checkValid();
        return TzdLanguage.getVariableSize(handle, varName);
    }

    public TzdLanguage.CompressionStats dynamicCompress() {
        checkValid();
        return TzdLanguage.dynamicCompress(handle);
    }

    public void setDynamicCompression(boolean enabled) {
        TzdLanguage.setDynamicCompression(enabled);
    }

    public boolean isDynamicCompressionEnabled() {
        return TzdLanguage.isDynamicCompressionEnabled();
    }

    @Override
    public synchronized void close() {
        if (handle != 0) {
            TzdLanguage.destroyContext(handle);
            handle = 0;
        }
    }

    private void checkValid() {
        if (handle == 0) {
            throw new IllegalStateException("TzdContext has already been closed");
        }
    }
}
