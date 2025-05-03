#ifndef RESULT_THREAD_POOL_H
#define RESULT_THREAD_POOL_H

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <atomic>
#include <string>

#define PRINT_TYPE(x) demangle(typeid(x).name())

class ResultThreadPool {
public:
    explicit ResultThreadPool(int maxCount);
    ~ResultThreadPool();

    void enqueue(const std::function<void()>& task);

    template<typename F, typename... Args>
    auto enqueueWithResult(F&& f, Args&&... args) -> std::future<decltype(f(args...))>;

    template<typename Container, typename Task, typename ResultType>
    auto parallelizeCollectionReturnOne(Container& collection, Task task, size_t minChunkSize = defaultMinimumChunkSize) -> std::optional<ResultType>;

    template<typename Container, typename Task, typename ResultType>
    auto parallelizeCollectionReturnOne(Container& collection, Task task, ResultType defaultValue, size_t minChunkSize = defaultMinimumChunkSize) -> ResultType;

    template<typename Container, typename Task, typename ResultCollection>
    auto parallelizeCollectionReturnMany(Container& collection, Task task, int count = 0, size_t minChunkSize = defaultMinimumChunkSize) -> ResultCollection;

    int getThreadCount();

private:
    template<typename Container, typename ResultType, typename Task>
    auto splitChunksWithFutures(Container& collection, Task task, size_t minChunkSize = defaultMinimumChunkSize) -> std::vector<std::future<ResultType>>;
    std::string demangle(const char* name);

    static const int defaultMinimumChunkSize = 4;
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::condition_variable condition;
    std::atomic<bool> stop;
    std::mutex mutex;
};

// Template implementation must be in the header
template<typename F, typename... Args>
auto ResultThreadPool::enqueueWithResult(F&& f, Args&&... args) -> std::future<decltype(f(args...))>
{
    using R = decltype(f(args...));
    auto prom = std::make_shared<std::promise<R>>();
    std::future<R> fut = prom->get_future();

    auto boundTask = std::bind(std::forward<F>(f), std::forward<Args>(args)...);

    enqueue([boundTask, prom]() {
        try {
            prom->set_value(boundTask());
        } catch (...) {
            prom->set_exception(std::current_exception());
        }
    });

    return fut;
}

template<typename Container, typename ResultType, typename Task>
auto ResultThreadPool::splitChunksWithFutures(Container& collection, Task task, size_t minChunkSize) -> std::vector<std::future<ResultType>> {
    using ContainerType = std::remove_reference_t<Container>;
    using Iter = typename ContainerType::iterator;

    Iter begin = collection.begin();
    Iter end = collection.end();
    size_t totalSize = std::distance(begin, end);
    int threadCnt = getThreadCount();
    size_t chunkSize = totalSize / threadCnt;
    size_t chunkMod = totalSize % threadCnt;

    std::vector<std::future<ResultType>> futures;

    if (chunkSize < minChunkSize || threadCnt <= 1) {
        return futures; 
    }
    
    Iter chunkStart = begin;
    for (int i = 0; i < threadCnt && chunkStart != end; ++i) {
        Iter chunkEnd = chunkStart;
        std::advance(chunkEnd, std::min(chunkSize + chunkMod, static_cast<size_t>(std::distance(chunkEnd, end))));

        futures.emplace_back(enqueueWithResult([chunkStart, chunkEnd, task]() -> ResultType {
            return task(chunkStart, chunkEnd);
        }));

        chunkStart = chunkEnd;
    }

    return futures;
}

template<typename Container, typename Task, typename ResultType>
auto ResultThreadPool::parallelizeCollectionReturnOne(Container& collection, Task task, ResultType defaultValue, size_t minChunkSize) -> ResultType {
    auto result = parallelizeCollectionReturnOne<decltype(collection), decltype(task), ResultType>(collection, task, minChunkSize) ;
    if (result.has_value()) {
        return result.value();
    } else {
        return defaultValue;
   }
}

template<typename Container, typename Task, typename ResultType>
auto ResultThreadPool::parallelizeCollectionReturnOne(Container& collection, Task task, size_t minChunkSize) -> std::optional<ResultType> {
    auto futures = splitChunksWithFutures<decltype(collection), std::optional<ResultType>, decltype(task)>(collection, task, minChunkSize);

    if (futures.size() == 0) {
        // Requested to run in main thread without threadPool
        return task(collection.begin(), collection.end());
    }
    
    std::optional<ResultType> result = std::nullopt;

    for (auto& fut : futures) {
        auto resultOpt = fut.get();

        if (resultOpt.has_value() && resultOpt != std::nullopt && result == std::nullopt) {
            result = resultOpt;
        }
    }

    return result;
    
}

template<typename Container, typename Task, typename ResultCollection>
auto ResultThreadPool::parallelizeCollectionReturnMany(Container& collection, Task task, int count, size_t minChunkSize) -> ResultCollection {
    auto futures = splitChunksWithFutures<decltype(collection), ResultCollection, decltype(task)>(collection, task, minChunkSize);

    if (futures.size() == 0) {
        // Requested to run in main thread without threadPool
        return task(collection.begin(), collection.end());
    }
    
    // Gather all results
    ResultCollection finalResults;
    bool pendingCount = true;
    count = count == 0 ? std::numeric_limits<decltype(count)>::max() : count;
    for (auto& fut : futures) {
        auto partialResults = fut.get();
        
        if (pendingCount) {
            finalResults.insert(finalResults.end(), std::make_move_iterator(partialResults.begin()), std::make_move_iterator(partialResults.end()));
            if (finalResults.size() >= count) {
                finalResults.resize(count);
                pendingCount = false;
            } 
        }
    }
    std::this_thread::yield();

    return finalResults;
}

#endif