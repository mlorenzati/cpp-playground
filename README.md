# cpp-playground
A space to play with C++ concepts

## Threading
Play with parallelizing tasks of different nature that can be executed with the very same strategy.
Implemented a Result Thread Pool that can split differnt lenghty collection processing operations and process it with the same approach.
In the same way that futures can process generic tasks with async threads, the approach is to get the same but with thread pools.
Async threads has the cost of creating the thread wereas Thread pools can be reused multiple times, but requires a strict worked definition.
With the use of lambdas and capture arguments plus optional arguments in futures, I managed to get the best of both worlds.

1. Parallelizing work over generic collections
2. tbd

## Others
To be defined
