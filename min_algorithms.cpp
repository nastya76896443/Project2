#include "min_algorithms.h"

#include <algorithm>
#include <execution>
#include <thread>

int minimumNoPolicy(const std::vector<int>& data)
{
    return *std::min_element(data.begin(), data.end());
}

int minimumSeq(const std::vector<int>& data)
{
    return *std::min_element(
        std::execution::seq,
        data.begin(),
        data.end()
    );
}

int minimumUnseq(const std::vector<int>& data)
{
    return *std::min_element(
        std::execution::unseq,
        data.begin(),
        data.end()
    );
}

int minimumPar(const std::vector<int>& data)
{
    return *std::min_element(
        std::execution::par,
        data.begin(),
        data.end()
    );
}

int minimumParUnseq(const std::vector<int>& data)
{
    return *std::min_element(
        std::execution::par_unseq,
        data.begin(),
        data.end()
    );
}

int parallelMinimum(const std::vector<int>& data, std::size_t k)
{
    const std::size_t n = data.size();

    const std::size_t base = n / k;
    const std::size_t remainder = n % k;

    std::vector<int> partial(k);

    std::vector<std::thread> workers;
    workers.reserve(k);

    std::size_t begin = 0;

    for (std::size_t i = 0; i < k; ++i)
    {
        const std::size_t length =
            base + (i < remainder ? 1 : 0);

        const std::size_t end = begin + length;

        workers.emplace_back(
            [&data, &partial, i, begin, end]()
            {
                partial[i] = *std::min_element(
                    data.begin() + begin,
                    data.begin() + end
                );
            }
        );

        begin = end;
    }

    for (auto& worker : workers)
    {
        worker.join();
    }

    return *std::min_element(partial.begin(), partial.end());
}