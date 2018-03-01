package com.sprd.fileexploreraddon.activities;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import android.widget.Toast;

import com.sprd.fileexplorer.activities.MultiSelectActivity;
import com.sprd.fileexplorer.drmplugin.MultiSelectActivityUtils;
import com.sprd.fileexploreraddon.R;
import com.sprd.fileexploreraddon.util.DRMFileUtil;

public class MultiSelectActivityUtilsAddon extends MultiSelectActivityUtils implements AddonManager.InitialCallback{

    private Context mAddonContext;

    public MultiSelectActivityUtilsAddon() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    @Override
    public boolean DRMFileShareClick(String filePath,MultiSelectActivity activity){
        if(DRMFileUtil.isDrmEnabled() && DRMFileUtil.isDrmFile(filePath)){
            Toast.makeText(activity, mAddonContext.getString(R.string.drm_invalid_share), Toast.LENGTH_SHORT).show();
            return true;
        }
        return false;
    }
}
