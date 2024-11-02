/**
 * @file main.cpp
 * @brief A basic demonstration of C++ thread support library features for Laboratory 3.
 *        This file implements a simple program as per the laboratory assignment, which
 *        demonstrates the usefulness of a specific threading or atomic feature selected based
 *        on the student's ID modulo 40. The goal is to showcase the feature with minimal use 
 *        of other constructs from the C++ std::thread or std::atomic libraries.
 * 
 * @author Dmitriev Evgeny
 *
 * This program is part of Laboratory 3, focusing on the C++ std::thread library.
 * The task is as follows:
 * 1. Take the remainder of the student's ID divided by 40.
 * 2. Use the resulting number to select a corresponding feature or method from the list provided 
 *    in the lecture slides (some of which may require standards beyond C++11).
 * 3. Implement a simple program that demonstrates the utility of the selected feature. 
 *    Limit the use of other constructs from the C++ std::thread support and std::atomic libraries.
 * 
 * Possible result of execution:
 * 
 * ```bash
 * +---------+---------+-------+---------+-------------------+---------------------+
 * | Readers | Writers | Reads | Updates | Shared Mutex Time | Standard Mutex Time |
 * +---------+---------+-------+---------+-------------------+---------------------+
 * |     100 |       5 | 10000 |       1 |           1501 ms |             3012 ms |
 * +---------+---------+-------+---------+-------------------+---------------------+
 * |     100 |       5 | 10000 |      10 |           1460 ms |             2904 ms |
 * +---------+---------+-------+---------+-------------------+---------------------+
 * ```
 */

#include <iostream>
#include <vector>
#include <tuple>
#include <thread>
#include <shared_mutex>
#include <mutex>
#include <chrono>
#include <string>
#include <random>
#include <map>
#include <iomanip>
#include <memory>
#include <algorithm>

/**
 * @class RandomStringGenerator
 * @brief A utility class for generating random strings of specified length.
 *
 * This class provides a static method to generate random alphanumeric strings,
 * which can be used for testing purposes or to simulate text data.
 */
class RandomStringGenerator {
public:
    /**
     * @brief Generates a random alphanumeric string of the specified length.
     * @param length The length of the generated string.
     * @return A randomly generated string of the given length.
     *
     * This function generates a random string consisting of lowercase and uppercase letters
     * and digits. It uses a thread-local Mersenne Twister engine to ensure thread safety and
     * avoid collisions in multi-threaded contexts.
     */
    static std::string generate(size_t length) {
        static const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"; /**< Character set for string generation. */
        static const size_t charsetSize = sizeof(charset) - 1; /**< Size of the character set. */

        static thread_local std::mt19937 generator(std::random_device{}()); /**< Thread-local random number generator. */
        static thread_local std::uniform_int_distribution<> distribution(0, charsetSize - 1); /**< Thread-local distribution for character selection. */

        std::string randomString;
        randomString.reserve(length);

        for (size_t i = 0; i < length; ++i) {
            randomString += charset[distribution(generator)];
        }

        return randomString;
    }
};

/**
 * @struct SharedData
 * @brief Represents shared data accessed by multiple threads in lock tests.
 *
 * This structure holds data that is accessed and modified by reader and writer threads.
 * It includes an integer counter and a text string.
 */
struct SharedData {
    int counter = 0;          /**< An integer counter that may be incremented by writer threads. */
    std::string text;         /**< A text string that may be updated by writer threads. */
};

/**
 * @class LockTester
 * @brief Demonstrates the performance differences between `std::shared_mutex` and `std::mutex` in a multi-threaded environment with multiple readers and writers.
 *
 * The `LockTester` class is designed to benchmark and compare the efficiency of `std::shared_mutex` and `std::mutex` under different scenarios,
 * particularly focusing on cases where the number of reader threads exceeds the number of writer threads.
 *
 * **Purpose and Demonstration:**
 *
 * - **Shared vs. Exclusive Locking:**
 *   - `std::shared_mutex` allows multiple threads to acquire a shared (read) lock simultaneously, enabling concurrent read operations.
 *   - `std::mutex` only allows one thread to hold the lock at any given time, blocking both read and write operations from other threads.
 * - **Use Case Scenario:**
 *   - In applications where read operations are frequent and write operations are infrequent, using `std::shared_mutex` can significantly improve performance by allowing multiple readers to access the shared data concurrently.
 *   - This class demonstrates how `std::shared_mutex` can be more efficient than `std::mutex` in such scenarios.
 *
 * **Functionality:**
 *
 * - **Benchmarking Methods:**
 *   - `testSharedMutex()`: Measures the execution time when using `std::shared_mutex` with multiple reader and writer threads.
 *   - `testStandardMutex()`: Measures the execution time when using `std::mutex` with multiple reader and writer threads.
 * - **Thread Functions:**
 *   - Reader threads execute either `readerSharedLock()` or `readerStandardLock()`, depending on the mutex type, to perform read operations.
 *   - Writer threads execute either `writerSharedLock()` or `writerStandardLock()`, depending on the mutex type, to perform write operations.
 *
 * **Usage:**
 *
 * - Create an instance of `LockTester` by specifying the number of readers, writers, reads per reader, and updates per writer.
 * - Call `testSharedMutex()` and `testStandardMutex()` to perform the benchmarks.
 * - Access the `times` map to retrieve the execution times for comparison.
 *
 * **Example:**
 *
 * ```cpp
 * LockTester tester(10, 2, 1000, 500); // 10 readers, 2 writers
 * tester.testSharedMutex();
 * tester.testStandardMutex();
 * std::cout << "Shared Mutex Time: " << tester.times["Shared Mutex Time"] << " ms\n";
 * std::cout << "Standard Mutex Time: " << tester.times["Standard Mutex Time"] << " ms\n";
 * ```
 *
 * **Conclusion:**
 *
 * - The results demonstrate that `std::shared_mutex` provides better performance in scenarios with a high read-to-write ratio.
 * - This class effectively showcases the conditions under which `std::shared_mutex` is more efficient compared to `std::mutex`.
 *
 * @note Copy and move constructors and assignment operators are deleted to prevent copying of the `LockTester` instance.
 */
class LockTester final {
public:
    /**
     * @brief Constructs a LockTester with the specified number of readers, writers, reads, and updates.
     * @param numReaders The number of reader threads.
     * @param numWriters The number of writer threads.
     * @param numReads The number of reads each reader performs.
     * @param numUpdates The number of updates each writer performs.
     */
    LockTester(int numReaders, int numWriters, int numReads, int numUpdates)
        : numReaders(numReaders), numWriters(numWriters), numReads(numReads), numUpdates(numUpdates) {}

    // Delete copy and move constructors and assignment operators
    LockTester(const LockTester&) = delete; /**< Deleted copy constructor. */
    LockTester& operator=(const LockTester&) = delete; /**< Deleted copy assignment operator. */
    LockTester(LockTester&&) = delete; /**< Deleted move constructor. */
    LockTester& operator=(LockTester&&) = delete; /**< Deleted move assignment operator. */

    /**
     * @brief Tests the performance of shared_mutex with multiple readers and writers.
     *
     * Launches reader and writer threads that access a shared resource protected by shared_mutex,
     * then measures the total execution time in milliseconds.
     */
    void testSharedMutex() {
        auto start = std::chrono::high_resolution_clock::now();

        std::vector<std::thread> readers, writers;
        for (int i = 0; i < numReaders; ++i)
            readers.emplace_back(&LockTester::readerSharedLock, this);

        for (int i = 0; i < numWriters; ++i)
            writers.emplace_back(&LockTester::writerSharedLock, this);

        for (auto& t : readers) t.join();
        for (auto& t : writers) t.join();

        auto end = std::chrono::high_resolution_clock::now();
        times["Shared Mutex Time"] = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }

    /**
     * @brief Tests the performance of standard mutex with multiple readers and writers.
     *
     * Launches reader and writer threads that access a shared resource protected by std::mutex,
     * then measures the total execution time in milliseconds.
     */
    void testStandardMutex() {
        auto start = std::chrono::high_resolution_clock::now();

        std::vector<std::thread> readers, writers;
        for (int i = 0; i < numReaders; ++i)
            readers.emplace_back(&LockTester::readerStandardLock, this);

        for (int i = 0; i < numWriters; ++i)
            writers.emplace_back(&LockTester::writerStandardLock, this);

        for (auto& t : readers) t.join();
        for (auto& t : writers) t.join();

        auto end = std::chrono::high_resolution_clock::now();
        times["Standard Mutex Time"] = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }

    /// Map to store execution times for shared and standard mutex tests, accessible for move semantics.
    std::map<std::string, long long> times;

    int numReaders;  /**< Number of reader threads. */
    int numWriters;  /**< Number of writer threads. */
    int numReads;    /**< Number of read operations per reader. */
    int numUpdates;  /**< Number of update operations per writer. */

private:
    /**
     * @brief Function executed by reader threads using shared_mutex.
     *
     * Each reader acquires a shared lock on shared_mutex and reads the shared data.
     */
    void readerSharedLock() {
        for (int i = 0; i < numReads; ++i) {
            std::shared_lock lock(sharedMutex);
            volatile int data = sharedData.counter;
            volatile std::string text = sharedData.text;
        }
    }

    /**
     * @brief Function executed by writer threads using shared_mutex.
     *
     * Each writer acquires an exclusive lock on shared_mutex and updates the shared data.
     */
    void writerSharedLock() {
        for (int i = 0; i < numUpdates; ++i) {
            std::unique_lock lock(sharedMutex);
            sharedData.counter++;
            sharedData.text = RandomStringGenerator::generate(100000);
        }
    }

    /**
     * @brief Function executed by reader threads using standard mutex.
     *
     * Each reader acquires a lock on standardMutex and reads the shared data.
     */
    void readerStandardLock() {
        for (int i = 0; i < numReads; ++i) {
            std::lock_guard lock(standardMutex);
            volatile int data = sharedData.counter;
            volatile std::string text = sharedData.text;
        }
    }

    /**
     * @brief Function executed by writer threads using standard mutex.
     *
     * Each writer acquires a lock on standardMutex and updates the shared data.
     */
    void writerStandardLock() {
        for (int i = 0; i < numUpdates; ++i) {
            std::lock_guard lock(standardMutex);
            sharedData.counter++;
            sharedData.text = RandomStringGenerator::generate(100000);
        }
    }

    SharedData sharedData;       /**< Shared data accessed by readers and writers. */
    std::shared_mutex sharedMutex; /**< Mutex for shared lock testing. */
    std::mutex standardMutex;    /**< Mutex for standard lock testing. */
};


/**
 * @class Benchmark
 * @brief A class for adding and running lock test cases, then outputting benchmark results in a formatted table.
 */
class Benchmark final {
public:
    /**
     * @brief Adds a new test case with specified reader/writer counts and operations.
     * @param numReaders Number of reader threads for this test case.
     * @param numWriters Number of writer threads for this test case.
     * @param numReads Number of read operations each reader will perform.
     * @param numUpdates Number of update operations each writer will perform.
     * @return Reference to the Benchmark object for chaining.
     *
     * This method creates a new `LockTester` instance and stores it as a unique pointer in `testCases`.
     */
    Benchmark& addTestCase(int numReaders, int numWriters, int numReads, int numUpdates) {
        testCases.emplace_back(std::make_unique<LockTester>(numReaders, numWriters, numReads, numUpdates));
        return *this;
    }

    /**
     * @brief Runs all added test cases and records their results.
     * @return Reference to the Benchmark object for chaining.
     *
     * Each test case is executed for both `shared_mutex` and `standard mutex`, and the execution times are
     * stored in the `results` vector as `Result` structures.
     */
    Benchmark& run() {
        for (auto& testerPtr : testCases) {
            auto& tester = *testerPtr;
            tester.testSharedMutex();
            tester.testStandardMutex();

            Result result;
            result.times = std::move(tester.times); // Move 'times' to avoid copying
            result.numReaders = tester.numReaders;
            result.numWriters = tester.numWriters;
            result.numReads = tester.numReads;
            result.numUpdates = tester.numUpdates;
            results.push_back(std::move(result));
        }
        return *this;
    }

    /**
     * @brief Prints the benchmark results in a formatted table.
     * @return Reference to the Benchmark object for chaining.
     *
     * Generates a dynamic-width table based on the contents of the `results` vector.
     * Each column represents either a fixed attribute (e.g., readers, writers) or a dynamically
     * obtained metric from the test cases (e.g., `Shared Mutex Time`).
     */
    Benchmark& printBenchmarkTable() {
        // Collect dynamic column headers from the first result's times map
        std::vector<std::string> columns;
        if (!results.empty()) {
            for (const auto& pair : results[0].times) {
                columns.push_back(pair.first);
            }
        }

        int readers_width = std::max(2, static_cast<int>(std::string("Readers").length()));
        int writers_width = std::max(2, static_cast<int>(std::string("Writers").length()));
        int reads_width = std::max(2, static_cast<int>(std::string("Reads").length()));
        int updates_width = std::max(2, static_cast<int>(std::string("Updates").length()));

        // Calculate dynamic widths for the time columns
        std::vector<int> column_widths;
        for (const auto& col : columns) {
            int max_len = static_cast<int>(col.length());
            for (const auto& result : results) {
                auto it = result.times.find(col);
                if (it != result.times.end()) {
                    int data_len = static_cast<int>(std::to_string(it->second).length()) + 3; // value length + " ms"
                    max_len = std::max(max_len, data_len);
                }
            }
            column_widths.push_back(max_len);
        }

        // Function to print a separator line
        auto printSeparator = [&]() {
            std::cout << "+" << std::setw(readers_width + 2) << std::setfill('-') << "-"
                    << "+" << std::setw(writers_width + 2) << "-"
                    << "+" << std::setw(reads_width + 2) << "-"
                    << "+" << std::setw(updates_width + 2) << "-";
            for (size_t i = 0; i < columns.size(); ++i) {
                std::cout << "+" << std::setw(column_widths[i] + 2) << "-";
            }
            std::cout << "+" << std::endl;
        };

        // Print the table header
        printSeparator();
        std::cout << "| " << std::setw(readers_width) << std::setfill(' ') << "Readers"
                << " | " << std::setw(writers_width) << "Writers"
                << " | " << std::setw(reads_width) << "Reads"
                << " | " << std::setw(updates_width) << "Updates";
        for (size_t i = 0; i < columns.size(); ++i) {
            std::cout << " | " << std::setw(column_widths[i]) << columns[i];
        }
        std::cout << " |" << std::endl;
        printSeparator();

        // Print each result row
        for (const auto& result : results) {
            std::cout << "| " << std::setw(readers_width) << std::setfill(' ') << result.numReaders
                    << " | " << std::setw(writers_width) << result.numWriters
                    << " | " << std::setw(reads_width) << result.numReads
                    << " | " << std::setw(updates_width) << result.numUpdates;
            for (size_t i = 0; i < columns.size(); ++i) {
                auto it = result.times.find(columns[i]);
                if (it != result.times.end()) {
                    std::cout << " | " << std::setfill(' ') << std::setw(column_widths[i]) << (std::to_string(it->second) + " ms");
                } else {
                    std::cout << " | " << std::setfill(' ') << std::setw(column_widths[i]) << "N/A";
                }
            }
            std::cout << " |" << std::endl;
            printSeparator();
        }        

        return *this;
    }

private:
    /**
     * @struct Result
     * @brief A structure to store the results of each test case.
     *
     * Holds the timing data for different mutex types as well as the configuration of the test case.
     */
    struct Result {
        std::map<std::string, long long> times; /**< Execution times for various mutexes (e.g., shared vs standard). */
        int numReaders; /**< Number of readers used in the test case. */
        int numWriters; /**< Number of writers used in the test case. */
        int numReads; /**< Number of read operations per reader in the test case. */
        int numUpdates; /**< Number of update operations per writer in the test case. */
    };

    std::vector<std::unique_ptr<LockTester>> testCases; /**< Stores all test cases to be run. */
    std::vector<Result> results; /**< Holds results from each test case after it is run. */
};

int main() {
    Benchmark()
        .addTestCase(100, 5, static_cast<int>(1e4), 1)
        .addTestCase(100, 5, static_cast<int>(1e4), 10)
        .run()
        .printBenchmarkTable();

    return 0;
}
