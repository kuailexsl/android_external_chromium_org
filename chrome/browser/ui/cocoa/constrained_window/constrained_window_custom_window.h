// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_CUSTOM_WINDOW_H_
#define CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_CUSTOM_WINDOW_H_

#import <Cocoa/Cocoa.h>

// A NSWindow subclass that gives a custom look (rounded corners and white
// background).
//
// Unfortunately the word "window" here is overused. ConstrainedWindow is the
// tab modal dialog controller. ConstrainedWindowCustomWindow is the custom
// NSWindow that gives us the new look (rounded corners and white background).
//
// If a ConstrainedWindow is using ConstrainedWindowAlert to display its UI
// then it doesn't have to use this class. On the other hand, if it has some
// custom UI (say from a nib) then it should use this class.
@interface ConstrainedWindowCustomWindow : NSWindow

// Initializes the window with the given content rect.
- (id)initWithContentRect:(NSRect)contentRect;

@end

#endif  // CHROME_BROWSER_UI_COCOA_CONSTRAINED_WINDOW_CONSTRAINED_WINDOW_CUSTOM_WINDOW_H_
