#include "common.h"

CRITICAL_SECTION g_logCs;
LARGE_INTEGER g_startTime;
LARGE_INTEGER g_freq;
LONG g_invariantViolations = 0;

double GetTimeMs() {
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (double)(now.QuadPart - g_startTime.QuadPart) * 1000.0 / (double)g_freq.QuadPart;
}

void LogEvent(int phId, const char* format, ...) {
    EnterCriticalSection(&g_logCs);

    double t = GetTimeMs();
    if (phId >= 0) {
        printf("[%06.0f ms] P%d ", t, phId);
    }
    else {
        printf("[%06.0f ms] SYSTEM ", t);
    }

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);

    printf("\n");
    LeaveCriticalSection(&g_logCs);
}

Config ParseArgs(int argc, char* argv[]) {
    Config cfg;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-p" && i + 1 < argc) cfg.numPhilosophers = std::atoi(argv[++i]);
        else if (arg == "-e" && i + 1 < argc) cfg.eatTimeMs = std::atoi(argv[++i]);
        else if (arg == "-t" && i + 1 < argc) cfg.thinkTimeMs = std::atoi(argv[++i]);
        else if (arg == "-o" && i + 1 < argc) cfg.operations = std::atoi(argv[++i]);
    }
    return cfg;
}

DWORD WINAPI PhilosopherThread(LPVOID lpParam) {
    ThreadData* data = (ThreadData*)lpParam;
    Philosopher* ph = data->ph;
    Config* cfg = data->cfg;
    Fork* forks = data->forks;

    ph->tid = GetCurrentThreadId();
    Sleep((ph->id * 10) % 50); 

    for (int i = 0; i < cfg->operations; ++i) {
        LogEvent(ph->id, "THINKING");
        Sleep(cfg->thinkTimeMs);

        int firstFork = (std::min)(ph->leftForkId, ph->rightForkId);
        int secondFork = (std::max)(ph->leftForkId, ph->rightForkId);

        LogEvent(ph->id, "WAITING RESOURCES %d,%d", firstFork, secondFork);

        LARGE_INTEGER waitStart, waitEnd;
        QueryPerformanceCounter(&waitStart);

        EnterCriticalSection(&forks[firstFork].cs);
        if (forks[firstFork].ownerId != -1) {
            InterlockedIncrement(&g_invariantViolations);
            LogEvent(ph->id, "INVARIANT VIOLATION! Fork %d already owned by %d", firstFork, forks[firstFork].ownerId);
        }
        forks[firstFork].ownerId = ph->id;

        EnterCriticalSection(&forks[secondFork].cs);
        if (forks[secondFork].ownerId != -1) {
            InterlockedIncrement(&g_invariantViolations);
            LogEvent(ph->id, "INVARIANT VIOLATION! Fork %d already owned by %d", secondFork, forks[secondFork].ownerId);
        }
        forks[secondFork].ownerId = ph->id;

        QueryPerformanceCounter(&waitEnd);
        double waitTime = (double)(waitEnd.QuadPart - waitStart.QuadPart) * 1000.0 / (double)g_freq.QuadPart;

        ph->totalWaitTimeMs += waitTime;
        if (waitTime > ph->maxWaitTimeMs) ph->maxWaitTimeMs = waitTime;

        LogEvent(ph->id, "EATING (waited %.3f ms)", waitTime);
        ph->eatCount++;
        Sleep(cfg->eatTimeMs);

        forks[secondFork].ownerId = -1;
        LeaveCriticalSection(&forks[secondFork].cs);

        forks[firstFork].ownerId = -1;
        LeaveCriticalSection(&forks[firstFork].cs);

        LogEvent(ph->id, "RELEASED RESOURCES %d,%d", firstFork, secondFork);
    }

    LogEvent(ph->id, "FINISHED");
    return 0;
}

int main(int argc, char* argv[])
{
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);

    QueryPerformanceFrequency(&g_freq);
    QueryPerformanceCounter(&g_startTime);
    InitializeCriticalSection(&g_logCs);

    Config cfg = ParseArgs(argc, argv);

    printf("====================================================\n");
    printf("Lab 4 | VARIANT %u (Ordered Capture) | STUDENT_ID: %u\n", VARIANT_ID, STUDENT_ID);
    printf("Config: Philosophers=%d, EatTime=%d ms, ThinkTime=%d ms, Ops=%d\n",
        cfg.numPhilosophers, cfg.eatTimeMs, cfg.thinkTimeMs, cfg.operations);
    printf("====================================================\n\n");

    std::vector<Fork> forks(cfg.numPhilosophers);
    std::vector<Philosopher> philosophers(cfg.numPhilosophers);
    std::vector<ThreadData> threadData(cfg.numPhilosophers);
    std::vector<HANDLE> hThreads(cfg.numPhilosophers);

    for (int i = 0; i < cfg.numPhilosophers; ++i) {
        forks[i].id = i;
        forks[i].ownerId = -1;
        InitializeCriticalSection(&forks[i].cs);
    }

    LogEvent(-1, "Starting threads with synchronization...");

    for (int i = 0; i < cfg.numPhilosophers; ++i) {
        philosophers[i].id = i;
        philosophers[i].leftForkId = i;
        philosophers[i].rightForkId = (i + 1) % cfg.numPhilosophers;

        threadData[i].ph = &philosophers[i];
        threadData[i].cfg = &cfg;
        threadData[i].forks = forks.data();

        hThreads[i] = CreateThread(NULL, 0, PhilosopherThread, &threadData[i], 0, NULL);
        philosophers[i].hThread = hThreads[i];
    }

    WaitForMultipleObjects(cfg.numPhilosophers, hThreads.data(), TRUE, INFINITE);

    LogEvent(-1, "All threads finished.\n");

    printf("\n=== STATISTICS ===\n");
    for (int i = 0; i < cfg.numPhilosophers; ++i) {
        double avgWait = philosophers[i].totalWaitTimeMs / philosophers[i].eatCount;
        printf("Philosopher %d | Eaten: %d | Avg Wait: %.3f ms | Max Wait: %.3f ms\n",
            philosophers[i].id,
            philosophers[i].eatCount,
            avgWait,
            philosophers[i].maxWaitTimeMs);
    }

    printf("\n=== INVARIANTS CHECK ===\n");
    if (g_invariantViolations == 0) {
        printf("SUCCESS: No invariant violations detected. Forks are safe.\n");
    }
    else {
        printf("FAILED: Detected %ld invariant violations!\n", g_invariantViolations);
    }

    for (int i = 0; i < cfg.numPhilosophers; ++i) {
        CloseHandle(hThreads[i]);
        DeleteCriticalSection(&forks[i].cs);
    }
    DeleteCriticalSection(&g_logCs);

    printf("\nPress Enter to exit...\n");
    getchar();
    return 0;
}