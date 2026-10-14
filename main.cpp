// Компілятор: GCC 16.2.0 (Rev4, MSYS2 UCRT64).
// Стандарт: C++20. Дослідження виконуються у Release (-DNDEBUG).
// Два збирання: -O0 -DBENCHMARK_OPTIMIZATION=0
//               -O3 -DBENCHMARK_OPTIMIZATION=3

#include "experiment.h"

#include <iostream>
#include <string>

#ifndef BENCHMARK_OPTIMIZATION
#define BENCHMARK_OPTIMIZATION -1
#endif

int main(int argc, char* argv[])
{
    ExperimentRunner runner;

    std::string command = argc > 1 ? argv[1] : "run";

    if (command == "generate")
    {
        return runner.generateInputs() ? 0 : 1;
    }

    if (command == "run")
    {
        return runner.run(BENCHMARK_OPTIMIZATION) ? 0 : 1;
    }

    std::cerr << "Usage: program.exe generate | run\n";

    return 1;
}