/* Copyright (c) 2012 The Chromium Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

/* From ppb_view.idl modified Mon Feb  6 14:05:16 2012. */

#ifndef PPAPI_C_PPB_VIEW_H_
#define PPAPI_C_PPB_VIEW_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_point.h"
#include "ppapi/c/pp_rect.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_size.h"
#include "ppapi/c/pp_stdint.h"

#define PPB_VIEW_INTERFACE_1_0 "PPB_View;1.0"
#define PPB_VIEW_INTERFACE PPB_VIEW_INTERFACE_1_0

/**
 * @file
 * Defines the <code>PPB_View</code> struct representing the state of the
 * view of an instance.
 */


/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * <code>PPB_View</code> represents the state of the view of an instance.
 * You will receive new view information via
 * <code>PPP_Instance.DidChangeView</code>.
 */
struct PPB_View_1_0 {
  /**
   * <code>IsView()</code> determines if the given resource is a valid
   * <code>PPB_View</code> resource. Note that <code>PPB_ViewChanged</code>
   * resources derive from <code>PPB_View</code> and will return true here
   * as well.
   *
   * @return <code>PP_TRUE</code> if the given resource supports
   * <code>PPB_View</code> or <code>PP_FALSE</code> if it is an invalid
   * resource or is a resource of another type.
   */
  PP_Bool (*IsView)(PP_Resource resource);
  /**
   * <code>GetRect()</code> retrieves the rectangle of the instance
   * associated with the view changed notification relative to the upper left
   * of the browser viewport. This position changes when the page is scrolled.
   *
   * The returned rectangle may not be inside the visible portion of the
   * viewport if the instance is scrolled off the page. Therefore, the position
   * may be negative or larger than the size of the page. The size will always
   * reflect the size of the plugin were it to be scrolled entirely into view.
   *
   * In general, most plugins will not need to worry about the position of the
   * instance in the viewport, and only need to use the size.
   *
   * @param rect Output argument receiving the rectangle on success.
   *
   * @return Returns <code>PP_TRUE</code> if the resource was valid and the
   * viewport rect was filled in, <code>PP_FALSE</code> if not.
   */
  PP_Bool (*GetRect)(PP_Resource resource, struct PP_Rect* rect);
  /**
   * <code>IsFullscreen()</code> returns whether the instance is currently
   * displaying in fullscreen mode.
   *
   * @return <code>PP_TRUE</code> if the instance is in full screen mode,
   * or <code>PP_FALSE</code> if it's not or the resource is invalid.
   */
  PP_Bool (*IsFullscreen)(PP_Resource resource);
  /**
   * <code>IsVisible()</code> returns whether the instance is plausibly
   * visible to the user. You should use this flag to throttle or stop updates
   * for invisible plugins.
   *
   * Thie measure incorporates both whether the instance is scrolled into
   * view (whether the clip rect is nonempty) and whether the page is plausibly
   * visible to the user (<code>IsPageVisible()</code>).
   *
   * @return <code>PP_TRUE</code> if the instance is plausibly visible to the
   * user, <code>PP_FALSE</code> if it is definitely not visible.
   */
  PP_Bool (*IsVisible)(PP_Resource resource);
  /**
   * <code>IsPageVisible()</code> determines if the page that contains the
   * instance is visible. The most common cause of invisible pages is that
   * the page is in a background tab in the browser.
   *
   * Most applications should use <code>IsVisible()</code> rather than
   * this function since the instance could be scrolled off of a visible
   * page, and this function will still return true. However, depending on
   * how your plugin interacts with the page, there may be certain updates
   * that you may want to perform when the page is visible even if your
   * specific instance isn't.
   *
   * @return <code>PP_TRUE</code> if the instance is plausibly visible to the
   * user, <code>PP_FALSE</code> if it is definitely not visible.
   */
  PP_Bool (*IsPageVisible)(PP_Resource resource);
  /**
   * <code>GetClipRect()</code> returns the clip rectangle relative to the
   * upper left corner of the instance. This rectangle indicates which parts of
   * the instance are scrolled into view.
   *
   * If the instance is scrolled off the view, the return value will be
   * (0, 0, 0, 0). This clip rect does <i>not</i> take into account page
   * visibility. This means if the instance is scrolled into view but the page
   * itself is in an invisible tab, the return rect will contain the visible
   * rect assuming the page was visible. See <code>IsPageVisible()</code> and
   * <code>IsVisible()</code> if you want to handle this case.
   *
   * Most applications will not need to worry about the clip. The recommended
   * behavior is to do full updates if the instance is visible as determined by
   * <code>IsVisible()</code> and do no updates if not.
   *
   * However, if the cost for computing pixels is very high for your
   * application or the pages you're targeting frequently have very large
   * instances with small visible portions, you may wish to optimize further.
   * In this case, the clip rect will tell you which parts of the plugin to
   * update.
   *
   * Note that painting of the page and sending of view changed updates
   * happens asynchronously. This means when the user scrolls, for example,
   * it is likely that the previous backing store of the instance will be used
   * for the first paint, and will be updated later when your application
   * generates new content with the new clip. This may cause flickering at the
   * boundaries when scrolling. If you do choose to do partial updates, you may
   * want to think about what color the invisible portions of your backing
   * store contain (be it transparent or some background color) or to paint
   * a certain region outside the clip to reduce the visual distraction when
   * this happens.
   *
   * @param clip Output argument receiving the clip rect on success.
   *
   * @return Returns <code>PP_TRUE</code> if the resource was valid and the
   * clip rect was filled in, <code>PP_FALSE</code> if not.
   */
  PP_Bool (*GetClipRect)(PP_Resource resource, struct PP_Rect* clip);
};

typedef struct PPB_View_1_0 PPB_View;
/**
 * @}
 */

#endif  /* PPAPI_C_PPB_VIEW_H_ */

