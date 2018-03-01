/** SPRD： Created by Spreadst */
package plugin.sprd.simlock;

import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneFactorySprd;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.AsyncResult;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.provider.Settings;
import android.telephony.PhoneStateListener;
import android.telephony.TelephonyManager;
import android.telephony.TelephonyManagerSprd;
import android.text.Editable;
import android.text.InputFilter;
import android.text.Spannable;
import android.text.Spanned;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.text.method.DialerKeyListener;
import android.util.Log;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.InputMethodManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.android.internal.telephony.PhoneSprd;
import com.android.internal.telephony.policy.RadioInteraction;
import com.android.internal.telephony.uicc.IccCardProxySprd;
import com.android.phone.SpecialCharSequenceMgr;
import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.PhoneConstants;
import android.telephony.SubscriptionManager;

public class IccSimLockDepersonalizationPanelSprd extends IccPanelSprd{
    private static final boolean DBG = true;
    protected static final int EVENT_ICC_NTWRK_DEPERSONALIZATION_RESULT = 100;
    protected static final int EVENT_ICC_NETWORK_SUBSET_DEPERSONALIZATION_RESULT = 101;
    protected static final int EVENT_ICC_SERVICE_PROVIDER_DEPERSONALIZATION_RESULT = 102;
    protected static final int EVENT_ICC_CORPORATE_DEPERSONALIZATION_RESULT = 103;
    protected static final int EVENT_ICC_SIM_DEPERSONALIZATION_RESULT = 104;

    protected static final String ACTION_EMERGENCY_DIAL_START = "com.android.phone.emergency_dial_start_intent";
    protected static final String ACTION_EMERGENCY_DIAL_STOP = "com.android.phone.emergency_dial_stop_intent";
    protected static final String ACTION_POWER = "com.android.phone.power_intent";
    protected static final String ACTION_EMERGENCY_DIAL = "com.android.phone.EmergencyDialer.DIAL";

    protected static final int PWDMINLENGTH = 8;
    protected static final int PWDMAXLENGTH = 16;
    protected static final int PWDSIMMINLENGTH = 6;

    protected Context mContext;
    protected int mCount;

    private int mType;

    private Phone mPhone;

    //UI elements
    private EditText     mPinEntry;
    private LinearLayout mEntryPanel;
    private LinearLayout mStatusPanel;
    private TextView     mStatusText;
    private TextView     mLabel;
    private Button       mUnlockButton;
    private Button       mDismissButton;
    private TextView     mRetrialCount;
    private Button       mEmergencyButton;

    protected boolean      mPowerHide = false;
    protected boolean      mPhoneHide = false;
    protected boolean      mDismissed = false;

    protected TelephonyManager mTelephonyManager;
    protected IntentFilter mFilter;

    public static final String IS_SIMLOCK = "is_sim_locked";

    //private textwatcher to control text entry.
    private TextWatcher mPinEntryWatcher = new TextWatcher() {
        public void beforeTextChanged(CharSequence buffer, int start, int olen, int nlen) {
        }

        public void onTextChanged(CharSequence buffer, int start, int olen, int nlen) {
        }

        public void afterTextChanged(Editable buffer) {
            if (SpecialCharSequenceMgr.handleChars(
                    getContext(), buffer.toString())) {
                mPinEntry.getText().clear();
            }
            /* SPRD: add @{ */
            if (mPinEntry.getText().length()< PWDMINLENGTH || mPinEntry.getText().length() > PWDMAXLENGTH){
                mUnlockButton.setEnabled(false);
            } else {
                mUnlockButton.setEnabled(true);
            }
            /* @} */
        }
    };

    protected BroadcastReceiver mEmergencyReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            // TODO Auto-generated method stub
            String action = intent.getAction();
            if(action.equals(ACTION_EMERGENCY_DIAL_START)){
                if(!mDismissed){
                    mPhoneHide = true;
                    hide();
                }
            } else if(action.equals(ACTION_EMERGENCY_DIAL_STOP)){
                if(!mDismissed){
                    mPhoneHide = false;
                    if(!mPowerHide) {
                        show();
                    }
                }
            } else if(action.equals(ACTION_POWER)) {
                if(!mDismissed){
                    mPowerHide = false;
                    if(!mPhoneHide) {
                        show();
                    }
                }
            } else if(action.equals(TelephonyIntents.ACTION_SIM_STATE_CHANGED)) {
                int phoneId = intent.getIntExtra(PhoneConstants.PHONE_KEY,
                        SubscriptionManager.INVALID_SUBSCRIPTION_ID);
                String state = intent.getStringExtra(IccCardConstants.INTENT_KEY_ICC_STATE);
                log("phoneId = "+phoneId + ", state = " + state + ", currentPhoneId = " + mPhone.getPhoneId());
                if (phoneId == mPhone.getPhoneId()
                        && (IccCardConstants.INTENT_VALUE_ICC_ABSENT
                                .equals(state) || IccCardConstants.INTENT_VALUE_ICC_READY
                                .equals(state))) {
                    dismiss();
                }
            }
        }
    };

    protected void getUnlockResp(Message msg, Handler handler, int type) {
        AsyncResult res = (AsyncResult) msg.obj;
        if (res.exception != null) {
            if (DBG)
                log("depersonalization request failure.");
            indicateError();
            // sim lock begin
            int remainCount = msg.arg1;
            log("remainCount = " + remainCount);
            //int remainCount = ((PhoneSprd)mPhone).getSimLockRemainTimes(type);
            if (remainCount > 0) {
                mRetrialCount.setText(getContext().getString(R.string.retrialCount, remainCount));
                handler.postDelayed(new Runnable() {
                    public void run() {
                        hideAlert();
                        mPinEntry.getText().clear();
                        mPinEntry.requestFocus();
                    }
                }, 3000);
            } else {
                if (DBG)
                    log("unlock remain count is 0");
                Settings.System.putInt(mContext.getContentResolver(), IS_SIMLOCK, 1);
                indicateLocked();
                closePanel(handler);
                //restartRadio();
                // reboot(handler);
            }
            // sim lock end
        } else {
            if (DBG) log("depersonalization success.");
            indicateSuccess();
//            restartRadio();
            closePanel(handler);
        }
    }

    private void restartRadio() {
        final RadioInteraction radioInteraction = new RadioInteraction(
                mContext, mPhone.getPhoneId());
        radioInteraction.setCallBack(new Runnable() {
            @Override
            public void run() {
                // TODO Auto-generated method stub
                radioInteraction.powerOnRadio();
                dismiss();
            }
        });
        radioInteraction.powerOffRadio(35000);
    }

    private void closePanel(Handler handler) {
        handler.postDelayed(new Runnable() {
            public void run() {
                dismiss();
                //PowerManager pm = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);
                //pm.reboot(null);
            }
        }, 3000);
    }

    // handler for unlock function results
    private Handler mHandler = new Handler() {
        public void handleMessage(Message msg) {
            int lockType = 0;
            switch (msg.what) {
                case EVENT_ICC_NTWRK_DEPERSONALIZATION_RESULT:
                    lockType = TelephonyManagerSprd.UNLOCK_NETWORK;
                    break;
                case EVENT_ICC_NETWORK_SUBSET_DEPERSONALIZATION_RESULT:
                    lockType = TelephonyManagerSprd.UNLOCK_NETWORK_SUBSET;
                    break;
                case EVENT_ICC_SERVICE_PROVIDER_DEPERSONALIZATION_RESULT:
                    lockType = TelephonyManagerSprd.UNLOCK_SERVICE_PORIVDER;
                    break;
                case EVENT_ICC_CORPORATE_DEPERSONALIZATION_RESULT:
                    lockType = TelephonyManagerSprd.UNLOCK_CORPORATE;
                    break;
                case EVENT_ICC_SIM_DEPERSONALIZATION_RESULT:
                    lockType = TelephonyManagerSprd.UNLOCK_SIM;
                    break;
            }
            getUnlockResp(msg, this, lockType);
        }
    };

    public IccSimLockDepersonalizationPanelSprd(Context context) {
        super(context);
    }

    public IccSimLockDepersonalizationPanelSprd(Context context,int count) {
        this(context);
        mContext = context;
        mCount = count;
    }

    public IccSimLockDepersonalizationPanelSprd(Context context, Phone phone, int count) {
        this(context, count);
        mPhone = phone;
    }

    public IccSimLockDepersonalizationPanelSprd(Context context, Phone phone, int count, int type) {
        this(context, phone, count);
        mType = type;
    }

    protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        setContentView(R.layout.sim_ndp_sprd);

        // PIN entry text field
        mPinEntry = (EditText) findViewById(R.id.pin_entry);
        mPinEntry.setKeyListener(DialerKeyListener.getInstance());
        //mPinEntry.setOnClickListener(mUnlockListener);

        mEntryPanel = (LinearLayout) findViewById(R.id.entry_panel);

        mUnlockButton = (Button) findViewById(R.id.ndp_unlock);
        //mUnlockButton.setOnClickListener(mUnlockListener);

        // The "Dismiss" button is present in some (but not all) products,
        // based on the "sim_network_unlock_allow_dismiss" resource.
        mDismissButton = (Button) findViewById(R.id.ndp_dismiss);
        if (getContext().getResources().getBoolean(R.bool.sim_network_unlock_allow_dismiss)) {
            if (DBG) log("Enabling 'Dismiss' button...");
            mDismissButton.setVisibility(View.VISIBLE);
            mDismissButton.setOnClickListener(mDismissListener);
        } else {
            if (DBG) log("Removing 'Dismiss' button...");
            mDismissButton.setVisibility(View.GONE);
        }

        //status panel is used since we're having problems with the alert dialog.
        mStatusPanel = (LinearLayout) findViewById(R.id.status_panel);
        mStatusText = (TextView) findViewById(R.id.status_text);

        mPinEntry.setFilters(new InputFilter[] {new InputFilter(){
            public CharSequence filter(CharSequence source, int start, int end,
                    Spanned dest, int dstart, int dend) {
                // TODO Auto-generated method stub
                CharSequence input = source.subSequence(start,end);
                if(TextUtils.isDigitsOnly(input)){
                    return input;
                } else {
                    return "";
                }
            }
        }});
        mPinEntry.setLongClickable(false);
        mLabel = (TextView) findViewById(R.id.label);
        mRetrialCount = (TextView) findViewById(R.id.retrialCount);
        mUnlockButton.setEnabled(false);
        mUnlockButton.setOnClickListener(mUnlockListener);
        mRetrialCount.setText(getContext().getResources().getString(R.string.retrialCount, mCount));
        mEmergencyButton = (Button)findViewById(R.id.emergency);
        mEmergencyButton.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                // TODO Auto-generated method stub
                Intent intent = new Intent(ACTION_EMERGENCY_DIAL);
                intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK
                      | Intent.FLAG_ACTIVITY_EXCLUDE_FROM_RECENTS);
                getContext().startActivity(intent);
            }
        });
        if(mPhone == null) {
            mLabel.setText(getContext().getResources().getString(getLabelTextId(true)));
            mPhone = PhoneFactorySprd.getDefaultPhone();
        } else {
            mLabel.setText(getContext().getResources().getString(getLabelTextId(false),mPhone.getPhoneId() + 1));
        }
        mTelephonyManager = (TelephonyManager) mContext.getSystemService(Context.TELEPHONY_SERVICE);
        mFilter =  new IntentFilter();
        mFilter.addAction(ACTION_EMERGENCY_DIAL_START);
        mFilter.addAction(ACTION_EMERGENCY_DIAL_STOP);
        mFilter.addAction(ACTION_POWER);
        mFilter.addAction(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
        CharSequence text = mPinEntry.getText();
        Spannable span = (Spannable) text;
        span.setSpan(mPinEntryWatcher, 0, text.length(), Spannable.SPAN_INCLUSIVE_INCLUSIVE);
    };

    protected PhoneStateListener mPhoneStateListener = new PhoneStateListener() {
        public void onCallStateChanged(int state, String incomingNumber) {
            switch (state) {
                case TelephonyManager.CALL_STATE_IDLE:
                    if (!mDismissed) {
                        mPhoneHide = false;
                        show();
                    }
                    break;
                case TelephonyManager.CALL_STATE_OFFHOOK:
                    if (!mDismissed) {
                        mPhoneHide = true;
                        hide();
                    }
                    break;
                case TelephonyManager.CALL_STATE_RINGING:
                    if (!mDismissed) {
                        mPhoneHide = true;
                        hide();
                    }
                    break;
            }
        };
    };

    protected void onStart() {
        super.onStart();
        mTelephonyManager.listen(mPhoneStateListener, PhoneStateListener.LISTEN_CALL_STATE);
        mContext.registerReceiver(mEmergencyReceiver, mFilter);
    };

    @Override
    public void onStop() {
        super.onStop();
        mTelephonyManager.listen(mPhoneStateListener, PhoneStateListener.LISTEN_NONE);
        if(mEmergencyReceiver != null) {
            mContext.unregisterReceiver(mEmergencyReceiver);
        }
    }

    //Mirrors IccPinUnlockPanel.onKeyDown().
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            return true;
        } else if(keyCode == KeyEvent.KEYCODE_POWER) {
             if(!mDismissed){
                mPowerHide = true;
                hide();
             }
        }

        return super.onKeyDown(keyCode, event);
    }

    public View.OnClickListener mUnlockListener = new View.OnClickListener() {
        public void onClick(View v) {
            String pin = mPinEntry.getText().toString();

            if (TextUtils.isEmpty(pin) || pin.length()< PWDMINLENGTH || pin.length() > PWDMAXLENGTH) {
                return;
            }
            hideInputMethod();
            if (DBG) log("requesting network depersonalization with code " + pin);
            unlockClick(pin);
        }
    };

    protected void unlockClick(String pin) {
        switch(mType) {
        case TelephonyManagerSprd.UNLOCK_SIM:
            ((IccCardProxySprd)mPhone.getIccCard()).setSimLockEnabled(false, pin,
                    Message.obtain(mHandler, EVENT_ICC_SIM_DEPERSONALIZATION_RESULT));
            break;
        case TelephonyManagerSprd.UNLOCK_NETWORK:
            ((IccCardProxySprd)mPhone.getIccCard()).setNetworkLockEnabled(false, pin,
                    Message.obtain(mHandler, EVENT_ICC_NTWRK_DEPERSONALIZATION_RESULT));
            break;
        case TelephonyManagerSprd.UNLOCK_NETWORK_SUBSET:
            ((IccCardProxySprd)mPhone.getIccCard()).setNetworkSubsetLockEnabled(false, pin,
                    Message.obtain(mHandler, EVENT_ICC_NETWORK_SUBSET_DEPERSONALIZATION_RESULT));
            break;
        case TelephonyManagerSprd.UNLOCK_SERVICE_PORIVDER:
            ((IccCardProxySprd)mPhone.getIccCard()).setServiceProviderLockEnabled(false, pin,
                    Message.obtain(mHandler, EVENT_ICC_SERVICE_PROVIDER_DEPERSONALIZATION_RESULT));
            break;
        case TelephonyManagerSprd.UNLOCK_CORPORATE:
            ((IccCardProxySprd)mPhone.getIccCard()).setCorporateLockEnabled(false, pin,
                    Message.obtain(mHandler, EVENT_ICC_CORPORATE_DEPERSONALIZATION_RESULT));
            break;
    	}
        indicateBusy();
    }

    protected boolean checkPin(int minLength){
        String pin = mPinEntry.getText().toString();
        if (TextUtils.isEmpty(pin) || pin.length()< minLength || pin.length() > PWDMAXLENGTH) {
            return false;
        }
        return true;
    }

    public void hideInputMethod(){
        InputMethodManager input = (InputMethodManager)mContext.getSystemService(Service.INPUT_METHOD_SERVICE);
        input.hideSoftInputFromWindow(this.getWindow().getDecorView().getWindowToken(), 0);
    }

    protected void indicateLocked() {
        mStatusText.setText(R.string.sim_lock_forever_body);
        mEntryPanel.setVisibility(View.GONE);
        mStatusPanel.setVisibility(View.VISIBLE);
    }

    protected View.OnClickListener mDismissListener = new View.OnClickListener() {
        public void onClick(View v) {
            if (DBG) log("mDismissListener: skipping depersonalization...");
            dismiss();
            mDismissed = true;
        }
    };

    private void indicateBusy() {
        setStatusText(0);
        mEntryPanel.setVisibility(View.GONE);
        mStatusPanel.setVisibility(View.VISIBLE);
    }

    private void indicateError() {
        setStatusText(-1);
        mEntryPanel.setVisibility(View.GONE);
        mStatusPanel.setVisibility(View.VISIBLE);
    }

    private void indicateSuccess() {
        setStatusText(1);
        mEntryPanel.setVisibility(View.GONE);
        mStatusPanel.setVisibility(View.VISIBLE);
    }

    private void log(String msg) {
        Log.d(TAG, "[IccSimLockDepersonalizationPanel] " + msg);
    }

    private void hideAlert() {
        mEntryPanel.setVisibility(View.VISIBLE);
        mStatusPanel.setVisibility(View.GONE);
    }

    private int getLabelTextId(boolean isPhoneNull) {
        int resId = -1;
        switch(mType) {
        case TelephonyManagerSprd.UNLOCK_SIM:
            resId =  isPhoneNull ? R.string.sim_label_ndp : R.string.sim_sim_label_ndp; 
            break;
        case TelephonyManagerSprd.UNLOCK_NETWORK:
            resId =  isPhoneNull ? R.string.label_ndp : R.string.nw_label_ndp;
            break;
        case TelephonyManagerSprd.UNLOCK_NETWORK_SUBSET:
            resId =  isPhoneNull ? R.string.nws_label_ndp : R.string.sim_nws_label_ndp;
            break;
        case TelephonyManagerSprd.UNLOCK_SERVICE_PORIVDER:
            resId =  isPhoneNull ? R.string.sp_label_ndp : R.string.sim_sp_label_ndp;
            break;
        case TelephonyManagerSprd.UNLOCK_CORPORATE:
            resId =  isPhoneNull ? R.string.corporate_label_ndp : R.string.sim_corporate_label_ndp;
            break;
        }
        return resId;
    }

    // error: -1, busy: 0, success 1
    private void setStatusText(final int flag) {
        int resId = -1;
        switch (mType) {
        case TelephonyManagerSprd.UNLOCK_SIM:
            resId = flag < 0 ? R.string.unlock_sim_failed
                    : (flag == 0 ? R.string.requseting_sim_unlock
                            : R.string.unlock_sim_success);
            break;
        case TelephonyManagerSprd.UNLOCK_NETWORK:
            resId = flag < 0 ? R.string.unlock_failed
                    : (flag == 0 ? R.string.requesting_unlock
                            : R.string.unlock_success);
            break;
        case TelephonyManagerSprd.UNLOCK_NETWORK_SUBSET:
            resId = flag < 0 ? R.string.unlock_nws_failed
                    : (flag == 0 ? R.string.requseting_nws_unlock
                            : R.string.unlock_nws_success);
            break;
        case TelephonyManagerSprd.UNLOCK_SERVICE_PORIVDER:
            resId = flag < 0 ? R.string.unlock_sp_failed
                    : (flag == 0 ? R.string.requseting_sp_unlock
                            : R.string.unlock_sp_success);
            break;
        case TelephonyManagerSprd.UNLOCK_CORPORATE:
            resId = flag < 0 ? R.string.unlock_corporate_failed
                    : (flag == 0 ? R.string.requseting_corporate_unlock
                            : R.string.unlock_corporate_success);
            break;
        }
        mStatusText.setText(resId);
    }
}
