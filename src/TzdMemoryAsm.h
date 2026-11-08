#ifndef TZD_MEMORY_ASM_H
#define TZD_MEMORY_ASM_H

#include <windows.h>
#include <vector>
#include <string>
#include <capstone/capstone.h>

class TzdMemoryAsm {
public:
    static void analyzeAndDumpAsm(DWORD processId, size_t maxInstructions = 100);

private:
    static void disassembleBuffer(HANDLE hProcess, const uint8_t* code, size_t codeSize, uint64_t address);
};

#endif