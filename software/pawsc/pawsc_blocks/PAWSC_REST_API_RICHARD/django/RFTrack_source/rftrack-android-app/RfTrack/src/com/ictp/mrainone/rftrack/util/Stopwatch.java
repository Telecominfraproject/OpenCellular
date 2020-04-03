// Class for measuring elapsed time in Android
// code from:
// https://gist.github.com/kristopherjohnson/6009597
//
// Example usage:
// Stopwatch stopwatch = new Stopwatch();
// doSomethingThatTakesALongTime();
// Log.d(LOGTAG, "elapsed time: " + stopwatch.getElapsedTimeString());
// stopwatch.reset();
//
// http://developer.android.com/reference/android/os/SystemClock.html
// Three different clocks are available, and they should not be confused:
// System.currentTimeMillis() is the standard "wall" clock (time and date) expressing milliseconds since the epoch. 
// The wall clock can be set by the user or the phone network (see setCurrentTimeMillis(long)), 
// so the time may jump backwards or forwards unpredictably. 
// This clock should only be used when correspondence with real-world dates and times is important, 
// such as in a calendar or alarm clock application. 
// Interval or elapsed time measurements should use a different clock. 
// uptimeMillis() is counted in milliseconds since the system was booted. 
// This clock stops when the system enters deep sleep (CPU off, display dark, device waiting for external input), 
// but is not affected by clock scaling, idle, or other power saving mechanisms. 
// This is the basis for most interval timing such as Thread.sleep(millls), Object.wait(millis), and System.nanoTime(). 
// This clock is guaranteed to be monotonic, and is suitable for interval timing when the interval does not span device sleep. 
// Most methods that accept a timestamp value currently expect the uptimeMillis() clock.
// elapsedRealtime() and elapsedRealtimeNanos() return the time since the system was booted, and include deep sleep. 
// This clock is guaranteed to be monotonic, and continues to tick even when the CPU is in power saving modes, 
// so is the recommend basis for general purpose interval timing.

package com.ictp.mrainone.rftrack.util;

import android.os.SystemClock;

/**
 * Measures elapsed time in milliseconds
 */
public class Stopwatch {

    private long startThreadMillis;
    private long startRealtimeMillis;
    private long startUptimeMillis;

    /**
     * Result of Stopwatch.getElapsedTime()
     */
    public static class ElapsedTime {
        private final long elapsedThreadMillis;
        private final long elapsedRealtimeMillis;
        private final long elapsedUptimeMillis;

        /**
         * Constructor
         *
         * @param stopwatch
         *            instance from which to calculate elapsed time
         */
        public ElapsedTime(Stopwatch stopwatch) {
            elapsedThreadMillis = SystemClock.currentThreadTimeMillis() - stopwatch.startThreadMillis;
            elapsedRealtimeMillis = SystemClock.elapsedRealtime() - stopwatch.startRealtimeMillis;
            elapsedUptimeMillis = SystemClock.uptimeMillis() - stopwatch.startUptimeMillis;
        }

        /**
         * Get milliseconds running in current thread
         *
         * This result is only valid if Stopwatch.getElapsedTime() is called from the same
         * thread as the Stopwatch constructor, or the last call to Stopwatch.reset().
         *
         * @return milliseconds
         */
        public long getElapsedThreadMillis() {
            return elapsedThreadMillis;
        }

        /**
         * Get elapsed milliseconds, including time spent in sleep
         *
         * @return milliseconds
         */
        public long getElapsedRealtimeMillis() {
            return elapsedRealtimeMillis;
        }

        /**
         * Get elapsed milliseconds, not counting time spent in deep sleep
         *
         * @return milliseconds
         */
        public long getElapsedUptimeMillis() {
            return elapsedUptimeMillis;
        }
        
        @Override
        public String toString() {
            return "realtime: " + elapsedRealtimeMillis 
                    + " ms; uptime: " + elapsedUptimeMillis
                    + " ms; thread: " + elapsedThreadMillis + " ms";
        }
    }

    /**
     * Constructor
     */
    public Stopwatch() {
        reset();
    }

    /**
     * Set stopwatch's start time to the current time
     */
    public void reset() {
        startThreadMillis = SystemClock.currentThreadTimeMillis();
        startRealtimeMillis = SystemClock.elapsedRealtime();
        startUptimeMillis = SystemClock.uptimeMillis();
    }

    /**
     * Get elapsed time since construction or last call to reset()
     *
     * @return Stopwatch.ElapsedTime
     */
    public ElapsedTime getElapsedTime() {
        return new ElapsedTime(this);
    }

    /**
     * Get elapsed time as a human-readable string
     *
     * If time is less than one second, it will be rendered as a number of milliseconds.
     * Otherwise, it will be rendered as a number of seconds.
     *
     * @return String
     */
    public String getElapsedTimeString() {
        double seconds = (double)getElapsedTime().getElapsedRealtimeMillis() / 1000.0;
        if (seconds < 1.0) {
            return String.format("%.0f ms", seconds * 1000);
        }
        else {
            return String.format("%.2f s", seconds);
        }
    }

    @Override
    public String toString() {
        return "Stopwatch: " + getElapsedTimeString();
    }
}
