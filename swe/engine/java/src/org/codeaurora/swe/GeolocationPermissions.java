/*
 * Copyright (c) 2013 The Linux Foundation. All rights reserved.
 * Not a contribution.
 *
 * Copyright (C) 2009 The Android Open Source Project
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

package org.codeaurora.swe;

import java.util.HashMap;
import java.util.Set;

import android.content.SharedPreferences;
import android.webkit.ValueCallback;

import org.chromium.android_webview.AwGeolocationPermissions;
import org.chromium.base.ThreadUtils;
import org.chromium.net.GURLUtils;
import org.json.JSONArray;
import org.json.JSONException;

/**
 * This class is used to manage permissions for the WebView's Geolocation
 * JavaScript API.
 *
 * Geolocation permissions are applied to an origin, which consists of the
 * host, scheme and port of a URI. In order for web content to use the
 * Geolocation API, permission must be granted for that content's origin.
 *
 * This class stores Geolocation permissions. An origin's permission state can
 * be either allowed or denied and is associated with an expiration time. This
 * class uses Strings to represent an origin.
 *
 * When an origin attempts to use the Geolocation API, but no permission state
 * is currently set for that origin,
 * WebChromeClient#onGeolocationPermissionsShowPrompt(String,GeolocationPermissions.Callback)
 * is called. This allows the permission state to be set for that origin.
 *
 * The methods of this class can be used to modify and interrogate the stored
 * Geolocation permissions at any time.
 *
 * Within WebKit, Geolocation permissions may be applied either temporarily
 * (for the duration of the page) or permanently. This class deals only with
 * permanent permissions.
*/
//SWE-FIXME AwGeolocationPermissions is now final
public final class GeolocationPermissions {

    private static GeolocationPermissions sGeolocationPermissions;
    private static GeolocationPermissions sIncognitoGeolocationPermissions;

    private static final String PREF_PREFIX = "SweGeolocationPermissions%";

    public HashMap<String, GeolocationPolicy> mPolicies;
    private OnGeolocationPolicyModifiedListener mListener = null;
    private boolean mPrivateBrowsing;
    private SharedPreferences mSharedPreferences;

    /**
     *  The following constant indicates that the Geolocation permission should
     *  never expire unless it is explicitly cleared.
     */
    public static final long DO_NOT_EXPIRE = -1;

    /**
     * Return value for getExpirationTime method when no Geolocation permission
     * state exists for the specified origin.
     */
    public static final long NO_STATE_EXISTS = -2;

    public interface OnGeolocationPolicyModifiedListener {
        void onGeolocationPolicyAdded(String origin, boolean allowed);
        void onGeolocationPolicyCleared(String origin);
        void onGeolocationPolicyClearedAll();
    }

    public void registerOnGeolocationPolicyModifiedListener(
            OnGeolocationPolicyModifiedListener listener) {
        mListener = (OnGeolocationPolicyModifiedListener) listener;
    }

    protected static GeolocationPermissions create(SharedPreferences sharedPreferences,
            boolean privateBrowsing) {
        if (privateBrowsing) {
            if (sIncognitoGeolocationPermissions == null) {
                sIncognitoGeolocationPermissions =
                        new GeolocationPermissions(sharedPreferences, true);
            }
            return sIncognitoGeolocationPermissions;
        } else {
            if (sGeolocationPermissions == null) {
                sGeolocationPermissions =
                        new GeolocationPermissions(sharedPreferences, false);
            }
            return sGeolocationPermissions;
        }
    }

    public static GeolocationPermissions getInstance() {
        if (sGeolocationPermissions == null) {
            sGeolocationPermissions = null;
            //SWE-FIXME
            //(GeolocationPermissions)
              //      Engine.getAwBrowserContext().getGeolocationPermissions();
        }
        return sGeolocationPermissions;
    }

    public static GeolocationPermissions getIncognitoInstance() {
        if (sIncognitoGeolocationPermissions == null) {
            sIncognitoGeolocationPermissions = null;
            //SWE-FIXME
            //(GeolocationPermissions)
              //      Engine.getAwBrowserContext().getIncognitoGeolocationPermissions();
        }
        return sIncognitoGeolocationPermissions;
    }

    public static boolean isIncognitoCreated() {
        return sIncognitoGeolocationPermissions == null ? false : true;
    }

    // Inner class to maintain the geolocation policy in memory
    private class GeolocationPolicy {

        private boolean mAllow;
        // Policy expiration time
        private long mExpirationTime;

        private GeolocationPolicy(boolean allow,
                long expirationTime) {
            mAllow = allow;
            mExpirationTime = expirationTime;
        }

        protected void update(boolean allow) {
            mAllow = allow;
        }

        protected void update(boolean allow, long expirationTime) {
            mAllow = allow;
            mExpirationTime = expirationTime;
        }

        protected boolean isAllowed() {
            return (expired() ? false : mAllow);
        }

        protected boolean expired() {
            return (mExpirationTime == DO_NOT_EXPIRE) ?
                    false : (mExpirationTime <= System.currentTimeMillis());
        }

        protected long getExpirationTime() {
            return mExpirationTime;
        }

        public String toString() {
            JSONArray jsonArray = new JSONArray();
            jsonArray.put(mAllow).put(mExpirationTime);
            return jsonArray.toString();
        }

    }

    /**
     * Creates or updates an existing policy with the specified expirationTime
     */
    public void setPolicy(String origin, boolean value, long expirationTime) {
        // Sanitize origin in case it is a URL
        String key = getOriginFromUrl(origin);
        if (key != null) {
            if (mPolicies.containsKey(key)) {
                mPolicies.get(key).update(value, expirationTime);
            } else {
                mPolicies.put(key, new GeolocationPolicy(value, expirationTime));
            }
            updatePolicyOnFile(key, mPolicies.get(key).toString());
        }
    }

    /**
     * Updates an existing policy.
     */
    public void updatePolicy(String origin, boolean value) {
        // Sanitize origin in case it is a URL
        String key = getOriginFromUrl(origin);
        if (key != null && mPolicies.containsKey(key)) {
            if (!policyExpired(key)) {
                GeolocationPolicy policy = mPolicies.get(key);
                policy.update(value);
                updatePolicyOnFile(key, policy.toString());
            }
        }
    }

    /**
     * Constructor parses the specified sharedPreferences and loads the
     * geolocation policy in memory.
     */
    private GeolocationPermissions(SharedPreferences sharedPreferences,
            boolean privateBrowsing) {
        //SWE-FIXME
        //super(sharedPreferences);
        mPrivateBrowsing = privateBrowsing;
        mPolicies = new HashMap<String, GeolocationPolicy>();
        //Parse sharedPreferences
        for (String name : mSharedPreferences.getAll().keySet()) {
            if (name.startsWith(PREF_PREFIX)) {
                String origin = name.substring(PREF_PREFIX.length());
                JSONArray jsonArray;
                try {
                    jsonArray = new JSONArray(mSharedPreferences.getString(name, "[]"));
                    if (jsonArray.length() == 2) {
                        long expiration = jsonArray.getLong(1);
                        // Remove entry if policy expired
                        if (expiration != DO_NOT_EXPIRE &&
                                expiration <= System.currentTimeMillis()) {
                            removePolicyOnFile(origin);
                        } else {
                            mPolicies.put(origin, new GeolocationPolicy(
                                jsonArray.getBoolean(0), //allow
                                jsonArray.getLong(1)));  // expirationTime
                        }
                    }
                } catch (JSONException e) {
                    // Remove entry if JSON parsing fails
                    removePolicyOnFile(origin);
                }
            }
        }
    }

    /**
     * Method to return if a policy associated with the specified key expired.
     * If expired, the policy is removed.
     */
    private boolean policyExpired(String key) {
        boolean expired = mPolicies.get(key).expired();
        if (expired) {
            mPolicies.remove(key);
            removePolicyOnFile(key);
        }
        return expired;
    }

    //SWE-FIXME
    //@Override
    public void allow(String origin) {
        try {
            // if origin is a JSON string create a policy with the specified expiration
            JSONArray jsonArray = new JSONArray(origin);
            Long expirationTime = jsonArray.getLong(0);
            origin = jsonArray.getString(1);
            setPolicy(origin, true, expirationTime);
        } catch (JSONException e) {
            // otherwise assume DO_NOT_EXPIRE
            setPolicy(origin, true, DO_NOT_EXPIRE);
        }
        // Inform the listener
        if (mListener != null) {
            mListener.onGeolocationPolicyAdded(origin, true);
        }
    }

    //SWE-FIXME
    //@Override
    public void deny(String origin) {
        try {
            // if origin is a JSON string create a policy with the specified expiration
            JSONArray jsonArray = new JSONArray(origin);
            Long expirationTime = jsonArray.getLong(0);
            origin = jsonArray.getString(1);
            setPolicy(origin, false, expirationTime);
        } catch (JSONException e) {
            // otherwise assume DO_NOT_EXPIRE
            setPolicy(origin, false, DO_NOT_EXPIRE);
        }
        // Inform the listener
        if (mListener != null) {
            mListener.onGeolocationPolicyAdded(origin, false);
        }
    }

    //SWE-FIXME
    //@Override
    public void clear(String origin) {
        // Sanitize origin in case it is a URL
        String key = getOriginFromUrl(origin);
        if (key != null) {
            mPolicies.remove(key);
            removePolicyOnFile(key);

            // Inform the listener
            if (mListener != null) {
                mListener.onGeolocationPolicyCleared(origin);
            }
        }
    }

    //SWE-FIXME
    //@Override
    public void clearAll() {
        mPolicies.clear();
        removeAllPoliciesOnFile();

        // Inform the listener
        if (mListener != null) {
            mListener.onGeolocationPolicyClearedAll();
        }
    }

    /**
     * Method to get if an origin is set to be allowed.
     */
    //SWE-FIXME
    //@Override
    public boolean isOriginAllowed(String origin) {
        // Sanitize origin in case it is a URL
        String key = getOriginFromUrl(origin);
        return (!mPolicies.containsKey(key) ? false :
            mPolicies.get(key).isAllowed());
    }

    /**
     * Method to get if there is a persistent policy for the specified origin
     * that has not yet expired.
     */
    //SWE-FIXME
    //@Override
    public boolean hasOrigin(String origin) {
        // Sanitize origin in case it is a URL
        String key = getOriginFromUrl(origin);
        return (key == null) ? false :
            (mPolicies.containsKey(key) && !policyExpired(key));
    }

    /**
     * Asynchronous method to get if there is a persistent policy for the
     * specified origin that has not yet expired.
     */
    public void hasOrigin(String origin, final ValueCallback<Boolean> callback) {
        final boolean finalHasOrigin = hasOrigin(origin);
        ThreadUtils.postOnUiThread(new Runnable() {
            @Override
            public void run() {
                callback.onReceiveValue(finalHasOrigin);
            }
        });
    }

    /**
     * Asynchronous method to get if an origin is to be allowed.
     */
    public void getAllowed(String origin, final ValueCallback<Boolean> callback) {
        final boolean finalAllowed = isOriginAllowed(origin);
        ThreadUtils.postOnUiThread(new Runnable() {
            @Override
            public void run() {
                callback.onReceiveValue(finalAllowed);
            }
        });
    }

    /**
     * Asynchronous method to get the domains currently allowed or denied.
     */
    public void getOrigins(final ValueCallback<Set<String>> callback) {
        final Set<String> origins = mPolicies.keySet();
        ThreadUtils.postOnUiThread(new Runnable() {
            @Override
            public void run() {
                callback.onReceiveValue(origins);
            }
        });
    }

    /**
     * Asynchronous method to get the expiration time of the policy associated
     * with the specified origin.
     */
    public void getExpirationTime(String origin, final ValueCallback<Long> callback) {
        final long finalExpirationTime;
        // Sanitize origin in case it is a URL
        String key = getOriginFromUrl(origin);

        if (key == null || !mPolicies.containsKey(key) ||
                policyExpired(key)) {
           finalExpirationTime = NO_STATE_EXISTS;
        } else {
            finalExpirationTime = mPolicies.get(key).getExpirationTime();
        }

        ThreadUtils.postOnUiThread(new Runnable() {
            @Override
            public void run() {
                callback.onReceiveValue(finalExpirationTime);
            }
        });
    }

    public static void onIncognitoTabsRemoved() {
        if (sIncognitoGeolocationPermissions != null) {
            sIncognitoGeolocationPermissions.clearAll();
            sIncognitoGeolocationPermissions = null;
            Engine.getAwBrowserContext().setIncognitoGeolocationPermissions(null);
        }
    }

    /**
     * Get the origin of a URL using the GURL library.
     */
    public static String getOriginFromUrl(String url) {
        String origin = GURLUtils.getOrigin(url);
        if (origin.isEmpty()) {
            return null;
        }
        return origin;
    }

    private void updatePolicyOnFile(String key, String policyString) {
        // Do not update the file when in private browsing mode
        if (!mPrivateBrowsing) {
            mSharedPreferences.edit()
                .putString(PREF_PREFIX + key, policyString).apply();
        }
    }

    private void removePolicyOnFile(String key) {
        // Do not update the file when in private browsing mode
        if (!mPrivateBrowsing) {
            mSharedPreferences.edit().remove(PREF_PREFIX + key).apply();
        }
    }

    private void removeAllPoliciesOnFile() {
        // Do not update the file when in private browsing mode
        if (!mPrivateBrowsing) {
            SharedPreferences.Editor editor = null;
            for (String name : mSharedPreferences.getAll().keySet()) {
                if (name.startsWith(PREF_PREFIX)) {
                    if (editor == null) {
                        editor = mSharedPreferences.edit();
                    }
                    editor.remove(name);
                }
            }
            if (editor != null) {
                editor.apply();
            }
        }
    }

}

