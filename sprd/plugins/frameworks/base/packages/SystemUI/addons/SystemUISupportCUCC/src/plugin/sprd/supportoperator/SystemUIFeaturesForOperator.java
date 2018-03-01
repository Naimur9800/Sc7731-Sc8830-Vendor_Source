package plugin.sprd.supportoperator;

import com.android.systemui.statusbar.policy.SystemUIPluginsHelper;
import android.app.AddonManager;
import android.content.Context;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.util.Log;
import android.view.View;
import android.widget.TextView;
import com.android.systemui.R;

public class SystemUIFeaturesForOperator extends SystemUIPluginsHelper implements AddonManager.InitialCallback {

    public static final String LOG_TAG = "SystemUIPluginForCUCC";
    private Context mAddonContext;

    public SystemUIFeaturesForOperator() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    /**
     * CUCC case:
     * 1.Can not show spn when plmn should be shown.
     * 2.Can not display something like "emergency call" no matter what the situation is.
     */
    public String updateNetworkName(Context context, boolean showSpn, String spn, boolean showPlmn,
            String plmn, int phoneId) {
        Log.d(LOG_TAG, "updateNetworkName showSpn=" + showSpn + " spn=" + spn
                + " showPlmn=" + showPlmn + " plmn=" + plmn);
        StringBuilder str = new StringBuilder();
        if (showPlmn && plmn != null) {
            if (context.getString(com.android.internal.R.string.emergency_calls_only).equals(plmn)) {
                plmn = context.getResources().getString(com.android.internal.R.string.lockscreen_carrier_default);
            }
            str.append(plmn);
        } else if (showSpn && spn != null) {
            str.append(spn);
        }

        return str.toString();
    }

    /**
     * CUCC case:
     * Operator names of multi-sim should be shown in different colors
     */
    public int getSubscriptionInfoColor(Context context, int subId) {
        int phoneId = SubscriptionManager.getSlotId(subId);
        return SubscriptionManager.isValidSlotId(phoneId)? COLORS[phoneId] : ABSENT_SIM_COLOR;
    }

    /**
     * CUCC case:
     * Signal strength icon should be shown in different colors
     */
    public int[][] getColorfulSignalStrengthIcons(int subId) {
        int phoneId = SubscriptionManager.getSlotId(subId);
        return phoneId == 0 ? TELEPHONY_SIGNAL_STRENGTH_COLOR_ONE
                : TELEPHONY_SIGNAL_STRENGTH_COLOR_TWO;
    }

    /**
     * CUCC case:
     * Show special no sim icon for each slot
     */
    public int getNoSimIconId() {
        return R.drawable.stat_sys_no_sim_sprd_cucc;
    }

    public int getNoServiceIconId() {
        return R.drawable.stat_sys_signal_null_sprd;
    }

    /**
     * CUCC case:
     * Show mobile card icon
     */
    public int getSimCardIconId(int subId) {
        int phoneId = SubscriptionManager.getSlotId(subId);
        return SubscriptionManager.isValidSlotId(phoneId)? SIM_CARD_ID[phoneId] : 0;
    }

    public int getSimStandbyIconId() {
        return R.drawable.stat_sys_signal_standby_sprd;
    }

}
