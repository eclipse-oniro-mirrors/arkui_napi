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

#include <atomic>
#include <cinttypes>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <string>
#include <thread>
#include <vector>

#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "gtest/gtest.h"
#include "multi_thread_error.h"
#include "native_api_internal.h"
#include "test.h"

namespace {
constexpr size_t MAX_CAPTURED_MSG_LEN = 256;

#if defined(OHOS_PLATFORM)
struct test_debug_msg {
    uint64_t timestamp;
    const char *msg;
};

volatile sig_atomic_t g_signalCount = 0;
volatile sig_atomic_t g_plainSignalCount = 0;
volatile sig_atomic_t g_capturedSiCode = 0;
uint64_t g_capturedTimestamp = 0;
uint64_t g_handlerTid = 0;
char g_capturedMsg[MAX_CAPTURED_MSG_LEN] = {0};

void ResetCapture()
{
    g_signalCount = 0;
    g_plainSignalCount = 0;
    g_capturedSiCode = 0;
    g_capturedTimestamp = 0;
    g_handlerTid = 0;
    g_capturedMsg[0] = '\0';
}

void TestSignalHandler(int, siginfo_t *si, void *)
{
    if (si == nullptr || si->si_value.sival_ptr == nullptr) {
        return;
    }
    auto *debugMessage = static_cast<test_debug_msg *>(si->si_value.sival_ptr);
    g_capturedTimestamp = debugMessage->timestamp;
    size_t i = 0;
    while (i < sizeof(g_capturedMsg) - 1 && debugMessage->msg[i] != '\0') {
        g_capturedMsg[i] = debugMessage->msg[i];
        ++i;
    }
    g_capturedMsg[i] = '\0';
    g_capturedSiCode = si->si_code;
    g_handlerTid = NativeEngine::GetCurSysTid();
    g_signalCount++;
}

void PlainSignalHandler(int)
{
    g_plainSignalCount++;
}

class SignalHandlerGuard {
public:
    SignalHandlerGuard()
    {
        struct sigaction action = {};
        action.sa_sigaction = TestSignalHandler;
        action.sa_flags = SA_SIGINFO;
        constexpr int multiThreadErrorSigno = 42;
        sigaction(multiThreadErrorSigno, &action, &oldAction_);
    }

    ~SignalHandlerGuard()
    {
        constexpr int multiThreadErrorSigno = 42;
        sigaction(multiThreadErrorSigno, &oldAction_, nullptr);
    }

private:
    struct sigaction oldAction_ {};
};

class DefaultHandlerGuard {
public:
    DefaultHandlerGuard()
    {
        constexpr int multiThreadErrorSigno = 42;
        struct sigaction action = {};
        action.sa_handler = SIG_DFL;
        sigemptyset(&action.sa_mask);
        sigaction(multiThreadErrorSigno, &action, &oldAction_);
    }

    ~DefaultHandlerGuard()
    {
        constexpr int multiThreadErrorSigno = 42;
        sigaction(multiThreadErrorSigno, &oldAction_, nullptr);
    }

private:
    struct sigaction oldAction_ {};
};

class PlainHandlerGuard {
public:
    PlainHandlerGuard()
    {
        constexpr int multiThreadErrorSigno = 42;
        struct sigaction action = {};
        action.sa_handler = PlainSignalHandler;
        sigemptyset(&action.sa_mask);
        sigaction(multiThreadErrorSigno, &action, &oldAction_);
    }

    ~PlainHandlerGuard()
    {
        constexpr int multiThreadErrorSigno = 42;
        sigaction(multiThreadErrorSigno, &oldAction_, nullptr);
    }

private:
    struct sigaction oldAction_ {};
};

class IgnoreHandlerGuard {
public:
    IgnoreHandlerGuard()
    {
        constexpr int multiThreadErrorSigno = 42;
        struct sigaction action = {};
        action.sa_handler = SIG_IGN;
        sigemptyset(&action.sa_mask);
        sigaction(multiThreadErrorSigno, &action, &oldAction_);
    }

    ~IgnoreHandlerGuard()
    {
        constexpr int multiThreadErrorSigno = 42;
        sigaction(multiThreadErrorSigno, &oldAction_, nullptr);
    }

private:
    struct sigaction oldAction_ {};
};

void SetDetectionConfigForTest(int64_t frequency, int64_t interval)
{
    auto &config = const_cast<panda::DFXJSNApi::MultithreadingDetectionOptions &>(
        panda::DFXJSNApi::GetDetectionConfig());
    config.frequency.store(frequency);
    config.interval.store(interval);
}

class DetectionConfigGuard {
public:
    DetectionConfigGuard()
    {
        const auto &config = panda::DFXJSNApi::GetDetectionConfig();
        frequency_ = config.frequency.load();
        interval_ = config.interval.load();
    }

    ~DetectionConfigGuard()
    {
        SetDetectionConfigForTest(frequency_, interval_);
    }

private:
    int64_t frequency_ = 100;
    int64_t interval_ = 5;
};

class SoftReportEngine final : public ArkNativeEngine {
public:
    SoftReportEngine(EcmaVM* vm, void* jsEngine) : ArkNativeEngine(vm, jsEngine) {}

    bool IsCrossThreadCheckEnabled() const override
    {
        return false;
    }
};
#endif // OHOS_PLATFORM
} // namespace

class MultiThreadErrorTest : public NativeEngineTest {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "MultiThreadErrorTest SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "MultiThreadErrorTest TearDownTestCase";
    }

    void SetUp() override {}

    void TearDown() override {}
};

/**
 * @tc.name: ConvertHilogToFmtTest001
 * @tc.desc: Test ConvertHilogToFmt strips HILOG privacy markers (%{public},
 *           %{private}, %{limit}) while keeping printf conversion specifiers,
 *           and tolerates empty / null / escaped / unterminated / adjacent
 *           marker inputs
 * @tc.type: FUNC
 */
HWTEST_F(MultiThreadErrorTest, ConvertHilogToFmtTest001, testing::ext::TestSize.Level1)
{
    ASSERT_EQ(ConvertHilogToFmt("%{public}s at %{public}s:%{public}d"), "%s at %s:%d");
    ASSERT_EQ(ConvertHilogToFmt("%{private}llu / %{limit}x"), "%llu / %x");
    ASSERT_EQ(ConvertHilogToFmt("no privacy marker"), "no privacy marker");
    ASSERT_EQ(ConvertHilogToFmt(""), "");
    ASSERT_EQ(ConvertHilogToFmt(nullptr), "");
    ASSERT_EQ(ConvertHilogToFmt("100%% and %{public}d"), "100%% and %d");
    ASSERT_EQ(ConvertHilogToFmt("unterminated %{public"), "unterminated %{public");
    ASSERT_EQ(ConvertHilogToFmt("trailing %{"), "trailing %{");
    ASSERT_EQ(ConvertHilogToFmt("%{public}s%{private}d"), "%s%d");
}

/**
 * @tc.name: FormatHilogMsgTest001
 * @tc.desc: Test FormatHilogMsg formats HILOG-style format strings with the
 *           real LOG_IF_SPECIAL call-site signatures, including PRIu64, the
 *           zero-argument case, a null format string, and the truncation
 *           fallback to the raw format string
 * @tc.type: FUNC
 */
HWTEST_F(MultiThreadErrorTest, FormatHilogMsgTest001, testing::ext::TestSize.Level1)
{
    ASSERT_EQ(FormatHilogMsg("param env not equal to its owner. %{public}s at %{public}s:%{public}d",
                             "Func", "File.cpp", 12),
              "param env not equal to its owner. Func at File.cpp:12");
    uint64_t ownerId = 7;
    ASSERT_EQ(FormatHilogMsg("owner id: %{public}" PRIu64, ownerId), "owner id: 7");
    ASSERT_EQ(FormatHilogMsg("no args at all"), "no args at all");
    ASSERT_EQ(FormatHilogMsg(nullptr), "");
    std::string big(5000, 'x');
    ASSERT_EQ(FormatHilogMsg("%{public}s", big.c_str()), "%s");
}

/**
 * @tc.name: FormatHilogMsgPrivateRejectTest001
 * @tc.desc: Test FormatHilogMsg refuses to render formats containing
 *           private hilog directives: the raw format text is returned
 *           without any argument value so the result can never bypass
 *           hilog privacy masking when logged as a public string
 * @tc.type: FUNC
 */
HWTEST_F(MultiThreadErrorTest, FormatHilogMsgPrivateRejectTest001, testing::ext::TestSize.Level1)
{
    std::string result = FormatHilogMsg("secret count: %{private}" PRIu64, 42ULL);
    ASSERT_EQ(result, "secret count: %{private}" PRIu64);
    ASSERT_EQ(result.find("42"), std::string::npos);

    result = FormatHilogMsg("%{public}s leak: %{private}s", "public-value", "secret-value");
    ASSERT_EQ(result, "%{public}s leak: %{private}s");
    ASSERT_EQ(result.find("public-value"), std::string::npos);
    ASSERT_EQ(result.find("secret-value"), std::string::npos);
}

#if defined(OHOS_PLATFORM)
/**
 * @tc.name: NotifyMultiThreadErrorTest001
 * @tc.desc: Test NotifyMultiThreadError reports through signal 42 with the
 *           message and millisecond timestamp carried in the debug_msg
 *           protocol, delivered to the calling thread with si_code -4, when
 *           no gate throttles the report
 * @tc.type: FUNC
 */
HWTEST_F(MultiThreadErrorTest, NotifyMultiThreadErrorTest001, testing::ext::TestSize.Level1)
{
    DetectionConfigGuard configGuard;
    SignalHandlerGuard handlerGuard;
    SetDetectionConfigForTest(1, 0);

    ResetCapture();
    NotifyMultiThreadError(engine_, "multi-thread violation detected");

    ASSERT_EQ(g_signalCount, 1);
    ASSERT_STREQ(g_capturedMsg, "multi-thread violation detected");
    ASSERT_EQ(g_capturedSiCode, -4);
    ASSERT_EQ(g_handlerTid, static_cast<uint64_t>(NativeEngine::GetCurSysTid()));
    uint64_t before = static_cast<uint64_t>(time(nullptr)) * 1000 - 1000;
    uint64_t after = static_cast<uint64_t>(time(nullptr)) * 1000 + 1000;
    ASSERT_GE(g_capturedTimestamp, before);
    ASSERT_LE(g_capturedTimestamp, after);
}

/**
 * @tc.name: NotifyMultiThreadErrorTest002
 * @tc.desc: Test the frequency gate: with frequency=2 the report is allowed
 *           at every second detection, and the modulo counter keeps the
 *           pass / block / pass sequence deterministic whatever
 *           the counter state left by earlier tests
 * @tc.type: FUNC
 */
HWTEST_F(MultiThreadErrorTest, NotifyMultiThreadErrorTest002, testing::ext::TestSize.Level1)
{
    DetectionConfigGuard configGuard;
    SignalHandlerGuard handlerGuard;
    SetDetectionConfigForTest(2, 0);

    constexpr int loopLimit = 100;
    int loops = 0;
    ResetCapture();
    while (g_signalCount == 0 && loops < loopLimit) {
        NotifyMultiThreadError(engine_, "frequency gate");
        ++loops;
    }
    ASSERT_GT(g_signalCount, 0);

    ResetCapture();
    NotifyMultiThreadError(engine_, "frequency gate");
    ASSERT_EQ(g_signalCount, 0);

    ResetCapture();
    NotifyMultiThreadError(engine_, "frequency gate");
    ASSERT_EQ(g_signalCount, 1);
}

/**
 * @tc.name: NotifyMultiThreadErrorTest003
 * @tc.desc: Test the interval gate: after a report an interval of 1440
 *           minutes blocks further reports while interval=0 lets them
 *           through again
 * @tc.type: FUNC
 */
HWTEST_F(MultiThreadErrorTest, NotifyMultiThreadErrorTest003, testing::ext::TestSize.Level1)
{
    DetectionConfigGuard configGuard;
    SignalHandlerGuard handlerGuard;

    SetDetectionConfigForTest(1, 0);
    ResetCapture();
    NotifyMultiThreadError(engine_, "interval gate");
    ASSERT_EQ(g_signalCount, 1);

    SetDetectionConfigForTest(1, 1440);
    ResetCapture();
    NotifyMultiThreadError(engine_, "interval gate");
    ASSERT_EQ(g_signalCount, 0);

    SetDetectionConfigForTest(1, 0);
    ResetCapture();
    NotifyMultiThreadError(engine_, "interval gate");
    ASSERT_EQ(g_signalCount, 1);
}

/**
 * @tc.name: NotifyMultiThreadErrorDeferredDeliveryTest001
 * @tc.desc: Test the signal payload stays valid when signal 42 is blocked on
 *           the calling thread: the report stays pending and the handler
 *           reads the exact message after the thread unblocks the signal,
 *           instead of dereferencing a dangling stack pointer
 * @tc.type: FUNC
 */
HWTEST_F(MultiThreadErrorTest, NotifyMultiThreadErrorDeferredDeliveryTest001, testing::ext::TestSize.Level1)
{
    DetectionConfigGuard configGuard;
    SignalHandlerGuard handlerGuard;
    SetDetectionConfigForTest(1, 0);

    constexpr int multiThreadErrorSigno = 42;
    sigset_t blockSet;
    sigset_t oldSet;
    sigemptyset(&blockSet);
    sigaddset(&blockSet, multiThreadErrorSigno);
    ASSERT_EQ(pthread_sigmask(SIG_BLOCK, &blockSet, &oldSet), 0);

    ResetCapture();
    NotifyMultiThreadError(engine_, "deferred delivery");
    int pendingCount = g_signalCount;
    ASSERT_EQ(pthread_sigmask(SIG_SETMASK, &oldSet, nullptr), 0);

    ASSERT_EQ(pendingCount, 0);
    ASSERT_EQ(g_signalCount, 1);
    ASSERT_STREQ(g_capturedMsg, "deferred delivery");
    ASSERT_EQ(g_capturedSiCode, -4);
    ASSERT_EQ(g_handlerTid, static_cast<uint64_t>(NativeEngine::GetCurSysTid()));
}

/**
 * @tc.name: NotifyMultiThreadErrorNoHandlerTest001
 * @tc.desc: Test NotifyMultiThreadError skips signal 42 and falls back to
 *           plain hilog when no handler is installed (SIG_DFL), so the
 *           real-time signal default action never terminates the process
 * @tc.type: FUNC
 */
HWTEST_F(MultiThreadErrorTest, NotifyMultiThreadErrorNoHandlerTest001, testing::ext::TestSize.Level1)
{
    ASSERT_NE(engine_, nullptr);
    DetectionConfigGuard configGuard;
    DefaultHandlerGuard defaultGuard;
    LoggerCollector collector;
    SetDetectionConfigForTest(1, 0);

    ResetCapture();
    NotifyMultiThreadError(engine_, "report without handler");

    ASSERT_EQ(g_signalCount, 0);
    ASSERT_TRUE(collector.Includes("no handler for signal 42"));
}

/**
 * @tc.name: LogIfSpecialIntegrationTest001
 * @tc.desc: Test the LOG_IF_SPECIAL else-branch full chain on a valid
 *           engine: the formatted message reaches the faultlogger signal
 *           together with the appended hybrid stack
 * @tc.type: FUNC
 */
HWTEST_F(MultiThreadErrorTest, LogIfSpecialIntegrationTest001, testing::ext::TestSize.Level1)
{
    ASSERT_NE(engine_, nullptr);
    DetectionConfigGuard configGuard;
    SignalHandlerGuard handlerGuard;
    SetDetectionConfigForTest(1, 0);

    ResetCapture();
    LOG_IF_SPECIAL(engine_, false, "current napi interface cannot run in multi-thread, id: %{public}d", 42);

    ASSERT_EQ(g_signalCount, 1);
    const std::string expectedPrefix = "current napi interface cannot run in multi-thread, id: 42";
    ASSERT_EQ(std::string(g_capturedMsg).substr(0, expectedPrefix.size()), expectedPrefix);
    ASSERT_NE(std::string(g_capturedMsg).find("\n"), std::string::npos);
}

/**
 * @tc.name: LogIfSpecialNullEngineTest001
 * @tc.desc: Test the LOG_IF_SPECIAL else-branch with a null engine: the report
 *           skips the hybrid stack and appends the backtrace-failure marker,
 *           and NotifyMultiThreadError's null path bypasses both gates so the
 *           signal is still delivered
 * @tc.type: FUNC
 */
HWTEST_F(MultiThreadErrorTest, LogIfSpecialNullEngineTest001, testing::ext::TestSize.Level1)
{
    DetectionConfigGuard configGuard;
    SignalHandlerGuard handlerGuard;
    SetDetectionConfigForTest(1000000, 1440);

    ResetCapture();
    LOG_IF_SPECIAL(nullptr, false, "null engine report");

    ASSERT_EQ(g_signalCount, 1);
    ASSERT_NE(std::string(g_capturedMsg).find("null engine report"), std::string::npos);
    ASSERT_NE(std::string(g_capturedMsg).find("Failed to backtrace."), std::string::npos);
}

/**
 * @tc.name: NotifyMultiThreadErrorFrequencyDisabledTest001
 * @tc.desc: Test the frequency gate bypass: frequency < 1 disables the gate
 *           so consecutive detections are all reported
 * @tc.type: FUNC
 */
HWTEST_F(MultiThreadErrorTest, NotifyMultiThreadErrorFrequencyDisabledTest001, testing::ext::TestSize.Level1)
{
    DetectionConfigGuard configGuard;
    SignalHandlerGuard handlerGuard;
    SetDetectionConfigForTest(0, 0);

    ResetCapture();
    NotifyMultiThreadError(engine_, "frequency disabled first");
    NotifyMultiThreadError(engine_, "frequency disabled second");

    ASSERT_EQ(g_signalCount, 2);
}

/**
 * @tc.name: NotifyMultiThreadErrorIgnoredHandlerTest001
 * @tc.desc: Test NotifyMultiThreadError treats SIG_IGN like a missing
 *           handler: no signal is sent, the report falls back to hilog and
 *           the process stays alive
 * @tc.type: FUNC
 */
HWTEST_F(MultiThreadErrorTest, NotifyMultiThreadErrorIgnoredHandlerTest001, testing::ext::TestSize.Level1)
{
    DetectionConfigGuard configGuard;
    IgnoreHandlerGuard ignoreGuard;
    LoggerCollector collector;
    SetDetectionConfigForTest(1, 0);

    ResetCapture();
    NotifyMultiThreadError(engine_, "report with ignored handler");

    ASSERT_EQ(g_signalCount, 0);
    ASSERT_TRUE(collector.Includes("no handler for signal 42"));
}

/**
 * @tc.name: NotifyMultiThreadErrorPlainHandlerTest001
 * @tc.desc: Test a non-SA_SIGINFO handler still counts as available: the
 *           report is delivered through signal 42 to the plain handler
 * @tc.type: FUNC
 */
HWTEST_F(MultiThreadErrorTest, NotifyMultiThreadErrorPlainHandlerTest001, testing::ext::TestSize.Level1)
{
    DetectionConfigGuard configGuard;
    PlainHandlerGuard plainGuard;
    SetDetectionConfigForTest(1, 0);

    ResetCapture();
    NotifyMultiThreadError(engine_, "report with plain handler");

    ASSERT_EQ(g_plainSignalCount, 1);
}

/**
 * @tc.name: NotifyMultiThreadErrorFallbackThrottleTest001
 * @tc.desc: Test a hilog-only fallback still claims the interval window:
 *           after the fallback report a 1440-minute interval blocks further
 *           reports and interval=0 releases the window again
 * @tc.type: FUNC
 */
HWTEST_F(MultiThreadErrorTest, NotifyMultiThreadErrorFallbackThrottleTest001, testing::ext::TestSize.Level1)
{
    DetectionConfigGuard configGuard;
    DefaultHandlerGuard defaultGuard;
    LoggerCollector collector;
    SetDetectionConfigForTest(1, 0);

    ResetCapture();
    NotifyMultiThreadError(engine_, "fallback claims window");
    ASSERT_EQ(g_signalCount, 0);
    ASSERT_TRUE(collector.Includes("no handler for signal 42"));

    SignalHandlerGuard handlerGuard;
    SetDetectionConfigForTest(1, 1440);
    ResetCapture();
    NotifyMultiThreadError(engine_, "within claimed window");
    ASSERT_EQ(g_signalCount, 0);

    SetDetectionConfigForTest(1, 0);
    ResetCapture();
    NotifyMultiThreadError(engine_, "window released");
    ASSERT_EQ(g_signalCount, 1);
}

/**
 * @tc.name: NotifyMultiThreadErrorNullEngineTest001
 * @tc.desc: Test a null engine bypasses both gates: with frequency and
 *           interval set to blocking values the report is still sent and
 *           carries the exact message
 * @tc.type: FUNC
 */
HWTEST_F(MultiThreadErrorTest, NotifyMultiThreadErrorNullEngineTest001, testing::ext::TestSize.Level1)
{
    DetectionConfigGuard configGuard;
    SignalHandlerGuard handlerGuard;
    SetDetectionConfigForTest(1000000, 1440);

    ResetCapture();
    NotifyMultiThreadError(nullptr, "null engine report");

    ASSERT_EQ(g_signalCount, 1);
    ASSERT_STREQ(g_capturedMsg, "null engine report");
}

/**
 * @tc.name: NotifyMultiThreadErrorConcurrentTest001
 * @tc.desc: Test the frequency gate under concurrency: after calibration
 *           with frequency equal to the thread count, simultaneous reports
 *           from all threads yield exactly one signal because the counter
 *           assigns each caller a unique slot
 * @tc.type: FUNC
 */
HWTEST_F(MultiThreadErrorTest, NotifyMultiThreadErrorConcurrentTest001, testing::ext::TestSize.Level1)
{
    constexpr size_t threadNum = 4;
    DetectionConfigGuard configGuard;
    SignalHandlerGuard handlerGuard;
    SetDetectionConfigForTest(static_cast<int64_t>(threadNum), 0);

    ResetCapture();
    constexpr int loopLimit = 100;
    int loops = 0;
    while (g_signalCount == 0 && loops < loopLimit) {
        NotifyMultiThreadError(engine_, "concurrent calibration");
        ++loops;
    }
    ASSERT_GT(g_signalCount, 0);
    ResetCapture();

    std::atomic<size_t> readyCount {0};
    std::atomic<bool> go {false};
    std::vector<std::thread> workers;
    for (size_t i = 0; i < threadNum; ++i) {
        workers.emplace_back([&]() {
            readyCount.fetch_add(1, std::memory_order_relaxed);
            while (!go.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            NotifyMultiThreadError(engine_, "concurrent report");
        });
    }
    while (readyCount.load(std::memory_order_relaxed) < threadNum) {
        std::this_thread::yield();
    }
    go.store(true, std::memory_order_release);
    for (auto &worker : workers) {
        worker.join();
    }

    ASSERT_EQ(g_signalCount, 1);
}

/**
 * @tc.name: ValidEngineCheckReportTest001
 * @tc.desc: Test both VALID_ENGINE_CHECK mismatch paths report through
 *           signal 42 with the fault message prefix when the engine reports
 *           the cross-thread check as disabled (soft-report branch)
 * @tc.type: FUNC
 */
HWTEST_F(MultiThreadErrorTest, ValidEngineCheckReportTest001, testing::ext::TestSize.Level1)
{
    panda::RuntimeOption option;
    option.SetArkProperties(0x105c);
    option.SetGcType(panda::RuntimeOption::GC_TYPE::GEN_GC);
    const int64_t poolSize = 0x1000000;
    option.SetGcPoolSize(poolSize);
    option.SetLogLevel(panda::RuntimeOption::LOG_LEVEL::ERROR);
    option.SetDebuggerLibraryPath("");
    EcmaVM* vm = panda::JSNApi::CreateJSVM(option);
    ASSERT_NE(vm, nullptr);
    SoftReportEngine engine(vm, nullptr);
    engine.SetCleanEnv([vm]() { panda::JSNApi::DestroyJSVM(vm); });
    ASSERT_FALSE(engine.IsCrossThreadCheckEnabled());
    DetectionConfigGuard configGuard;
    SignalHandlerGuard handlerGuard;
    SetDetectionConfigForTest(1, 0);

    ResetCapture();
    ASSERT_FALSE(VALID_ENGINE_CHECK(&engine, nullptr, 0));
    ASSERT_EQ(g_signalCount, 1);
    const std::string ownerPrefix = "param env not equal to its owner.";
    ASSERT_EQ(std::string(g_capturedMsg).substr(0, ownerPrefix.size()), ownerPrefix);

    ResetCapture();
    uint64_t ownerId = engine.GetId() + 1;
    ASSERT_FALSE(VALID_ENGINE_CHECK(&engine, &engine, ownerId));
    ASSERT_EQ(g_signalCount, 1);
    const std::string destroyedPrefix = "owner env has been destroyed, owner id: " + std::to_string(ownerId) +
        ", current env id: " + std::to_string(engine.GetId()) + ".";
    ASSERT_EQ(std::string(g_capturedMsg).substr(0, destroyedPrefix.size()), destroyedPrefix);
}

/**
 * @tc.name: LogIfSpecialFatalBranchTest001
 * @tc.desc: Test the LOG_IF_SPECIAL true-branch routes to HILOG_FATAL and
 *           aborts the (child) process instead of reporting through
 *           signal 42
 * @tc.type: FUNC
 */
HWTEST_F(MultiThreadErrorTest, LogIfSpecialFatalBranchTest001, testing::ext::TestSize.Level1)
{
    BasicDeathTest deathTest([this]() {
        LOG_IF_SPECIAL(engine_, true, "LOG_IF_SPECIAL fatal branch test");
    });
    deathTest.AssertSignal(SIGABRT);
    ASSERT_TRUE(deathTest.GetResult());
}

/**
 * @tc.name: NotifyMultiThreadErrorThrottleLogTest001
 * @tc.desc: Test the gates log the suppression reason at error level: a
 *           detection blocked by the frequency gate or the interval gate
 *           emits a hilog error line while no signal is sent
 * @tc.type: FUNC
 */
HWTEST_F(MultiThreadErrorTest, NotifyMultiThreadErrorThrottleLogTest001, testing::ext::TestSize.Level1)
{
    DetectionConfigGuard configGuard;
    SignalHandlerGuard handlerGuard;
    LoggerCollector collector(LogLevel::LOG_ERROR);
    SetDetectionConfigForTest(2, 0);

    ResetCapture();
    constexpr int loopLimit = 100;
    int loops = 0;
    while (g_signalCount == 0 && loops < loopLimit) {
        NotifyMultiThreadError(engine_, "throttle log");
        ++loops;
    }
    ASSERT_GT(g_signalCount, 0);

    ResetCapture();
    NotifyMultiThreadError(engine_, "throttle log");
    ASSERT_EQ(g_signalCount, 0);
    ASSERT_TRUE(collector.Includes("blocked by frequency gate"));

    SetDetectionConfigForTest(1, 1440);
    ResetCapture();
    NotifyMultiThreadError(engine_, "throttle log");
    ASSERT_EQ(g_signalCount, 0);
    ASSERT_TRUE(collector.Includes("blocked by interval gate"));
}
#endif // OHOS_PLATFORM
