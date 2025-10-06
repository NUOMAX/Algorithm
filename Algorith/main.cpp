#include <fstream>
#include <chrono>
#include <random>
#include <iostream>
#include <windows.h>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <functional>
#include <cmath>
#include <queue>
#include <stack>
#include <thread>
#include <future>
#include <typeinfo>
#include <type_traits>
#include <map>
#include <numeric>
#include <iomanip>
#include <atomic>
#include <ctime>
using namespace std;

// Вспомогательная функция для конвертации string в wstring
wstring string_to_wstring(const string& str) {
    if (str.empty()) return wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// ==================== ОСНОВНЫЕ СТРУКТУРЫ ДАННЫХ ====================

struct OperationCounts {
    long long comparisons = 0;
    long long swaps = 0;
    long long memory_access = 0;
    size_t extra_memory = 0;
    size_t current_memory = 0;
    vector<int> accessed_indices;

    void reset() {
        comparisons = swaps = memory_access = 0;
        extra_memory = current_memory = 0;
        accessed_indices.clear();
        accessed_indices.shrink_to_fit();
    }

    void update_peak_memory() {
        if (current_memory > extra_memory) {
            extra_memory = current_memory;
        }
    }

    void add_memory(size_t bytes) {
        current_memory += bytes;
        update_peak_memory();
    }

    void remove_memory(size_t bytes) {
        if (current_memory >= bytes) {
            current_memory -= bytes;
        }
    }

    OperationCounts& operator+=(const OperationCounts& other) {
        comparisons += other.comparisons;
        swaps += other.swaps;
        memory_access += other.memory_access;
        if (other.extra_memory > extra_memory) {
            extra_memory = other.extra_memory;
        }
        return *this;
    }
};

struct DetailedMetrics {
    double time;
    OperationCounts operations;
    size_t memory_used;
    bool stable;

    DetailedMetrics() : time(0.0), memory_used(0), stable(false) {}
};

struct StatisticalResults {
    double mean_time;
    double std_dev;
    double min_time;
    double max_time;
    double confidence_interval;
    vector<double> all_measurements;

    void calculate() {
        if (all_measurements.empty()) return;

        mean_time = accumulate(all_measurements.begin(), all_measurements.end(), 0.0) / all_measurements.size();

        double sq_sum = 0.0;
        for (double t : all_measurements) {
            sq_sum += (t - mean_time) * (t - mean_time);
        }
        std_dev = sqrt(sq_sum / all_measurements.size());

        min_time = *min_element(all_measurements.begin(), all_measurements.end());
        max_time = *max_element(all_measurements.begin(), all_measurements.end());

        confidence_interval = 1.96 * std_dev / sqrt(all_measurements.size());
    }
};

struct AlgorithmResult {
    string name;
    vector<DetailedMetrics> metrics;
    StatisticalResults stats;
    double cache_efficiency;
    bool stable;
    string complexity;
    vector<double> times_by_size;
    OperationCounts avg_operations;

    AlgorithmResult(const string& n) : name(n), cache_efficiency(0), stable(false) {}

    void calculateAverageOperations() {
        if (metrics.empty()) return;

        avg_operations.reset();
        for (const auto& metric : metrics) {
            avg_operations += metric.operations;
        }

        avg_operations.comparisons /= metrics.size();
        avg_operations.swaps /= metrics.size();
        avg_operations.memory_access /= metrics.size();
        avg_operations.extra_memory /= metrics.size();
    }
};

struct DataTypeAnalysis {
    string type_name;
    vector<AlgorithmResult> algorithms;
    map<string, double> best_times;
    vector<int> test_sizes;
    map<string, vector<AlgorithmResult>> algorithms_by_distribution;

    vector<AlgorithmResult> getAlgorithmsForDistribution(const string& distribution) const {
        auto it = algorithms_by_distribution.find(distribution);
        if (it != algorithms_by_distribution.end()) {
            return it->second;
        }
        return algorithms;
    }
};

// ==================== СИСТЕМА СОХРАНЕНИЯ РЕЗУЛЬТАТОВ ====================

struct SavedOperationCounts {
    long long comparisons = 0;
    long long swaps = 0;
    long long memory_access = 0;
    size_t extra_memory = 0;
};

struct SavedStatisticalResults {
    double mean_time;
    double std_dev;
    double min_time;
    double max_time;
    double confidence_interval;
    vector<double> all_measurements;
};

struct SavedAlgorithmResult {
    string name;
    SavedStatisticalResults stats;
    double cache_efficiency;
    bool stable;
    string complexity;
    vector<double> times_by_size;
    SavedOperationCounts avg_operations;
};

struct SavedDataTypeAnalysis {
    string type_name;
    vector<SavedAlgorithmResult> algorithms;
    map<string, double> best_times;
    vector<int> test_sizes;
    map<string, vector<SavedAlgorithmResult>> algorithms_by_distribution;
};

struct AnalysisSession {
    string timestamp;
    string version = "1.0";
    vector<SavedDataTypeAnalysis> results;
    vector<string> distributions;
    vector<int> original_test_sizes;
    int num_threads;
    double total_duration_seconds;

    bool saveToJSON(const string& filename) {
        ofstream file(filename);
        if (!file.is_open()) {
            cerr << "Error: Cannot open file " << filename << " for writing" << endl;
            return false;
        }

        file << "{\n";
        file << "  \"timestamp\": \"" << timestamp << "\",\n";
        file << "  \"version\": \"" << version << "\",\n";
        file << "  \"num_threads\": " << num_threads << ",\n";
        file << "  \"total_duration_seconds\": " << total_duration_seconds << ",\n";
        file << "  \"distributions\": [";
        for (size_t i = 0; i < distributions.size(); i++) {
            file << "\"" << distributions[i] << "\"";
            if (i < distributions.size() - 1) file << ", ";
        }
        file << "],\n";
        file << "  \"test_sizes\": [";
        for (size_t i = 0; i < original_test_sizes.size(); i++) {
            file << original_test_sizes[i];
            if (i < original_test_sizes.size() - 1) file << ", ";
        }
        file << "],\n";
        file << "  \"results\": [\n";

        for (size_t data_idx = 0; data_idx < results.size(); data_idx++) {
            const auto& data_type = results[data_idx];
            file << "    {\n";
            file << "      \"type_name\": \"" << data_type.type_name << "\",\n";
            file << "      \"test_sizes\": [";
            for (size_t i = 0; i < data_type.test_sizes.size(); i++) {
                file << data_type.test_sizes[i];
                if (i < data_type.test_sizes.size() - 1) file << ", ";
            }
            file << "],\n";

            file << "      \"best_times\": {\n";
            size_t best_count = 0;
            for (const auto& [dist, time] : data_type.best_times) {
                file << "        \"" << dist << "\": " << time;
                if (++best_count < data_type.best_times.size()) file << ",\n";
                else file << "\n";
            }
            file << "      },\n";

            // Основные алгоритмы
            file << "      \"algorithms\": [\n";
            for (size_t algo_idx = 0; algo_idx < data_type.algorithms.size(); algo_idx++) {
                const auto& algo = data_type.algorithms[algo_idx];
                file << "        {\n";
                file << "          \"name\": \"" << algo.name << "\",\n";
                file << "          \"cache_efficiency\": " << algo.cache_efficiency << ",\n";
                file << "          \"stable\": " << (algo.stable ? "true" : "false") << ",\n";
                file << "          \"complexity\": \"" << algo.complexity << "\",\n";

                file << "          \"stats\": {\n";
                file << "            \"mean_time\": " << algo.stats.mean_time << ",\n";
                file << "            \"std_dev\": " << algo.stats.std_dev << ",\n";
                file << "            \"min_time\": " << algo.stats.min_time << ",\n";
                file << "            \"max_time\": " << algo.stats.max_time << ",\n";
                file << "            \"confidence_interval\": " << algo.stats.confidence_interval << "\n";
                file << "          },\n";

                file << "          \"times_by_size\": [";
                for (size_t i = 0; i < algo.times_by_size.size(); i++) {
                    file << algo.times_by_size[i];
                    if (i < algo.times_by_size.size() - 1) file << ", ";
                }
                file << "],\n";

                file << "          \"operations\": {\n";
                file << "            \"comparisons\": " << algo.avg_operations.comparisons << ",\n";
                file << "            \"swaps\": " << algo.avg_operations.swaps << ",\n";
                file << "            \"memory_access\": " << algo.avg_operations.memory_access << ",\n";
                file << "            \"extra_memory\": " << algo.avg_operations.extra_memory << "\n";
                file << "          }\n";

                file << "        }";
                if (algo_idx < data_type.algorithms.size() - 1) file << ",";
                file << "\n";
            }
            file << "      ],\n";

            // Алгоритмы по распределениям
            file << "      \"algorithms_by_distribution\": {\n";
            size_t dist_count = 0;
            for (const auto& [dist_name, algos] : data_type.algorithms_by_distribution) {
                file << "        \"" << dist_name << "\": [\n";
                for (size_t algo_idx = 0; algo_idx < algos.size(); algo_idx++) {
                    const auto& algo = algos[algo_idx];
                    file << "          {\n";
                    file << "            \"name\": \"" << algo.name << "\",\n";
                    file << "            \"cache_efficiency\": " << algo.cache_efficiency << ",\n";
                    file << "            \"stable\": " << (algo.stable ? "true" : "false") << ",\n";
                    file << "            \"complexity\": \"" << algo.complexity << "\",\n";

                    file << "            \"stats\": {\n";
                    file << "              \"mean_time\": " << algo.stats.mean_time << ",\n";
                    file << "              \"std_dev\": " << algo.stats.std_dev << ",\n";
                    file << "              \"min_time\": " << algo.stats.min_time << ",\n";
                    file << "              \"max_time\": " << algo.stats.max_time << ",\n";
                    file << "              \"confidence_interval\": " << algo.stats.confidence_interval << "\n";
                    file << "            },\n";

                    file << "            \"times_by_size\": [";
                    for (size_t i = 0; i < algo.times_by_size.size(); i++) {
                        file << algo.times_by_size[i];
                        if (i < algo.times_by_size.size() - 1) file << ", ";
                    }
                    file << "],\n";

                    file << "            \"operations\": {\n";
                    file << "              \"comparisons\": " << algo.avg_operations.comparisons << ",\n";
                    file << "              \"swaps\": " << algo.avg_operations.swaps << ",\n";
                    file << "              \"memory_access\": " << algo.avg_operations.memory_access << ",\n";
                    file << "              \"extra_memory\": " << algo.avg_operations.extra_memory << "\n";
                    file << "            }\n";

                    file << "          }";
                    if (algo_idx < algos.size() - 1) file << ",";
                    file << "\n";
                }
                file << "        ]";
                if (++dist_count < data_type.algorithms_by_distribution.size()) file << ",\n";
                else file << "\n";
            }
            file << "      }\n";

            file << "    }";
            if (data_idx < results.size() - 1) file << ",";
            file << "\n";
        }

        file << "  ]\n";
        file << "}\n";

        file.close();
        cout << "Results saved to: " << filename << endl;
        return true;
    }
};

class ResultsSaver {
public:
    static SavedOperationCounts convert(const OperationCounts& ops) {
        SavedOperationCounts saved;
        saved.comparisons = ops.comparisons;
        saved.swaps = ops.swaps;
        saved.memory_access = ops.memory_access;
        saved.extra_memory = ops.extra_memory;
        return saved;
    }

    static SavedStatisticalResults convert(const StatisticalResults& stats) {
        SavedStatisticalResults saved;
        saved.mean_time = stats.mean_time;
        saved.std_dev = stats.std_dev;
        saved.min_time = stats.min_time;
        saved.max_time = stats.max_time;
        saved.confidence_interval = stats.confidence_interval;
        saved.all_measurements = stats.all_measurements;
        return saved;
    }

    static SavedAlgorithmResult convert(const AlgorithmResult& algo) {
        SavedAlgorithmResult saved;
        saved.name = algo.name;
        saved.stats = convert(algo.stats);
        saved.cache_efficiency = algo.cache_efficiency;
        saved.stable = algo.stable;
        saved.complexity = algo.complexity;
        saved.times_by_size = algo.times_by_size;
        saved.avg_operations = convert(algo.avg_operations);
        return saved;
    }

    static SavedDataTypeAnalysis convert(const DataTypeAnalysis& analysis) {
        SavedDataTypeAnalysis saved;
        saved.type_name = analysis.type_name;
        saved.test_sizes = analysis.test_sizes;
        saved.best_times = analysis.best_times;

        // Сохраняем основные алгоритмы
        for (const auto& algo : analysis.algorithms) {
            saved.algorithms.push_back(convert(algo));
        }

        // Сохраняем алгоритмы по распределениям
        for (const auto& [dist, algos] : analysis.algorithms_by_distribution) {
            vector<SavedAlgorithmResult> saved_algos;
            for (const auto& algo : algos) {
                saved_algos.push_back(convert(algo));
            }
            saved.algorithms_by_distribution[dist] = saved_algos;
        }

        return saved;
    }

    static bool saveResults(const vector<DataTypeAnalysis>& results,
                          const vector<int>& test_sizes,
                          int num_threads,
                          double duration_seconds,
                          const string& custom_filename = "") {
        AnalysisSession session;

        auto now = chrono::system_clock::now();
        auto time_t = chrono::system_clock::to_time_t(now);
        char timestamp[100];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H-%M-%S", localtime(&time_t));
        session.timestamp = timestamp;

        session.distributions = {"Random", "Sorted", "Reverse", "Nearly Sorted", "Few Unique"};
        session.original_test_sizes = test_sizes;
        session.num_threads = num_threads;
        session.total_duration_seconds = duration_seconds;

        for (const auto& result : results) {
            session.results.push_back(convert(result));
        }

        string filename = custom_filename;
        if (filename.empty()) {
            filename = "sorting_results_" + session.timestamp + ".json";
        }

        return session.saveToJSON(filename);
    }
};

// ==================== СУЩЕСТВУЮЩИЙ КОД ====================

mt19937& get_random_engine() {
    static mt19937 engine(chrono::steady_clock::now().time_since_epoch().count());
    return engine;
}

int rand_uns(int min, int max) {
    static uniform_int_distribution<int> dist;
    return dist(get_random_engine(), uniform_int_distribution<int>::param_type(min, max));
}

double get_time() {
    return chrono::duration_cast<chrono::microseconds>
    (chrono::steady_clock::now().time_since_epoch()).count()/1e6;
}

// Алгоритмы сортировки с инструментацией

template<typename T>
void bubble_sort_instrumented(T arr[], int n, OperationCounts& ops) {
    ops.add_memory(sizeof(bool) + sizeof(int) * 2);

    bool swapped;
    for (int i = 0; i < n-1; i++) {
        swapped = false;
        for (int j = 0; j < n-i-1; j++) {
            ops.comparisons++;
            ops.memory_access += 2;
            if (ops.accessed_indices.size() < 10000) {
                ops.accessed_indices.push_back(j);
                ops.accessed_indices.push_back(j+1);
            }

            if (arr[j] > arr[j+1]) {
                ops.swaps++;
                ops.memory_access += 4;
                swap(arr[j], arr[j+1]);
                swapped = true;
            }
        }
        if (!swapped) break;
    }

    ops.remove_memory(sizeof(bool) + sizeof(int) * 2);
}

template<typename T>
void selection_sort_instrumented(T arr[], int n, OperationCounts& ops) {
    ops.add_memory(sizeof(int) * 3);

    for (int i = 0; i < n-1; i++) {
        int min_idx = i;
        ops.memory_access++;
        if (ops.accessed_indices.size() < 10000) {
            ops.accessed_indices.push_back(i);
        }

        for (int j = i+1; j < n; j++) {
            ops.comparisons++;
            ops.memory_access += 2;
            if (ops.accessed_indices.size() < 10000) {
                ops.accessed_indices.push_back(j);
            }

            if (arr[j] < arr[min_idx]) {
                min_idx = j;
                ops.memory_access++;
            }
        }

        if (min_idx != i) {
            ops.swaps++;
            ops.memory_access += 4;
            swap(arr[i], arr[min_idx]);
        }
    }

    ops.remove_memory(sizeof(int) * 3);
}

template<typename T>
void insertion_sort_instrumented(T arr[], int n, OperationCounts& ops) {
    ops.add_memory(sizeof(T) + sizeof(int) * 2);

    for (int i = 1; i < n; i++) {
        T key = arr[i];
        ops.memory_access++;
        int j = i - 1;
        if (ops.accessed_indices.size() < 10000) {
            ops.accessed_indices.push_back(i);
        }

        while (j >= 0) {
            ops.comparisons++;
            ops.memory_access++;
            if (ops.accessed_indices.size() < 10000) {
                ops.accessed_indices.push_back(j);
            }

            if (arr[j] > key) {
                arr[j + 1] = arr[j];
                ops.memory_access += 2;
                ops.swaps++;
                j--;
            } else {
                break;
            }
        }
        arr[j + 1] = key;
        ops.memory_access++;
    }

    ops.remove_memory(sizeof(T) + sizeof(int) * 2);
}

template<typename T>
void quick_sort_instrumented(T arr[], int low, int high, OperationCounts& ops) {
    ops.add_memory(sizeof(int) * 4);

    if (low < high) {
        int pivot_idx = (low + high) / 2;
        T pivot = arr[pivot_idx];
        ops.memory_access++;
        int i = low - 1;
        int j = high + 1;

        while (true) {
            do {
                i++;
                ops.comparisons++;
                ops.memory_access++;
                if (ops.accessed_indices.size() < 10000) {
                    ops.accessed_indices.push_back(i);
                }
            } while (arr[i] < pivot);

            do {
                j--;
                ops.comparisons++;
                ops.memory_access++;
                if (ops.accessed_indices.size() < 10000) {
                    ops.accessed_indices.push_back(j);
                }
            } while (arr[j] > pivot);

            if (i >= j) break;

            ops.swaps++;
            ops.memory_access += 4;
            swap(arr[i], arr[j]);
        }

        quick_sort_instrumented(arr, low, j, ops);
        quick_sort_instrumented(arr, j + 1, high, ops);
    }

    ops.remove_memory(sizeof(int) * 4);
}

template<typename T>
void quick_sort_instrumented(T arr[], int n, OperationCounts& ops) {
    quick_sort_instrumented(arr, 0, n - 1, ops);
}

template<typename T>
void merge_instrumented(T arr[], int left, int mid, int right, OperationCounts& ops) {
    int n1 = mid - left + 1;
    int n2 = right - mid;

    size_t temp_memory = (n1 + n2) * sizeof(T);
    ops.add_memory(temp_memory);

    vector<T> L(n1), R(n2);

    for (int i = 0; i < n1; i++) {
        L[i] = arr[left + i];
        ops.memory_access += 2;
    }
    for (int j = 0; j < n2; j++) {
        R[j] = arr[mid + 1 + j];
        ops.memory_access += 2;
    }

    int i = 0, j = 0, k = left;

    while (i < n1 && j < n2) {
        ops.comparisons++;
        ops.memory_access += 2;
        if (ops.accessed_indices.size() < 10000) {
            ops.accessed_indices.push_back(left + i);
            ops.accessed_indices.push_back(mid + 1 + j);
        }

        if (L[i] <= R[j]) {
            arr[k] = L[i];
            ops.memory_access += 2;
            i++;
        } else {
            arr[k] = R[j];
            ops.memory_access += 2;
            j++;
        }
        k++;
    }

    while (i < n1) {
        arr[k] = L[i];
        ops.memory_access += 2;
        i++;
        k++;
    }

    while (j < n2) {
        arr[k] = R[j];
        ops.memory_access += 2;
        j++;
        k++;
    }

    ops.remove_memory(temp_memory);
}

template<typename T>
void merge_sort_instrumented(T arr[], int left, int right, OperationCounts& ops) {
    ops.add_memory(sizeof(int) * 2);

    if (left < right) {
        int mid = left + (right - left) / 2;

        merge_sort_instrumented(arr, left, mid, ops);
        merge_sort_instrumented(arr, mid + 1, right, ops);

        merge_instrumented(arr, left, mid, right, ops);
    }

    ops.remove_memory(sizeof(int) * 2);
}

template<typename T>
void merge_sort_instrumented(T arr[], int n, OperationCounts& ops) {
    merge_sort_instrumented(arr, 0, n - 1, ops);
}

template<typename T>
void heapify_instrumented(T arr[], int n, int i, OperationCounts& ops) {
    ops.add_memory(sizeof(int) * 4);

    int largest = i;
    int left = 2 * i + 1;
    int right = 2 * i + 2;

    if (left < n) {
        ops.comparisons++;
        ops.memory_access += 2;
        if (ops.accessed_indices.size() < 10000) {
            ops.accessed_indices.push_back(left);
        }
        if (arr[left] > arr[largest]) {
            largest = left;
            ops.memory_access++;
        }
    }

    if (right < n) {
        ops.comparisons++;
        ops.memory_access += 2;
        if (ops.accessed_indices.size() < 10000) {
            ops.accessed_indices.push_back(right);
        }
        if (arr[right] > arr[largest]) {
            largest = right;
            ops.memory_access++;
        }
    }

    if (largest != i) {
        ops.swaps++;
        ops.memory_access += 4;
        swap(arr[i], arr[largest]);
        heapify_instrumented(arr, n, largest, ops);
    }

    ops.remove_memory(sizeof(int) * 4);
}

template<typename T>
void heap_sort_instrumented(T arr[], int n, OperationCounts& ops) {
    ops.add_memory(sizeof(int));

    for (int i = n / 2 - 1; i >= 0; i--)
        heapify_instrumented(arr, n, i, ops);

    for (int i = n - 1; i > 0; i--) {
        ops.swaps++;
        ops.memory_access += 4;
        swap(arr[0], arr[i]);
        heapify_instrumented(arr, i, 0, ops);
    }

    ops.remove_memory(sizeof(int));
}

template<typename T>
void std_sort_instrumented(T arr[], int n, OperationCounts& ops) {
    size_t estimated_memory = sizeof(T) * n * 0.1;
    if (estimated_memory < 1000) estimated_memory = 1000;

    ops.add_memory(estimated_memory);

    vector<T> temp(arr, arr + n);
    ops.memory_access += n * 2;

    auto start = chrono::high_resolution_clock::now();
    std::sort(temp.begin(), temp.end());
    auto end = chrono::high_resolution_clock::now();

    for (int i = 0; i < n; i++) {
        arr[i] = temp[i];
    }

    ops.comparisons = n * log2(n) * 1.5;
    ops.swaps = n * log2(n) * 0.5;

    ops.remove_memory(estimated_memory);
}

// Функции создания массивов
template<typename T>
T* create_array(int n, int type) {
    T* arr = new T[n];

    if constexpr (is_integral_v<T> && !is_same_v<T, bool>) {
        switch (type) {
            case 0:
                for (int i = 0; i < n; i++) {
                    arr[i] = rand_uns(1, 10000);
                }
                break;
            case 1:
                for (int i = 0; i < n; i++) {
                    arr[i] = i + 1;
                }
                break;
            case 2:
                for (int i = 0; i < n; i++) {
                    arr[i] = n - i;
                }
                break;
            case 3:
                for (int i = 0; i < n; i++) {
                    arr[i] = i + 1;
                }
                for (int i = 0; i < n/10; i++) {
                    int idx1 = rand_uns(0, n-1);
                    int idx2 = rand_uns(0, n-1);
                    swap(arr[idx1], arr[idx2]);
                }
                break;
            case 4:
                for (int i = 0; i < n; i++) {
                    arr[i] = rand_uns(1, 10);
                }
                break;
        }
    } else if constexpr (is_floating_point_v<T>) {
        uniform_real_distribution<double> dist(0.0, 10000.0);
        switch (type) {
            case 0:
                for (int i = 0; i < n; i++) {
                    arr[i] = dist(get_random_engine());
                }
                break;
            case 1:
                for (int i = 0; i < n; i++) {
                    arr[i] = i * 1.0;
                }
                break;
            case 2:
                for (int i = 0; i < n; i++) {
                    arr[i] = (n - i) * 1.0;
                }
                break;
            case 3:
                for (int i = 0; i < n; i++) {
                    arr[i] = i * 1.0;
                }
                for (int i = 0; i < n/10; i++) {
                    int idx1 = rand_uns(0, n-1);
                    int idx2 = rand_uns(0, n-1);
                    swap(arr[idx1], arr[idx2]);
                }
                break;
            case 4:
                for (int i = 0; i < n; i++) {
                    arr[i] = rand_uns(1, 10) * 1.0;
                }
                break;
        }
    } else if constexpr (is_same_v<T, string>) {
        const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        const int charset_size = sizeof(chars) - 1;

        switch (type) {
            case 0:
                for (int i = 0; i < n; i++) {
                    int len = rand_uns(5, 15);
                    string s;
                    for (int j = 0; j < len; j++) {
                        s += chars[rand_uns(0, charset_size - 1)];
                    }
                    arr[i] = s;
                }
                break;
            case 1:
                for (int i = 0; i < n; i++) {
                    arr[i] = "str_" + to_string(i);
                }
                break;
            case 2:
                for (int i = 0; i < n; i++) {
                    arr[i] = "str_" + to_string(n - i);
                }
                break;
            case 3:
                for (int i = 0; i < n; i++) {
                    arr[i] = "str_" + to_string(i);
                }
                for (int i = 0; i < n/10; i++) {
                    int idx1 = rand_uns(0, n-1);
                    int idx2 = rand_uns(0, n-1);
                    swap(arr[idx1], arr[idx2]);
                }
                break;
            case 4:
                {
                    vector<string> unique_strings = {"apple", "banana", "cherry", "date", "elderberry",
                                                   "fig", "grape", "honeydew", "kiwi", "lemon"};
                    for (int i = 0; i < n; i++) {
                        arr[i] = unique_strings[rand_uns(0, static_cast<int>(unique_strings.size()) - 1)];
                    }
                }
                break;
        }
    }

    return arr;
}

// Специализация для bool
template<>
bool* create_array<bool>(int n, int type) {
    bool* arr = new bool[n];
    switch (type) {
        case 0:
            for (int i = 0; i < n; i++) {
                arr[i] = rand_uns(0, 1);
            }
            break;
        case 1:
            for (int i = 0; i < n; i++) {
                arr[i] = (i >= n/2);
            }
            break;
        case 2:
            for (int i = 0; i < n; i++) {
                arr[i] = (i < n/2);
            }
            break;
        case 3:
            for (int i = 0; i < n; i++) {
                arr[i] = (i >= n/2);
            }
            for (int i = 0; i < n/10; i++) {
                int idx = rand_uns(0, n-1);
                arr[idx] = !arr[idx];
            }
            break;
        case 4:
            for (int i = 0; i < n; i++) {
                arr[i] = rand_uns(0, 1);
            }
            break;
    }
    return arr;
}

// Классы для отображения результатов
class GraphWindow {
private:
    HWND hwnd;
    HDC hdc;
    vector<DataTypeAnalysis> results;
    vector<string> distributions;
    size_t current_data_type;
    size_t current_distribution;
    int current_display;
    bool log_scale_x;
    bool log_scale_y;
    bool normalized_view;

public:
    GraphWindow(const vector<DataTypeAnalysis>& res) : results(res) {
        distributions = {
            "Random",
            "Sorted",
            "Reverse",
            "Nearly Sorted",
            "Few Unique"
        };
        current_data_type = 0;
        current_distribution = 0;
        current_display = 0;
        log_scale_x = false;
        log_scale_y = true;
        normalized_view = false;
        createWindow();
    }

    void createWindow() {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = GraphWindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = L"GraphWindow";
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);

        RegisterClassW(&wc);

        hwnd = CreateWindowW(
            L"GraphWindow", L"Performance Graphs - Log Scale + Normalized View",
            WS_OVERLAPPEDWINDOW, 100, 100, 1200, 800,
            NULL, NULL, GetModuleHandle(NULL), this
        );

        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);
    }

    void drawGraph() {
        PAINTSTRUCT ps;
        hdc = BeginPaint(hwnd, &ps);

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        FillRect(hdc, &clientRect, (HBRUSH)(COLOR_WINDOW + 1));

        wchar_t title[400];
        wchar_t data_type_name[50];
        wchar_t algo_display[100];
        wchar_t scale_info[100];
        wchar_t distribution_name[50];

        if (current_data_type < results.size()) {
            wstring type_w = string_to_wstring(results[current_data_type].type_name);
            wcscpy(data_type_name, type_w.c_str());
        } else {
            wcscpy(data_type_name, L"Unknown");
        }

        if (current_distribution < distributions.size()) {
            wstring dist_w = string_to_wstring(distributions[current_distribution]);
            wcscpy(distribution_name, dist_w.c_str());
        } else {
            wcscpy(distribution_name, L"Unknown");
        }

        if (current_display == 0) {
            wcscpy(algo_display, L"All Algorithms");
        } else {
            const wchar_t* algo_names[] = {
                L"Bubble Sort", L"Selection Sort", L"Insertion Sort",
                L"Quick Sort", L"Merge Sort", L"Heap Sort", L"std::sort"
            };
            if (current_display - 1 < 7) {
                wcscpy(algo_display, algo_names[current_display - 1]);
            } else {
                wcscpy(algo_display, L"Unknown Algorithm");
            }
        }

        swprintf(scale_info, 100, L"Scale: X-%ls Y-%ls %ls",
                log_scale_x ? L"Log" : L"Linear",
                log_scale_y ? L"Log" : L"Linear",
                normalized_view ? L"[Normalized]" : L"");

        swprintf(title, 400, L"Data Type: %ls | Distribution: %ls | Algorithm: %ls | %ls",
                data_type_name, distribution_name, algo_display, scale_info);

        HFONT hFont = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Arial");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        TextOutW(hdc, 50, 20, title, wcslen(title));
        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);

        int margin = 80;
        int graphWidth = clientRect.right - 2 * margin;
        int graphHeight = clientRect.bottom - 2 * margin;
        int graphTop = margin + 60;
        int graphBottom = graphTop + graphHeight;

        if (graphWidth <= 0 || graphHeight <= 0) {
            EndPaint(hwnd, &ps);
            return;
        }

        auto current_algorithms = results[current_data_type].getAlgorithmsForDistribution(distributions[current_distribution]);
        auto& sizes = results[current_data_type].test_sizes;

        vector<double> x_values;
        vector<vector<double>> y_values(current_algorithms.size());
        vector<vector<double>> original_y_values(current_algorithms.size());

        for (int size : sizes) {
            if (log_scale_x) {
                x_values.push_back(log10(max(1, size)));
            } else {
                x_values.push_back(size);
            }
        }

        double min_x = *min_element(x_values.begin(), x_values.end());
        double max_x = *max_element(x_values.begin(), x_values.end());

        double min_time = numeric_limits<double>::max();
        double max_time = numeric_limits<double>::lowest();

        for (size_t algo_idx = 0; algo_idx < current_algorithms.size(); algo_idx++) {
            const auto& algo = current_algorithms[algo_idx];
            if (algo.times_by_size.size() != sizes.size()) continue;

            for (size_t i = 0; i < sizes.size(); i++) {
                double time_val = algo.times_by_size[i];
                original_y_values[algo_idx].push_back(time_val);

                if (time_val <= 0) continue;

                double display_val = time_val;

                if (normalized_view) {
                    double std_sort_time = 1.0;
                    for (const auto& a : current_algorithms) {
                        if (a.name == "std::sort" && a.times_by_size.size() > i) {
                            std_sort_time = max(a.times_by_size[i], 1e-10);
                            break;
                        }
                    }
                    display_val = time_val / std_sort_time;
                }

                if (log_scale_y) {
                    display_val = log10(max(1e-10, display_val));
                }

                y_values[algo_idx].push_back(display_val);

                if (current_display == 0 || current_display == static_cast<int>(algo_idx) + 1) {
                    min_time = min(min_time, display_val);
                    max_time = max(max_time, display_val);
                }
            }
        }

        if (max_time <= min_time) {
            if (normalized_view) {
                min_time = 0.0;
                max_time = 2.0;
            } else if (log_scale_y) {
                min_time = -3.0;
                max_time = 2.0;
            } else {
                min_time = 0.0;
                max_time = 1.0;
            }
        } else {
            double range = max_time - min_time;
            min_time -= range * 0.05;
            max_time += range * 0.05;
        }

        HPEN axisPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
        HPEN oldPen = (HPEN)SelectObject(hdc, axisPen);

        MoveToEx(hdc, margin, graphBottom, NULL);
        LineTo(hdc, margin + graphWidth, graphBottom);

        MoveToEx(hdc, margin, graphTop, NULL);
        LineTo(hdc, margin, graphBottom);

        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, TRANSPARENT);

        wchar_t x_label[50];
        if (log_scale_x) {
            swprintf(x_label, 50, L"log10(Array Size)");
        } else {
            swprintf(x_label, 50, L"Array Size");
        }
        TextOutW(hdc, margin + graphWidth/2 - 40, graphBottom + 20, x_label, wcslen(x_label));

        wchar_t y_label[50];
        if (normalized_view) {
            swprintf(y_label, 50, L"Normalized Time");
        } else if (log_scale_y) {
            swprintf(y_label, 50, L"log10(Time (s))");
        } else {
            swprintf(y_label, 50, L"Time (s)");
        }

        LOGFONTW lf;
        GetObjectW(GetCurrentObject(hdc, OBJ_FONT), sizeof(LOGFONTW), &lf);
        lf.lfEscapement = 900;
        HFONT hFontVert = CreateFontIndirectW(&lf);
        HFONT hOldFontVert = (HFONT)SelectObject(hdc, hFontVert);
        TextOutW(hdc, margin - 50, graphTop + graphHeight/2 + 40, y_label, wcslen(y_label));
        SelectObject(hdc, hOldFontVert);
        DeleteObject(hFontVert);

        for (int i = 0; i <= 5; i++) {
            double normalized_val = min_time + (max_time - min_time) * i / 5.0;
            double y_pos = graphBottom - (graphHeight * i) / 5.0;

            wchar_t label[30];

            if (normalized_view) {
                swprintf(label, 30, L"%.1fx", normalized_val);
            } else if (log_scale_y) {
                double real_time = pow(10.0, normalized_val);
                if (real_time < 0.001) {
                    swprintf(label, 30, L"%.1e", real_time);
                } else if (real_time < 1.0) {
                    swprintf(label, 30, L"%.4f", real_time);
                } else {
                    swprintf(label, 30, L"%.2f", real_time);
                }
            } else {
                if (normalized_val < 0.001) {
                    swprintf(label, 30, L"%.1e", normalized_val);
                } else if (normalized_val < 1.0) {
                    swprintf(label, 30, L"%.4f", normalized_val);
                } else {
                    swprintf(label, 30, L"%.2f", normalized_val);
                }
            }

            TextOutW(hdc, margin - 60, y_pos - 8, label, wcslen(label));

            HPEN gridPen = CreatePen(PS_DOT, 1, RGB(200, 200, 200));
            SelectObject(hdc, gridPen);
            MoveToEx(hdc, margin, y_pos, NULL);
            LineTo(hdc, margin + graphWidth, y_pos);
            SelectObject(hdc, axisPen);
            DeleteObject(gridPen);
        }

        if (!sizes.empty()) {
            int step = max(1, static_cast<int>(sizes.size()) / 8);
            for (size_t i = 0; i < sizes.size(); i += step) {
                double x_val = x_values[i];
                int x = margin + static_cast<int>((graphWidth * (x_val - min_x)) / (max_x - min_x));

                wchar_t size_label[20];
                if (log_scale_x) {
                    swprintf(size_label, 20, L"10^%.0f", x_val);
                } else if (sizes[i] >= 1000) {
                    swprintf(size_label, 20, L"%dk", sizes[i] / 1000);
                } else {
                    swprintf(size_label, 20, L"%d", sizes[i]);
                }
                TextOutW(hdc, x - 15, graphBottom + 5, size_label, wcslen(size_label));
            }
        }

        vector<COLORREF> colors = {
            RGB(255, 0, 0),     // Red - Bubble
            RGB(0, 0, 255),     // Blue - Selection
            RGB(0, 128, 0),     // Green - Insertion
            RGB(255, 0, 255),   // Magenta - Quick
            RGB(0, 128, 128),   // Teal - Merge
            RGB(128, 0, 128),   // Purple - Heap
            RGB(255, 128, 0)    // Orange - std::sort
        };

        if (!sizes.empty() && !current_algorithms.empty()) {
            for (size_t algo_idx = 0; algo_idx < current_algorithms.size(); algo_idx++) {
                if (current_display != 0 && current_display != static_cast<int>(algo_idx) + 1) {
                    continue;
                }

                if (y_values[algo_idx].size() != sizes.size()) continue;

                HPEN linePen = CreatePen(PS_SOLID, 2, colors[algo_idx % colors.size()]);
                SelectObject(hdc, linePen);

                for (size_t i = 0; i < sizes.size() - 1; i++) {
                    if (original_y_values[algo_idx][i] <= 0 || original_y_values[algo_idx][i + 1] <= 0) {
                        continue;
                    }

                    int x1 = margin + static_cast<int>((graphWidth * (x_values[i] - min_x)) / (max_x - min_x));
                    int y1 = graphBottom - static_cast<int>((graphHeight * (y_values[algo_idx][i] - min_time)) / (max_time - min_time));

                    int x2 = margin + static_cast<int>((graphWidth * (x_values[i + 1] - min_x)) / (max_x - min_x));
                    int y2 = graphBottom - static_cast<int>((graphHeight * (y_values[algo_idx][i + 1] - min_time)) / (max_time - min_time));

                    y1 = max(graphTop, min(graphBottom, y1));
                    y2 = max(graphTop, min(graphBottom, y2));

                    MoveToEx(hdc, x1, y1, NULL);
                    LineTo(hdc, x2, y2);

                    HBRUSH pointBrush = CreateSolidBrush(colors[algo_idx % colors.size()]);
                    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, pointBrush);
                    Ellipse(hdc, x1 - 3, y1 - 3, x1 + 3, y1 + 3);
                    SelectObject(hdc, oldBrush);
                    DeleteObject(pointBrush);
                }

                SelectObject(hdc, oldPen);
                DeleteObject(linePen);
            }
        }

        int legendX = margin + graphWidth - 200;
        int legendY = graphTop + 20;

        TextOutW(hdc, legendX, legendY, L"Legend:", 7);

        const wchar_t* algo_names[] = {
            L"Bubble Sort", L"Selection Sort", L"Insertion Sort",
            L"Quick Sort", L"Merge Sort", L"Heap Sort", L"std::sort"
        };

        for (int i = 0; i < 7; i++) {
            if (current_display == 0 || current_display == i + 1) {
                HBRUSH legendBrush = CreateSolidBrush(colors[i]);
                HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, legendBrush);
                Rectangle(hdc, legendX, legendY + 25 + i * 20, legendX + 15, legendY + 40 + i * 20);
                SelectObject(hdc, oldBrush);
                DeleteObject(legendBrush);

                SetTextColor(hdc, colors[i]);
                TextOutW(hdc, legendX + 20, legendY + 25 + i * 20, algo_names[i], wcslen(algo_names[i]));
            }
        }

        SetTextColor(hdc, RGB(0, 0, 0));
        TextOutW(hdc, margin, graphTop - 80,
                L"Q/W/E/R/T: Data Types | A/S/D/F/G: Distributions", 50);
        TextOutW(hdc, margin, graphTop - 60,
                L"0-7: Algorithms (0-all, 1-7-specific) | L: Toggle Log X | K: Toggle Log Y", 75);
        TextOutW(hdc, margin, graphTop - 40,
                L"N: Toggle Normalized View | ESC: Exit", 38);

        SelectObject(hdc, oldPen);
        DeleteObject(axisPen);
        EndPaint(hwnd, &ps);
    }

    void setDataType(int type) {
        if (type >= 0 && static_cast<size_t>(type) < results.size()) {
            current_data_type = type;
            InvalidateRect(hwnd, NULL, TRUE);
        }
    }

    void setDistribution(int dist) {
        if (dist >= 0 && static_cast<size_t>(dist) < distributions.size()) {
            current_distribution = dist;
            InvalidateRect(hwnd, NULL, TRUE);
        }
    }

    void setDisplay(int display) {
        if (display >= 0 && display <= 7) {
            current_display = display;
            InvalidateRect(hwnd, NULL, TRUE);
        }
    }

    void toggleLogScaleX() {
        log_scale_x = !log_scale_x;
        InvalidateRect(hwnd, NULL, TRUE);
    }

    void toggleLogScaleY() {
        log_scale_y = !log_scale_y;
        InvalidateRect(hwnd, NULL, TRUE);
    }

    void toggleNormalizedView() {
        normalized_view = !normalized_view;
        InvalidateRect(hwnd, NULL, TRUE);
    }

    static LRESULT CALLBACK GraphWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        GraphWindow* pThis = (GraphWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

        switch (uMsg) {
            case WM_PAINT:
                if (pThis) pThis->drawGraph();
                break;

            case WM_KEYDOWN:
                if (pThis) {
                    if (wParam == 'Q') pThis->setDataType(0);
                    else if (wParam == 'W') pThis->setDataType(1);
                    else if (wParam == 'E') pThis->setDataType(2);
                    else if (wParam == 'R') pThis->setDataType(3);
                    else if (wParam == 'T') pThis->setDataType(4);
                    else if (wParam == 'A') pThis->setDistribution(0);
                    else if (wParam == 'S') pThis->setDistribution(1);
                    else if (wParam == 'D') pThis->setDistribution(2);
                    else if (wParam == 'F') pThis->setDistribution(3);
                    else if (wParam == 'G') pThis->setDistribution(4);
                    else if (wParam >= '0' && wParam <= '7') pThis->setDisplay(wParam - '0');
                    else if (wParam == 'L') pThis->toggleLogScaleX();
                    else if (wParam == 'K') pThis->toggleLogScaleY();
                    else if (wParam == 'N') pThis->toggleNormalizedView();
                    else if (wParam == VK_ESCAPE) DestroyWindow(hwnd);
                }
                break;

            case WM_DESTROY:
                PostQuitMessage(0);
                break;

            default:
                return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
        return 0;
    }

    void runMessageLoop() {
        MSG msg = {};
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
};

class ResultsTableWindow {
private:
    HWND hwnd;
    HDC hdc;
    vector<DataTypeAnalysis> results;
    vector<string> distributions;
    size_t current_data_type;
    size_t current_distribution;

public:
    ResultsTableWindow(const vector<DataTypeAnalysis>& res) : results(res) {
        distributions = {
            "Random",
            "Sorted",
            "Reverse",
            "Nearly Sorted",
            "Few Unique"
        };
        current_data_type = 0;
        current_distribution = 0;
        createWindow();
    }

    void createWindow() {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = TableWindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = L"ResultsTable";

        RegisterClassW(&wc);

        hwnd = CreateWindowW(
            L"ResultsTable", L"Analysis Results - Fixed Memory Counters",
            WS_OVERLAPPEDWINDOW, 400, 200, 1200, 800,
            NULL, NULL, GetModuleHandle(NULL), this
        );

        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);
    }

    void drawTable() {
        PAINTSTRUCT ps;
        hdc = BeginPaint(hwnd, &ps);

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        FillRect(hdc, &clientRect, (HBRUSH)(COLOR_WINDOW + 1));

        wchar_t title[300];
        wstring data_type_w = string_to_wstring(results[current_data_type].type_name);
        wstring distribution_w = string_to_wstring(distributions[current_distribution]);

        swprintf(title, 300, L"Data Type: %ls | Distribution: %ls",
                data_type_w.c_str(),
                distribution_w.c_str());

        HFONT hFont = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Arial");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        TextOutW(hdc, 50, 20, title, wcslen(title));
        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);

        int y = 60;
        int x_algorithm = 50;
        int x_time = 200;
        int x_stddev = 350;
        int x_comparisons = 500;
        int x_swaps = 650;
        int x_memory = 800;
        int x_efficiency = 950;
        int x_stable = 1100;

        TextOutW(hdc, x_algorithm, y, L"Algorithm", 9);
        TextOutW(hdc, x_time, y, L"Avg Time (s)", 12);
        TextOutW(hdc, x_stddev, y, L"Std Dev", 7);
        TextOutW(hdc, x_comparisons, y, L"Comparisons", 11);
        TextOutW(hdc, x_swaps, y, L"Swaps", 5);
        TextOutW(hdc, x_memory, y, L"Memory (KB)", 11);
        TextOutW(hdc, x_efficiency, y, L"Efficiency %", 12);
        TextOutW(hdc, x_stable, y, L"Stable", 6);

        y += 30;

        auto current_algorithms = results[current_data_type].getAlgorithmsForDistribution(distributions[current_distribution]);

        for (const auto& algo : current_algorithms) {
            wstring algo_name_w = string_to_wstring(algo.name);

            TextOutW(hdc, x_algorithm, y, algo_name_w.c_str(), algo_name_w.length());

            wchar_t time_str[20];
            swprintf(time_str, 20, L"%.6f", algo.stats.mean_time);
            TextOutW(hdc, x_time, y, time_str, wcslen(time_str));

            wchar_t stddev_str[20];
            swprintf(stddev_str, 20, L"%.6f", algo.stats.std_dev);
            TextOutW(hdc, x_stddev, y, stddev_str, wcslen(stddev_str));

            wchar_t comp_str[20];
            swprintf(comp_str, 20, L"%lld", algo.avg_operations.comparisons);
            TextOutW(hdc, x_comparisons, y, comp_str, wcslen(comp_str));

            wchar_t swaps_str[20];
            swprintf(swaps_str, 20, L"%lld", algo.avg_operations.swaps);
            TextOutW(hdc, x_swaps, y, swaps_str, wcslen(swaps_str));

            wchar_t mem_str[20];
            size_t memory_kb = algo.avg_operations.extra_memory / 1024;
            swprintf(mem_str, 20, L"%zu", memory_kb);
            TextOutW(hdc, x_memory, y, mem_str, wcslen(mem_str));

            wchar_t eff_str[20];
            swprintf(eff_str, 20, L"%.1f", algo.cache_efficiency * 100);
            TextOutW(hdc, x_efficiency, y, eff_str, wcslen(eff_str));

            TextOutW(hdc, x_stable, y, algo.stable ? L"Yes" : L"No", algo.stable ? 3 : 2);

            y += 20;
        }

        y += 30;
        TextOutW(hdc, 50, y, L"Memory Usage Notes:", 18);
        y += 20;
        TextOutW(hdc, 50, y, L"- Bubble/Selection/Insertion: O(1) extra space", 45);
        y += 20;
        TextOutW(hdc, 50, y, L"- Quick Sort: O(log n) stack space", 35);
        y += 20;
        TextOutW(hdc, 50, y, L"- Merge Sort: O(n) temporary arrays", 35);
        y += 20;
        TextOutW(hdc, 50, y, L"- Heap Sort: O(1) extra space", 30);

        y += 30;
        TextOutW(hdc, 50, y, L"Controls:", 9);
        y += 25;
        TextOutW(hdc, 50, y, L"Q/W/E/R/T: Data Types | A/S/D/F/G: Distributions | ESC: Exit", 60);

        EndPaint(hwnd, &ps);
    }

    void setDataType(int type) {
        if (type >= 0 && static_cast<size_t>(type) < results.size()) {
            current_data_type = type;
            InvalidateRect(hwnd, NULL, TRUE);
        }
    }

    void setDistribution(int dist) {
        if (dist >= 0 && static_cast<size_t>(dist) < distributions.size()) {
            current_distribution = dist;
            InvalidateRect(hwnd, NULL, TRUE);
        }
    }

    static LRESULT CALLBACK TableWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        ResultsTableWindow* pThis = (ResultsTableWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

        switch (uMsg) {
            case WM_PAINT:
                if (pThis) pThis->drawTable();
                break;

            case WM_KEYDOWN:
                if (pThis) {
                    if (wParam == 'Q') pThis->setDataType(0);
                    else if (wParam == 'W') pThis->setDataType(1);
                    else if (wParam == 'E') pThis->setDataType(2);
                    else if (wParam == 'R') pThis->setDataType(3);
                    else if (wParam == 'T') pThis->setDataType(4);
                    else if (wParam == 'A') pThis->setDistribution(0);
                    else if (wParam == 'S') pThis->setDistribution(1);
                    else if (wParam == 'D') pThis->setDistribution(2);
                    else if (wParam == 'F') pThis->setDistribution(3);
                    else if (wParam == 'G') pThis->setDistribution(4);
                    else if (wParam == VK_ESCAPE) DestroyWindow(hwnd);
                }
                break;

            case WM_DESTROY:
                PostQuitMessage(0);
                break;

            default:
                return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
        return 0;
    }

    void runMessageLoop() {
        MSG msg = {};
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
};

class ComprehensiveAnalyzer {
private:
    vector<int> test_sizes;
    vector<string> data_types = {"int", "double", "float", "string", "bool"};
    vector<string> distributions = {
        "Random",
        "Sorted",
        "Reverse",
        "Nearly Sorted",
        "Few Unique"
    };
    int num_threads;

    double calculate_cache_efficiency(const vector<int>& accessed_indices, int array_size) {
        if (accessed_indices.empty()) return 0.0;
        const int CACHE_LINE = 64;
        int spatial_locality = 0;
        for (size_t i = 1; i < accessed_indices.size(); i++) {
            if (abs(accessed_indices[i] - accessed_indices[i-1]) < CACHE_LINE) {
                spatial_locality++;
            }
        }
        return static_cast<double>(spatial_locality) / accessed_indices.size();
    }

    template<typename T>
    vector<AlgorithmResult> analyze_data_type(int data_type_index, int distribution_type) {
        vector<AlgorithmResult> algorithms = {
            AlgorithmResult("Bubble"),
            AlgorithmResult("Selection"),
            AlgorithmResult("Insertion"),
            AlgorithmResult("Quick"),
            AlgorithmResult("Merge"),
            AlgorithmResult("Heap"),
            AlgorithmResult("std::sort")
        };

        algorithms[0].complexity = "O(n^2)";
        algorithms[1].complexity = "O(n^2)";
        algorithms[2].complexity = "O(n^2)";
        algorithms[3].complexity = "O(n log n)";
        algorithms[4].complexity = "O(n log n)";
        algorithms[5].complexity = "O(n log n)";
        algorithms[6].complexity = "O(n log n)";

        for (size_t algo_index = 0; algo_index < algorithms.size(); algo_index++) {
            cout << "  " << data_types[data_type_index] << " - " << algorithms[algo_index].name << "...\n";

            StatisticalResults stats;
            vector<double> times_for_sizes;

            for (size_t i = 0; i < test_sizes.size(); i++) {
                int size = test_sizes[i];

                if (size > 10000 && (algo_index == 0 || algo_index == 1 || algo_index == 2)) {
                    times_for_sizes.push_back(0.0);
                    continue;
                }

                DetailedMetrics metrics;
                OperationCounts ops;

                T* test_data = create_array<T>(size, distribution_type);

                auto start = chrono::high_resolution_clock::now();

                switch (algo_index) {
                    case 0: bubble_sort_instrumented(test_data, size, ops); break;
                    case 1: selection_sort_instrumented(test_data, size, ops); break;
                    case 2: insertion_sort_instrumented(test_data, size, ops); break;
                    case 3: quick_sort_instrumented(test_data, size, ops); break;
                    case 4: merge_sort_instrumented(test_data, size, ops); break;
                    case 5: heap_sort_instrumented(test_data, size, ops); break;
                    case 6: std_sort_instrumented(test_data, size, ops); break;
                }

                auto end = chrono::high_resolution_clock::now();
                metrics.time = chrono::duration<double>(end - start).count();
                metrics.operations = ops;
                metrics.memory_used = ops.extra_memory;

                algorithms[algo_index].cache_efficiency = calculate_cache_efficiency(ops.accessed_indices, size);
                algorithms[algo_index].metrics.push_back(metrics);
                stats.all_measurements.push_back(metrics.time);
                times_for_sizes.push_back(metrics.time);

                delete[] test_data;

                ops.accessed_indices.clear();
                ops.accessed_indices.shrink_to_fit();
            }

            stats.calculate();
            algorithms[algo_index].stats = stats;
            algorithms[algo_index].times_by_size = times_for_sizes;

            algorithms[algo_index].calculateAverageOperations();

            if (algo_index == 2 || algo_index == 4) {
                algorithms[algo_index].stable = true;
            } else if (algo_index == 6) {
                algorithms[algo_index].stable = true;
            } else {
                algorithms[algo_index].stable = false;
            }

            algorithms[algo_index].metrics.clear();
            algorithms[algo_index].metrics.shrink_to_fit();
        }

        return algorithms;
    }

public:
    ComprehensiveAnalyzer(const vector<int>& sizes, int threads)
        : test_sizes(sizes), num_threads(threads) {}

    vector<DataTypeAnalysis> run_comprehensive_analysis() {
        cout << "\n=== STARTING COMPREHENSIVE ANALYSIS ===\n";
        cout << "Using threads: " << num_threads << endl;

        vector<DataTypeAnalysis> all_results;

        for (int data_type = 0; data_type < static_cast<int>(data_types.size()); data_type++) {
            DataTypeAnalysis analysis;
            analysis.type_name = data_types[data_type];
            analysis.test_sizes = test_sizes;

            cout << "\n--- Analyzing data type: " << data_types[data_type] << " ---\n";

            for (int dist = 0; dist < static_cast<int>(distributions.size()); dist++) {
                cout << "  Distribution: " << distributions[dist] << endl;

                vector<AlgorithmResult> algorithms;

                switch (data_type) {
                    case 0: algorithms = analyze_data_type<int>(data_type, dist); break;
                    case 1: algorithms = analyze_data_type<double>(data_type, dist); break;
                    case 2: algorithms = analyze_data_type<float>(data_type, dist); break;
                    case 3: algorithms = analyze_data_type<string>(data_type, dist); break;
                    case 4: algorithms = analyze_data_type<bool>(data_type, dist); break;
                }

                double best_time = numeric_limits<double>::max();
                for (const auto& algo : algorithms) {
                    if (algo.stats.mean_time < best_time && algo.stats.mean_time > 0) {
                        best_time = algo.stats.mean_time;
                    }
                }
                analysis.best_times[distributions[dist]] = best_time;

                analysis.algorithms_by_distribution[distributions[dist]] = algorithms;

                if (dist == 0) {
                    analysis.algorithms = algorithms;
                }
            }

            all_results.push_back(analysis);
        }

        cout << "\n=== ANALYSIS COMPLETED ===\n";
        return all_results;
    }
};

int main() {
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);

    int num_arrays, num_points, num_threads;

    cout << "=== COMPREHENSIVE SORTING ALGORITHMS ANALYSIS ===\n\n";

    cout << "Enter number of arrays (sizes): ";
    cin >> num_arrays;
    cout << "Enter number of points on graphs: ";
    cin >> num_points;
    cout << "Enter number of threads: ";
    cin >> num_threads;

    if (num_arrays <= 0 || num_points <= 0 || num_threads <= 0) {
        cout << "Error: values must be positive!" << endl;
        return 1;
    }

    if (num_points > num_arrays) {
        cout << "Warning: number of points exceeds number of arrays. Using " << num_arrays << " points.\n";
        num_points = num_arrays;
    }

    vector<int> test_sizes(num_arrays);
    int max_size = min(10000, num_arrays * 10);
    for (int i = 0; i < num_arrays; i++) {
        test_sizes[i] = (i + 1) * (max_size / num_arrays);
        if (test_sizes[i] < 10) test_sizes[i] = 10;
    }

    cout << "\nAnalysis Configuration:\n";
    cout << "=============================================\n";
    cout << "* Number of sizes: " << num_arrays << " (" << test_sizes[0] << " - " << test_sizes.back() << " elements)\n";
    cout << "* Number of points: " << num_points << "\n";
    cout << "* Number of threads: " << num_threads << "\n";
    cout << "* Data types: 5 (int, double, float, string, bool)\n";
    cout << "* Distributions: 5\n";
    cout << "* Algorithms: 7\n";
    cout << "* Total measurements: " << num_arrays * 5 * 5 * 7 << endl;

    cout << "\nStarting comprehensive analysis...\n";

    auto start_time = chrono::steady_clock::now();

    ComprehensiveAnalyzer analyzer(test_sizes, num_threads);
    auto results = analyzer.run_comprehensive_analysis();

    auto end_time = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::seconds>(end_time - start_time);

    cout << "\nAnalysis completed in " << duration.count() << " seconds\n";

    cout << "\nSaving results to JSON file...\n";
    bool save_success = ResultsSaver::saveResults(results, test_sizes, num_threads, duration.count());

    if (save_success) {
        cout << "Results successfully saved! You can reload them later using the viewer program.\n";
    } else {
        cout << "Warning: Failed to save results to file.\n";
    }

    cout << "\nDo you want to view results now? (y/n): ";
    char choice;
    cin >> choice;

    if (choice == 'y' || choice == 'Y') {
        cout << "\nLaunching results table window...\n";
        ResultsTableWindow tableWindow(results);

        cout << "\nLaunching graph window system...\n";
        GraphWindow graphWindow(results);

        cout << "\n=== ANALYSIS SYSTEM LAUNCHED ===\n";

        thread tableThread([&tableWindow]() {
            tableWindow.runMessageLoop();
        });

        graphWindow.runMessageLoop();
        tableThread.join();
    } else {
        cout << "\nResults saved. You can use the viewer program later to visualize the data.\n";
    }

    return 0;
}
