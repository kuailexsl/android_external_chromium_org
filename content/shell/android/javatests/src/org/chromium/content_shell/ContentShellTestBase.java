// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content_shell;

import android.content.ComponentName;
import android.content.Intent;
import android.net.Uri;
import android.test.ActivityInstrumentationTestCase2;
import android.text.TextUtils;

import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicReference;

import org.chromium.content.browser.test.util.Criteria;
import org.chromium.content.browser.test.util.CriteriaHelper;

/**
 * Base test class for all ContentShell based tests.
 */
public class ContentShellTestBase extends ActivityInstrumentationTestCase2<ContentShellActivity> {

    public ContentShellTestBase() {
        super(ContentShellActivity.class);
    }

    /**
     * Starts the ContentShell activity and loads the given URL.
     */
    protected ContentShellActivity launchContentShellWithUrl(String url) {
        Intent intent = new Intent(Intent.ACTION_MAIN);
        intent.addCategory(Intent.CATEGORY_LAUNCHER);
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setData(Uri.parse(url));
        intent.setComponent(new ComponentName(getInstrumentation().getTargetContext(),
              ContentShellActivity.class));
        setActivityIntent(intent);
        return getActivity();
    }

    /**
     * Waits for the Active shell to finish loading.  This times out after three seconds,
     * so it shouldn't be used for long loading pages.  Instead it should be used more for
     * test initialization.  The proper way to wait is to use a TestCallbackHelperContainer
     * after the initial load is completed.
     * @return Whether or not the Shell was actually finished loading.
     * @throws Exception
     */
    protected boolean waitForActiveShellToBeDoneLoading() throws Exception {
        final ContentShellActivity activity = getActivity();

        // Wait for the Content Shell to be initialized.
        return CriteriaHelper.pollForCriteria(new Criteria() {
            @Override
            public boolean isSatisfied() {
                try {
                    final AtomicBoolean isLoaded = new AtomicBoolean(false);
                    runTestOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            Shell shell = activity.getActiveShell();
                            if (shell != null) {
                                // There are two cases here that need to be accounted for.
                                // The first is that we've just created a Shell and it isn't
                                // loading because it has no URL set yet.  The second is that
                                // we've set a URL and it actually is loading.
                                isLoaded.set(!shell.isLoading()
                                        && !TextUtils.isEmpty(shell.getContentView().getUrl()));
                            } else {
                                isLoaded.set(false);
                            }
                        }
                    });

                    return isLoaded.get();
                } catch (Throwable e) {
                    return false;
                }
            }
        });
    }
}
