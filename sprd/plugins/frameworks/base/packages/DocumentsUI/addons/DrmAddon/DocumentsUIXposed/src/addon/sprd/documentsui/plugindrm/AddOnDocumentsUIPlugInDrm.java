/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package addon.sprd.documentsui.plugindrm;

import android.content.Context;
import android.util.Log;
import android.widget.Toast;
import android.provider.MediaStore;
import android.net.Uri;
import android.content.Intent;
import android.app.Activity;
import android.os.SystemProperties;
import android.drm.DrmManagerClient;
import android.database.Cursor;
import com.android.documentsui.model.DocumentInfo;
import android.app.AddonManager;
import android.provider.DocumentsContract.Document;
import android.drm.DrmStore.RightsStatus;
import com.android.documentsui.PlugInDrm.DocumentsUIPlugInDrm;

public class AddOnDocumentsUIPlugInDrm extends DocumentsUIPlugInDrm implements AddonManager.InitialCallback {

    private Context mContext;
    private Context sContext;
    private boolean isDrmEnable = false;
    public final static int ERROR = 0;
    public final static int NORMAL_FILE = 0;
    public final static int DRM_SD_FILE = 1;
    public final static int DRM_OTHER_FILE = 2;
    public final static int ABNORMAL_FILE = -1;
    public static final String DRM_DCF_FILE = ".dcf";
    public String mDrmPath;
    public boolean mIsDrm;

    public AddOnDocumentsUIPlugInDrm(){
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        sContext = context;
        return clazz;
    }

    @Override
    public void getDocumentsActivityContext (Context context){
        mContext = context;
        return;
    }

    @Override
    public boolean alertDrmError(Activity activity , Uri uri){
        Log.i("AddOnDocumentsUIPlugInDrm", "DocumentActivity -- uris   "+uri);
        String path = DocumentUriUtil.getPath(activity, uri);
        Log.i("AddOnDocumentsUIPlugInDrm", "DocumentActivity -- path   "+path);
        if (DocumentsUiDrmHelper.isDrmMimeType(mContext, path, null,isDrmEnable)){
            boolean wallpaperExtra = activity.getIntent().hasExtra("applyForWallpaper");
            boolean outExtra = activity.getIntent().hasExtra(MediaStore.EXTRA_OUTPUT);
            if (wallpaperExtra || outExtra){
                Toast.makeText(sContext, R.string.drm_not_be_selected, Toast.LENGTH_SHORT).show();
                return false;
            }
            if (!DocumentsUiDrmHelper.isDrmSDFile(mContext, path, null)){
                Log.d("AddOnDocumentsUIPlugInDrm", "DocumentActivity -- isSDfile   ");
                Toast.makeText(sContext, R.string.drm_not_be_shared, Toast.LENGTH_SHORT).show();
                return false;
            }
        }
        return true;
    }

    @Override
    public boolean isDrmEnabled() {
        return isDrmEnable;
    }

    @Override
    public void getDrmEnabled() {
        String prop = SystemProperties.get("drm.service.enabled");
        isDrmEnable =  prop != null && prop.equals("true");
    }

    public String getDocMimeType(Context context, String path, String mimeType){
        String type = mimeType;
        boolean drm = DocumentsUiDrmHelper.isDrmMimeType(context, path, mimeType,isDrmEnable);
        if (drm){
            String originType = DocumentsUiDrmHelper.getOriginalMimeType(context, path, mimeType,isDrmEnable);
            type = originType;
        }
        return type;
    }

    @Override
    public boolean setDocMimeType(DocumentInfo doc){
        int fileType = getDrmCanSharedType(mContext, doc.derivedUri, doc.mimeType);
        if (fileType == DRM_SD_FILE){
            String path = DocumentUriUtil.getPath(mContext, doc.derivedUri);
            doc.mimeType = DocumentsUiDrmHelper.getOriginalMimeType(mContext, path, doc.mimeType,isDrmEnable);
        }else if (fileType == DRM_OTHER_FILE){
            Toast.makeText(sContext, R.string.drm_not_be_shared, Toast.LENGTH_SHORT).show();
            return false;
        }else if (fileType == ABNORMAL_FILE){
            Toast.makeText(sContext, R.string.error_in_shared, Toast.LENGTH_SHORT).show();
            return false;
        }
        return true;
    }

    private int getDrmCanSharedType(Context context, Uri uri, String mimetype){
        if (!DocumentsUiDrmHelper.isDrmMimeType(context, null, mimetype,isDrmEnable)){
            return NORMAL_FILE;
        }
        String path = DocumentUriUtil.getPath(context, uri);
        Log.i("AddOnDocumentsUIPlugInDrm", "isDrmCanShard -- uri -- "+uri);
        Log.i("AddOnDocumentsUIPlugInDrm", "isDrmCanShard -- path -- "+path);
        if (path != null) {
            if (DocumentsUiDrmHelper.isDrmCanTransfer(context, path, mimetype)){
                return DRM_SD_FILE;
            }else{
                return DRM_OTHER_FILE;
            }
        }
        return ABNORMAL_FILE;
    }

    public int getDrmIconMimeDrawableId(Context context, String type, String name) {
        String originType = null;
        originType = DocumentsUiDrmHelper.getOriginalMimeType(context, name, type,isDrmEnable);
        Log.i("AddOnDocumentsUIPlugInDrm", "getOriginalMimeType  result --  " + originType);
        int rightsStates = DocumentsUiDrmHelper.getRightsStatus(context, name, type);
        if (originType == null){
            return R.drawable.ic_doc_generic_am;
        }
        if (originType.startsWith("image/")) {
            if (rightsStates == RightsStatus.RIGHTS_VALID) {
                return R.drawable.drm_image_unlock;
            } else {
                return R.drawable.drm_image_lock;
            }

        } else if (originType.startsWith("audio/") || originType.equalsIgnoreCase("application/ogg")) {
            if (rightsStates == RightsStatus.RIGHTS_VALID) {
                return R.drawable.drm_audio_unlock;
            } else {
                return R.drawable.drm_audio_lock;
            }
        } else if (originType.startsWith("video/")) {
            if (rightsStates == RightsStatus.RIGHTS_VALID) {
                return R.drawable.drm_video_unlock;
            } else {
                return R.drawable.drm_video_lock;
            }
        } else {
            return R.drawable.ic_doc_generic_am;
        }
    }

    public String getDrmFilenameFromPath(String path){
        String drmName = path;
        if (path != null){
            int index = path.lastIndexOf("/");
            if (index > 0){
                drmName = path.substring(index+1);
            }
        }
        return drmName;
    }

    @Override
    public String getDrmPath(Context context , Uri uri){
        String mDrmPath;
        mDrmPath = DocumentUriUtil.getPath(context, uri);
        return mDrmPath;
    }

    @Override
    public boolean getIsDrm(Context context , String drmPath){
        boolean mIsDrm = false;
        if (drmPath != null){
            mIsDrm = DocumentsUiDrmHelper.isDrmMimeType(context, drmPath, null,isDrmEnable);
        }
        return mIsDrm;
    }

    @Override
    public int getIconImage(Context context , String docMimeType , String docDisplayName){
        if (isDrmEnable && docMimeType != null && docMimeType.equals(DocumentsUiDrmHelper.DRM_CONTENT_TYPE))
            return getDrmIconMimeDrawableId(context, docMimeType, docDisplayName);
        return ERROR;
    }
}
