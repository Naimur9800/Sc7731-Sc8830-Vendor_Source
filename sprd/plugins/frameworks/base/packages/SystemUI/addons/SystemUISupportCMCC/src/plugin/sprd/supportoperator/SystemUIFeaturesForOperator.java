
package plugin.sprd.supportoperator;

import com.android.systemui.statusbar.policy.SystemUIPluginsHelper;
import android.app.AddonManager;
import android.content.Context;
import android.telephony.SubscriptionManager;
import android.util.Log;
import com.android.systemui.R;

public class SystemUIFeaturesForOperator extends SystemUIPluginsHelper implements
        AddonManager.InitialCallback {

    public static final String LOG_TAG = "SystemUIPluginForCMCC";
    private Context mAddonContext;

    public SystemUIFeaturesForOperator() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    /**
     * CMCC case:
     * Can not show spn when plmn should be shown.
     */
    public String updateNetworkName(Context context, boolean showSpn, String spn, boolean showPlmn,
            String plmn, int phoneId) {
        Log.d(LOG_TAG, "updateNetworkName showSpn=" + showSpn + " spn=" + spn
                + " showPlmn=" + showPlmn + " plmn=" + plmn);
        StringBuilder str = new StringBuilder();
        if (showPlmn && plmn != null) {
            str.append(plmn);
        } else if (showSpn && spn != null) {
            str.append(spn);
        }
        return str.toString();
    }

    /**
     * CMCC case:
     * Operator names of different sims should be shown in different colors.
     */
    public int getSubscriptionInfoColor(Context context, int subId) {
        int phoneId = SubscriptionManager.getSlotId(subId);
        return SubscriptionManager.isValidSlotId(phoneId)? COLORS[phoneId] : ABSENT_SIM_COLOR;
    }

    public int[][] getColorfulSignalStrengthIcons(int subId) {
        int phoneId = SubscriptionManager.getSlotId(subId);
        return phoneId == 0 ? TELEPHONY_SIGNAL_STRENGTH_COLOR_ONE
                : TELEPHONY_SIGNAL_STRENGTH_COLOR_TWO;
    }

    public int getNoSimIconId() {
        return R.drawable.stat_sys_no_sim_sprd;
    }

    public int getNoServiceIconId() {
        return R.drawable.stat_sys_signal_null_sprd;
    }

    public int getSimStandbyIconId() {
        return R.drawable.stat_sys_signal_standby_sprd;
    }
}
