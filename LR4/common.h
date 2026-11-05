#pragma once
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <vector>
#include <iostream>

const uint32_t STUDENT_ID = 8;
const uint32_t VARIANT_ID = 1;
const uint32_t SEED = 1000 + (STUDENT_ID * 97); // 1776
const uint32_t BASE_OPERATIONS = 100;
const uint32_t OPERATIONS = BASE_OPERATIONS + (STUDENT_ID * 5); // 140

struct Config {
    int numPhilosophers = 5;
    int eatTimeMs = 5;
    int thinkTimeMs = 10;
    int operations = OPERATIONS;
};

struct Fork {
    int id;
    int ownerId = -1; 
    CRITICAL_SECTION cs;
};

struct Philosopher {
    int id;
    int leftForkId;
    int rightForkId;
    int eatCount = 0;
    double totalWaitTimeMs = 0.0;
    double maxWaitTimeMs = 0.0;
    HANDLE hThread = NULL;
};

extern CRITICAL_SECTION g_logCs;
extern LARGE_INTEGER g_startTime;
extern LARGE_INTEGER g_freq;

double GetTimeMs();
void LogEvent(int phId, const char* format, ...);
Config ParseArgs(int argc, char* argv[]);