package tech.smallwonder.libmediafy;

import android.graphics.SurfaceTexture;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.view.Surface;
import android.widget.TextView;

public class VideoRenderer implements SurfaceTexture.OnFrameAvailableListener {
    private SurfaceTexture texture;
    private boolean isAttached;
    private long id;

    private Surface surface;
    private TextView subtitleTextView;
    private final Object lock = new Object();

    public VideoRenderer(long id) {
        this.id = id;
        texture = new SurfaceTexture(-1);
        texture.detachFromGLContext();
        texture.setOnFrameAvailableListener(this);
        isAttached = false;

        surface = new Surface(texture);
    }

    private void setTextureID(int id) {
        resetTextureID();
        texture.attachToGLContext(id);
        isAttached = true;
        Log.i("Libmediafy", "Texture attached");
        texture.updateTexImage();
    }

    private void resetTextureID() {
        if (isAttached) texture.detachFromGLContext();
        isAttached = false;
        Log.i("Libmediafy", "Texture detached");
    }

    private void updateImage() {
        if (isAttached) {
            texture.updateTexImage();
        }
    }

    private Surface getSurface() { return surface; }

    private void setSubtitleTextView(TextView textView) {
        synchronized (lock) {
            subtitleTextView = textView;
        }
    }

    private void renderText(String text) {
        synchronized (lock) {
            if (subtitleTextView != null) {
                if (!subtitleTextView.getText().equals(text)) {
                    new Handler(Looper.getMainLooper()).post(new Runnable() {
                        @Override
                        public void run() {
                            subtitleTextView.setText(text);
                        }
                    });
                }
            }
        }
    }

    @Override
    public void onFrameAvailable(SurfaceTexture surfaceTexture) {
        nativeOnFrameAvailable(id);
    }

    private native void nativeOnFrameAvailable(long id);
}
