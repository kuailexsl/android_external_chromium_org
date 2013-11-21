// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser;

import org.chromium.base.CalledByNative;
import org.chromium.chrome.browser.profiles.Profile;

import java.util.ArrayList;
import java.util.List;

/**
 * This class allows Java code to get and clear the list of recently closed tabs.
 */
public class RecentlyClosedBridge {
    private long mNativeRecentlyClosedTabsBridge;

    /**
     * Callback interface for getting notified when the list of recently closed tabs is updated.
     */
    public interface RecentlyClosedCallback {
        /**
         * This method will be called every time the list of recently closed tabs is updated.
         *
         * It's a good place to call {@link RecentlyClosedBridge#getRecentlyClosedTabs()} to get the
         * updated list of tabs.
         */
        @CalledByNative("RecentlyClosedCallback")
        void onUpdated();
    }

    /**
     * Represents a recently closed tab.
     */
    public static class RecentlyClosedTab {
        public final int id;
        public final String title;
        public final String url;

        private RecentlyClosedTab(int id, String title, String url) {
            this.id = id;
            this.title = title;
            this.url = url;
        }
    }

    @CalledByNative
    private static void pushTab(
            List<RecentlyClosedTab> tabs, int id, String title, String url) {
        RecentlyClosedTab tab = new RecentlyClosedTab(id, title, url);
        tabs.add(tab);
    }

    /**
     * Initializes this class with the given profile.
     * @param profile The Profile whose recently closed tabs will be queried.
     */
    public RecentlyClosedBridge(Profile profile) {
        mNativeRecentlyClosedTabsBridge = nativeInit(profile);
    }

    @Override
    protected void finalize() {
        // Ensure that destroy() was called.
        assert mNativeRecentlyClosedTabsBridge == 0;
    }

    /**
     * Cleans up the C++ side of this class. This instance must not be used after calling destroy().
     */
    public void destroy() {
        assert mNativeRecentlyClosedTabsBridge != 0;
        nativeDestroy(mNativeRecentlyClosedTabsBridge);
    }

    /**
     * Sets the callback to be called whenever the list of recently closed tabs changes.
     * @param callback The RecentlyClosedCallback to be notified, or null.
     */
    public void setRecentlyClosedCallback(RecentlyClosedCallback callback) {
        nativeSetRecentlyClosedCallback(mNativeRecentlyClosedTabsBridge, callback);
    }

    /**
     * @param maxTabCount The maximum number of recently closed tabs to return.
     * @return The list of recently closed tabs, with up to maxTabCount elements.
     */
    public List<RecentlyClosedTab> getRecentlyClosedTabs(int maxTabCount) {
        List<RecentlyClosedTab> tabs = new ArrayList<RecentlyClosedTab>();
        boolean received = nativeGetRecentlyClosedTabs(mNativeRecentlyClosedTabsBridge, tabs,
                maxTabCount);
        return received ? tabs : null;
    }

    /**
     * TODO(newt): remove this before marking http://crbug.com/308820 as fixed.
     * @return The list of recently closed tabs, with up to 5 elements.
     */
    public List<RecentlyClosedTab> getRecentlyClosedTabs() {
        final int maxTabCount = 5;
        return getRecentlyClosedTabs(maxTabCount);
    }

    /**
     * Opens a recently closed tab in the current tab.
     *
     * @param tab The current TabBase.
     * @param recentTab The RecentlyClosedTab to open.
     * @return Whether the tab was successfully opened.
     */
    public boolean openRecentlyClosedTab(TabBase tab, RecentlyClosedTab recentTab) {
        return nativeOpenRecentlyClosedTab(mNativeRecentlyClosedTabsBridge, tab, recentTab.id);
    }

    /**
     * Clears all recently closed tabs.
     */
    public void clearRecentlyClosedTabs() {
        nativeClearRecentlyClosedTabs(mNativeRecentlyClosedTabsBridge);
    }

    private native long nativeInit(Profile profile);
    private native void nativeDestroy(long nativeRecentlyClosedTabsBridge);
    private native void nativeSetRecentlyClosedCallback(
            long nativeRecentlyClosedTabsBridge, RecentlyClosedCallback callback);
    private native boolean nativeGetRecentlyClosedTabs(
            long nativeRecentlyClosedTabsBridge, List<RecentlyClosedTab> tabs, int maxTabCount);
    private native boolean nativeOpenRecentlyClosedTab(
            long nativeRecentlyClosedTabsBridge, TabBase tab, int recentTabId);
    private native void nativeClearRecentlyClosedTabs(long nativeRecentlyClosedTabsBridge);
}
