/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "module_load_checker.h"

#include "utils/log.h"

bool ModuleLoadChecker::CheckModuleLoadable(const char* moduleName)
{
    std::shared_lock lock(moduleCheckerDelegateMutex_);
    if (!moduleCheckerDelegate_) {
        HILOG_INFO("Not check moduleLoadable, moduleCheckerDelegate_ not set");
        return true;
    }
    return moduleCheckerDelegate_->CheckModuleLoadable(moduleName);
}

bool ModuleLoadChecker::DiskCheckOnly()
{
    std::shared_lock lock(moduleCheckerDelegateMutex_);
    if (!moduleCheckerDelegate_) {
        HILOG_INFO("Not check moduleLoadable, moduleCheckerDelegate_ not set");
        return true;
    }
    return moduleCheckerDelegate_->DiskCheckOnly();
}

void ModuleLoadChecker::SetDelegate(const std::shared_ptr<ModuleCheckerDelegate>& moduleCheckerDelegate)
{
    std::lock_guard lock(moduleCheckerDelegateMutex_);
    moduleCheckerDelegate_ = moduleCheckerDelegate;
}