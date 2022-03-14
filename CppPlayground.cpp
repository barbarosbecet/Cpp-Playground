#include <iostream>
#include <thread>
#include <mutex>
#include <future>
#include <vector>
#include <chrono>
#include <memory>
#include <exception>

using namespace std;
using chrono::seconds;
using chrono::high_resolution_clock;
using chrono::duration_cast;
using chrono::milliseconds;

vector<int>::const_iterator Find(const vector<int>& data, const int x)
{
    return find(data.cbegin(), data.cend(), x);
}

vector<int>::const_iterator AsyncFind(const std::vector<int>& data, const int x)
{
    future<vector<int>::const_iterator> futureIt = async(
        launch::async,
        [&data, x]()
        {
            return find(data.cbegin(), data.cend(), x);
        });

    return futureIt.get();
}

vector<int>::const_iterator MTAsyncFind(const std::vector<int>& data, const int x)
{
    constexpr size_t NumThreads = 16u;
    vector<future<vector<int>::const_iterator>> futures;
    for (size_t i = 0; i < NumThreads; i++)
    {
        futures.push_back(
            async(
                launch::async,
                [&data, x, i, NumThreads]()
                {
                    const size_t step = data.size() / NumThreads;
                    auto start = data.cbegin() + i * step;
                    const bool isLast = (i == NumThreads - 1);
                    auto end = isLast ? data.cend() : start + step;
                    auto it = find(start, end, x);
                    return (*it) == x ? it : data.cend();
                }));
    }
    auto result = data.cend();
    for (auto& f : futures)
    {
        auto it = f.get();
        if (it != data.cend())
        {
            result = it;
        }
    }
    return result;
}

void doSomething(std::function<void(int)> callback)
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

/*std::string operator+(const string first, const string second)
{
    if (!(first.size() == second.size()))
        return string();
    std::string result(first.size(), 0);
    for (int i = 0; i < first.size(); i++)
    {
        result[i] = (static_cast<int>(first[i]) + static_cast<int>(second[i])) % 255;
    }
    return result;
}*/

int main()
{
    constexpr size_t bigSize = 500'000'000u;
    std::vector<int> bigData(bigSize, 13);
    bigData.back() = 42;

    auto t1 = high_resolution_clock::now();
    //auto it = Find(bigData, 42);
    //auto it = AsyncFind(bigData, 42);
    auto it = MTAsyncFind(bigData, 42);
    auto t2 = high_resolution_clock::now();
    cout << "Found this: " << *it <<
        " in ms: " << duration_cast<milliseconds>(t2 - t1).count() << endl;

    //cout << boolalpha << "Found: " << (it != bigData.cend()) << endl;

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
