/*
 * Copyright (C) 2006 The Android Open Source Project
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

package com.sprd.dialer.plugin;

import android.app.AddonManager;
import android.content.Context;
import android.util.Log;
import android.widget.TextView;

import com.sprd.dialer.DialerPlugin;

/**
 * Various utilities for dealing with phone number strings.
 */
public class AddonDialerPlugin extends DialerPlugin implements AddonManager.InitialCallback
{
    private Context mAddonContext;
    private static final String TAG = "[CMCC::AddonDialerPlugin]";

    public AddonDialerPlugin() {
    }

    @Override
    public Class onCreateAddon(Context context, Class clazz) {
        log("clazz: " + clazz);
        mAddonContext = context;
        return clazz;
    }

    public boolean callLogDetail_showDuration() {
        log("callLogDetail_showDuration");
        return false;
    }

    private static void log(String msg) {
        Log.d(TAG, msg);
    }
}
