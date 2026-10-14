#ifndef EXPERIMENT_H
#define EXPERIMENT_H

#include <cstddef>
#include <functional>
#include <ostream>
#include <string>
#include <vector>

class ExperimentRunner
{
public:
    ExperimentRunner(
        const std::vector<std::size_t>& sizes =
            {1'000, 10'000, 100'000, 1'000'000, 10'000'000},
        int repeats = 5
    );

    bool generateInputs() const;

    bool run(int optimizationLevel) const;

private:
    struct Measurement
    {
        double medianMs = 0.0;
        bool correct = true;
        std::vector<double> times;
    };

    std::vector<std::size_t> dataSizes;
    int repeatCount;
    unsigned int hardwareThreads;

    std::string dataFilename(std::size_t n) const;

    bool loadData(
        std::size_t n,
        std::vector<int>& data
    ) const;

    Measurement measure(
        const std::function<int()>& operation,
        int expected
    ) const;

    void printEnvironment(int optimizationLevel) const;

    void writeSamples(
        std::ostream& file,
        int optimizationLevel,
        std::size_t n,
        const std::string& method,
        std::size_t k,
        const Measurement& result,
        int expected
    ) const;

    bool runLibrary(
        const std::vector<int>& data,
        int expected,
        int optimizationLevel,
        std::ostream& summary,
        std::ostream& samples
    ) const;

    bool runCustom(
        const std::vector<int>& data,
        int expected,
        int optimizationLevel,
        std::ostream& summary,
        std::ostream& samples,
        std::ostream& bestFile
    ) const;
};

#endif