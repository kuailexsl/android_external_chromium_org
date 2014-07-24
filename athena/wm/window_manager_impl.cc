// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "athena/wm/public/window_manager.h"

#include "athena/input/public/accelerator_manager.h"
#include "athena/screen/public/screen_manager.h"
#include "athena/wm/bezel_controller.h"
#include "athena/wm/public/window_manager_observer.h"
#include "athena/wm/split_view_controller.h"
#include "athena/wm/window_overview_mode.h"
#include "base/logging.h"
#include "base/observer_list.h"
#include "ui/aura/layout_manager.h"
#include "ui/aura/window.h"
#include "ui/wm/core/window_util.h"
#include "ui/wm/public/window_types.h"

namespace athena {
namespace {

class WindowManagerImpl : public WindowManager,
                          public WindowOverviewModeDelegate,
                          public aura::WindowObserver,
                          public AcceleratorHandler {
 public:
  WindowManagerImpl();
  virtual ~WindowManagerImpl();

  void Layout();

  // WindowManager:
  virtual void ToggleOverview() OVERRIDE;

 private:
  enum Command {
    COMMAND_TOGGLE_OVERVIEW,
  };

  void InstallAccelerators();

  // WindowManager:
  virtual void AddObserver(WindowManagerObserver* observer) OVERRIDE;
  virtual void RemoveObserver(WindowManagerObserver* observer) OVERRIDE;

  // WindowOverviewModeDelegate:
  virtual void OnSelectWindow(aura::Window* window) OVERRIDE;

  // aura::WindowObserver
  virtual void OnWindowDestroying(aura::Window* window) OVERRIDE;

  // AcceleratorHandler:
  virtual bool IsCommandEnabled(int command_id) const OVERRIDE;
  virtual bool OnAcceleratorFired(int command_id,
                                  const ui::Accelerator& accelerator) OVERRIDE;

  scoped_ptr<aura::Window> container_;
  scoped_ptr<WindowOverviewMode> overview_;
  scoped_ptr<BezelController> bezel_controller_;
  scoped_ptr<SplitViewController> split_view_controller_;
  ObserverList<WindowManagerObserver> observers_;

  DISALLOW_COPY_AND_ASSIGN(WindowManagerImpl);
};

class AthenaContainerLayoutManager : public aura::LayoutManager {
 public:
  AthenaContainerLayoutManager();
  virtual ~AthenaContainerLayoutManager();

 private:
  // aura::LayoutManager:
  virtual void OnWindowResized() OVERRIDE;
  virtual void OnWindowAddedToLayout(aura::Window* child) OVERRIDE;
  virtual void OnWillRemoveWindowFromLayout(aura::Window* child) OVERRIDE;
  virtual void OnWindowRemovedFromLayout(aura::Window* child) OVERRIDE;
  virtual void OnChildWindowVisibilityChanged(aura::Window* child,
                                              bool visible) OVERRIDE;
  virtual void SetChildBounds(aura::Window* child,
                              const gfx::Rect& requested_bounds) OVERRIDE;

  DISALLOW_COPY_AND_ASSIGN(AthenaContainerLayoutManager);
};

class WindowManagerImpl* instance = NULL;

WindowManagerImpl::WindowManagerImpl() {
  ScreenManager::ContainerParams params("DefaultContainer");
  params.can_activate_children = true;
  container_.reset(ScreenManager::Get()->CreateDefaultContainer(params));
  container_->SetLayoutManager(new AthenaContainerLayoutManager);
  container_->AddObserver(this);
  bezel_controller_.reset(new BezelController(container_.get()));
  split_view_controller_.reset(new SplitViewController());
  bezel_controller_->set_left_right_delegate(split_view_controller_.get());
  container_->AddPreTargetHandler(bezel_controller_.get());
  instance = this;
  InstallAccelerators();
}

WindowManagerImpl::~WindowManagerImpl() {
  if (container_) {
    container_->RemoveObserver(this);
    container_->RemovePreTargetHandler(bezel_controller_.get());
  }
  container_.reset();
  instance = NULL;
}

void WindowManagerImpl::Layout() {
  if (!container_)
    return;
  gfx::Rect bounds = gfx::Rect(container_->bounds().size());
  const aura::Window::Windows& children = container_->children();
  for (aura::Window::Windows::const_iterator iter = children.begin();
       iter != children.end();
       ++iter) {
    if ((*iter)->type() == ui::wm::WINDOW_TYPE_NORMAL ||
        (*iter)->type() == ui::wm::WINDOW_TYPE_POPUP)
      (*iter)->SetBounds(bounds);
  }
}

void WindowManagerImpl::ToggleOverview() {
  if (overview_) {
    overview_.reset();
    FOR_EACH_OBSERVER(WindowManagerObserver, observers_,
                      OnOverviewModeExit());
  } else {
    overview_ = WindowOverviewMode::Create(container_.get(), this);
    FOR_EACH_OBSERVER(WindowManagerObserver, observers_,
                      OnOverviewModeEnter());
  }
}

void WindowManagerImpl::InstallAccelerators() {
  const AcceleratorData accelerator_data[] = {
      {TRIGGER_ON_PRESS, ui::VKEY_F6, ui::EF_NONE, COMMAND_TOGGLE_OVERVIEW,
       AF_NONE},
  };
  AcceleratorManager::Get()->RegisterAccelerators(
      accelerator_data, arraysize(accelerator_data), this);
}

void WindowManagerImpl::AddObserver(WindowManagerObserver* observer) {
  observers_.AddObserver(observer);
}

void WindowManagerImpl::RemoveObserver(WindowManagerObserver* observer) {
  observers_.RemoveObserver(observer);
}

void WindowManagerImpl::OnSelectWindow(aura::Window* window) {
  CHECK_EQ(container_.get(), window->parent());
  container_->StackChildAtTop(window);
  wm::ActivateWindow(window);
  overview_.reset();
  FOR_EACH_OBSERVER(WindowManagerObserver, observers_,
                    OnOverviewModeExit());
}

void WindowManagerImpl::OnWindowDestroying(aura::Window* window) {
  if (window == container_)
    container_.reset();
}

bool WindowManagerImpl::IsCommandEnabled(int command_id) const {
  return true;
}

bool WindowManagerImpl::OnAcceleratorFired(int command_id,
                                           const ui::Accelerator& accelerator) {
  switch (command_id) {
    case COMMAND_TOGGLE_OVERVIEW:
      ToggleOverview();
      break;
  }
  return true;
}

AthenaContainerLayoutManager::AthenaContainerLayoutManager() {
}

AthenaContainerLayoutManager::~AthenaContainerLayoutManager() {
}

void AthenaContainerLayoutManager::OnWindowResized() {
  instance->Layout();
}

void AthenaContainerLayoutManager::OnWindowAddedToLayout(aura::Window* child) {
  instance->Layout();
}

void AthenaContainerLayoutManager::OnWillRemoveWindowFromLayout(
    aura::Window* child) {
}

void AthenaContainerLayoutManager::OnWindowRemovedFromLayout(
    aura::Window* child) {
  instance->Layout();
}

void AthenaContainerLayoutManager::OnChildWindowVisibilityChanged(
    aura::Window* child,
    bool visible) {
  instance->Layout();
}

void AthenaContainerLayoutManager::SetChildBounds(
    aura::Window* child,
    const gfx::Rect& requested_bounds) {
  if (!requested_bounds.IsEmpty())
    SetChildBoundsDirect(child, requested_bounds);
}

}  // namespace

// static
WindowManager* WindowManager::Create() {
  DCHECK(!instance);
  new WindowManagerImpl;
  DCHECK(instance);
  return instance;
}

// static
void WindowManager::Shutdown() {
  DCHECK(instance);
  delete instance;
  DCHECK(!instance);
}

// static
WindowManager* WindowManager::GetInstance() {
  DCHECK(instance);
  return instance;
}

}  // namespace athena
