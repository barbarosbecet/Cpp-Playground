#include <iostream>
#include <thread>
#include <mutex>
#include <future>
#include <vector>
#include <chrono>
#include <memory>
#include <exception>
#include <thread>
#include <execution>

using namespace std;
using chrono::seconds;
using chrono::high_resolution_clock;
using chrono::duration_cast;
using chrono::milliseconds;

constexpr size_t NumThreads = 16u;

template<typename T>
typename vector<T>::const_iterator Find(const vector<T>& data, const T& x)
{
    return find(data.cbegin(), data.cend(), x);
}

template<typename T>
typename vector<T>::const_iterator FindWithExecutionPolicy(const vector<T>& data, const T& x)
{
    return find(execution::par, data.cbegin(), data.cend(), x);
}

template<typename T>
typename vector<T>::const_iterator AsyncFind(const vector<T>& data, const T& x)
{
    using Iterator = typename vector<T>::const_iterator;
    auto findTask = [&data](
        Iterator from,
        Iterator to,
        const T& x) -> Iterator
    {
        auto it = find(from, to, x);
        return (*it) == x ? it : data.cend();
    };
    future<Iterator> futureIt = async(
        launch::async,
        findTask,
        data.cbegin(),
        data.cend(),
        x);

    return futureIt.get();
}

template<typename T>
typename vector<T>::const_iterator MTAsyncFind(const vector<T>& data, const T& x)
{
    using Iterator = typename vector<T>::const_iterator;
    auto findTask = [&data](
        Iterator from,
        Iterator to,
        const T& x) -> Iterator
    {
        auto it = find(from, to, x);
        return (*it) == x ? it : data.cend();
    };
    const size_t step = data.size() / NumThreads;
    vector<future<Iterator>> futures;
    for (size_t i = 0; i < NumThreads; i++)
    {
        auto start = data.cbegin() + i * step;
        const bool isLast = (i == NumThreads - 1);
        auto end = isLast ? data.cend() : start + step;
        futures.push_back(async(launch::async, findTask, start, end, x));
    }
    auto result = data.cend();
    for (future<Iterator>& f : futures)
    {
        auto it = f.get();
        if (it != data.cend())
        {
            result = it;
        }
    }
    return result;
}

template<typename T>
typename vector<T>::const_iterator MTAsyncFindWithPackagedTask(const vector<T>& data, const T& x)
{
    using Iterator = typename vector<T>::const_iterator;
    auto findTask = [&data](
        Iterator from,
        Iterator to,
        const T& x) -> Iterator
    {
        auto it = find(from, to, x);
        return (*it) == x ? it : data.cend();
    };
    const size_t step = data.size() / NumThreads;
    vector<future<Iterator>> futures;
    vector<thread> threads;
    for (size_t i = 0; i < NumThreads; i++)
    {
        packaged_task<Iterator(Iterator, Iterator, const T&) > packagedTask(findTask);
        auto start = data.cbegin() + i * step;
        const bool isLast = (i == NumThreads - 1);
        auto end = isLast ? data.cend() : start + step;
        futures.push_back(packagedTask.get_future());
        threads.emplace_back(move(packagedTask), start, end, x);
    }
    auto result = data.cend();
    for (future<Iterator>& f : futures)
    {
        auto it = f.get();
        if (it != data.cend())
        {
            result = it;
        }
    }
    for (thread& t : threads)
    {
        t.join();
    }
    return result;
}

void doSomething(function<void(int)> callback)
{
    this_thread::sleep_for(seconds(1));
    callback(42);
}

template<typename T1, typename T2>
auto sum(const T1 first, const T2 second)
{
    return first + second;
}

template<typename T>
auto square(const T x)
{
    return x * x;
}

/*string operator+(const string first, const string second)
{
    if (!(first.size() == second.size()))
        return string();
    string result(first.size(), 0);
    for (int i = 0; i < first.size(); i++)
    {
        result[i] = (static_cast<int>(first[i]) + static_cast<int>(second[i])) % 255;
    }
    return result;
}*/

int main()
{
    constexpr size_t bigSize = 500'000'000u;
    vector<int> bigData(bigSize, 13);
    bigData.back() = 42;

    auto t1 = high_resolution_clock::now();
    //auto it = Find(bigData, 42);
    //auto it = FindWithExecutionPolicy(bigData, 42);
    //auto it = AsyncFind(bigData, 42);
    //auto it = MTAsyncFind(bigData, 42);
    auto it = MTAsyncFindWithPackagedTask(bigData, 42);
    auto t2 = high_resolution_clock::now();
    cout << "Found this: " << *it <<
        " in ms: " << duration_cast<milliseconds>(t2 - t1).count() << endl;

    //int foo = 0;
    //doSomething(
    //    [&foo](const int result)
    //    {
    //        foo = result;
    //        cout << "something is done" << endl; 
    //    });
    //cout << "foo: " << foo << endl;

    //cout << sum(21.5f, 42) << endl;

    //cout << string("Hello") + string("World") << endl;
}
