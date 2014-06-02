// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file intentionally does not have header guards, it's included
// inside a macro to generate enum values.
// This defines the canonical button mapping order for gamepad-like devices.

// TODO(SaurabhK): Consolidate with CanonicalButtonIndex enum in
// gamepad_standard_mappings.h, crbug.com/351558.
CANONICAL_BUTTON_INDEX(BUTTON_PRIMARY, 0)
CANONICAL_BUTTON_INDEX(BUTTON_SECONDARY, 1)
CANONICAL_BUTTON_INDEX(BUTTON_TERTIARY, 2)
CANONICAL_BUTTON_INDEX(BUTTON_QUATERNARY, 3)
CANONICAL_BUTTON_INDEX(BUTTON_LEFT_SHOULDER, 4)
CANONICAL_BUTTON_INDEX(BUTTON_RIGHT_SHOULDER, 5)
CANONICAL_BUTTON_INDEX(BUTTON_LEFT_TRIGGER, 6)
CANONICAL_BUTTON_INDEX(BUTTON_RIGHT_TRIGGER, 7)
CANONICAL_BUTTON_INDEX(BUTTON_BACK_SELECT, 8)
CANONICAL_BUTTON_INDEX(BUTTON_START, 9)
CANONICAL_BUTTON_INDEX(BUTTON_LEFT_THUMBSTICK, 10)
CANONICAL_BUTTON_INDEX(BUTTON_RIGHT_THUMBSTICK, 11)
CANONICAL_BUTTON_INDEX(BUTTON_DPAD_UP, 12)
CANONICAL_BUTTON_INDEX(BUTTON_DPAD_DOWN, 13)
CANONICAL_BUTTON_INDEX(BUTTON_DPAD_LEFT, 14)
CANONICAL_BUTTON_INDEX(BUTTON_DPAD_RIGHT, 15)
CANONICAL_BUTTON_INDEX(BUTTON_META, 16)
CANONICAL_BUTTON_INDEX(NUM_CANONICAL_BUTTONS, 17)