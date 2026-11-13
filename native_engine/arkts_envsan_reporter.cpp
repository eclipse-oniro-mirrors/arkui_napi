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

#include "arkts_envsan_reporter.h"

#include <cstdarg>
#include <cstring>

#include "securec.h"
#include "utils/log.h"

// Strip HILOG privacy tags from a HILOG-style format string, producing a plain
// printf format. Example conversions, shown as before and after pairs.
//   before: "param env not equal to its owner. %{public}s at %{public}s:%{public}d"
//   after : "param env not equal to its owner. %s at %s:%d"
//   before: "owner env has been destroyed, "
//           "owner id: %{public}" PRIu64 ", current env id: %{public}" PRIu64 ". "
//           "%{public}s at %{public}s:%{public}d, "
//   after : "owner env has been destroyed, owner id: %" PRIu64 ", current env id: %" PRIu64 ". %s at %s:%d, "

std::string ConvertHilogToFmt(const char* hilogFmt)
{
    if (hilogFmt == nullptr) {
        return "";
    }
    const std::string fmt(hilogFmt);
    std::string result;
    result.reserve(fmt.size());
    size_t pos = 0;
    size_t begin = 0;
    while ((begin = fmt.find("%{", pos)) != std::string::npos) {
        const size_t end = fmt.find('}', begin);
        if (end == std::string::npos) {
            break;
        }
        result.append(fmt, pos, begin - pos);
        result.push_back('%');
        pos = end + 1;
    }
    result.append(fmt, pos, std::string::npos);
    return result;
}

// Render a HILOG-style format string with printf arguments:
//   before: "param env not equal to its owner. %{public}s at %{public}s:%{public}d"
//   after : "param env not equal to its owner. Func at File.cpp:12"
//   before: "owner env has been destroyed, "
//           "owner id: %{public}" PRIu64 ", current env id: %{public}" PRIu64 ". "
//           "%{public}s at %{public}s:%{public}d, " with args (7, 9, "Func", "File.cpp", 12)
//   after : "owner env has been destroyed, owner id: 7, current env id: 9. Func at File.cpp:12, "

std::string FormatHilogMsg(const char* hilogFmt, ...)
{
    if (hilogFmt == nullptr) {
        return "";
    }
    if (strstr(hilogFmt, "%{private}") != nullptr) {
        HILOG_WARN("FormatHilogMsg does not support private hilog directive, message is not formatted "
                   "to avoid bypassing hilog privacy masking, fmt: %{public}s", hilogFmt);
        return hilogFmt;
    }
    const std::string fmt = ConvertHilogToFmt(hilogFmt);
    constexpr size_t maxMsgSize = 4096;
    std::string msg(maxMsgSize, '\0');
    va_list args;
    va_start(args, hilogFmt);
    int size = vsnprintf_s(msg.data(), maxMsgSize, maxMsgSize - 1, fmt.c_str(), args);
    va_end(args);
    if (size <= 0) {
        return fmt;
    }
    msg.resize(static_cast<size_t>(size));
    return msg;
}

#if defined(OHOS_PLATFORM)

#include <atomic>
#include <csignal>
#include <ctime>
#include <unistd.h>

#if defined(__aarch64__)
#include <asm/unistd.h>
#else
#include <sys/syscall.h>
#endif

#include "native_engine.h"
#include "utils/log.h"

namespace {
struct MultiThreadDebugMsg {
    uint64_t timestamp;
    const char *msg;
};

constexpr size_t MAX_DEBUG_MSG_LEN = 8192;
thread_local MultiThreadDebugMsg g_debugMsg = {0, nullptr};
thread_local char g_debugMsgBuf[MAX_DEBUG_MSG_LEN] = {0};

#if defined(__aarch64__)
static long Syscall(unsigned long n, unsigned long a, unsigned long b, unsigned long c, unsigned long d)
{
    register unsigned long x8 asm("x8") = n;
    register unsigned long x0 asm("x0") = a;
    register unsigned long x1 asm("x1") = b;
    register unsigned long x2 asm("x2") = c;
    register unsigned long x3 asm("x3") = d;
    asm volatile("svc 0" : "=r"(x0) : "r"(x8), "0"(x0), "r"(x1), "r"(x2), "r"(x3)
                 : "memory", "cc");
    return x0;
}
#endif

std::atomic<uint64_t> g_lastReportTime {0};
std::atomic<uint64_t> g_reportCount {0};

constexpr uint64_t SECOND_PER_MINUTE = 60;
constexpr uint64_t MS_PER_SECOND = 1000;
constexpr uint64_t NS_PER_MILLISECOND = 1000000;

uint64_t NowMs()
{
    struct timespec ts;
    (void)clock_gettime(CLOCK_REALTIME, &ts);
    return (static_cast<uint64_t>(ts.tv_sec) * MS_PER_SECOND) +
        (static_cast<uint64_t>(ts.tv_nsec) / NS_PER_MILLISECOND);
}

bool CheckCountNum(int64_t frequency)
{
    if (frequency < 1) {
        return true;
    }
    uint64_t previous = g_reportCount.fetch_add(1, std::memory_order_relaxed);
    return previous % static_cast<uint64_t>(frequency) == 0;
}

bool CheckTimeInterval(uint64_t now, int64_t interval)
{
    uint64_t lastTime = g_lastReportTime.load(std::memory_order_relaxed);
    if (lastTime != 0 &&
        now - lastTime < static_cast<uint64_t>(interval) * SECOND_PER_MINUTE * MS_PER_SECOND) {
        return false;
    }
    return g_lastReportTime.compare_exchange_strong(lastTime, now, std::memory_order_relaxed);
}

bool IsSignalHandlerAvailable(int signo)
{
    struct sigaction prevAction;
    if (sigaction(signo, nullptr, &prevAction) != 0) {
        return false;
    }
    if (prevAction.sa_flags & SA_SIGINFO) {
        return prevAction.sa_sigaction != nullptr;
    }
    return prevAction.sa_handler != SIG_DFL && prevAction.sa_handler != SIG_IGN;
}
} // namespace

void ReportArkTSEnvSanEvent(NativeEngine* engine, const std::string &msg)
{
    uint64_t now = NowMs();
    if (engine != nullptr) {
        if (!CheckCountNum(engine->GetDetectionFrequency())) {
            HILOG_DEBUG("ArkTS_ENVSAN event not reported to faultlogger: blocked by frequency gate");
            return;
        }
        if (!CheckTimeInterval(now, engine->GetDetectionInterval())) {
            HILOG_DEBUG("ArkTS_ENVSAN event not reported to faultlogger: blocked by interval gate");
            return;
        }
    } else {
        g_lastReportTime.store(now, std::memory_order_relaxed);
    }

    static constexpr int signo = 42;
    if (!IsSignalHandlerAvailable(signo)) {
        HILOG_ERROR("no handler for signal %{public}d, report by hilog: %{public}s", signo, msg.c_str());
        return;
    }

    size_t msgLen = msg.size();
    if (msgLen > MAX_DEBUG_MSG_LEN - 1) {
        msgLen = MAX_DEBUG_MSG_LEN - 1;
    }
    if (memcpy_s(g_debugMsgBuf, MAX_DEBUG_MSG_LEN, msg.data(), msgLen) != EOK) {
        HILOG_WARN("copy ArkTS_ENVSAN event message failed");
        return;
    }
    g_debugMsgBuf[msgLen] = '\0';
    g_debugMsg.timestamp = now;
    g_debugMsg.msg = g_debugMsgBuf;

    static constexpr int siCode = -4;
    siginfo_t info = {};
    info.si_signo = signo;
    info.si_code = siCode;
    info.si_value.sival_ptr = &g_debugMsg;

#if defined(__aarch64__)
    pid_t tid = static_cast<pid_t>(Syscall(__NR_gettid, 0, 0, 0, 0));
    if (Syscall(__NR_rt_tgsigqueueinfo, static_cast<unsigned long>(getpid()),
                static_cast<unsigned long>(tid), static_cast<unsigned long>(signo),
                reinterpret_cast<unsigned long>(&info)) < 0) {
#else
    if (syscall(SYS_rt_tgsigqueueinfo, getpid(), syscall(SYS_gettid), signo, &info) == -1) {
#endif
        HILOG_WARN("%{public}s", msg.c_str());
    }
}

#else // !OHOS_PLATFORM

#include "utils/log.h"

void ReportArkTSEnvSanEvent(NativeEngine* /*engine*/, const std::string &msg)
{
    HILOG_ERROR("ReportArkTSEnvSanEvent is not supported on this platform, msg: %{public}s", msg.c_str());
}

#endif // OHOS_PLATFORM
