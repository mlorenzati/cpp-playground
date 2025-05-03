#include <gtest/gtest.h>
#include <spdlog/spdlog.h>
#include <vector>
#include "thread/resultThreadPool.h"
#include <chrono>
#include <optional>
#include <algorithm>

std::string convertBelowThousand(int num) {
    const std::vector<std::string> below_20 = {
        "", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten", "eleven", "twelve", "thirteen", 
        "fourteen", "fifteen", "sixteen", "seventeen", "eighteen", "nineteen"
    };

    const std::vector<std::string> tens = {
        "", "", "twenty", "thirty", "forty", "fifty", "sixty", "seventy", "eighty", "ninety"
    };

    std::string result;

    if (num >= 100) {
        result += below_20[num / 100] + " hundred";
        num %= 100;
        if (num > 0) result += " and ";
    }

    if (num >= 20) {
        result += tens[num / 10];
        if (num % 10 > 0) result += "-" + below_20[num % 10];
    } else if (num > 0) {
        result += below_20[num];
    }

    return result;
}

std::string numberToWords(int num) {
    if (num == 0) return "zero";
    if (num >= 1000000) return "number too large";

    int thousands = num / 1000;
    int remainder = num % 1000;
    std::string result;

    if (thousands > 0) {
        result += convertBelowThousand(thousands) + " thousand";
        if (remainder > 0) {
            result += remainder < 100 ? " and " : " ";
        }
    }

    if (remainder > 0) {
        result += convertBelowThousand(remainder);
    }

    return result;
}

bool isPrime(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true;

    if (n % 2 == 0 || n % 3 == 0) return false;

    for (int i = 5; i * i <= n; i += 6)
        if (n % i == 0 || n % (i + 2) == 0)
            return false;

    return true;
}

std::optional<std::pair<int, int>> isProductOfTwoPrimes(int n) {
    for (int i = 2; i <= std::sqrt(n); ++i) {
        if (n % i == 0) {
            int j = n / i;
            if (isPrime(i) && isPrime(j)) {
                return std::make_pair(i, j);
            }
        }
    }
    return std::nullopt;
}

TEST(ThreadTest, manyResponsesSingleVsMulti) {
    int threadNumber = 4;
    spdlog::info("Running test with resulThreadPool of {} threads", threadNumber);
    auto threadPool = ResultThreadPool(4);
    int collectionSize = 10000;
    std::vector<std::pair<int, std::string>> collection;
    for (int cnt = 1; cnt <= collectionSize; cnt++) {
        collection.push_back(std::make_pair(cnt, std::string()));
    }

    auto numberText = [](std::vector<std::pair<int, std::string>>::iterator begin, std::vector<std::pair<int, std::string>>::iterator end) -> std::vector<std::pair<int, std::string>> {
        std::vector<std::pair<int, std::string>> responses;
        for (auto it = begin; it != end; it++) {
            int number = it->first;
            std::string numberStr = numberToWords(number);
            responses.push_back(std::make_pair(number, numberStr));
        }
        return responses;
    };

    // Single thread mode
    auto reqStart = std::chrono::steady_clock::now();
    auto responsesSingleThread = numberText(collection.begin(), collection.end());
    auto repEnd = std::chrono::steady_clock::now();
    auto responsesSingleThreadLatency = std::chrono::duration_cast<std::chrono::milliseconds>(repEnd - reqStart).count();
    spdlog::info("Single threaded operation took {} ms with collection of {} elements", responsesSingleThreadLatency, responsesSingleThread.size());

    reqStart = std::chrono::steady_clock::now();
    auto responsesMultiThread = threadPool.parallelizeCollectionReturnMany<decltype(collection), decltype(numberText), std::vector<std::pair<int, std::string>>>(collection, numberText);
    repEnd = std::chrono::steady_clock::now();
    auto responsesMultiThreadLatency = std::chrono::duration_cast<std::chrono::milliseconds>(repEnd - reqStart).count();
    spdlog::info("Multi threaded operation took {} ms with collection of {} elements", responsesMultiThreadLatency, responsesMultiThread.size());
    
    EXPECT_GT(responsesSingleThreadLatency, responsesMultiThreadLatency);
    EXPECT_EQ(responsesSingleThread, responsesMultiThread);
}

TEST(ThreadTest, singleResponseSingleVsMulti) {
    std::vector<int> onePrimeOnList;
    std::vector<int> twoNonPrime;
    std::vector<int> twoPrime;
    auto threadPool = ResultThreadPool(4);

    int onePrimeOnListMaxSize = 10000;
    int smallerPrimeNumerStart = onePrimeOnListMaxSize / 3;
    for (int i=0; i<30000 && onePrimeOnList.size() < onePrimeOnListMaxSize; i++) {
        if (twoNonPrime.size() >= 2) {
            onePrimeOnList.push_back(twoNonPrime[0]*twoNonPrime[1]);
            twoNonPrime.clear();
        }
        if (!isPrime(i)) {
            twoNonPrime.push_back(i);
        } else if (i > smallerPrimeNumerStart && twoPrime.size() < 2) {
            twoPrime.push_back(i);
        }
    }
    if (twoPrime.size() >= 2) {
        onePrimeOnList.push_back(twoPrime[0]*twoPrime[1]);
        spdlog::info("added at last position {} prime mult of {}*{}={}", onePrimeOnList.size(), twoPrime[0], twoPrime[1], twoPrime[0]*twoPrime[1]);
    }

    auto searchProductOfTwoPrimes = [](std::vector<int>::iterator begin, std::vector<int>::iterator end) -> std::optional<std::pair<int, int>> {
        auto foundIt = std::find_if(begin, end, [](const int val) {
            return isProductOfTwoPrimes(val).has_value(); 
        });

        return foundIt == end ? std::nullopt : isProductOfTwoPrimes(*foundIt);
    };

    auto reqStart = std::chrono::steady_clock::now();
    auto responseSingleThread = searchProductOfTwoPrimes(onePrimeOnList.begin(), onePrimeOnList.end());
    auto repEnd = std::chrono::steady_clock::now();
    auto responseSingleThreadLatency = std::chrono::duration_cast<std::chrono::milliseconds>(repEnd - reqStart).count();
    spdlog::info("Searching in 1 thread in a collection of {} elements returned {}:{} in {} ms", 
        onePrimeOnList.size(), responseSingleThread.value().first, responseSingleThread.value().second, responseSingleThreadLatency);
    
    reqStart = std::chrono::steady_clock::now();
    auto responseMultiThread = threadPool.parallelizeCollectionReturnOne<decltype(onePrimeOnList), decltype(searchProductOfTwoPrimes), std::pair<int, int>>(onePrimeOnList, searchProductOfTwoPrimes);
    repEnd = std::chrono::steady_clock::now();
    auto responseMultiThreadLatency = std::chrono::duration_cast<std::chrono::milliseconds>(repEnd - reqStart).count();
    spdlog::info("Searching in multi thread in a collection of {} elements returned {}:{} in {} ms", 
        onePrimeOnList.size(), responseMultiThread.value().first, responseMultiThread.value().second, responseMultiThreadLatency);

    EXPECT_GT(responseSingleThreadLatency, responseMultiThreadLatency);
    EXPECT_EQ(responseSingleThread, responseMultiThread);
}