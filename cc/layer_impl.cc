// Copyright 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "cc/layer_impl.h"

#include "base/debug/trace_event.h"
#include "base/stringprintf.h"
#include "cc/debug_border_draw_quad.h"
#include "cc/debug_colors.h"
#include "cc/layer_tree_host_impl.h"
#include "cc/math_util.h"
#include "cc/proxy.h"
#include "cc/quad_sink.h"
#include "cc/scrollbar_animation_controller.h"
#include "ui/gfx/point_conversions.h"
#include "ui/gfx/rect_conversions.h"

namespace cc {

LayerImpl::LayerImpl(LayerTreeHostImpl* hostImpl, int id)
    : m_parent(0)
    , m_maskLayerId(-1)
    , m_replicaLayerId(-1)
    , m_layerId(id)
    , m_layerTreeHostImpl(hostImpl)
    , m_anchorPoint(0.5, 0.5)
    , m_anchorPointZ(0)
    , m_contentsScaleX(1.0)
    , m_contentsScaleY(1.0)
    , m_scrollable(false)
    , m_shouldScrollOnMainThread(false)
    , m_haveWheelEventHandlers(false)
    , m_backgroundColor(0)
    , m_doubleSided(true)
    , m_layerPropertyChanged(false)
    , m_layerSurfacePropertyChanged(false)
    , m_masksToBounds(false)
    , m_contentsOpaque(false)
    , m_preserves3D(false)
    , m_useParentBackfaceVisibility(false)
    , m_drawCheckerboardForMissingTiles(false)
    , m_useLCDText(false)
    , m_drawsContent(false)
    , m_forceRenderSurface(false)
    , m_isContainerForFixedPositionLayers(false)
    , m_fixedToContainerLayer(false)
    , m_drawDepth(0)
#ifndef NDEBUG
    , m_betweenWillDrawAndDidDraw(false)
#endif
    , m_layerAnimationController(LayerAnimationController::create())
{
    DCHECK(m_layerId > 0);
    DCHECK(m_layerTreeHostImpl);
    m_layerAnimationController->setId(m_layerId);
    m_layerAnimationController->setAnimationRegistrar(hostImpl);
}

LayerImpl::~LayerImpl()
{
#ifndef NDEBUG
    DCHECK(!m_betweenWillDrawAndDidDraw);
#endif
}

void LayerImpl::addChild(scoped_ptr<LayerImpl> child)
{
    child->setParent(this);
    DCHECK_EQ(layerTreeHostImpl(), child->layerTreeHostImpl());
    m_children.append(child.Pass());
}

void LayerImpl::removeFromParent()
{
    if (!m_parent)
        return;

    LayerImpl* parent = m_parent;
    m_parent = 0;

    for (size_t i = 0; i < parent->m_children.size(); ++i) {
        if (parent->m_children[i] == this) {
            parent->m_children.remove(i);
            return;
        }
    }
}

void LayerImpl::removeAllChildren()
{
    while (m_children.size())
        m_children[0]->removeFromParent();
}

void LayerImpl::clearChildList()
{
    m_children.clear();
}

void LayerImpl::createRenderSurface()
{
    DCHECK(!m_drawProperties.render_surface);
    m_drawProperties.render_surface = make_scoped_ptr(new RenderSurfaceImpl(this));
    m_drawProperties.render_target = this;
}

int LayerImpl::descendantsDrawContent()
{
    int result = 0;
    for (size_t i = 0; i < m_children.size(); ++i) {
        if (m_children[i]->drawsContent())
            ++result;
        result += m_children[i]->descendantsDrawContent();
        if (result > 1)
            return result;
    }
    return result;
}

scoped_ptr<SharedQuadState> LayerImpl::createSharedQuadState() const
{
  scoped_ptr<SharedQuadState> state = SharedQuadState::Create();
  state->SetAll(m_drawProperties.target_space_transform,
                m_drawProperties.visible_content_rect,
                m_drawProperties.drawable_content_rect,
                m_drawProperties.clip_rect,
                m_drawProperties.is_clipped,
                m_drawProperties.opacity);
  return state.Pass();
}

void LayerImpl::willDraw(ResourceProvider*)
{
#ifndef NDEBUG
    // willDraw/didDraw must be matched.
    DCHECK(!m_betweenWillDrawAndDidDraw);
    m_betweenWillDrawAndDidDraw = true;
#endif
}

void LayerImpl::didDraw(ResourceProvider*)
{
#ifndef NDEBUG
    DCHECK(m_betweenWillDrawAndDidDraw);
    m_betweenWillDrawAndDidDraw = false;
#endif
}

bool LayerImpl::showDebugBorders() const
{
    return m_layerTreeHostImpl->debugState().showDebugBorders;
}

void LayerImpl::getDebugBorderProperties(SkColor* color, float* width) const
{
    if (m_drawsContent) {
        *color = DebugColors::ContentLayerBorderColor();
        *width = DebugColors::ContentLayerBorderWidth(m_layerTreeHostImpl);
        return;
    }

    if (m_masksToBounds) {
        *color = DebugColors::MaskingLayerBorderColor();
        *width = DebugColors::MaskingLayerBorderWidth(m_layerTreeHostImpl);
        return;
    }

    *color = DebugColors::ContainerLayerBorderColor();
    *width = DebugColors::ContainerLayerBorderWidth(m_layerTreeHostImpl);
}

void LayerImpl::appendDebugBorderQuad(QuadSink& quadList, const SharedQuadState* sharedQuadState, AppendQuadsData& appendQuadsData) const
{
    if (!showDebugBorders())
        return;

    SkColor color;
    float width;
    getDebugBorderProperties(&color, &width);

    gfx::Rect contentRect(gfx::Point(), contentBounds());
    scoped_ptr<DebugBorderDrawQuad> debugBorderQuad = DebugBorderDrawQuad::Create();
    debugBorderQuad->SetNew(sharedQuadState, contentRect, color, width);
    quadList.append(debugBorderQuad.PassAs<DrawQuad>(), appendQuadsData);
}

bool LayerImpl::hasContributingDelegatedRenderPasses() const
{
    return false;
}

RenderPass::Id LayerImpl::firstContributingRenderPassId() const
{
    return RenderPass::Id(0, 0);
}

RenderPass::Id LayerImpl::nextContributingRenderPassId(RenderPass::Id) const
{
    return RenderPass::Id(0, 0);
}

ResourceProvider::ResourceId LayerImpl::contentsResourceId() const
{
    NOTREACHED();
    return 0;
}

gfx::Vector2dF LayerImpl::scrollBy(const gfx::Vector2dF& scroll)
{
    gfx::Vector2dF minDelta = -m_scrollOffset;
    gfx::Vector2dF maxDelta = m_maxScrollOffset - m_scrollOffset;
    // Clamp newDelta so that position + delta stays within scroll bounds.
    gfx::Vector2dF newDelta = (m_scrollDelta + scroll);
    newDelta.ClampToMin(minDelta);
    newDelta.ClampToMax(maxDelta);
    gfx::Vector2dF unscrolled = m_scrollDelta + scroll - newDelta;

    if (m_scrollDelta == newDelta)
        return unscrolled;

    m_scrollDelta = newDelta;
    if (m_scrollbarAnimationController)
        m_scrollbarAnimationController->updateScrollOffset(this);
    noteLayerPropertyChangedForSubtree();

    return unscrolled;
}

InputHandlerClient::ScrollStatus LayerImpl::tryScroll(const gfx::PointF& screenSpacePoint, InputHandlerClient::ScrollInputType type) const
{
    if (shouldScrollOnMainThread()) {
        TRACE_EVENT0("cc", "LayerImpl::tryScroll: Failed shouldScrollOnMainThread");
        return InputHandlerClient::ScrollOnMainThread;
    }

    if (!screenSpaceTransform().IsInvertible()) {
        TRACE_EVENT0("cc", "LayerImpl::tryScroll: Ignored nonInvertibleTransform");
        return InputHandlerClient::ScrollIgnored;
    }

    if (!nonFastScrollableRegion().IsEmpty()) {
        bool clipped = false;
        gfx::PointF hitTestPointInContentSpace = MathUtil::projectPoint(MathUtil::inverse(screenSpaceTransform()), screenSpacePoint, clipped);
        gfx::PointF hitTestPointInLayerSpace = gfx::ScalePoint(hitTestPointInContentSpace, 1 / contentsScaleX(), 1 / contentsScaleY());
        if (!clipped && nonFastScrollableRegion().Contains(gfx::ToRoundedPoint(hitTestPointInLayerSpace))) {
            TRACE_EVENT0("cc", "LayerImpl::tryScroll: Failed nonFastScrollableRegion");
            return InputHandlerClient::ScrollOnMainThread;
        }
    }

    if (type == InputHandlerClient::Wheel && haveWheelEventHandlers()) {
        TRACE_EVENT0("cc", "LayerImpl::tryScroll: Failed wheelEventHandlers");
        return InputHandlerClient::ScrollOnMainThread;
    }

    if (!scrollable()) {
        TRACE_EVENT0("cc", "LayerImpl::tryScroll: Ignored not scrollable");
        return InputHandlerClient::ScrollIgnored;
    }

    return InputHandlerClient::ScrollStarted;
}

bool LayerImpl::drawCheckerboardForMissingTiles() const
{
    return m_drawCheckerboardForMissingTiles && !m_layerTreeHostImpl->settings().backgroundColorInsteadOfCheckerboard;
}

gfx::Rect LayerImpl::layerRectToContentRect(const gfx::RectF& layerRect) const
{
    gfx::RectF contentRect = gfx::ScaleRect(layerRect, contentsScaleX(), contentsScaleY());
    // Intersect with content rect to avoid the extra pixel because for some
    // values x and y, ceil((x / y) * y) may be x + 1.
    contentRect.Intersect(gfx::Rect(gfx::Point(), contentBounds()));
    return gfx::ToEnclosingRect(contentRect);
}

std::string LayerImpl::indentString(int indent)
{
    std::string str;
    for (int i = 0; i != indent; ++i)
        str.append("  ");
    return str;
}

void LayerImpl::dumpLayerProperties(std::string* str, int indent) const
{
    std::string indentStr = indentString(indent);
    str->append(indentStr);
    base::StringAppendF(str, "layer ID: %d\n", m_layerId);

    str->append(indentStr);
    base::StringAppendF(str, "bounds: %d, %d\n", bounds().width(), bounds().height());

    if (m_drawProperties.render_target) {
        str->append(indentStr);
        base::StringAppendF(str, "renderTarget: %d\n", m_drawProperties.render_target->m_layerId);
    }

    str->append(indentStr);
    base::StringAppendF(str, "position: %f, %f\n", m_position.x(), m_position.y());

    str->append(indentStr);
    base::StringAppendF(str, "contentsOpaque: %d\n", m_contentsOpaque);

    str->append(indentStr);
    const gfx::Transform& transform = m_drawProperties.target_space_transform;
    base::StringAppendF(str, "drawTransform: %f, %f, %f, %f  //  %f, %f, %f, %f  //  %f, %f, %f, %f  //  %f, %f, %f, %f\n",
        transform.matrix().getDouble(0, 0), transform.matrix().getDouble(0, 1), transform.matrix().getDouble(0, 2), transform.matrix().getDouble(0, 3),
        transform.matrix().getDouble(1, 0), transform.matrix().getDouble(1, 1), transform.matrix().getDouble(1, 2), transform.matrix().getDouble(1, 3),
        transform.matrix().getDouble(2, 0), transform.matrix().getDouble(2, 1), transform.matrix().getDouble(2, 2), transform.matrix().getDouble(2, 3),
        transform.matrix().getDouble(3, 0), transform.matrix().getDouble(3, 1), transform.matrix().getDouble(3, 2), transform.matrix().getDouble(3, 3));

    str->append(indentStr);
    base::StringAppendF(str, "drawsContent: %s\n", m_drawsContent ? "yes" : "no");
}

std::string LayerImpl::layerTreeAsText() const
{
    std::string str;
    dumpLayer(&str, 0);
    return str;
}

void LayerImpl::dumpLayer(std::string* str, int indent) const
{
    str->append(indentString(indent));
    base::StringAppendF(str, "%s(%s)\n", layerTypeAsString(), m_debugName.data());
    dumpLayerProperties(str, indent+2);
    if (m_replicaLayer) {
        str->append(indentString(indent+2));
        str->append("Replica:\n");
        m_replicaLayer->dumpLayer(str, indent+3);
    }
    if (m_maskLayer) {
        str->append(indentString(indent+2));
        str->append("Mask:\n");
        m_maskLayer->dumpLayer(str, indent+3);
    }
    for (size_t i = 0; i < m_children.size(); ++i)
        m_children[i]->dumpLayer(str, indent+1);
}

void LayerImpl::setStackingOrderChanged(bool stackingOrderChanged)
{
    // We don't need to store this flag; we only need to track that the change occurred.
    if (stackingOrderChanged)
        noteLayerPropertyChangedForSubtree();
}

bool LayerImpl::layerSurfacePropertyChanged() const
{
    if (m_layerSurfacePropertyChanged)
        return true;

    // If this layer's surface property hasn't changed, we want to see if
    // some layer above us has changed this property. This is done for the
    // case when such parent layer does not draw content, and therefore will
    // not be traversed by the damage tracker. We need to make sure that
    // property change on such layer will be caught by its descendants.
    LayerImpl* current = this->m_parent;
    while (current && !current->m_drawProperties.render_surface) {
        if (current->m_layerSurfacePropertyChanged)
            return true;
        current = current->m_parent;
    }

    return false;
}

void LayerImpl::noteLayerPropertyChangedForSubtree()
{
    m_layerPropertyChanged = true;
    noteLayerPropertyChangedForDescendants();
}

void LayerImpl::noteLayerPropertyChangedForDescendants()
{
    for (size_t i = 0; i < m_children.size(); ++i)
        m_children[i]->noteLayerPropertyChangedForSubtree();
}

const char* LayerImpl::layerTypeAsString() const
{
    return "Layer";
}

void LayerImpl::resetAllChangeTrackingForSubtree()
{
    m_layerPropertyChanged = false;
    m_layerSurfacePropertyChanged = false;

    m_updateRect = gfx::RectF();

    if (m_drawProperties.render_surface)
        m_drawProperties.render_surface->resetPropertyChangedFlag();

    if (m_maskLayer)
        m_maskLayer->resetAllChangeTrackingForSubtree();

    if (m_replicaLayer)
        m_replicaLayer->resetAllChangeTrackingForSubtree(); // also resets the replica mask, if it exists.

    for (size_t i = 0; i < m_children.size(); ++i)
        m_children[i]->resetAllChangeTrackingForSubtree();
}

bool LayerImpl::layerIsAlwaysDamaged() const
{
    return false;
}

int LayerImpl::id() const
{
     return m_layerId;
}

void LayerImpl::setBounds(const gfx::Size& bounds)
{
    if (m_bounds == bounds)
        return;

    m_bounds = bounds;

    if (masksToBounds())
        noteLayerPropertyChangedForSubtree();
    else
        m_layerPropertyChanged = true;
}

void LayerImpl::setMaskLayer(scoped_ptr<LayerImpl> maskLayer)
{
    if (maskLayer)
        DCHECK_EQ(layerTreeHostImpl(), maskLayer->layerTreeHostImpl());
    m_maskLayer = maskLayer.Pass();

    int newLayerId = m_maskLayer ? m_maskLayer->id() : -1;
    if (newLayerId == m_maskLayerId)
        return;

    m_maskLayerId = newLayerId;
    noteLayerPropertyChangedForSubtree();
}

void LayerImpl::setReplicaLayer(scoped_ptr<LayerImpl> replicaLayer)
{
    if (replicaLayer)
        DCHECK_EQ(layerTreeHostImpl(), replicaLayer->layerTreeHostImpl());
    m_replicaLayer = replicaLayer.Pass();

    int newLayerId = m_replicaLayer ? m_replicaLayer->id() : -1;
    if (newLayerId == m_replicaLayerId)
        return;

    m_replicaLayerId = newLayerId;
    noteLayerPropertyChangedForSubtree();
}

void LayerImpl::setDrawsContent(bool drawsContent)
{
    if (m_drawsContent == drawsContent)
        return;

    m_drawsContent = drawsContent;
    m_layerPropertyChanged = true;
}

void LayerImpl::setAnchorPoint(const gfx::PointF& anchorPoint)
{
    if (m_anchorPoint == anchorPoint)
        return;

    m_anchorPoint = anchorPoint;
    noteLayerPropertyChangedForSubtree();
}

void LayerImpl::setAnchorPointZ(float anchorPointZ)
{
    if (m_anchorPointZ == anchorPointZ)
        return;

    m_anchorPointZ = anchorPointZ;
    noteLayerPropertyChangedForSubtree();
}

void LayerImpl::setBackgroundColor(SkColor backgroundColor)
{
    if (m_backgroundColor == backgroundColor)
        return;

    m_backgroundColor = backgroundColor;
    m_layerPropertyChanged = true;
}

void LayerImpl::setFilters(const WebKit::WebFilterOperations& filters)
{
    if (m_filters == filters)
        return;

    DCHECK(!m_filter);
    m_filters = filters;
    noteLayerPropertyChangedForSubtree();
}

void LayerImpl::setBackgroundFilters(const WebKit::WebFilterOperations& backgroundFilters)
{
    if (m_backgroundFilters == backgroundFilters)
        return;

    m_backgroundFilters = backgroundFilters;
    m_layerPropertyChanged = true;
}

void LayerImpl::setFilter(const skia::RefPtr<SkImageFilter>& filter)
{
    if (m_filter.get() == filter.get())
        return;

    DCHECK(m_filters.isEmpty());
    m_filter = filter;
    noteLayerPropertyChangedForSubtree();
}

void LayerImpl::setMasksToBounds(bool masksToBounds)
{
    if (m_masksToBounds == masksToBounds)
        return;

    m_masksToBounds = masksToBounds;
    noteLayerPropertyChangedForSubtree();
}

void LayerImpl::setContentsOpaque(bool opaque)
{
    if (m_contentsOpaque == opaque)
        return;

    m_contentsOpaque = opaque;
    noteLayerPropertyChangedForSubtree();
}

void LayerImpl::setOpacity(float opacity)
{
    if (!m_layerAnimationController->setOpacity(opacity))
        return;
    m_layerSurfacePropertyChanged = true;
}

float LayerImpl::opacity() const
{
    return m_layerAnimationController->opacity();
}

bool LayerImpl::opacityIsAnimating() const
{
    return m_layerAnimationController->isAnimatingProperty(ActiveAnimation::Opacity);
}

void LayerImpl::setPosition(const gfx::PointF& position)
{
    if (m_position == position)
        return;

    m_position = position;
    noteLayerPropertyChangedForSubtree();
}

void LayerImpl::setPreserves3D(bool preserves3D)
{
    if (m_preserves3D == preserves3D)
        return;

    m_preserves3D = preserves3D;
    noteLayerPropertyChangedForSubtree();
}

void LayerImpl::setSublayerTransform(const gfx::Transform& sublayerTransform)
{
    if (m_sublayerTransform == sublayerTransform)
        return;

    m_sublayerTransform = sublayerTransform;
    // sublayer transform does not affect the current layer; it affects only its children.
    noteLayerPropertyChangedForDescendants();
}

void LayerImpl::setTransform(const gfx::Transform& transform)
{
    if (!m_layerAnimationController->setTransform(transform))
        return;
    m_layerSurfacePropertyChanged = true;
}

const gfx::Transform& LayerImpl::transform() const
{
    return m_layerAnimationController->transform();
}

bool LayerImpl::transformIsAnimating() const
{
    return m_layerAnimationController->isAnimatingProperty(ActiveAnimation::Transform);
}

void LayerImpl::setContentBounds(const gfx::Size& contentBounds)
{
    if (m_contentBounds == contentBounds)
        return;

    m_contentBounds = contentBounds;
    m_layerPropertyChanged = true;
}

void LayerImpl::setContentsScale(float contentsScaleX, float contentsScaleY)
{
    if (m_contentsScaleX == contentsScaleX && m_contentsScaleY == contentsScaleY)
        return;

    m_contentsScaleX = contentsScaleX;
    m_contentsScaleY = contentsScaleY;
    m_layerPropertyChanged = true;
}

void LayerImpl::setScrollOffset(gfx::Vector2d scrollOffset)
{
    if (m_scrollOffset == scrollOffset)
        return;

    m_scrollOffset = scrollOffset;
    noteLayerPropertyChangedForSubtree();
}

void LayerImpl::setScrollDelta(const gfx::Vector2dF& scrollDelta)
{
    if (m_scrollDelta == scrollDelta)
        return;

    m_scrollDelta = scrollDelta;
    noteLayerPropertyChangedForSubtree();
}

void LayerImpl::setImplTransform(const gfx::Transform& transform)
{
    if (m_implTransform == transform)
        return;

    m_implTransform = transform;
    noteLayerPropertyChangedForSubtree();
}

void LayerImpl::setDoubleSided(bool doubleSided)
{
    if (m_doubleSided == doubleSided)
        return;

    m_doubleSided = doubleSided;
    noteLayerPropertyChangedForSubtree();
}

Region LayerImpl::visibleContentOpaqueRegion() const
{
    if (contentsOpaque())
        return visibleContentRect();
    return Region();
}

void LayerImpl::didLoseOutputSurface()
{
}

void LayerImpl::setMaxScrollOffset(gfx::Vector2d maxScrollOffset)
{
    m_maxScrollOffset = maxScrollOffset;

    if (!m_scrollbarAnimationController)
        return;
    m_scrollbarAnimationController->updateScrollOffset(this);
}

ScrollbarLayerImpl* LayerImpl::horizontalScrollbarLayer()
{
    return m_scrollbarAnimationController ? m_scrollbarAnimationController->horizontalScrollbarLayer() : 0;
}

const ScrollbarLayerImpl* LayerImpl::horizontalScrollbarLayer() const
{
    return m_scrollbarAnimationController ? m_scrollbarAnimationController->horizontalScrollbarLayer() : 0;
}

void LayerImpl::setHorizontalScrollbarLayer(ScrollbarLayerImpl* scrollbarLayer)
{
    if (!m_scrollbarAnimationController)
        m_scrollbarAnimationController = ScrollbarAnimationController::create(this);
    m_scrollbarAnimationController->setHorizontalScrollbarLayer(scrollbarLayer);
    m_scrollbarAnimationController->updateScrollOffset(this);
}

ScrollbarLayerImpl* LayerImpl::verticalScrollbarLayer()
{
    return m_scrollbarAnimationController ? m_scrollbarAnimationController->verticalScrollbarLayer() : 0;
}

const ScrollbarLayerImpl* LayerImpl::verticalScrollbarLayer() const
{
    return m_scrollbarAnimationController ? m_scrollbarAnimationController->verticalScrollbarLayer() : 0;
}

void LayerImpl::setVerticalScrollbarLayer(ScrollbarLayerImpl* scrollbarLayer)
{
    if (!m_scrollbarAnimationController)
        m_scrollbarAnimationController = ScrollbarAnimationController::create(this);
    m_scrollbarAnimationController->setVerticalScrollbarLayer(scrollbarLayer);
    m_scrollbarAnimationController->updateScrollOffset(this);
}

}  // namespace cc
