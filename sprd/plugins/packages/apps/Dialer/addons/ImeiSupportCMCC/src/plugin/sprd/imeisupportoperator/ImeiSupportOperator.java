package plugin.sprd.imeisupportoperator;

import android.telephony.TelephonyManager;
import android.util.Log;
import android.app.AddonManager;
import android.content.Context;
import com.android.dialer.DialerPluginsHelper;

public class ImeiSupportOperator extends DialerPluginsHelper implements
        AddonManager.InitialCallback {

    private static final String LOG_TAG = "ImeiSupportOperator";
    private Context mAddonContext;

    public ImeiSupportOperator() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public int displayIMEICount() {
        return 1;
    }
}
