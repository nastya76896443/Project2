#include "experiment.h"
#include "min_algorithms.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <thread>

ExperimentRunner::ExperimentRunner(
    const std::vector<std::size_t>& sizes,
    int repeats
)
    : dataSizes(sizes),
      repeatCount(repeats),
      hardwareThreads(std::thread::hardware_concurrency())
{
}

std::string ExperimentRunner::dataFilename(std::size_t n) const
{
    return "data/n_" + std::to_string(n) + ".txt";
}

bool ExperimentRunner::generateInputs() const
{
    std::filesystem::create_directories("data");

    std::mt19937 generator(2026);

    std::uniform_int_distribution<int> distribution(
        -1'000'000,
        1'000'000
    );

    for (std::size_t n : dataSizes)
    {
        std::ofstream file(dataFilename(n));

        if (!file)
        {
            std::cerr << "Cannot create "
                      << dataFilename(n) << '\n';

            return false;
        }

        file << n << '\n';

        for (std::size_t i = 0; i < n; ++i)
        {
            file << distribution(generator) << '\n';
        }

        file.close();

        if (!file)
        {
            std::cerr << "Cannot finish writing "
                      << dataFilename(n) << '\n';

            return false;
        }

        std::cout << "Generated: "
                  << dataFilename(n) << '\n';
    }

    return true;
}

bool ExperimentRunner::loadData(
    std::size_t n,
    std::vector<int>& data
) const
{
    std::ifstream file(dataFilename(n));

    std::size_t storedSize = 0;

    if (!(file >> storedSize) || storedSize != n || n == 0)
    {
        std::cerr << "Cannot read "
                  << dataFilename(n)
                  << ". Run generate first.\n";

        return false;
    }

    data.resize(n);

    for (int& value : data)
    {
        if (!(file >> value))
        {
            std::cerr << "Incomplete data: "
                      << dataFilename(n) << '\n';

            return false;
        }
    }

    return true;
}

ExperimentRunner::Measurement ExperimentRunner::measure(
    const std::function<int()>& operation,
    int expected
) const
{
    Measurement result;

    if (operation() != expected)
    {
        result.correct = false;

        return result;
    }

    result.times.reserve(repeatCount);

    for (int repeat = 0; repeat < repeatCount; ++repeat)
    {
        auto start = std::chrono::steady_clock::now();

        int value = operation();

        auto finish = std::chrono::steady_clock::now();

        double time = std::chrono::duration<double, std::milli>(
            finish - start
        ).count();

        result.times.push_back(time);

        if (value != expected)
        {
            result.correct = false;

            return result;
        }
    }

    std::vector<double> ordered = result.times;

    std::sort(ordered.begin(), ordered.end());

    const std::size_t middle = ordered.size() / 2;

    result.medianMs = ordered[middle];

    if (ordered.size() % 2 == 0)
    {
        result.medianMs =
            (ordered[middle - 1] + ordered[middle]) / 2.0;
    }

    return result;
}

void ExperimentRunner::printEnvironment(
    int optimizationLevel
) const
{
    std::cout << std::fixed << std::setprecision(6);

    std::cout << "Variant: 5\n";

#ifdef __VERSION__
    std::cout << "Compiler: " << __VERSION__ << '\n';
#endif

    std::cout << "Standard: C++20\n";
    std::cout << "Build: Release (NDEBUG)\n";

    std::cout << "Optimization: O"
              << optimizationLevel << '\n';

    std::cout << "Hardware threads H: "
              << hardwareThreads << '\n';

    std::cout
        << "Random: mt19937, seed=2026, integers [-1000000,1000000]\n";

    std::cout << "Repeats: "
              << repeatCount << " + 1 warm-up\n";

    std::cout << "Statistic: median; time unit: milliseconds\n";

    std::cout << "File I/O is outside measured time\n";

    std::cout
        << "Custom time includes thread creation, join and reduction\n";

#if defined(_PSTL_PAR_BACKEND_TBB)
    std::cout << "Parallel backend: TBB\n";
#elif defined(_PSTL_PAR_BACKEND_SERIAL)
    std::cout << "Parallel backend: serial\n";
#else
    std::cout << "Parallel backend: unknown\n";
#endif

    if (hardwareThreads == 0)
    {
        std::cout << "H is unknown; custom K limit is 16\n";
    }
}

void ExperimentRunner::writeSamples(
    std::ostream& file,
    int optimizationLevel,
    std::size_t n,
    const std::string& method,
    std::size_t k,
    const Measurement& result,
    int expected
) const
{
    for (std::size_t i = 0; i < result.times.size(); ++i)
    {
        file << 'O' << optimizationLevel << ','
             << n << ','
             << method << ',';

        if (k != 0)
        {
            file << k;
        }

        file << ','
             << i + 1 << ','
             << result.times[i] << ','
             << expected << '\n';
    }
}

bool ExperimentRunner::runLibrary(
    const std::vector<int>& data,
    int expected,
    int optimizationLevel,
    std::ostream& summary,
    std::ostream& samples
) const
{
    struct Method
    {
        const char* name;
        int (*function)(const std::vector<int>&);
    };

    const Method methods[] = {
        {"no_policy", minimumNoPolicy},
        {"seq", minimumSeq},
        {"unseq", minimumUnseq},
        {"par", minimumPar},
        {"par_unseq", minimumParUnseq}
    };

    std::cout << "Library algorithms\n";

    std::cout << std::left << std::setw(16) << "Method"
              << std::right << std::setw(16) << "Median, ms"
              << std::setw(14) << "Minimum" << '\n';

    for (const Method& method : methods)
    {
        Measurement result = measure(
            [&data, &method]()
            {
                return method.function(data);
            },
            expected
        );

        if (!result.correct)
        {
            std::cerr << "Incorrect result: "
                      << method.name << '\n';

            return false;
        }

        std::cout << std::left << std::setw(16) << method.name
                  << std::right << std::setw(16) << result.medianMs
                  << std::setw(14) << expected << '\n';

        summary << 'O' << optimizationLevel << ','
                << data.size() << ','
                << method.name << ','
                << result.medianMs << ','
                << expected << '\n';

        writeSamples(
            samples,
            optimizationLevel,
            data.size(),
            method.name,
            0,
            result,
            expected
        );
    }

    return true;
}

bool ExperimentRunner::runCustom(
    const std::vector<int>& data,
    int expected,
    int optimizationLevel,
    std::ostream& summary,
    std::ostream& samples,
    std::ostream& bestFile
) const
{
    const std::size_t limit = hardwareThreads == 0
        ? 16
        : 2 * static_cast<std::size_t>(hardwareThreads);

    const std::size_t maxK = std::min(data.size(), limit);

    std::size_t bestK = 0;
    double bestTime = 0.0;

    std::cout << "\nCustom parallel algorithm: K=1.."
              << maxK << '\n';

    std::cout << std::right << std::setw(6) << "K"
              << std::setw(16) << "Median, ms"
              << std::setw(14) << "Minimum" << '\n';

    for (std::size_t k = 1; k <= maxK; ++k)
    {
        Measurement result = measure(
            [&data, k]()
            {
                return parallelMinimum(data, k);
            },
            expected
        );

        if (!result.correct)
        {
            std::cerr << "Incorrect custom result: K="
                      << k << '\n';

            return false;
        }

        std::cout << std::setw(6) << k
                  << std::setw(16) << result.medianMs
                  << std::setw(14) << expected << '\n';

        summary << 'O' << optimizationLevel << ','
                << data.size() << ','
                << k << ','
                << result.medianMs << ','
                << expected << '\n';

        writeSamples(
            samples,
            optimizationLevel,
            data.size(),
            "custom",
            k,
            result,
            expected
        );

        if (bestK == 0 || result.medianMs < bestTime)
        {
            bestK = k;
            bestTime = result.medianMs;
        }
    }

    std::cout << "Best tested K: "
              << bestK << '\n';

    std::cout << "Best time: "
              << bestTime << " ms\n";

    std::cout << "Hardware threads H: "
              << hardwareThreads << '\n';

    bestFile << 'O' << optimizationLevel << ','
             << data.size() << ','
             << bestK << ','
             << bestTime << ','
             << hardwareThreads << ',';

    if (hardwareThreads != 0)
    {
        const double ratio =
            static_cast<double>(bestK) / hardwareThreads;

        std::cout << "K/H: " << ratio << '\n';

        bestFile << ratio;
    }
    else
    {
        std::cout << "K/H: unknown\n";
    }

    bestFile << '\n';

    return true;
}

bool ExperimentRunner::run(int optimizationLevel) const
{
#ifndef NDEBUG
    std::cerr << "Build in Release: add -DNDEBUG.\n";

    return false;
#endif

    if ((optimizationLevel != 0 && optimizationLevel != 3)
        || repeatCount <= 0
        || dataSizes.empty())
    {
        std::cerr
            << "Use -DBENCHMARK_OPTIMIZATION=0 or 3 and valid settings.\n";

        return false;
    }

#if defined(__GNUC__)
#ifdef __OPTIMIZE__
    const bool optimized = true;
#else
    const bool optimized = false;
#endif

    if ((optimizationLevel == 0 && optimized)
        || (optimizationLevel == 3 && !optimized))
    {
        std::cerr
            << "Optimization label does not match optimization on/off.\n";

        return false;
    }
#endif

    std::filesystem::create_directories("results");

    const std::string suffix =
        "_O" + std::to_string(optimizationLevel) + ".csv";

    std::ofstream library("results/library" + suffix);
    std::ofstream custom("results/custom" + suffix);
    std::ofstream best("results/best" + suffix);
    std::ofstream samples("results/samples" + suffix);

    if (!library || !custom || !best || !samples)
    {
        std::cerr << "Cannot create result files.\n";

        return false;
    }

    for (std::ostream* file : {&library, &custom, &best, &samples})
    {
        *file << std::fixed << std::setprecision(9);
    }

    library << "optimization,N,method,median_ms,minimum\n";

    custom << "optimization,N,K,median_ms,minimum\n";

    best << "optimization,N,best_K,best_ms,H,K_over_H\n";

    samples << "optimization,N,method,K,repeat,time_ms,minimum\n";

    printEnvironment(optimizationLevel);

    for (std::size_t n : dataSizes)
    {
        std::vector<int> data;

        if (!loadData(n, data))
        {
            return false;
        }

        const int expected = minimumNoPolicy(data);

        std::cout << "\nN=" << n
                  << "; reference minimum="
                  << expected << '\n';

        if (!runLibrary(
                data,
                expected,
                optimizationLevel,
                library,
                samples
            )
            || !runCustom(
                data,
                expected,
                optimizationLevel,
                custom,
                samples,
                best
            ))
        {
            return false;
        }

        std::cout << "Finished N=" << n << std::endl;
    }

    library.close();
    custom.close();
    best.close();
    samples.close();

    if (!library || !custom || !best || !samples)
    {
        std::cerr << "Cannot finish writing result files.\n";

        return false;
    }

    std::cout << "\nAll results correct: yes\n";
    std::cout << "CSV files saved in results/\n";

    return true;
}