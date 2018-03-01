
package com.sprd.drmgalleryplugin.gif;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import android.app.AddonManager;
import android.content.Context;
import android.drm.DrmManagerClient;
import android.drm.DecryptHandle;
import android.net.Uri;
import android.nfc.Tag;
import android.util.Log;
import com.sprd.drmgalleryplugin.R;
import com.sprd.drmgalleryplugin.util.DrmUtil;
import com.sprd.gallery3d.drm.GifDecoderUtils;

public class AddonGifDecoder extends GifDecoderUtils implements
        AddonManager.InitialCallback {
    private Context mAddonContext;
    private DrmManagerClient mClient = null;
    private DecryptHandle mDecryptHandle = null;
    private boolean mIsDrmValid = false;
    private String mFilePath = null;
    private static final String TAG = "AddonGalleryDrm";

    public AddonGifDecoder() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public void initDrm(Uri uri) {
        mFilePath = DrmUtil.getFilePathByUri(uri, mAddonContext);
        mIsDrmValid = DrmUtil.isDrmValid(mFilePath);
        if (mClient == null) {
            mClient = DrmUtil.getDrmManagerClient();
        }
        mDecryptHandle = mClient.openDecryptSession(mFilePath);
        Log.d(TAG, "AddonGifDecoder.initDrm---end+mFilePath=" + mFilePath);
    }

    @Override
    public boolean isReadDrmUri() {
        if (mIsDrmValid && mClient != null && mDecryptHandle != null) {
            return true;
        } else {
            return false;
        }
    }

    @Override
    public InputStream readDrmUri() {
        InputStream is = null;
        int fileSize = 0;
        FileInputStream fis = null;
        try {
            File file = new File(mFilePath);
            if (file.exists()) {
                fis = new FileInputStream(file);
                fileSize = fis.available();
            }
        } catch (Exception e) {
            Log.d(TAG, "readDrmUri.file error");
            e.printStackTrace();
        } finally {
            try {
                if (fis != null)
                    fis.close();
            } catch (IOException e) {
                Log.d(TAG, "readDrmUri.file close error");
                e.printStackTrace();
            }
        }
        byte[] ret = mClient.pread(mDecryptHandle, fileSize, 0);
        Log.d(TAG, "Drm loadGif pread ret = " + ret);
        is = new ByteArrayInputStream(ret);
        return is;
    }
}
