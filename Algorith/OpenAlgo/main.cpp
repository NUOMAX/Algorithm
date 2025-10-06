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
#include "json.hpp"
using json = nlohmann::json;
using namespace std;

// Вспомогательная функция для конвертации string в wstring
wstring string_to_wstring(const string& str) {
    if (str.empty()) return wstring();
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// ==================== СТРУКТУРЫ ДАННЫХ (полная копия) ====================

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
        if (it != algorithms_by_distribution.end() && !it->second.empty()) {
            return it->second;
        }

        // Если для распределения нет данных, возвращаем основные алгоритмы
        // и выводим предупреждение
        if (!algorithms_by_distribution.empty()) {
            cout << "Warning: No data for distribution '" << distribution
                 << "'. Using default algorithms." << endl;
        }

        return algorithms;
    }
};

// ==================== СТРУКТУРЫ ДЛЯ JSON (полная копия) ====================

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
};

// ==================== СИСТЕМА ЗАГРУЗКИ РЕЗУЛЬТАТОВ ====================

class ResultsLoader {
public:
    static OperationCounts convert(const SavedOperationCounts& saved) {
        OperationCounts ops;
        ops.comparisons = saved.comparisons;
        ops.swaps = saved.swaps;
        ops.memory_access = saved.memory_access;
        ops.extra_memory = saved.extra_memory;
        ops.current_memory = 0;
        ops.accessed_indices.clear();
        return ops;
    }

    static StatisticalResults convert(const SavedStatisticalResults& saved) {
        StatisticalResults stats;
        stats.mean_time = saved.mean_time;
        stats.std_dev = saved.std_dev;
        stats.min_time = saved.min_time;
        stats.max_time = saved.max_time;
        stats.confidence_interval = saved.confidence_interval;
        stats.all_measurements = saved.all_measurements;
        return stats;
    }

    static AlgorithmResult convert(const SavedAlgorithmResult& saved) {
        AlgorithmResult algo(saved.name);
        algo.stats = convert(saved.stats);
        algo.cache_efficiency = saved.cache_efficiency;
        algo.stable = saved.stable;
        algo.complexity = saved.complexity;
        algo.times_by_size = saved.times_by_size;
        algo.avg_operations = convert(saved.avg_operations);

        // Восстанавливаем metrics
        if (!saved.times_by_size.empty()) {
            algo.metrics.clear();
            for (size_t i = 0; i < saved.times_by_size.size(); i++) {
                DetailedMetrics metric;
                metric.time = saved.times_by_size[i];
                metric.operations = algo.avg_operations;
                metric.memory_used = saved.avg_operations.extra_memory;
                metric.stable = saved.stable;
                algo.metrics.push_back(metric);
            }
        }

        return algo;
    }

    static DataTypeAnalysis convert(const SavedDataTypeAnalysis& saved) {
        DataTypeAnalysis analysis;
        analysis.type_name = saved.type_name;
        analysis.test_sizes = saved.test_sizes;
        analysis.best_times = saved.best_times;

        // Восстанавливаем основные алгоритмы
        for (const auto& saved_algo : saved.algorithms) {
            analysis.algorithms.push_back(convert(saved_algo));
        }

        // Восстанавливаем алгоритмы по распределениям
        for (const auto& [dist, saved_algos] : saved.algorithms_by_distribution) {
            vector<AlgorithmResult> algos;
            for (const auto& saved_algo : saved_algos) {
                algos.push_back(convert(saved_algo));
            }
            analysis.algorithms_by_distribution[dist] = algos;
        }

        return analysis;
    }

    static vector<DataTypeAnalysis> loadFromJSON(const string& filename) {
        vector<DataTypeAnalysis> results;

        ifstream file(filename);
        if (!file.is_open()) {
            cerr << "Error: Cannot open file " << filename << " for reading" << endl;
            return results;
        }

        try {
            cout << "Parsing JSON file: " << filename << endl;

            // Парсим JSON
            json j;
            file >> j;

            // Проверяем валидность
            if (!validateJSON(j)) {
                cerr << "Error: Invalid JSON format or missing required fields" << endl;
                return results;
            }

            cout << "JSON version: " << j["version"] << ", timestamp: " << j["timestamp"] << endl;
            cout << "Test sizes: " << j["test_sizes"].size() << ", distributions: " << j["distributions"].size() << endl;

            // Конвертируем каждую DataTypeAnalysis
            for (const auto& json_data_type : j["results"]) {
                SavedDataTypeAnalysis saved = parseDataTypeAnalysis(json_data_type);
                DataTypeAnalysis analysis = convert(saved);
                results.push_back(analysis);

                cout << "Loaded data type: " << analysis.type_name
                     << " with " << analysis.algorithms.size() << " main algorithms"
                     << " and " << analysis.algorithms_by_distribution.size() << " distributions" << endl;

                // Выводим информацию о распределениях
                for (const auto& [dist, algos] : analysis.algorithms_by_distribution) {
                    cout << "  - " << dist << ": " << algos.size() << " algorithms" << endl;
                }
            }

            cout << "Successfully loaded " << results.size() << " data types from: " << filename << endl;

        } catch (const exception& e) {
            cerr << "JSON parsing error: " << e.what() << endl;
        }

        return results;
    }

private:
    static bool validateJSON(const json& j) {
        if (!j.contains("timestamp") || !j.contains("version") ||
            !j.contains("results") || !j.contains("test_sizes")) {
            return false;
        }

        // Проверяем что results - массив
        if (!j["results"].is_array()) {
            return false;
        }

        return true;
    }

    static SavedDataTypeAnalysis parseDataTypeAnalysis(const json& j) {
        SavedDataTypeAnalysis saved;

        saved.type_name = j["type_name"];
        saved.test_sizes = j["test_sizes"].get<vector<int>>();

        // Парсим best_times
        if (j.contains("best_times") && j["best_times"].is_object()) {
            for (auto& [key, value] : j["best_times"].items()) {
                saved.best_times[key] = value.get<double>();
            }
        }

        // Парсим основные алгоритмы
        if (j.contains("algorithms") && j["algorithms"].is_array()) {
            for (const auto& json_algo : j["algorithms"]) {
                saved.algorithms.push_back(parseAlgorithmResult(json_algo));
            }
        }

        // Парсим алгоритмы по распределениям - ВАЖНО: теперь загружаем реальные данные
        if (j.contains("algorithms_by_distribution") && j["algorithms_by_distribution"].is_object()) {
            for (auto& [dist, json_algos] : j["algorithms_by_distribution"].items()) {
                if (json_algos.is_array()) {
                    vector<SavedAlgorithmResult> algos;
                    for (const auto& json_algo : json_algos) {
                        algos.push_back(parseAlgorithmResult(json_algo));
                    }
                    saved.algorithms_by_distribution[dist] = algos;
                }
            }
        }

        return saved;
    }

    static SavedAlgorithmResult parseAlgorithmResult(const json& j) {
        SavedAlgorithmResult saved;

        saved.name = j["name"];
        saved.cache_efficiency = j["cache_efficiency"];
        saved.stable = j["stable"];
        saved.complexity = j["complexity"];

        // Парсим times_by_size
        if (j.contains("times_by_size") && j["times_by_size"].is_array()) {
            saved.times_by_size = j["times_by_size"].get<vector<double>>();
        }

        // Парсим статистику
        if (j.contains("stats") && j["stats"].is_object()) {
            saved.stats = parseStatisticalResults(j["stats"]);
        }

        // Парсим операции
        if (j.contains("operations") && j["operations"].is_object()) {
            saved.avg_operations = parseOperationCounts(j["operations"]);
        }

        return saved;
    }

    static SavedStatisticalResults parseStatisticalResults(const json& j) {
        SavedStatisticalResults saved;

        saved.mean_time = j["mean_time"];
        saved.std_dev = j["std_dev"];
        saved.min_time = j["min_time"];
        saved.max_time = j["max_time"];
        saved.confidence_interval = j["confidence_interval"];

        // Парсим all_measurements (если есть)
        if (j.contains("all_measurements") && j["all_measurements"].is_array()) {
            saved.all_measurements = j["all_measurements"].get<vector<double>>();
        }

        return saved;
    }

    static SavedOperationCounts parseOperationCounts(const json& j) {
        SavedOperationCounts saved;

        saved.comparisons = j["comparisons"];
        saved.swaps = j["swaps"];
        saved.memory_access = j["memory_access"];
        saved.extra_memory = j["extra_memory"];

        return saved;
    }
};

// ==================== ОКНА ВИЗУАЛИЗАЦИИ (полная копия) ====================

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

// ==================== ГЛАВНАЯ ФУНКЦИЯ ПРОСМОТРЩИКА ====================

int main() {
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);

    cout << "=== SORTING RESULTS VIEWER ===\n\n";

    string filename;
    cout << "Enter JSON filename to load: ";
    cin >> filename;

    // Загрузка результатов
    auto results = ResultsLoader::loadFromJSON(filename);

    if (results.empty()) {
        cout << "No data loaded. Exiting.\n";
        return 1;
    }

    cout << "\nData loaded successfully!\n";
    cout << "Found " << results.size() << " data types\n";

    // Запуск окон визуализации
    cout << "\nLaunching results table window...\n";
    ResultsTableWindow tableWindow(results);

    cout << "Launching graph window system...\n";
    GraphWindow graphWindow(results);

    cout << "\n=== VIEWER SYSTEM LAUNCHED ===\n";
    cout << "Use controls in windows to navigate results\n";

    // Запуск message loop в основном потоке для GraphWindow
    // TableWindow запустится в отдельном потоке
    thread tableThread([&tableWindow]() {
        tableWindow.runMessageLoop();
    });

    graphWindow.runMessageLoop();
    tableThread.join();

    return 0;
}
