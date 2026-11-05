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
    printf("Seed: %u | Total cycles per philosopher: %u\n", SEED, OPERATIONS);
    printf("====================================================\n\n");

    std::vector<Fork> forks(cfg.numPhilosophers);
    std::vector<Philosopher> philosophers(cfg.numPhilosophers);

    for (int i = 0; i < cfg.numPhilosophers; ++i) {
        forks[i].id = i;
        forks[i].ownerId = -1;
        InitializeCriticalSection(&forks[i].cs);
    }

    for (int i = 0; i < cfg.numPhilosophers; ++i) {
        philosophers[i].id = i;
        philosophers[i].leftForkId = i;
        philosophers[i].rightForkId = (i + 1) % cfg.numPhilosophers;
    }

    LogEvent(-1, "Infrastructure initialized. Ready to start threads.");
    LogEvent(-1, "Press Enter to exit (threads not implemented yet)...");
    getchar();

    for (int i = 0; i < cfg.numPhilosophers; ++i) {
        DeleteCriticalSection(&forks[i].cs);
    }
    DeleteCriticalSection(&g_logCs);

    return 0;
}