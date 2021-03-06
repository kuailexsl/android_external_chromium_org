// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/supervised_user/supervised_user_service_factory.h"

#include "chrome/browser/profiles/incognito_helpers.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/profile_oauth2_token_service_factory.h"
#include "chrome/browser/supervised_user/supervised_user_service.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "extensions/browser/extension_system_provider.h"
#include "extensions/browser/extensions_browser_client.h"

// static
SupervisedUserService* SupervisedUserServiceFactory::GetForProfile(
    Profile* profile) {
  return static_cast<SupervisedUserService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

// static
SupervisedUserServiceFactory* SupervisedUserServiceFactory::GetInstance() {
  return Singleton<SupervisedUserServiceFactory>::get();
}

// static
KeyedService* SupervisedUserServiceFactory::BuildInstanceFor(Profile* profile) {
  return new SupervisedUserService(profile);
}

SupervisedUserServiceFactory::SupervisedUserServiceFactory()
    : BrowserContextKeyedServiceFactory(
        "SupervisedUserService",
        BrowserContextDependencyManager::GetInstance()) {
  DependsOn(
      extensions::ExtensionsBrowserClient::Get()->GetExtensionSystemFactory());
  DependsOn(ProfileOAuth2TokenServiceFactory::GetInstance());
}

SupervisedUserServiceFactory::~SupervisedUserServiceFactory() {}

content::BrowserContext* SupervisedUserServiceFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return chrome::GetBrowserContextRedirectedInIncognito(context);
}

KeyedService* SupervisedUserServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* profile) const {
  return BuildInstanceFor(static_cast<Profile*>(profile));
}
