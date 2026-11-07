package com.tzd;

/**
 * Exception thrown when a TzdLanguage execution or runtime error occurs.
 */
public class TzdException extends RuntimeException {
    private final int errorCode;

    public TzdException(int errorCode, String message) {
        super(message + " (error code: " + errorCode + ")");
        this.errorCode = errorCode;
    }

    public TzdException(String message) {
        super(message);
        this.errorCode = -1;
    }

    public int getErrorCode() {
        return errorCode;
    }
}
