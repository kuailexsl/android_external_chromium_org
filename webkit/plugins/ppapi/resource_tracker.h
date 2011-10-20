// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBKIT_PLUGINS_PPAPI_RESOURCE_TRACKER_H_
#define WEBKIT_PLUGINS_PPAPI_RESOURCE_TRACKER_H_

#include <map>
#include <set>
#include <utility>

#include "base/basictypes.h"
#include "base/gtest_prod_util.h"
#include "base/hash_tables.h"
#include "base/memory/linked_ptr.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_module.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/proxy/interface_id.h"
#include "ppapi/shared_impl/function_group_base.h"
#include "ppapi/shared_impl/resource_tracker.h"
#include "ppapi/shared_impl/tracker_base.h"
#include "ppapi/shared_impl/var_tracker.h"

typedef struct NPObject NPObject;

namespace ppapi {
class NPObjectVar;
class Var;
}

namespace webkit {
namespace ppapi {

class PluginInstance;
class PluginModule;
class ResourceTrackerTest;

// This class maintains a global list of all live pepper resources. It allows
// us to check resource ID validity and to map them to a specific module.
//
// This object is NOT threadsafe.
class ResourceTracker : public ::ppapi::TrackerBase,
                        public ::ppapi::ResourceTracker {
 public:
  ResourceTracker();
  virtual ~ResourceTracker();

  // PP_Resources --------------------------------------------------------------

  // TrackerBase.
  virtual ::ppapi::FunctionGroupBase* GetFunctionAPI(
      PP_Instance pp_instance,
      ::ppapi::proxy::InterfaceID id) OVERRIDE;
  virtual PP_Module GetModuleForInstance(PP_Instance instance) OVERRIDE;

  // ppapi::ResourceTracker overrides.
  virtual void LastPluginRefWasDeleted(::ppapi::Resource* object) OVERRIDE;

  // PP_Vars -------------------------------------------------------------------

  // Tracks all live NPObjectVar. This is so we can map between instance +
  // NPObject and get the NPObjectVar corresponding to it. This Add/Remove
  // function is called by the NPObjectVar when it is created and
  // destroyed.
  void AddNPObjectVar(::ppapi::NPObjectVar* object_var);
  void RemoveNPObjectVar(::ppapi::NPObjectVar* object_var);

  // Looks up a previously registered NPObjectVar for the given NPObject and
  // instance. Returns NULL if there is no NPObjectVar corresponding to the
  // given NPObject for the given instance. See AddNPObjectVar above.
  ::ppapi::NPObjectVar* NPObjectVarForNPObject(PP_Instance instance,
                                               NPObject* np_object);

  // Returns the number of NPObjectVar's associated with the given instance.
  // Returns 0 if the instance isn't known.
  int GetLiveNPObjectVarsForInstance(PP_Instance instance) const;

  // PP_Modules ----------------------------------------------------------------

  // Adds a new plugin module to the list of tracked module, and returns a new
  // module handle to identify it.
  PP_Module AddModule(PluginModule* module);

  // Called when a plugin modulde was deleted and should no longer be tracked.
  // The given handle should be one generated by AddModule.
  void ModuleDeleted(PP_Module module);

  // Returns a pointer to the plugin modulde object associated with the given
  // modulde handle. The return value will be NULL if the handle is invalid.
  PluginModule* GetModule(PP_Module module);

  // PP_Instances --------------------------------------------------------------

  // Adds a new plugin instance to the list of tracked instances, and returns a
  // new instance handle to identify it.
  PP_Instance AddInstance(PluginInstance* instance);

  // Called when a plugin instance was deleted and should no longer be tracked.
  // The given handle should be one generated by AddInstance.
  void InstanceDeleted(PP_Instance instance);

  void InstanceCrashed(PP_Instance instance);

  // Returns a pointer to the plugin instance object associated with the given
  // instance handle. The return value will be NULL if the handle is invalid or
  // if the instance has crashed.
  PluginInstance* GetInstance(PP_Instance instance);

 private:
  friend class ResourceTrackerTest;

  typedef std::set<PP_Resource> ResourceSet;

  // Per-instance data we track.
  struct InstanceData;

  // Force frees all vars and resources associated with the given instance.
  // If delete_instance is true, the instance tracking information will also
  // be deleted.
  void CleanupInstanceData(PP_Instance instance, bool delete_instance);

  // Like ResourceAndRefCount but for vars, which are associated with modules.
  typedef std::pair<scoped_refptr< ::ppapi::Var>, size_t> VarAndRefCount;
  typedef base::hash_map<int32, VarAndRefCount> VarMap;
  VarMap live_vars_;

  // Tracks all live instances and their associated data.
  typedef std::map<PP_Instance, linked_ptr<InstanceData> > InstanceMap;
  InstanceMap instance_map_;

  // Tracks all live modules. The pointers are non-owning, the PluginModule
  // destructor will notify us when the module is deleted.
  typedef std::map<PP_Module, PluginModule*> ModuleMap;
  ModuleMap module_map_;

  DISALLOW_COPY_AND_ASSIGN(ResourceTracker);
};

}  // namespace ppapi
}  // namespace webkit

#endif  // WEBKIT_PLUGINS_PPAPI_RESOURCE_TRACKER_H_
