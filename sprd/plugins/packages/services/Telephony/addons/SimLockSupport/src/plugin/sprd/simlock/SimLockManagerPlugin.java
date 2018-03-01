package plugin.sprd.simlock;

import com.sprd.phone.SimLockManager;
import android.app.AddonManager;
import android.content.Context;
import android.os.AsyncResult;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.telephony.TelephonyManager;
import android.telephony.TelephonyManagerSprd;
import com.android.internal.telephony.uicc.IccCardProxySprd;
import com.android.phone.PhoneGlobals;
import com.android.internal.telephony.IccCard;
import com.android.internal.telephony.Phone;
import java.util.ArrayList;
import android.app.Notification;
import android.app.NotificationManager;
import android.widget.Toast;
import android.provider.Settings.System;
import com.android.internal.telephony.gsm.GSMPhoneSprd;
import com.android.internal.telephony.PhoneSprd;
import com.android.internal.telephony.PhoneFactorySprd;
import android.content.Intent;
import plugin.sprd.simlock.IccSimLockDepersonalizationPanelSprd;



public class SimLockManagerPlugin extends SimLockManager implements AddonManager.InitialCallback {

    private static final String TAG = "SimLockManagerPlugin";
    private Context mAddonContext;

    public static final String IS_SIMLOCK = "is_sim_locked";
    protected static final int EVENT_SIM_SIM_LOCKED = 2;
    protected static final int EVENT_SIM_NETWORK_LOCKED = 3;
    protected static final int EVENT_SIM_NETWORK_SUBSET_LOCKED = 4;
    protected static final int EVENT_SIM_SERVICE_PROVIDER_LOCKED = 5;
    protected static final int EVENT_SIM_CORPORATE_LOCKED = 6;
    protected static final int EVENT_SIM_SIM_LOCKED_FOREVER = 7;
    private int sim_network_locked_count = 0;
    private int sim_network_subset_locked_count = 0;
    private int sim_service_provider_locked_count = 0;
    private int sim_corporate_locked_count = 0;
    private int sim_sim_locked_count = 0;
    private int sim_sim_locked_forver_count = 0;
    public boolean isPinOrPuk = false;
    public ArrayList<IccPanelSprd> mPanelArr = new ArrayList<IccPanelSprd>();

    private static final String ACTION_EMERGENCY_DIAL_START = "com.android.phone.emergency_dial_start_intent";
    private static final String ACTION_EMERGENCY_DIAL_STOP = "com.android.phone.emergency_dial_stop_intent";

    public SimLockManagerPlugin() {

    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        mAddonContext = context;
        return clazz;
    }

    public void registerForSimLocked(Context context, Handler handler) {
        Log.d(TAG, "SimLockManagerPlugin registerForSimLocked");
        TelephonyManagerSprd telephonyManager = (TelephonyManagerSprd) TelephonyManager.from(context);
        int phoneCount = telephonyManager.getPhoneCount();
        for (int i = 0; i < phoneCount; i++) {
            Phone phone = PhoneFactorySprd.getPhone(i);//PhoneGlobals/*Sprd*/.getInstance().getPhone(i);
            IccCard sim = phone.getIccCard();
            if (sim != null) {
                Log.v(TAG, "register for ICC status");
                sim.registerForNetworkLocked(handler, EVENT_SIM_NETWORK_LOCKED, Integer.valueOf(i));
                ((IccCardProxySprd)sim).registerForNetworkSubsetLocked(handler, EVENT_SIM_NETWORK_SUBSET_LOCKED, Integer.valueOf(i));
                ((IccCardProxySprd)sim).registerForServiceProviderLocked(handler, EVENT_SIM_SERVICE_PROVIDER_LOCKED, Integer.valueOf(i));
                ((IccCardProxySprd)sim).registerForCorporateLocked(handler, EVENT_SIM_CORPORATE_LOCKED, Integer.valueOf(i));
                ((IccCardProxySprd)sim).registerForSimLocked(handler, EVENT_SIM_SIM_LOCKED, Integer.valueOf(i));
                ((IccCardProxySprd)sim).registerForSimLockedForever(handler, EVENT_SIM_SIM_LOCKED_FOREVER, Integer.valueOf(i));
            }
        }
	}

    public void processSimStateChanged(Context context) {
        final TelephonyManagerSprd tm = (TelephonyManagerSprd)context.getSystemService(Context.TELEPHONY_SERVICE);
        int[] simLockstate;
        boolean[] isSimLocked;
        int phoneCount = TelephonyManagerSprd.getDefault().getPhoneCount();
        simLockstate = new int[phoneCount];
        isSimLocked = new boolean[phoneCount];

        for (int i = 0; i < phoneCount; i++) {
            simLockstate[i] = tm.getSimState(i);
            switch (simLockstate[i]) {
                case TelephonyManager.SIM_STATE_PIN_REQUIRED :
                case TelephonyManager.SIM_STATE_PUK_REQUIRED :
                    isSimLocked[i] = true;
                    break;
                default:
                    isSimLocked[i] = false;
                    break;
            }
            if(isSimLocked[i]){
                isPinOrPuk = true;
            }
        }
        if (!isPinOrPuk) {
            return;
        } else {
            isPinOrPuk = false;
            if (mPanelArr != null) {
                for (IccPanelSprd panel : mPanelArr) {
                    panel.show();
                    mPanelArr.remove(panel);
                }
            }
        }
    }

    public void showPanel(Context context, Message msg) {
        int event_type = msg.what;
        if ((event_type != EVENT_SIM_NETWORK_LOCKED)
                && (event_type != EVENT_SIM_NETWORK_SUBSET_LOCKED)
                && (event_type != EVENT_SIM_SIM_LOCKED)
                && (event_type != EVENT_SIM_SERVICE_PROVIDER_LOCKED)
                && (event_type != EVENT_SIM_CORPORATE_LOCKED)
                && (event_type != EVENT_SIM_SIM_LOCKED_FOREVER)) {
            Log.d(TAG, "event_type -> " + event_type + " is not sim lock event");
            return;
        }

        int type = 0;
        switch (event_type) {
        case EVENT_SIM_NETWORK_LOCKED:
            type = TelephonyManagerSprd.UNLOCK_NETWORK;
            if (sim_network_locked_count++ > 0) {
                return;
            }
            break;
        case EVENT_SIM_NETWORK_SUBSET_LOCKED:
            type = TelephonyManagerSprd.UNLOCK_NETWORK_SUBSET;
            if (sim_network_subset_locked_count++ > 0) {
                return;
            }
            break;
        case EVENT_SIM_SERVICE_PROVIDER_LOCKED:
            type = TelephonyManagerSprd.UNLOCK_SERVICE_PORIVDER;
            if (sim_service_provider_locked_count++ > 0) {
                return;
            }
            break;
        case EVENT_SIM_CORPORATE_LOCKED:
            type = TelephonyManagerSprd.UNLOCK_CORPORATE;
            if (sim_corporate_locked_count++ > 0) {
                return;
            }
            break;
        case EVENT_SIM_SIM_LOCKED:
            type = TelephonyManagerSprd.UNLOCK_SIM;
            if (sim_sim_locked_count++ > 0) {
                return;
            }
            break;
        case EVENT_SIM_SIM_LOCKED_FOREVER:
            if( sim_sim_locked_forver_count++ == 0) {
                showNotification();
            }
            return;
        }

        if (mAddonContext.getResources().getBoolean(
                R.bool.ignore_sim_network_locked_events)) {
            // Some products don't have the concept of a "SIM network lock"
            Log.i(TAG, "Ignoring type: " + type);
        } else {
            AsyncResult r = (AsyncResult) msg.obj;
            int phoneId = 0;
            if (r.userObj instanceof Integer) {
                Integer integer = (Integer) r.userObj;
                phoneId = integer.intValue();
            }
            Log.i(TAG, "show sim depersonal panel type: " + type);
            Log.i(TAG, "phoneId: " + phoneId);
            // Phone/*Sprd kenny*/ phone = PhoneGlobals/*Sprd
            // kenny*/.getInstance().getPhone/*Sprd kenny*/(phoneId);
            PhoneSprd phone = PhoneFactorySprd.getGsmPhone(phoneId);
            // Normal case: show the "SIM network unlock" PIN entry screen.
            // The user won't be able to do anything else until
            // they enter a valid SIM network PIN.
            int remainCount = phone.getSimLockRemainTimes(type);
            boolean isLocked = System.getInt(context.getContentResolver(),
                    IS_SIMLOCK, 0) == 0 ? false : true;
            if (remainCount <= 0 || isLocked) {
                Toast.makeText(context,
                        mAddonContext.getString(R.string.phone_locked),
                        Toast.LENGTH_LONG).show();
                Log.i(TAG, "remainCount = 0");
                return;
            }
            Log.i(TAG, "remainCount = " + remainCount);
            IccPanelSprd ndpPanel = null;
            ndpPanel = new IccSimLockDepersonalizationPanelSprd(mAddonContext, phone,
                    remainCount, type);
            if (isPinOrPuk) {
                mPanelArr.add(ndpPanel);
                return;
            }
            if (ndpPanel != null) {
                ndpPanel.show();
            }
        }
    }

    public void sendEMDialStart(Context context) {
        context.sendBroadcast(new Intent(ACTION_EMERGENCY_DIAL_START));
    }

    public void sendEMDialStop(Context context) {
        // TelephonyManager tm = (TelephonyManager)
        // context.getSystemService(Context.TELEPHONY_SERVICE);
        TelephonyManager tm = (TelephonyManager) TelephonyManager.from(context);
        if (tm.getCallState() == TelephonyManager.CALL_STATE_IDLE) {
            context.sendBroadcast(new Intent(ACTION_EMERGENCY_DIAL_STOP));
        }
    }
    
    private void showNotification() {
        Log.i(TAG, "showNotification");
        Notification.Builder builder = new Notification.Builder(mAddonContext)
         .setSmallIcon(R.drawable.lock_permanently)
                .setContentTitle(mAddonContext.getString(R.string.sim_lock_forever_title)).
                setContentText(mAddonContext.getString(R.string.sim_lock_forever_body));
        Notification notification = builder.build();
        notification.flags |= Notification.FLAG_NO_CLEAR;
        NotificationManager notificationManager = (NotificationManager) mAddonContext
                .getSystemService(Context.NOTIFICATION_SERVICE);
        notificationManager.notify(2, notification);
    }
}
