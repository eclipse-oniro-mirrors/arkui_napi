/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef FOUNDATION_ACE_NAPI_NATIVE_ENGINE_ARKTS_ENVSAN_REPORTER_H
#define FOUNDATION_ACE_NAPI_NATIVE_ENGINE_ARKTS_ENVSAN_REPORTER_H

#include <string>

class NativeEngine;

// Strip HILOG privacy tags ("%{public}s" -> "%s") to get a plain printf format.
std::string ConvertHilogToFmt(const char* hilogFmt);
// printf-style formatting of a HILOG format string; refuses "%{private}" formats.
std::string FormatHilogMsg(const char* hilogFmt, ...);

// Reports the ArkTS_ENVSAN fault event to faultloggerd via signal 42,
// throttled by the engine's detection frequency / interval; falls back to
// hilog when signal delivery is unavailable.
void ReportArkTSEnvSanEvent(NativeEngine* engine, const std::string& msg);

#endif /* FOUNDATION_ACE_NAPI_NATIVE_ENGINE_ARKTS_ENVSAN_REPORTER_H */
