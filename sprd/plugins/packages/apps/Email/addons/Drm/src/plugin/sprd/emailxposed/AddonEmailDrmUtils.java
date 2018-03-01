package plugin.sprd.emailxposed;

import java.util.List;

import com.android.mail.compose.AttachmentsView;
import com.android.mail.compose.AttachmentsView.AttachmentFailureException;
import com.android.mail.compose.ComposeActivity;
import com.android.mail.providers.Account;
import com.android.mail.providers.Attachment;
import com.sprd.xposed.DrmXposedUtils;

import plugin.sprd.emailxposed.R;

import android.app.AddonManager;
import android.content.Context;
import android.drm.DrmStore;
import android.net.Uri;
import android.util.Log;
import android.widget.Toast;

public class AddonEmailDrmUtils extends DrmXposedUtils implements
		AddonManager.InitialCallback {

	private Context mAddonContext;
	private DrmUtils mDrmUtils;
	private static final String TAG = "AddonEmailDrmUtils";

	public AddonEmailDrmUtils() {
	}

	@Override
	public Class onCreateAddon(Context context, Class clazz) {
		mAddonContext = context;
		//mDrmUtils = new DrmUtils(context);
		return clazz;
	}

	@Override
	public boolean isSDFile(Context context, Uri uri) {
		boolean result = true;
		if (mDrmUtils.isDrmType(uri)
				&& !mDrmUtils
						.haveRightsForAction(uri, DrmStore.Action.TRANSFER)) {
			Log.d(TAG, "Enter ComposeActivity: isSDFile=false");
			Toast.makeText(context,
					mAddonContext.getString(R.string.drm_protected_file),
					Toast.LENGTH_LONG).show();
			result = false;
		}
		return result;
	}

	@Override
	public boolean isDrmFile(String attName, String attType) {
		Log.d(TAG,
				"Enter AttachmentTile: isDrmType="
						+ mDrmUtils.isDrmType(attName, attType));
		return mDrmUtils.isDrmType(attName, attType);
	}

	@Override
	public boolean canOpenDrm(Context context, String attName, String attType) {
		Log.d(TAG,
				"Enter MessageAttachmentBar: canOpenDrm ="
						+ mDrmUtils.isDrmType(attName, attType));
		boolean result = true;
		if (mDrmUtils.isDrmType(attName, attType)) {
			Toast.makeText(context,
					mAddonContext.getString(R.string.not_support_open_drm),
					Toast.LENGTH_LONG).show();
			result = false;
		}
		return result;
	}

	@Override
	public boolean drmPluginEnabled() {
		return true;
	}

	public long addAttachmentsXposed(ComposeActivity composeActivity,
			List<Attachment> attachments, AttachmentsView mAttachmentsView,
			Account mAccount) {

		long size = 0;
		AttachmentFailureException error = null;

		int drmNum = 0;
		for (Attachment a : attachments) {
			if (mDrmUtils.isDrmType(a.getName(), a.getContentType())
					&& !mDrmUtils.haveRightsForAction(a.contentUri,
							DrmStore.Action.TRANSFER)) {
				drmNum++;
				continue;
			}

			try {
				size += mAttachmentsView.addAttachment(mAccount, a);
			} catch (AttachmentFailureException e) {
				error = e;
			}
		}

		if (drmNum > 0) {
			composeActivity.showErrorToast(mAddonContext
					.getString(R.string.not_add_drm_file));
		}

		if (error != null) {
			Log.e(TAG, "Error adding attachment: " + error);
			if (attachments.size() > 1) {
				composeActivity
						.showAttachmentTooBigToast(com.android.mail.R.string.too_large_to_attach_multiple);
			} else {
				composeActivity.showAttachmentTooBigToast(error.getErrorRes());
			}
		}
		return size;
	}
	
	public void sendContext(Context context) {
		mDrmUtils = new DrmUtils(context);
	}
}
