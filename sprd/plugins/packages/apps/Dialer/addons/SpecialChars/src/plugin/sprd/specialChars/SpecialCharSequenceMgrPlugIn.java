package plugin.sprd.specialChars;

import android.util.Log;
import android.app.AddonManager;
import android.content.Context;
import android.content.Intent;

import com.sprd.dialer.plugins.SpecialCharsPluginsHelper;

public class SpecialCharSequenceMgrPlugIn extends SpecialCharsPluginsHelper implements
        AddonManager.InitialCallback {

    private static final String TAG = "SpecialCharSequenceMgrPlugIn";

    private static final String DM_SETTING = "#*4560#";
    private static final String CMCCTEST_AGPS_CONFIG = "*#612345#";
    private static final String CMCCTEST_AGPS_LOG_SHOW = "*#812345#";
    private Context mAddonContext;

    public SpecialCharSequenceMgrPlugIn() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public boolean handleChars(Context context, String dialString) {
        /* SPRD: add for DM & CMCC test setting @{ */
        return handleDmCode(context, dialString)
                || handleAgpsCfg(context, dialString)
                || handleAgpsLogShow(context, dialString);
    }

    /**
     * SPRD: add for DM & CMCC test setting
     *
     * @{
     */
    private boolean handleDmCode(Context context, String input) {
        if (input.equals(DM_SETTING))
        {
            Intent intent = new Intent(Intent.ACTION_VIEW);
            intent.setClassName("com.spreadtrum.dm", "com.spreadtrum.dm.DmDebugMenu");
            if (context.getPackageManager().resolveActivity(intent, 0) == null) {
                Log.w("dm code", "com.spreadtrum.dm is not exist");
                return false;
            }
            else {
                context.startActivity(intent);
                return true;
            }
        }

        return false;
    }

    private boolean handleAgpsCfg(Context context, String input) {
        if (input.equals(CMCCTEST_AGPS_CONFIG)) {
            try {
                Intent i = new Intent();
                i.setAction("android.intent.action.MAIN");
                i.setClassName("com.android.settings", "com.sprd.settings.LocationGpsConfig");
                i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startActivity(i);
            } catch (Exception ex) {
                Log.e(TAG, ex.toString());
            }
            return true;
        }
        return false;
    }

    private boolean handleAgpsLogShow(Context context, String input) {
        if (input.equals(CMCCTEST_AGPS_LOG_SHOW)) {
            try {
                Intent i = new Intent();
                i.setAction("android.intent.action.MAIN");
                i.setClassName("com.android.settings", "com.sprd.settings.LocationAgpsLogShow");
                i.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startActivity(i);
            } catch (Exception ex) {
                Log.e(TAG, ex.toString());
            }
            return true;
        }
        return false;
    }
}
