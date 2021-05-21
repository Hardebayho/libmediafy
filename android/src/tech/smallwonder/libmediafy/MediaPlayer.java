package tech.smallwonder.libmediafy;

import android.widget.TextView;

public class MediaPlayer {
    private long id;

    /**
     * Create a new media player instance
     */
    public MediaPlayer() {
        newPlayer();
    }

    private native long newPlayer();

    /**
     * Set data source for the media player to play from
     * @param url URL of the media to play
     * @return true if the media has been loaded and can be played and false otherwise
     */
    public native boolean setDataSource(String url);

    /**
     * Whether we currently have a playable media
     * @return true if there is media loaded and false otherwise
     */
    public native boolean hasMedia();

    /**
     * Play the current media
     */
    public native void play();

    /**
     * Pause the current media
     */
    public native void pause();

    /**
     * Stop the current media
     */
    public native void stop();

    /**
     * Whether the media player is playing some media
     * @return true if a media is currently playing and false otherwise
     */
    public native boolean isPlaying();

    /**
     * Returns the number of audio equalizer bands
     */
    public native int getEqualizerBands();

    /**
     * Enables the audio equalizer
     */
    public native void enableEqualizer();

    /**
     * Disables the audio equalizer
     */
    public native void disableEqualizer();

    /**
     * Whether the equalizer is enabled
     * @return true if the equalizer is enabled and false otherwise
     */
    public native boolean isEqualizerEnabled();

    /**
     * Get the frequency value at the specified index
     * @param index frequency index to get value from. This must be greater than 0 and greater than value returned by getEqualizerBands
     * @return the frequency value at the specified index (in decibels) and 0.0 if index specified is invalid
     */
    public native float getEqualizerFrequency(int index);

    /**
     * Set the frequency value at the specified index
     * @param index index of the frequency value to set. This must be within the range of -20.0 to +20.0
     */
    public native void setEqualizerFrequency(int index);

    /**
     * Returns the number of presets this equalizer has
     */
    public native int getNumEqualizerPresets();

    /**
     * Returns the name of the preset at the specified index. The index must be between 0 and getNumEqualizerPresets() - 1
     */
    public native String getEqualizerPresetName(int index);

    /**
     * Returns the equalizer frequency at the specified index
     */
    public native float getEqualizerPresetFrequency(int index);

    /**
     * Returns the current preamp value
     */
    public native float getPreamp();

    /**
     * Set the current preamp value. This must be between -20 and +20
     */
    public native void setPreamp(float value);

    /**
     * Set the equalizer to use the preset at the specified index
     */
    public native void setEqualizerPreset(int index);

    public native void setSurface(android.view.Surface surface);

    public native void setSubtitlesTextView(TextView textView);
}
