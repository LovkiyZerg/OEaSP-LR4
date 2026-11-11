#include "common.h"

CRITICAL_SECTION g_logCs;
LARGE_INTEGER g_startTime;
LARGE_INTEGER g_freq;

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

// Функция, выполняемая в каждом потоке
DWORD WINAPI PhilosopherThread(LPVOID lpParam) {
    ThreadData* data = (ThreadData*)lpParam;
    Philosopher* ph = data->ph;
    Config* cfg = data->cfg;
    Fork* forks = data->forks;

    ph->tid = GetCurrentThreadId();
    LogEvent(ph->id, "STARTED | TID=%lu", ph->tid);

    Sleep((ph->id * 10) % 50);

    for (int i = 0; i < cfg->operations; ++i) {
        LogEvent(ph->id, "THINKING");
        Sleep(cfg->thinkTimeMs);

        int firstFork = (std::min)(ph->leftForkId, ph->rightForkId);
        int secondFork = (std::max)(ph->leftForkId, ph->rightForkId);

        LogEvent(ph->id, "WAITING RESOURCE=%d", firstFork);
        LARGE_INTEGER waitStart, waitEnd;
        QueryPerformanceCounter(&waitStart);

        forks[firstFork].ownerId = ph->id;

        LogEvent(ph->id, "WAITING RESOURCE=%d", secondFork);
        forks[secondFork].ownerId = ph->id;

        QueryPerformanceCounter(&waitEnd);
        double waitTime = (double)(waitEnd.QuadPart - waitStart.QuadPart) * 1000.0 / (double)g_freq.QuadPart;

        ph->totalWaitTimeMs += waitTime;
        if (waitTime > ph->maxWaitTimeMs) ph->maxWaitTimeMs = waitTime;

        LogEvent(ph->id, "EATING");
        ph->eatCount++;
        Sleep(cfg->eatTimeMs);

        forks[secondFork].ownerId = -1;
        forks[firstFork].ownerId = -1;
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

    LogEvent(-1, "Starting threads...");

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

    LogEvent(-1, "All threads finished. Collecting statistics...\n");

    printf("\n=== STATISTICS ===\n");
    int totalViolations = 0;
    for (int i = 0; i < cfg.numPhilosophers; ++i) {
        double avgWait = philosophers[i].totalWaitTimeMs / philosophers[i].eatCount;
        printf("Philosopher %d | Eaten: %d | Avg Wait: %.3f ms | Max Wait: %.3f ms\n",
            philosophers[i].id,
            philosophers[i].eatCount,
            avgWait,
            philosophers[i].maxWaitTimeMs);

        if (philosophers[i].eatCount != cfg.operations) {
            printf("  [ERROR] Philosopher ate %d times, expected %d!\n", philosophers[i].eatCount, cfg.operations);
            totalViolations++;
        }
    }

    if (totalViolations > 0) {
        printf("\n!!! DETECTED %d RACE CONDITION VIOLATIONS !!!\n", totalViolations);
        printf("(This is expected at Stage 2 without synchronization)\n");
    }

    // Очистка
    for (int i = 0; i < cfg.numPhilosophers; ++i) {
        CloseHandle(hThreads[i]);
        DeleteCriticalSection(&forks[i].cs);
    }
    DeleteCriticalSection(&g_logCs);

    printf("\nPress Enter to exit...\n");
    getchar();
    return 0;
}