#ifndef MIN_ALGORITHMS_H
#define MIN_ALGORITHMS_H

#include <cstddef>
#include <vector>

// Усі функції отримують непорожній масив.
int minimumNoPolicy(const std::vector<int>& data);

int minimumSeq(const std::vector<int>& data);

int minimumUnseq(const std::vector<int>& data);

int minimumPar(const std::vector<int>& data);

int minimumParUnseq(const std::vector<int>& data);

// Для власного алгоритму: 1 <= k <= data.size().
int parallelMinimum(const std::vector<int>& data, std::size_t k);

#endif