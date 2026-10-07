#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <chrono>
#include <memory>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <algorithm>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#ifdef ERROR
#undef ERROR
#endif
#endif

#include "TzdInterpreter.h"
#include "TzdOop.h"

namespace TzdThreading {

enum class ThreadState {
    NEW = 0,
    RUNNING = 1,
    BLOCKED = 2,
    SLEEPING = 3,
    PAUSED = 4,
    TERMINATED = 5,
    FAILED = 6
};

inline std::string threadStateToString(ThreadState s) {
    switch (s) {
    case ThreadState::NEW: return "NEW";
    case ThreadState::RUNNING: return "RUNNING";
    case ThreadState::BLOCKED: return "BLOCKED";
    case ThreadState::SLEEPING: return "SLEEPING";
    case ThreadState::PAUSED: return "PAUSED";
    case ThreadState::TERMINATED: return "TERMINATED";
    case ThreadState::FAILED: return "ERROR";
    default: return "UNKNOWN";
    }
}

inline int64_t nowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

inline std::string getIsoTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::stringstream ss;
    struct tm timeinfo;
#ifdef _WIN32
    localtime_s(&timeinfo, &in_time_t);
#else
    localtime_r(&in_time_t, &timeinfo);
#endif
    ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S")
       << '.' << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

struct ThreadLogEntry {
    int64_t timestamp = 0;
    std::string timeStr;
    uint64_t threadId = 0;
    std::string threadName;
    std::string eventType;
    std::string message;

    TzdValue toMap() const {
        std::unordered_map<std::string, TzdValue> m;
        m["timestamp"] = TzdValue((double)timestamp);
        m["time"] = TzdValue(timeStr);
        m["thread_id"] = TzdValue((double)threadId);
        m["thread_name"] = TzdValue(threadName);
        m["event"] = TzdValue(eventType);
        m["message"] = TzdValue(message);
        return TzdValue(m);
    }
};

struct ThreadControlBlock {
    uint64_t id = 0;
    uint32_t osThreadId = 0;
    std::string name;
    std::atomic<ThreadState> state{ThreadState::NEW};
    uint64_t parentThreadId = 0;
    std::string targetName;
    TzdValue target;
    std::vector<TzdValue> args;
    TzdValue result;

    bool hasError = false;
    std::string errorMessage;
    std::vector<std::string> errorStackTrace;

    int64_t startTimestamp = 0;
    int64_t endTimestamp = 0;

    std::thread* sysThread = nullptr;
    TzdInterpreter* childInterp = nullptr;
    bool isDetached = false;
    bool isFinished = false;

    // Pausing / Debugging
    std::atomic<bool> isPaused{false};
    std::condition_variable pauseCv;
    std::mutex pauseMutex;

    // Lifecycle
    mutable std::mutex tcbMutex;
    std::condition_variable joinCv;

    // Mailbox
    std::queue<TzdValue> mailbox;
    std::mutex mailboxMutex;
    std::condition_variable mailboxCv;

    // Mutex tracking for Deadlock Detection
    std::atomic<uint64_t> waitingOnMutexId{0};
    std::unordered_set<uint64_t> heldMutexIds;

    std::vector<std::string> getStackTrace() const {
        if (hasError && !errorStackTrace.empty()) {
            return errorStackTrace;
        }
        if (childInterp) {
            return childInterp->getCallStackSnapshot();
        }
        return {};
    }

    void sendMailbox(const TzdValue& msg) {
        {
            std::lock_guard<std::mutex> lock(mailboxMutex);
            mailbox.push(msg);
        }
        mailboxCv.notify_one();
    }

    bool receiveMailbox(TzdValue& outMsg, int64_t timeoutMs) {
        std::unique_lock<std::mutex> lock(mailboxMutex);
        if (timeoutMs < 0) {
            mailboxCv.wait(lock, [this]() { return !mailbox.empty(); });
            outMsg = mailbox.front();
            mailbox.pop();
            return true;
        } else if (timeoutMs == 0) {
            if (!mailbox.empty()) {
                outMsg = mailbox.front();
                mailbox.pop();
                return true;
            }
            return false;
        } else {
            bool ok = mailboxCv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this]() { return !mailbox.empty(); });
            if (ok) {
                outMsg = mailbox.front();
                mailbox.pop();
                return true;
            }
            return false;
        }
    }

    bool tryReceiveMailbox(TzdValue& outMsg) {
        std::lock_guard<std::mutex> lock(mailboxMutex);
        if (!mailbox.empty()) {
            outMsg = mailbox.front();
            mailbox.pop();
            return true;
        }
        return false;
    }

    size_t getMailboxSize() {
        std::lock_guard<std::mutex> lock(mailboxMutex);
        return mailbox.size();
    }

    void pause() {
        isPaused = true;
        state = ThreadState::PAUSED;
    }

    void resume() {
        isPaused = false;
        state = ThreadState::RUNNING;
        pauseCv.notify_all();
    }

    void checkPause() {
        if (isPaused.load()) {
            std::unique_lock<std::mutex> lock(pauseMutex);
            while (isPaused.load()) {
                state = ThreadState::PAUSED;
                pauseCv.wait(lock);
            }
            state = ThreadState::RUNNING;
        }
    }

    bool join(int64_t timeoutMs = -1) {
        std::unique_lock<std::mutex> lock(tcbMutex);
        if (isFinished) {
            if (sysThread && sysThread->joinable()) {
                sysThread->join();
            }
            return true;
        }
        if (timeoutMs < 0) {
            joinCv.wait(lock, [this]() { return isFinished; });
            if (sysThread && sysThread->joinable()) {
                sysThread->join();
            }
            return true;
        } else {
            bool ok = joinCv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this]() { return isFinished; });
            if (ok && sysThread && sysThread->joinable()) {
                sysThread->join();
            }
            return ok;
        }
    }

    void detach() {
        std::lock_guard<std::mutex> lock(tcbMutex);
        isDetached = true;
        if (sysThread && sysThread->joinable()) {
            sysThread->detach();
        }
    }

    TzdValue toMap() const {
        std::unordered_map<std::string, TzdValue> m;
        m["id"] = TzdValue((double)id);
        m["os_id"] = TzdValue((double)osThreadId);
        m["name"] = TzdValue(name);
        m["state"] = TzdValue((double)(int)state.load());
        m["state_str"] = TzdValue(threadStateToString(state.load()));
        m["parent_id"] = TzdValue((double)parentThreadId);
        m["target"] = TzdValue(targetName);
        m["start_time"] = TzdValue((double)startTimestamp);
        m["end_time"] = TzdValue((double)endTimestamp);
        int64_t dur = (endTimestamp > 0 ? endTimestamp : (startTimestamp > 0 ? nowMs() : 0)) - startTimestamp;
        m["duration_ms"] = TzdValue((double)(dur > 0 ? dur : 0));
        m["is_alive"] = TzdValue(state.load() == ThreadState::RUNNING || state.load() == ThreadState::BLOCKED || state.load() == ThreadState::SLEEPING || state.load() == ThreadState::PAUSED);
        m["has_error"] = TzdValue(hasError);
        m["error_message"] = TzdValue(errorMessage);
        return TzdValue(m);
    }
};

// --- Native Synchronization Primitives ---

struct TzdNativeMutex {
    uint64_t id = 0;
    std::recursive_mutex mtx;
    std::atomic<uint64_t> ownerThreadId{0};
    std::atomic<int> lockCount{0};
    std::mutex trackingMutex;
    std::unordered_set<uint64_t> waitingThreads;
};

struct TzdNativeCondVar {
    uint64_t id = 0;
    std::condition_variable_any cv;
};

struct TzdNativeEvent {
    uint64_t id = 0;
    bool manualReset = false;
    std::atomic<bool> signaled{false};
    std::mutex mtx;
    std::condition_variable cv;
};

struct TzdNativeChannel {
    uint64_t id = 0;
    size_t capacity = 0;
    bool closed = false;
    std::queue<TzdValue> buffer;
    std::mutex mtx;
    std::condition_variable notFullCv;
    std::condition_variable notEmptyCv;
};

struct TzdNativeAtomic {
    uint64_t id = 0;
    std::atomic<int64_t> val{0};
};

class TzdThreadManager {
public:
    static TzdThreadManager& instance() {
        static TzdThreadManager s_mgr;
        return s_mgr;
    }

    void ensureMainThreadRegistered(TzdInterpreter* mainInterp) {
        std::lock_guard<std::recursive_mutex> lock(m_threadsMutex);
        if (m_threads.find(1) == m_threads.end()) {
            auto tcb = std::make_shared<ThreadControlBlock>();
            tcb->id = 1;
#ifdef _WIN32
            tcb->osThreadId = GetCurrentThreadId();
#endif
            tcb->name = "MainThread";
            tcb->state = ThreadState::RUNNING;
            tcb->targetName = "main";
            tcb->startTimestamp = nowMs();
            tcb->childInterp = mainInterp;
            m_threads[1] = tcb;
            m_osToThreadId[tcb->osThreadId] = 1;
            m_currentTcb = tcb;
            log(1, "MainThread", "INIT", "Main thread registered");
        }
    }

    void registerCurrentThread(std::shared_ptr<ThreadControlBlock> tcb) {
        m_currentTcb = tcb;
        std::lock_guard<std::recursive_mutex> lock(m_threadsMutex);
        m_osToThreadId[tcb->osThreadId] = tcb->id;
    }

    uint64_t getCurrentThreadId() {
        if (m_currentTcb) {
            return m_currentTcb->id;
        }
#ifdef _WIN32
        uint32_t osId = GetCurrentThreadId();
        std::lock_guard<std::recursive_mutex> lock(m_threadsMutex);
        auto it = m_osToThreadId.find(osId);
        if (it != m_osToThreadId.end()) {
            return it->second;
        }
#endif
        return 1;
    }

    std::shared_ptr<ThreadControlBlock> getCurrentThread() {
        if (m_currentTcb) return m_currentTcb;
        uint64_t tid = getCurrentThreadId();
        return getThread(tid);
    }

    std::shared_ptr<ThreadControlBlock> getThread(uint64_t id) {
        std::lock_guard<std::recursive_mutex> lock(m_threadsMutex);
        auto it = m_threads.find(id);
        if (it != m_threads.end()) return it->second;
        return nullptr;
    }

    std::shared_ptr<ThreadControlBlock> createThread(
        const TzdValue& target,
        const std::vector<TzdValue>& args,
        const std::string& name,
        TzdInterpreter* parentInterp)
    {
        uint64_t id = m_nextThreadId.fetch_add(1);
        auto tcb = std::make_shared<ThreadControlBlock>();
        tcb->id = id;
        tcb->name = name.empty() ? ("Thread-" + std::to_string(id)) : name;
        tcb->target = target;
        tcb->targetName = target.name.empty() ? "<anonymous>" : target.name;
        tcb->args = args;
        tcb->parentThreadId = getCurrentThreadId();
        tcb->state = ThreadState::NEW;

        tcb->childInterp = new TzdInterpreter();
        if (parentInterp && !parentInterp->scopes.empty()) {
            tcb->childInterp->scopes[0] = parentInterp->scopes[0];
        }

        {
            std::lock_guard<std::recursive_mutex> lock(m_threadsMutex);
            m_threads[id] = tcb;
        }

        log(id, tcb->name, "CREATE", "Thread created (parent ID: " + std::to_string(tcb->parentThreadId) + ")");
        return tcb;
    }

    bool startThread(std::shared_ptr<ThreadControlBlock> tcb) {
        if (!tcb || tcb->state != ThreadState::NEW) return false;

        tcb->sysThread = new std::thread([this, tcb]() {
#ifdef _WIN32
            tcb->osThreadId = GetCurrentThreadId();
#endif
            registerCurrentThread(tcb);
            g_CurrentInterpreter = tcb->childInterp;

            tcb->startTimestamp = nowMs();
            tcb->state = ThreadState::RUNNING;
            log(tcb->id, tcb->name, "START", "Thread started execution");

            try {
                TzdValue res = tcb->childInterp->callFunction(tcb->target, tcb->args);
                tcb->result = res;
                tcb->state = ThreadState::TERMINATED;
                log(tcb->id, tcb->name, "FINISH", "Thread finished successfully");
            }
            catch (const TzdRuntimeException& e) {
                tcb->hasError = true;
                tcb->errorMessage = e.what();
                tcb->errorStackTrace = e.stackTrace;
                if (tcb->errorStackTrace.empty() && tcb->childInterp) {
                    tcb->errorStackTrace = tcb->childInterp->getCallStackSnapshot();
                }
                tcb->state = ThreadState::FAILED;
                log(tcb->id, tcb->name, "ERROR", "Thread runtime error: " + std::string(e.what()));
            }
            catch (const std::exception& e) {
                tcb->hasError = true;
                tcb->errorMessage = e.what();
                if (tcb->childInterp) {
                    tcb->errorStackTrace = tcb->childInterp->getCallStackSnapshot();
                }
                tcb->state = ThreadState::FAILED;
                log(tcb->id, tcb->name, "ERROR", "Thread exception: " + std::string(e.what()));
            }
            catch (...) {
                tcb->hasError = true;
                tcb->errorMessage = "Unknown exception in thread";
                tcb->state = ThreadState::FAILED;
                log(tcb->id, tcb->name, "ERROR", "Thread unknown exception");
            }

            tcb->endTimestamp = nowMs();
            {
                std::lock_guard<std::mutex> lock(tcb->tcbMutex);
                tcb->isFinished = true;
            }
            tcb->joinCv.notify_all();

            if (tcb->isDetached) {
                if (tcb->childInterp) {
                    delete tcb->childInterp;
                    tcb->childInterp = nullptr;
                }
            }
        });

        return true;
    }

    void checkCurrentThreadPause() {
        if (m_currentTcb) {
            m_currentTcb->checkPause();
        }
    }

    std::vector<uint64_t> getAllThreadIds() {
        std::lock_guard<std::recursive_mutex> lock(m_threadsMutex);
        std::vector<uint64_t> ids;
        for (const auto& pair : m_threads) ids.push_back(pair.first);
        return ids;
    }

    std::vector<TzdValue> getAllThreadInfos() {
        std::lock_guard<std::recursive_mutex> lock(m_threadsMutex);
        std::vector<TzdValue> list;
        for (const auto& pair : m_threads) {
            list.push_back(pair.second->toMap());
        }
        return list;
    }

    // --- Logging ---
    void log(uint64_t threadId, const std::string& threadName, const std::string& eventType, const std::string& msg) {
        ThreadLogEntry entry;
        entry.timestamp = nowMs();
        entry.timeStr = getIsoTimestamp();
        entry.threadId = threadId;
        entry.threadName = threadName;
        entry.eventType = eventType;
        entry.message = msg;

        std::lock_guard<std::mutex> lock(m_logsMutex);
        m_logs.push_back(entry);
        if (m_logs.size() > m_maxLogs) {
            m_logs.pop_front();
        }
    }

    std::vector<TzdValue> getLogs(uint64_t filterThreadId = 0, size_t limit = 100) {
        std::lock_guard<std::mutex> lock(m_logsMutex);
        std::vector<TzdValue> result;
        size_t count = 0;
        for (auto it = m_logs.rbegin(); it != m_logs.rend() && count < limit; ++it) {
            if (filterThreadId == 0 || it->threadId == filterThreadId) {
                result.push_back(it->toMap());
                count++;
            }
        }
        std::reverse(result.begin(), result.end());
        return result;
    }

    void clearLogs() {
        std::lock_guard<std::mutex> lock(m_logsMutex);
        m_logs.clear();
    }

    // --- Deadlock Detection ---
    std::vector<std::unordered_map<std::string, TzdValue>> detectDeadlocks() {
        std::lock_guard<std::recursive_mutex> lock(m_threadsMutex);
        std::unordered_map<uint64_t, uint64_t> waitFor;
        std::unordered_map<uint64_t, uint64_t> waitingMutexMap;

        for (const auto& pair : m_threads) {
            auto tcb = pair.second;
            uint64_t mutId = tcb->waitingOnMutexId.load();
            if (mutId > 0) {
                std::lock_guard<std::mutex> mtxLock(m_syncMutex);
                auto it = m_mutexes.find(mutId);
                if (it != m_mutexes.end()) {
                    uint64_t owner = it->second->ownerThreadId.load();
                    if (owner > 0 && owner != tcb->id) {
                        waitFor[tcb->id] = owner;
                        waitingMutexMap[tcb->id] = mutId;
                    }
                }
            }
        }

        std::vector<std::unordered_map<std::string, TzdValue>> deadlocks;
        std::unordered_set<uint64_t> visited;

        for (const auto& entry : waitFor) {
            uint64_t start = entry.first;
            if (visited.count(start)) continue;

            std::vector<uint64_t> path;
            std::unordered_set<uint64_t> inPath;
            uint64_t curr = start;

            while (curr > 0 && waitFor.count(curr)) {
                if (inPath.count(curr)) {
                    auto it = std::find(path.begin(), path.end(), curr);
                    for (; it != path.end(); ++it) {
                        uint64_t tid = *it;
                        visited.insert(tid);
                        std::unordered_map<std::string, TzdValue> node;
                        node["thread_id"] = TzdValue((double)tid);
                        auto tIt = m_threads.find(tid);
                        node["thread_name"] = TzdValue(tIt != m_threads.end() ? tIt->second->name : "");
                        node["waiting_on_mutex"] = TzdValue((double)waitingMutexMap[tid]);
                        node["blocked_by_thread"] = TzdValue((double)waitFor[tid]);
                        deadlocks.push_back(node);
                    }
                    break;
                }
                path.push_back(curr);
                inPath.insert(curr);
                curr = waitFor[curr];
            }
        }
        return deadlocks;
    }

    std::string dumpAllStackTraces() {
        std::lock_guard<std::recursive_mutex> lock(m_threadsMutex);
        std::ostringstream oss;
        oss << "=== TzdLanguage Thread Dump (" << getIsoTimestamp() << ") ===\n";
        oss << "Total threads tracked: " << m_threads.size() << "\n\n";

        for (const auto& pair : m_threads) {
            auto tcb = pair.second;
            oss << "Thread \"" << tcb->name << "\" [ID: " << tcb->id
                << ", OS_TID: " << tcb->osThreadId << "] state: "
                << threadStateToString(tcb->state.load());

            if (tcb->waitingOnMutexId.load() > 0) {
                oss << " (waiting on Mutex #" << tcb->waitingOnMutexId.load() << ")";
            }
            if (!tcb->heldMutexIds.empty()) {
                oss << " (holding mutexes: ";
                for (uint64_t mId : tcb->heldMutexIds) oss << "#" << mId << " ";
                oss << ")";
            }
            oss << "\n";

            if (tcb->hasError) {
                oss << "  ** Error: " << tcb->errorMessage << " **\n";
            }

            std::vector<std::string> trace = tcb->getStackTrace();
            if (trace.empty()) {
                oss << "  (no stack frames)\n";
            } else {
                for (auto it = trace.rbegin(); it != trace.rend(); ++it) {
                    oss << "    at " << *it << "\n";
                }
            }
            oss << "\n";
        }

        auto deadlocks = detectDeadlocks();
        if (!deadlocks.empty()) {
            oss << "!!! DEADLOCK DETECTED !!!\n";
            for (const auto& dl : deadlocks) {
                oss << "  Thread #" << (uint64_t)dl.at("thread_id").dVal
                    << " (\"" << dl.at("thread_name").sVal << "\") is waiting on Mutex #"
                    << (uint64_t)dl.at("waiting_on_mutex").dVal << " held by Thread #"
                    << (uint64_t)dl.at("blocked_by_thread").dVal << "\n";
            }
            oss << "\n";
        } else {
            oss << "No deadlocks detected.\n";
        }
        return oss.str();
    }

    // --- Mutex Management ---
    uint64_t createMutex() {
        std::lock_guard<std::mutex> lock(m_syncMutex);
        uint64_t id = m_nextSyncId.fetch_add(1);
        auto m = std::make_shared<TzdNativeMutex>();
        m->id = id;
        m_mutexes[id] = m;
        return id;
    }

    bool lockMutex(uint64_t mutexId) {
        std::shared_ptr<TzdNativeMutex> m;
        {
            std::lock_guard<std::mutex> lock(m_syncMutex);
            auto it = m_mutexes.find(mutexId);
            if (it == m_mutexes.end()) return false;
            m = it->second;
        }

        auto tcb = getCurrentThread();
        uint64_t tid = tcb ? tcb->id : getCurrentThreadId();
        if (tcb) {
            tcb->waitingOnMutexId = mutexId;
            tcb->state = ThreadState::BLOCKED;
        }
        {
            std::lock_guard<std::mutex> tLock(m->trackingMutex);
            m->waitingThreads.insert(tid);
        }

        m->mtx.lock();

        if (tcb) {
            tcb->waitingOnMutexId = 0;
            tcb->state = ThreadState::RUNNING;
            tcb->heldMutexIds.insert(mutexId);
        }
        {
            std::lock_guard<std::mutex> tLock(m->trackingMutex);
            m->waitingThreads.erase(tid);
        }
        m->ownerThreadId = tid;
        m->lockCount++;
        return true;
    }

    bool unlockMutex(uint64_t mutexId) {
        std::shared_ptr<TzdNativeMutex> m;
        {
            std::lock_guard<std::mutex> lock(m_syncMutex);
            auto it = m_mutexes.find(mutexId);
            if (it == m_mutexes.end()) return false;
            m = it->second;
        }

        auto tcb = getCurrentThread();
        if (m->lockCount > 0) {
            m->lockCount--;
            if (m->lockCount == 0) {
                m->ownerThreadId = 0;
                if (tcb) {
                    tcb->heldMutexIds.erase(mutexId);
                }
            }
            m->mtx.unlock();
            return true;
        }
        return false;
    }

    bool tryLockMutex(uint64_t mutexId, int64_t timeoutMs = 0) {
        (void)timeoutMs;
        std::shared_ptr<TzdNativeMutex> m;
        {
            std::lock_guard<std::mutex> lock(m_syncMutex);
            auto it = m_mutexes.find(mutexId);
            if (it == m_mutexes.end()) return false;
            m = it->second;
        }

        if (m->mtx.try_lock()) {
            auto tcb = getCurrentThread();
            uint64_t tid = tcb ? tcb->id : getCurrentThreadId();
            if (tcb) {
                tcb->heldMutexIds.insert(mutexId);
            }
            m->ownerThreadId = tid;
            m->lockCount++;
            return true;
        }
        return false;
    }

    void destroyMutex(uint64_t mutexId) {
        std::lock_guard<std::mutex> lock(m_syncMutex);
        m_mutexes.erase(mutexId);
    }

    // --- CondVar Management ---
    uint64_t createCondVar() {
        std::lock_guard<std::mutex> lock(m_syncMutex);
        uint64_t id = m_nextSyncId.fetch_add(1);
        auto cv = std::make_shared<TzdNativeCondVar>();
        cv->id = id;
        m_condVars[id] = cv;
        return id;
    }

    bool waitCondVar(uint64_t condId, uint64_t mutexId, int64_t timeoutMs = -1) {
        std::shared_ptr<TzdNativeCondVar> cv;
        std::shared_ptr<TzdNativeMutex> m;
        {
            std::lock_guard<std::mutex> lock(m_syncMutex);
            auto itC = m_condVars.find(condId);
            auto itM = m_mutexes.find(mutexId);
            if (itC == m_condVars.end() || itM == m_mutexes.end()) return false;
            cv = itC->second;
            m = itM->second;
        }

        auto tcb = getCurrentThread();
        if (tcb) tcb->state = ThreadState::BLOCKED;

        bool result = false;
        if (timeoutMs < 0) {
            cv->cv.wait(m->mtx);
            result = true;
        } else {
            auto status = cv->cv.wait_for(m->mtx, std::chrono::milliseconds(timeoutMs));
            result = (status == std::cv_status::no_timeout);
        }

        if (tcb) tcb->state = ThreadState::RUNNING;
        return result;
    }

    void notifyCondVar(uint64_t condId) {
        std::shared_ptr<TzdNativeCondVar> cv;
        {
            std::lock_guard<std::mutex> lock(m_syncMutex);
            auto it = m_condVars.find(condId);
            if (it != m_condVars.end()) cv = it->second;
        }
        if (cv) cv->cv.notify_one();
    }

    void notifyAllCondVar(uint64_t condId) {
        std::shared_ptr<TzdNativeCondVar> cv;
        {
            std::lock_guard<std::mutex> lock(m_syncMutex);
            auto it = m_condVars.find(condId);
            if (it != m_condVars.end()) cv = it->second;
        }
        if (cv) cv->cv.notify_all();
    }

    void destroyCondVar(uint64_t condId) {
        std::lock_guard<std::mutex> lock(m_syncMutex);
        m_condVars.erase(condId);
    }

    // --- Event Management ---
    uint64_t createEvent(bool manualReset = false) {
        std::lock_guard<std::mutex> lock(m_syncMutex);
        uint64_t id = m_nextSyncId.fetch_add(1);
        auto ev = std::make_shared<TzdNativeEvent>();
        ev->id = id;
        ev->manualReset = manualReset;
        m_events[id] = ev;
        return id;
    }

    bool setEvent(uint64_t eventId) {
        std::shared_ptr<TzdNativeEvent> ev;
        {
            std::lock_guard<std::mutex> lock(m_syncMutex);
            auto it = m_events.find(eventId);
            if (it != m_events.end()) ev = it->second;
        }
        if (!ev) return false;
        {
            std::lock_guard<std::mutex> lock(ev->mtx);
            ev->signaled = true;
        }
        ev->cv.notify_all();
        return true;
    }

    bool resetEvent(uint64_t eventId) {
        std::shared_ptr<TzdNativeEvent> ev;
        {
            std::lock_guard<std::mutex> lock(m_syncMutex);
            auto it = m_events.find(eventId);
            if (it != m_events.end()) ev = it->second;
        }
        if (!ev) return false;
        std::lock_guard<std::mutex> lock(ev->mtx);
        ev->signaled = false;
        return true;
    }

    bool waitEvent(uint64_t eventId, int64_t timeoutMs = -1) {
        std::shared_ptr<TzdNativeEvent> ev;
        {
            std::lock_guard<std::mutex> lock(m_syncMutex);
            auto it = m_events.find(eventId);
            if (it != m_events.end()) ev = it->second;
        }
        if (!ev) return false;

        auto tcb = getCurrentThread();
        if (tcb) tcb->state = ThreadState::BLOCKED;

        std::unique_lock<std::mutex> lock(ev->mtx);
        bool ok = false;
        if (timeoutMs < 0) {
            ev->cv.wait(lock, [ev]() { return ev->signaled.load(); });
            ok = true;
        } else {
            ok = ev->cv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [ev]() { return ev->signaled.load(); });
        }

        if (ok && !ev->manualReset) {
            ev->signaled = false;
        }
        if (tcb) tcb->state = ThreadState::RUNNING;
        return ok;
    }

    bool isEventSet(uint64_t eventId) {
        std::lock_guard<std::mutex> lock(m_syncMutex);
        auto it = m_events.find(eventId);
        if (it != m_events.end()) {
            return it->second->signaled.load();
        }
        return false;
    }

    void destroyEvent(uint64_t eventId) {
        std::lock_guard<std::mutex> lock(m_syncMutex);
        m_events.erase(eventId);
    }

    // --- Channel Management ---
    uint64_t createChannel(size_t capacity = 0) {
        std::lock_guard<std::mutex> lock(m_syncMutex);
        uint64_t id = m_nextSyncId.fetch_add(1);
        auto ch = std::make_shared<TzdNativeChannel>();
        ch->id = id;
        ch->capacity = (capacity == 0) ? 1 : capacity; // 0 is handled as 1-capacity rendezvous
        m_channels[id] = ch;
        return id;
    }

    bool sendChannel(uint64_t chanId, const TzdValue& val, int64_t timeoutMs = -1) {
        std::shared_ptr<TzdNativeChannel> ch;
        {
            std::lock_guard<std::mutex> lock(m_syncMutex);
            auto it = m_channels.find(chanId);
            if (it != m_channels.end()) ch = it->second;
        }
        if (!ch) return false;

        auto tcb = getCurrentThread();
        std::unique_lock<std::mutex> lock(ch->mtx);
        if (ch->closed) return false;

        auto canSend = [ch]() { return ch->closed || ch->buffer.size() < ch->capacity; };
        if (timeoutMs < 0) {
            if (tcb) tcb->state = ThreadState::BLOCKED;
            ch->notFullCv.wait(lock, canSend);
            if (tcb) tcb->state = ThreadState::RUNNING;
        } else {
            if (tcb) tcb->state = ThreadState::BLOCKED;
            bool ok = ch->notFullCv.wait_for(lock, std::chrono::milliseconds(timeoutMs), canSend);
            if (tcb) tcb->state = ThreadState::RUNNING;
            if (!ok && !canSend()) return false;
        }

        if (ch->closed) return false;
        ch->buffer.push(val);
        ch->notEmptyCv.notify_one();
        return true;
    }

    bool recvChannel(uint64_t chanId, TzdValue& outVal, int64_t timeoutMs = -1) {
        std::shared_ptr<TzdNativeChannel> ch;
        {
            std::lock_guard<std::mutex> lock(m_syncMutex);
            auto it = m_channels.find(chanId);
            if (it != m_channels.end()) ch = it->second;
        }
        if (!ch) return false;

        auto tcb = getCurrentThread();
        std::unique_lock<std::mutex> lock(ch->mtx);

        auto canRecv = [ch]() { return !ch->buffer.empty() || ch->closed; };
        if (timeoutMs < 0) {
            if (tcb) tcb->state = ThreadState::BLOCKED;
            ch->notEmptyCv.wait(lock, canRecv);
            if (tcb) tcb->state = ThreadState::RUNNING;
        } else {
            if (tcb) tcb->state = ThreadState::BLOCKED;
            bool ok = ch->notEmptyCv.wait_for(lock, std::chrono::milliseconds(timeoutMs), canRecv);
            if (tcb) tcb->state = ThreadState::RUNNING;
            if (!ok && !canRecv()) return false;
        }

        if (!ch->buffer.empty()) {
            outVal = ch->buffer.front();
            ch->buffer.pop();
            ch->notFullCv.notify_one();
            return true;
        }
        return false;
    }

    bool closeChannel(uint64_t chanId) {
        std::shared_ptr<TzdNativeChannel> ch;
        {
            std::lock_guard<std::mutex> lock(m_syncMutex);
            auto it = m_channels.find(chanId);
            if (it != m_channels.end()) ch = it->second;
        }
        if (!ch) return false;
        {
            std::lock_guard<std::mutex> lock(ch->mtx);
            ch->closed = true;
        }
        ch->notEmptyCv.notify_all();
        ch->notFullCv.notify_all();
        return true;
    }

    bool isChannelClosed(uint64_t chanId) {
        std::lock_guard<std::mutex> lock(m_syncMutex);
        auto it = m_channels.find(chanId);
        if (it != m_channels.end()) {
            return it->second->closed;
        }
        return true;
    }

    size_t getChannelSize(uint64_t chanId) {
        std::lock_guard<std::mutex> lock(m_syncMutex);
        auto it = m_channels.find(chanId);
        if (it != m_channels.end()) {
            std::lock_guard<std::mutex> cLock(it->second->mtx);
            return it->second->buffer.size();
        }
        return 0;
    }

    size_t getChannelCapacity(uint64_t chanId) {
        std::lock_guard<std::mutex> lock(m_syncMutex);
        auto it = m_channels.find(chanId);
        if (it != m_channels.end()) {
            return it->second->capacity;
        }
        return 0;
    }

    void destroyChannel(uint64_t chanId) {
        closeChannel(chanId);
        std::lock_guard<std::mutex> lock(m_syncMutex);
        m_channels.erase(chanId);
    }

    // --- Atomic Management ---
    uint64_t createAtomic(int64_t initialVal = 0) {
        std::lock_guard<std::mutex> lock(m_syncMutex);
        uint64_t id = m_nextSyncId.fetch_add(1);
        auto a = std::make_shared<TzdNativeAtomic>();
        a->id = id;
        a->val.store(initialVal);
        m_atomics[id] = a;
        return id;
    }

    int64_t getAtomic(uint64_t atomicId) {
        std::lock_guard<std::mutex> lock(m_syncMutex);
        auto it = m_atomics.find(atomicId);
        if (it != m_atomics.end()) {
            return it->second->val.load();
        }
        return 0;
    }

    void setAtomic(uint64_t atomicId, int64_t val) {
        std::lock_guard<std::mutex> lock(m_syncMutex);
        auto it = m_atomics.find(atomicId);
        if (it != m_atomics.end()) {
            it->second->val.store(val);
        }
    }

    int64_t addAtomic(uint64_t atomicId, int64_t delta) {
        std::lock_guard<std::mutex> lock(m_syncMutex);
        auto it = m_atomics.find(atomicId);
        if (it != m_atomics.end()) {
            return it->second->val.fetch_add(delta);
        }
        return 0;
    }

    bool casAtomic(uint64_t atomicId, int64_t expected, int64_t desired) {
        std::lock_guard<std::mutex> lock(m_syncMutex);
        auto it = m_atomics.find(atomicId);
        if (it != m_atomics.end()) {
            return it->second->val.compare_exchange_strong(expected, desired);
        }
        return false;
    }

    void destroyAtomic(uint64_t atomicId) {
        std::lock_guard<std::mutex> lock(m_syncMutex);
        m_atomics.erase(atomicId);
    }

private:
    TzdThreadManager() = default;
    ~TzdThreadManager() = default;

    std::recursive_mutex m_threadsMutex;
    std::unordered_map<uint64_t, std::shared_ptr<ThreadControlBlock>> m_threads;
    std::unordered_map<uint32_t, uint64_t> m_osToThreadId;
    std::atomic<uint64_t> m_nextThreadId{2};

    static inline thread_local std::shared_ptr<ThreadControlBlock> m_currentTcb = nullptr;

    std::mutex m_logsMutex;
    std::deque<ThreadLogEntry> m_logs;
    size_t m_maxLogs = 2000;

    std::mutex m_syncMutex;
    std::atomic<uint64_t> m_nextSyncId{1};
    std::unordered_map<uint64_t, std::shared_ptr<TzdNativeMutex>> m_mutexes;
    std::unordered_map<uint64_t, std::shared_ptr<TzdNativeCondVar>> m_condVars;
    std::unordered_map<uint64_t, std::shared_ptr<TzdNativeEvent>> m_events;
    std::unordered_map<uint64_t, std::shared_ptr<TzdNativeChannel>> m_channels;
    std::unordered_map<uint64_t, std::shared_ptr<TzdNativeAtomic>> m_atomics;
};

} // namespace TzdThreading
