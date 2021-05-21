package tech.smallwonder.libmediafy;

import android.content.Context;
import android.graphics.Color;
import android.graphics.Typeface;
import android.util.AttributeSet;
import android.util.Log;
import android.view.Gravity;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.widget.RelativeLayout;
import android.widget.TextView;

import static android.view.ViewGroup.LayoutParams.MATCH_PARENT;
import static android.view.ViewGroup.LayoutParams.WRAP_CONTENT;

public class MediafyVideoView extends RelativeLayout {
    private SurfaceView videoSurface = null;
    private TextView subtitlesText = null;
    private MediaPlayer mediaPlayer = null;

    private volatile boolean surfaceCreated = false;

    public MediafyVideoView(Context context) {
        super(context);
    }

    public MediafyVideoView(Context context, AttributeSet attrs) {
        super(context, attrs);
        initializeViews(context);
    }

    public void setFont(Typeface typeface) {
        subtitlesText.setTypeface(typeface);
    }

    public void setTextColor(int color) {
        subtitlesText.setTextColor(color);
    }

    public void setTextSize(float sizeSp) {
        subtitlesText.setTextSize(sizeSp);
    }

    private void initializeViews(Context context) {
        setBackgroundColor(Color.BLACK);
        Log.d("MediafyVideoView", "Initializing views...");
        videoSurface = new SurfaceView(context);
        subtitlesText = new TextView(context);

        LayoutParams params = new LayoutParams(MATCH_PARENT, MATCH_PARENT);

        videoSurface.setLayoutParams(params);
        addView(videoSurface);

        params = new LayoutParams(MATCH_PARENT, WRAP_CONTENT);
        params.addRule(ALIGN_PARENT_BOTTOM);
        params.bottomMargin = 50;
        subtitlesText.setShadowLayer(2.0f, 1.0f, 1.0f, Color.BLACK);
        subtitlesText.setTextColor(Color.WHITE);
        subtitlesText.setLayoutParams(params);
        subtitlesText.setTextAlignment(TEXT_ALIGNMENT_CENTER);
        subtitlesText.setGravity(Gravity.CENTER_HORIZONTAL);
        addView(subtitlesText);

        videoSurface.getHolder().addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                Log.d("MediafyVideoView", "onSurfaceCreated!");
                surfaceCreated = true;
                if (mediaPlayer != null) {
                    videoSurface.setKeepScreenOn(true);
                    mediaPlayer.setSurface(holder.getSurface());
                    mediaPlayer.setSubtitlesTextView(subtitlesText);
                }
            }

            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {}

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                Log.d("MediafyVideoView", "onSurfaceDestroyed!");
                surfaceCreated = false;
                videoSurface.setKeepScreenOn(false);
                if (mediaPlayer != null) {
                    mediaPlayer.setSurface(null);
                    mediaPlayer.setSubtitlesTextView(null);
                }
            }
        });
    }

    public void setMediaPlayer(MediaPlayer mediaPlayer) {
        this.mediaPlayer = mediaPlayer;
        if (surfaceCreated) {
            videoSurface.setKeepScreenOn(true);
            SurfaceHolder.Callback c;
            videoSurface.getHolder().addCallback(new SurfaceHolder.Callback(){
                @Override
                public void surfaceChanged(SurfaceHolder arg0, int arg1, int arg2, int arg3) {

                }

                @Override
                public void surfaceCreated(SurfaceHolder arg0) {
                    
                }

                @Override
                public void surfaceDestroyed(SurfaceHolder arg0) {
                    
                }
            });
            mediaPlayer.setSurface(videoSurface.getHolder().getSurface());
        }
    }
}
