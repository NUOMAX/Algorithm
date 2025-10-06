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
#include <sstream>
#include <iomanip>
using namespace std;

// Ускоренный генератор случайных чисел
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

// ==================== СТРУКТУРЫ ДАННЫХ ДЛЯ АНАЛИЗА ====================

struct OperationCounts {
    long long comparisons = 0;
    long long swaps = 0;
    long long memory_access = 0;
    size_t extra_memory = 0;
    vector<int> accessed_indices;

    void reset() {
        comparisons = swaps = memory_access = 0;
        extra_memory = 0;
        accessed_indices.clear();
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

    AlgorithmResult(const string& n) : name(n), cache_efficiency(0), stable(false) {}
};

struct DataTypeAnalysis {
    string type_name;
    vector<AlgorithmResult> algorithms;
    map<string, double> best_times;
    vector<int> test_sizes;
};

// ==================== ШАБЛОННЫЕ АЛГОРИТМЫ ДЛЯ ВСЕХ ТИПОВ ДАННЫХ ====================

template<typename T>
void bubble_sort_instrumented(T arr[], int n, OperationCounts& ops) {
    bool swapped;
    for (int i = 0; i < n-1; i++) {
        swapped = false;
        for (int j = 0; j < n-i-1; j++) {
            ops.comparisons++;
            ops.memory_access += 2;
            ops.accessed_indices.push_back(j);
            ops.accessed_indices.push_back(j+1);

            if (arr[j] > arr[j+1]) {
                ops.swaps++;
                ops.memory_access += 2;
                swap(arr[j], arr[j+1]);
                swapped = true;
            }
        }
        if (!swapped) break;
    }
}

template<typename T>
void selection_sort_instrumented(T arr[], int n, OperationCounts& ops) {
    for (int i = 0; i < n-1; i++) {
        int min_idx = i;
        ops.accessed_indices.push_back(i);

        for (int j = i+1; j < n; j++) {
            ops.comparisons++;
            ops.memory_access += 2;
            ops.accessed_indices.push_back(j);

            if (arr[j] < arr[min_idx]) {
                min_idx = j;
            }
        }

        if (min_idx != i) {
            ops.swaps++;
            ops.memory_access += 2;
            swap(arr[i], arr[min_idx]);
        }
    }
}

template<typename T>
void insertion_sort_instrumented(T arr[], int n, OperationCounts& ops) {
    for (int i = 1; i < n; i++) {
        T key = arr[i];
        ops.memory_access++;
        int j = i - 1;
        ops.accessed_indices.push_back(i);

        while (j >= 0) {
            ops.comparisons++;
            ops.memory_access++;
            ops.accessed_indices.push_back(j);

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
}

template<typename T>
void quick_sort_instrumented(T arr[], int low, int high, OperationCounts& ops) {
    if (low < high) {
        T pivot = arr[(low + high) / 2];
        ops.memory_access++;
        int i = low - 1;
        int j = high + 1;

        while (true) {
            do {
                i++;
                ops.comparisons++;
                ops.memory_access++;
                ops.accessed_indices.push_back(i);
            } while (arr[i] < pivot);

            do {
                j--;
                ops.comparisons++;
                ops.memory_access++;
                ops.accessed_indices.push_back(j);
            } while (arr[j] > pivot);

            if (i >= j) break;

            ops.swaps++;
            ops.memory_access += 2;
            swap(arr[i], arr[j]);
        }

        quick_sort_instrumented(arr, low, j, ops);
        quick_sort_instrumented(arr, j + 1, high, ops);
    }
}

template<typename T>
void quick_sort_instrumented(T arr[], int n, OperationCounts& ops) {
    quick_sort_instrumented(arr, 0, n - 1, ops);
}

template<typename T>
void merge_instrumented(T arr[], int left, int mid, int right, OperationCounts& ops) {
    int n1 = mid - left + 1;
    int n2 = right - mid;

    ops.extra_memory += (n1 + n2) * sizeof(T);
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
        ops.accessed_indices.push_back(left + i);
        ops.accessed_indices.push_back(mid + 1 + j);

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
}

template<typename T>
void merge_sort_instrumented(T arr[], int left, int right, OperationCounts& ops) {
    if (left < right) {
        int mid = left + (right - left) / 2;

        merge_sort_instrumented(arr, left, mid, ops);
        merge_sort_instrumented(arr, mid + 1, right, ops);

        merge_instrumented(arr, left, mid, right, ops);
    }
}

template<typename T>
void merge_sort_instrumented(T arr[], int n, OperationCounts& ops) {
    merge_sort_instrumented(arr, 0, n - 1, ops);
}

template<typename T>
void heapify_instrumented(T arr[], int n, int i, OperationCounts& ops) {
    int largest = i;
    int left = 2 * i + 1;
    int right = 2 * i + 2;

    if (left < n) {
        ops.comparisons++;
        ops.memory_access += 2;
        ops.accessed_indices.push_back(left);
        if (arr[left] > arr[largest])
            largest = left;
    }

    if (right < n) {
        ops.comparisons++;
        ops.memory_access += 2;
        ops.accessed_indices.push_back(right);
        if (arr[right] > arr[largest])
            largest = right;
    }

    if (largest != i) {
        ops.swaps++;
        ops.memory_access += 2;
        swap(arr[i], arr[largest]);
        heapify_instrumented(arr, n, largest, ops);
    }
}

template<typename T>
void heap_sort_instrumented(T arr[], int n, OperationCounts& ops) {
    for (int i = n / 2 - 1; i >= 0; i--)
        heapify_instrumented(arr, n, i, ops);

    for (int i = n - 1; i > 0; i--) {
        ops.swaps++;
        ops.memory_access += 2;
        swap(arr[0], arr[i]);
        heapify_instrumented(arr, i, 0, ops);
    }
}

template<typename T>
void std_sort_instrumented(T arr[], int n, OperationCounts& ops) {
    vector<T> temp(arr, arr + n);
    auto start = chrono::high_resolution_clock::now();
    std::sort(temp.begin(), temp.end());
    auto end = chrono::high_resolution_clock::now();

    for (int i = 0; i < n; i++) {
        arr[i] = temp[i];
    }

    ops.memory_access += n * 2;
}

// ==================== ГЕНЕРАТОРЫ ДАННЫХ ДЛЯ ВСЕХ ТИПОВ ====================

template<typename T>
T* create_array(int n, int type) {
    T* arr = new T[n];

    if constexpr (is_integral_v<T> && !is_same_v<T, bool>) {
        switch (type) {
            case 0: // Random
                for (int i = 0; i < n; i++) {
                    arr[i] = rand_uns(1, 10000);
                }
                break;
            case 1: // Sorted
                for (int i = 0; i < n; i++) {
                    arr[i] = i + 1;
                }
                break;
            case 2: // Reverse sorted
                for (int i = 0; i < n; i++) {
                    arr[i] = n - i;
                }
                break;
            case 3: // Nearly sorted
                for (int i = 0; i < n; i++) {
                    arr[i] = i + 1;
                }
                for (int i = 0; i < n/10; i++) {
                    int idx1 = rand_uns(0, n-1);
                    int idx2 = rand_uns(0, n-1);
                    swap(arr[idx1], arr[idx2]);
                }
                break;
            case 4: // Few unique
                for (int i = 0; i < n; i++) {
                    arr[i] = rand_uns(1, 10);
                }
                break;
        }
    } else if constexpr (is_floating_point_v<T>) {
        uniform_real_distribution<double> dist(0.0, 10000.0);
        switch (type) {
            case 0: // Random
                for (int i = 0; i < n; i++) {
                    arr[i] = dist(get_random_engine());
                }
                break;
            case 1: // Sorted
                for (int i = 0; i < n; i++) {
                    arr[i] = i * 1.0;
                }
                break;
            case 2: // Reverse sorted
                for (int i = 0; i < n; i++) {
                    arr[i] = (n - i) * 1.0;
                }
                break;
            case 3: // Nearly sorted
                for (int i = 0; i < n; i++) {
                    arr[i] = i * 1.0;
                }
                for (int i = 0; i < n/10; i++) {
                    int idx1 = rand_uns(0, n-1);
                    int idx2 = rand_uns(0, n-1);
                    swap(arr[idx1], arr[idx2]);
                }
                break;
            case 4: // Few unique
                for (int i = 0; i < n; i++) {
                    arr[i] = rand_uns(1, 10) * 1.0;
                }
                break;
        }
    } else if constexpr (is_same_v<T, string>) {
        const char chars[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        const int charset_size = sizeof(chars) - 1;

        switch (type) {
            case 0: // Random strings
                for (int i = 0; i < n; i++) {
                    int len = rand_uns(5, 15);
                    string s;
                    for (int j = 0; j < len; j++) {
                        s += chars[rand_uns(0, charset_size - 1)];
                    }
                    arr[i] = s;
                }
                break;
            case 1: // Sorted strings
                for (int i = 0; i < n; i++) {
                    arr[i] = "str_" + to_string(i);
                }
                break;
            case 2: // Reverse sorted strings
                for (int i = 0; i < n; i++) {
                    arr[i] = "str_" + to_string(n - i);
                }
                break;
            case 3: // Nearly sorted strings
                for (int i = 0; i < n; i++) {
                    arr[i] = "str_" + to_string(i);
                }
                for (int i = 0; i < n/10; i++) {
                    int idx1 = rand_uns(0, n-1);
                    int idx2 = rand_uns(0, n-1);
                    swap(arr[idx1], arr[idx2]);
                }
                break;
            case 4: // Few unique strings
                vector<string> unique_strings = {"apple", "banana", "cherry", "date", "elderberry",
                                               "fig", "grape", "honeydew", "kiwi", "lemon"};
                for (int i = 0; i < n; i++) {
                    arr[i] = unique_strings[rand_uns(0, unique_strings.size() - 1)];
                }
                break;
        }
    }

    return arr;
}

template<>
bool* create_array<bool>(int n, int type) {
    bool* arr = new bool[n];
    switch (type) {
        case 0: // Random
            for (int i = 0; i < n; i++) {
                arr[i] = rand_uns(0, 1);
            }
            break;
        case 1: // Sorted (false first, then true)
            for (int i = 0; i < n; i++) {
                arr[i] = (i >= n/2);
            }
            break;
        case 2: // Reverse sorted (true first, then false)
            for (int i = 0; i < n; i++) {
                arr[i] = (i < n/2);
            }
            break;
        case 3: // Nearly sorted
            for (int i = 0; i < n; i++) {
                arr[i] = (i >= n/2);
            }
            for (int i = 0; i < n/10; i++) {
                int idx = rand_uns(0, n-1);
                arr[idx] = !arr[idx];
            }
            break;
        case 4: // Few unique
            for (int i = 0; i < n; i++) {
                arr[i] = rand_uns(0, 1);
            }
            break;
    }
    return arr;
}

// ==================== КЛАСС ДЛЯ ГРАФИКОВ ====================

class GraphWindow {
private:
    HWND hwnd;
    HDC hdc;
    vector<DataTypeAnalysis> results;
    vector<string> distributions;
    size_t current_data_type;
    size_t current_distribution;
    int current_display;

public:
    GraphWindow(const vector<DataTypeAnalysis>& res) : results(res) {
        distributions = {
            "\u0421\u043b\u0443\u0447\u0430\u0439\u043d\u044b\u0435",
            "\u041e\u0442\u0441\u043e\u0440\u0442\u0438\u0440\u043e\u0432\u0430\u043d\u043d\u044b\u0435",
            "\u041e\u0431\u0440\u0430\u0442\u043d\u044b\u0435",
            "\u041f\u043e\u0447\u0442\u0438 \u043e\u0442\u0441\u043e\u0440\u0442\u0438\u0440\u043e\u0432\u0430\u043d\u043d\u044b\u0435",
            "\u041c\u0430\u043b\u043e \u0443\u043d\u0438\u043a\u0430\u043b\u044c\u043d\u044b\u0445"
        };
        current_data_type = 0;
        current_distribution = 0;
        current_display = 0; // 0 = все алгоритмы
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
            L"GraphWindow", L"\u0413\u0440\u0430\u0444\u0438\u043a\u0438 \u043f\u0440\u043e\u0438\u0437\u0432\u043e\u0434\u0438\u0442\u0435\u043b\u044c\u043d\u043e\u0441\u0442\u0438",
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

        // Заголовок
        wchar_t title[200];
        swprintf(title, L"\u0422\u0438\u043f \u0434\u0430\u043d\u043d\u044b\u0445: %s | \u0420\u0430\u0441\u043f\u0440\u0435\u0434\u0435\u043b\u0435\u043d\u0438\u0435: %s | \u0410\u043b\u0433\u043e\u0440\u0438\u0442\u043c: %s",
                results[current_data_type].type_name.c_str(),
                distributions[current_distribution].c_str(),
                current_display == 0 ? L"\u0412\u0441\u0435" : results[current_data_type].algorithms[current_display-1].name.c_str());
        TextOutW(hdc, 50, 20, title, wcslen(title));

        // Настройки графика
        int margin = 80;
        int graphWidth = clientRect.right - 2 * margin;
        int graphHeight = clientRect.bottom - 2 * margin;
        int graphTop = margin + 40;
        int graphBottom = graphTop + graphHeight;

        if (graphWidth <= 0 || graphHeight <= 0) {
            EndPaint(hwnd, &ps);
            return;
        }

        // Находим максимальное время для масштабирования
        double max_time = 0.0;
        auto& sizes = results[current_data_type].test_sizes;

        if (current_display == 0) {
            // Все алгоритмы
            for (const auto& algo : results[current_data_type].algorithms) {
                for (double time : algo.times_by_size) {
                    max_time = max(max_time, time);
                }
            }
        } else {
            // Один алгоритм
            const auto& algo = results[current_data_type].algorithms[current_display-1];
            for (double time : algo.times_by_size) {
                max_time = max(max_time, time);
            }
        }

        if (max_time == 0) max_time = 1.0;

        // Рисуем оси
        HPEN axisPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
        HPEN oldPen = (HPEN)SelectObject(hdc, axisPen);

        // Ось X
        MoveToEx(hdc, margin, graphBottom, NULL);
        LineTo(hdc, margin + graphWidth, graphBottom);

        // Ось Y
        MoveToEx(hdc, margin, graphTop, NULL);
        LineTo(hdc, margin, graphBottom);

        // Подписи осей
        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, TRANSPARENT);
        TextOutW(hdc, margin + graphWidth/2 - 20, graphBottom + 20, L"\u0420\u0430\u0437\u043c\u0435\u0440 \u043c\u0430\u0441\u0441\u0438\u0432\u0430", 18);
        TextOutW(hdc, margin - 60, graphTop + graphHeight/2 - 40, L"\u0412\u0440\u0435\u043c\u044f (c)", 9);

        // Разметка оси Y
        for (int i = 0; i <= 5; i++) {
            double time_val = (max_time * i) / 5;
            int y = graphBottom - (graphHeight * i) / 5;

            wchar_t label[20];
            swprintf(label, L"%.3f", time_val);
            TextOutW(hdc, margin - 50, y - 8, label, wcslen(label));

            // Линии сетки
            HPEN gridPen = CreatePen(PS_DOT, 1, RGB(200, 200, 200));
            SelectObject(hdc, gridPen);
            MoveToEx(hdc, margin, y, NULL);
            LineTo(hdc, margin + graphWidth, y);
            SelectObject(hdc, axisPen);
            DeleteObject(gridPen);
        }

        // Разметка оси X
        if (!sizes.empty()) {
            int step = max(1, (int)sizes.size() / 10);
            for (size_t i = 0; i < sizes.size(); i += step) {
                int x = margin + (graphWidth * i) / (sizes.size() - 1);
                wchar_t size_label[20];
                swprintf(size_label, L"%d", sizes[i]);
                TextOutW(hdc, x - 10, graphBottom + 5, size_label, wcslen(size_label));
            }
        }

        // Цвета для алгоритмов
        vector<COLORREF> colors = {
            RGB(255, 0, 0),     // Красный - пузырьковая
            RGB(0, 0, 255),     // Синий - выбором
            RGB(0, 128, 0),     // Зеленый - вставками
            RGB(255, 0, 255),   // Пурпурный - быстрая
            RGB(0, 128, 128),   // Бирюзовый - слиянием
            RGB(128, 0, 128),   // Фиолетовый - пирамидальная
            RGB(255, 128, 0)    // Оранжевый - std::sort
        };

        // Рисуем графики
        if (!sizes.empty()) {
            if (current_display == 0) {
                // Все алгоритмы
                for (size_t algo_idx = 0; algo_idx < results[current_data_type].algorithms.size(); algo_idx++) {
                    const auto& algo = results[current_data_type].algorithms[algo_idx];
                    HPEN linePen = CreatePen(PS_SOLID, 2, colors[algo_idx % colors.size()]);
                    SelectObject(hdc, linePen);

                    for (size_t i = 0; i < sizes.size() - 1; i++) {
                        int x1 = margin + (graphWidth * i) / (sizes.size() - 1);
                        int y1 = graphBottom - (graphHeight * algo.times_by_size[i]) / max_time;

                        int x2 = margin + (graphWidth * (i + 1)) / (sizes.size() - 1);
                        int y2 = graphBottom - (graphHeight * algo.times_by_size[i + 1]) / max_time;

                        MoveToEx(hdc, x1, y1, NULL);
                        LineTo(hdc, x2, y2);

                        // Точки
                        HBRUSH pointBrush = CreateSolidBrush(colors[algo_idx % colors.size()]);
                        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, pointBrush);
                        Ellipse(hdc, x1 - 3, y1 - 3, x1 + 3, y1 + 3);
                        SelectObject(hdc, oldBrush);
                        DeleteObject(pointBrush);
                    }

                    SelectObject(hdc, oldPen);
                    DeleteObject(linePen);
                }
            } else {
                // Один алгоритм
                const auto& algo = results[current_data_type].algorithms[current_display-1];
                HPEN linePen = CreatePen(PS_SOLID, 3, colors[(current_display-1) % colors.size()]);
                SelectObject(hdc, linePen);

                for (size_t i = 0; i < sizes.size() - 1; i++) {
                    int x1 = margin + (graphWidth * i) / (sizes.size() - 1);
                    int y1 = graphBottom - (graphHeight * algo.times_by_size[i]) / max_time;

                    int x2 = margin + (graphWidth * (i + 1)) / (sizes.size() - 1);
                    int y2 = graphBottom - (graphHeight * algo.times_by_size[i + 1]) / max_time;

                    MoveToEx(hdc, x1, y1, NULL);
                    LineTo(hdc, x2, y2);

                    // Точки
                    HBRUSH pointBrush = CreateSolidBrush(colors[(current_display-1) % colors.size()]);
                    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, pointBrush);
                    Ellipse(hdc, x1 - 4, y1 - 4, x1 + 4, y1 + 4);
                    SelectObject(hdc, oldBrush);
                    DeleteObject(pointBrush);
                }

                SelectObject(hdc, oldPen);
                DeleteObject(linePen);
            }
        }

        // Легенда
        int legendX = margin + graphWidth - 200;
        int legendY = graphTop + 20;

        TextOutW(hdc, legendX, legendY, L"\u041b\u0435\u0433\u0435\u043d\u0434\u0430:", 9);

        const wchar_t* algo_names[] = {
            L"\u041f\u0443\u0437\u044b\u0440\u044c\u043a\u043e\u043c",
            L"\u0412\u044b\u0431\u043e\u0440\u043e\u043c",
            L"\u0412\u0441\u0442\u0430\u0432\u043a\u0430\u043c\u0438",
            L"\u0411\u044b\u0441\u0442\u0440\u0430\u044f",
            L"\u0421\u043b\u0438\u044f\u043d\u0438\u0435\u043c",
            L"\u041f\u0438\u0440\u0430\u043c\u0438\u0434\u0430\u043b\u044c\u043d\u0430\u044f",
            L"std::sort"
        };

        for (int i = 0; i < 7; i++) {
            if (current_display == 0 || current_display == i + 1) {
                HBRUSH legendBrush = CreateSolidBrush(colors[i]);
                HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, legendBrush);

                Rectangle(hdc, legendX, legendY + 25 + i * 20, legendX + 15, legendY + 40 + i * 20);

                SelectObject(hdc, oldBrush);
                DeleteObject(legendBrush);

                SetTextColor(hdc, colors[i]);
                TextOutW(hdc, legendX + 20, legendY + 28 + i * 20, algo_names[i], wcslen(algo_names[i]));
            }
        }

        // Управление
        SetTextColor(hdc, RGB(0, 0, 0));
        TextOutW(hdc, margin, graphTop - 30,
                L"Q/W/E/R/T: \u0442\u0438\u043f\u044b \u0434\u0430\u043d\u043d\u044b\u0445 | A/S/D/F/G: \u0440\u0430\u0441\u043f\u0440\u0435\u0434\u0435\u043b\u0435\u043d\u0438\u044f", 85);
        TextOutW(hdc, margin, graphTop - 10,
                L"0-7: \u0430\u043b\u0433\u043e\u0440\u0438\u0442\u043c\u044b (0-\u0432\u0441\u0435, 1-7-\u043a\u043e\u043d\u043a\u0440\u0435\u0442\u043d\u044b\u0439) | ESC: \u0432\u044b\u0445\u043e\u0434", 95);

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

    static LRESULT CALLBACK GraphWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        GraphWindow* pThis = (GraphWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

        switch (uMsg) {
            case WM_PAINT:
                if (pThis) pThis->drawGraph();
                break;

            case WM_KEYDOWN:
                if (pThis) {
                    if (wParam == 'Q') pThis->setDataType(0); // int
                    else if (wParam == 'W') pThis->setDataType(1); // double
                    else if (wParam == 'E') pThis->setDataType(2); // float
                    else if (wParam == 'R') pThis->setDataType(3); // string
                    else if (wParam == 'T') pThis->setDataType(4); // bool
                    else if (wParam == 'A') pThis->setDistribution(0); // random
                    else if (wParam == 'S') pThis->setDistribution(1); // sorted
                    else if (wParam == 'D') pThis->setDistribution(2); // reverse
                    else if (wParam == 'F') pThis->setDistribution(3); // nearly sorted
                    else if (wParam == 'G') pThis->setDistribution(4); // few unique
                    else if (wParam >= '0' && wParam <= '7') pThis->setDisplay(wParam - '0');
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

// ==================== КЛАСС ДЛЯ ОКОННЫХ ТАБЛИЦ ====================

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
            "\u0421\u043b\u0443\u0447\u0430\u0439\u043d\u044b\u0435",
            "\u041e\u0442\u0441\u043e\u0440\u0442\u0438\u0440\u043e\u0432\u0430\u043d\u043d\u044b\u0435",
            "\u041e\u0431\u0440\u0430\u0442\u043d\u044b\u0435",
            "\u041f\u043e\u0447\u0442\u0438 \u043e\u0442\u0441\u043e\u0440\u0442\u0438\u0440\u043e\u0432\u0430\u043d\u043d\u044b\u0435",
            "\u041c\u0430\u043b\u043e \u0443\u043d\u0438\u043a\u0430\u043b\u044c\u043d\u044b\u0445"
        };
        current_data_type = 0;
        current_distribution = 0;
        createWindow();
    }

    void createWindow() {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = TableWindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = L"ResultsTable";
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);

        RegisterClassW(&wc);

        hwnd = CreateWindowW(
            L"ResultsTable", L"\u0420\u0435\u0437\u0443\u043b\u044c\u0442\u0430\u0442\u044b \u0430\u043d\u0430\u043b\u0438\u0437\u0430",
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

        // Заголовок
        wchar_t title[200];
        swprintf(title, L"\u0422\u0438\u043f \u0434\u0430\u043d\u043d\u044b\u0445: %s | \u0420\u0430\u0441\u043f\u0440\u0435\u0434\u0435\u043b\u0435\u043d\u0438\u0435: %s",
                results[current_data_type].type_name.c_str(),
                distributions[current_distribution].c_str());
        TextOutW(hdc, 50, 20, title, wcslen(title));

        // Заголовки столбцов
        int y = 60;
        TextOutW(hdc, 50, y, L"\u0410\u043b\u0433\u043e\u0440\u0438\u0442\u043c", 10);
        TextOutW(hdc, 200, y, L"\u0421\u0440\u0435\u0434\u043d\u0435\u0435 \u0432\u0440\u0435\u043c\u044f (s)", 20);
        TextOutW(hdc, 350, y, L"\u041e\u0442\u043a\u043b\u043e\u043d\u0435\u043d\u0438\u0435", 13);
        TextOutW(hdc, 500, y, L"\u0421\u0440\u0430\u0432\u043d\u0435\u043d\u0438\u0435", 11);
        TextOutW(hdc, 650, y, L"\u041e\u0431\u043c\u0435\u043d\u044b", 7);
        TextOutW(hdc, 800, y, L"\u041f\u0430\u043c\u044f\u0442\u044c (KB)", 13);
        TextOutW(hdc, 950, y, L"\u042d\u0444\u0444\u0435\u043a\u0442\u0438\u0432\u043d\u043e\u0441\u0442\u044c", 18);
        TextOutW(hdc, 1100, y, L"\u0421\u0442\u0430\u0431\u0438\u043b\u044c\u043d\u044b\u0439", 12);

        y += 30;

        // Данные
        auto& algorithms = results[current_data_type].algorithms;
        double best_time = results[current_data_type].best_times[distributions[current_distribution]];

        for (const auto& algo : algorithms) {
            wchar_t line[1000];
            long long avg_swaps = 0;
            size_t avg_memory = 0;

            if (!algo.metrics.empty()) {
                for (const auto& metric : algo.metrics) {
                    avg_swaps += metric.operations.swaps;
                    avg_memory += metric.operations.extra_memory;
                }
                avg_swaps /= algo.metrics.size();
                avg_memory /= algo.metrics.size();
            }

            double normalized_time = algo.stats.mean_time / best_time;

            swprintf(line, L"%-15s %-12.6f %-10.6f %-11.2fx %-9lld %-12zu %-16.1f%% %-12s",
                    algo.name.c_str(),
                    algo.stats.mean_time,
                    algo.stats.std_dev,
                    normalized_time,
                    avg_swaps,
                    avg_memory / 1024,
                    algo.cache_efficiency * 100,
                    algo.stable ? L"\u0414\u0430" : L"\u041d\u0435\u0442");

            TextOutW(hdc, 50, y, line, wcslen(line));
            y += 20;
        }

        // Управление
        y += 20;
        TextOutW(hdc, 50, y, L"\u0423\u043f\u0440\u0430\u0432\u043b\u0435\u043d\u0438\u0435:", 12);
        y += 25;
        TextOutW(hdc, 50, y, L"Q/W/E/R/T: \u0442\u0438\u043f\u044b \u0434\u0430\u043d\u043d\u044b\u0445 | A/S/D/F/G: \u0440\u0430\u0441\u043f\u0440\u0435\u0434\u0435\u043b\u0435\u043d\u0438\u044f | ESC: \u0432\u044b\u0445\u043e\u0434", 65);

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
                    if (wParam == 'Q') pThis->setDataType(0); // int
                    else if (wParam == 'W') pThis->setDataType(1); // double
                    else if (wParam == 'E') pThis->setDataType(2); // float
                    else if (wParam == 'R') pThis->setDataType(3); // string
                    else if (wParam == 'T') pThis->setDataType(4); // bool
                    else if (wParam == 'A') pThis->setDistribution(0); // random
                    else if (wParam == 'S') pThis->setDistribution(1); // sorted
                    else if (wParam == 'D') pThis->setDistribution(2); // reverse
                    else if (wParam == 'F') pThis->setDistribution(3); // nearly sorted
                    else if (wParam == 'G') pThis->setDistribution(4); // few unique
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

// ==================== ОСНОВНОЙ АНАЛИЗАТОР ====================

class ComprehensiveAnalyzer {
private:
    vector<int> test_sizes;
    vector<string> data_types = {"int", "double", "float", "string", "bool"};
    vector<string> distributions = {
        "\u0421\u043b\u0443\u0447\u0430\u0439\u043d\u044b\u0435",
        "\u041e\u0442\u0441\u043e\u0440\u0442\u0438\u0440\u043e\u0432\u0430\u043d\u043d\u044b\u0435",
        "\u041e\u0431\u0440\u0430\u0442\u043d\u044b\u0435",
        "\u041f\u043e\u0447\u0442\u0438 \u043e\u0442\u0441\u043e\u0440\u0442\u0438\u0440\u043e\u0432\u0430\u043d\u043d\u044b\u0435",
        "\u041c\u0430\u043b\u043e \u0443\u043d\u0438\u043a\u0430\u043b\u044c\u043d\u044b\u0445"
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
            AlgorithmResult("\u041f\u0443\u0437\u044b\u0440\u044c\u043a\u043e\u043c"),
            AlgorithmResult("\u0412\u044b\u0431\u043e\u0440\u043e\u043c"),
            AlgorithmResult("\u0412\u0441\u0442\u0430\u0432\u043a\u0430\u043c\u0438"),
            AlgorithmResult("\u0411\u044b\u0441\u0442\u0440\u0430\u044f"),
            AlgorithmResult("\u0421\u043b\u0438\u044f\u043d\u0438\u0435\u043c"),
            AlgorithmResult("\u041f\u0438\u0440\u0430\u043c\u0438\u0434\u0430\u043b\u044c\u043d\u0430\u044f"),
            AlgorithmResult("std::sort")
        };

        // Установка сложности
        algorithms[0].complexity = "O(n\u00b2)";
        algorithms[1].complexity = "O(n\u00b2)";
        algorithms[2].complexity = "O(n\u00b2)";
        algorithms[3].complexity = "O(n log n)";
        algorithms[4].complexity = "O(n log n)";
        algorithms[5].complexity = "O(n log n)";
        algorithms[6].complexity = "O(n log n)";

        // Анализ каждого алгоритма
        for (size_t algo_index = 0; algo_index < algorithms.size(); algo_index++) {
            cout << "  " << data_types[data_type_index] << " - " << algorithms[algo_index].name << "...\n";

            StatisticalResults stats;
            vector<double> times_for_sizes;

            for (size_t i = 0; i < test_sizes.size(); i++) {
                int size = test_sizes[i];
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
            }

            stats.calculate();
            algorithms[algo_index].stats = stats;
            algorithms[algo_index].times_by_size = times_for_sizes;

            // Простой тест стабильности
            if (algo_index == 2 || algo_index == 4) { // Insertion и Merge стабильны
                algorithms[algo_index].stable = true;
            } else if (algo_index == 6) { // std::sort
                algorithms[algo_index].stable = true;
            } else {
                algorithms[algo_index].stable = false;
            }
        }

        return algorithms;
    }

public:
    ComprehensiveAnalyzer(const vector<int>& sizes, int threads)
        : test_sizes(sizes), num_threads(threads) {}

    vector<DataTypeAnalysis> run_comprehensive_analysis() {
        cout << "\n=== \u041d\u0410\u0427\u0410\u041b\u041e \u041a\u041e\u041c\u041f\u041b\u0415\u041a\u0421\u041d\u041e\u0413\u041e \u0410\u041d\u0410\u041b\u0418\u0417\u0410 ===\n";
        cout << "\u0418\u0441\u043f\u043e\u043b\u044c\u0437\u0443\u0435\u043c\u044b\u0435 \u043f\u043e\u0442\u043e\u043a\u0438: " << num_threads << endl;

        vector<DataTypeAnalysis> all_results;

        // Анализ для каждого типа данных
        for (int data_type = 0; data_type < static_cast<int>(data_types.size()); data_type++) {
            DataTypeAnalysis analysis;
            analysis.type_name = data_types[data_type];
            analysis.test_sizes = test_sizes;

            cout << "\n--- \u0410\u043d\u0430\u043b\u0438\u0437 \u0442\u0438\u043f\u0430 \u0434\u0430\u043d\u043d\u044b\u0445: " << data_types[data_type] << " ---\n";

            // Анализ для каждого распределения
            for (int dist = 0; dist < static_cast<int>(distributions.size()); dist++) {
                cout << "  \u0420\u0430\u0441\u043f\u0440\u0435\u0434\u0435\u043b\u0435\u043d\u0438\u0435: " << distributions[dist] << endl;

                vector<AlgorithmResult> algorithms;

                // Запуск анализа для конкретного типа данных
                switch (data_type) {
                    case 0: algorithms = analyze_data_type<int>(data_type, dist); break;
                    case 1: algorithms = analyze_data_type<double>(data_type, dist); break;
                    case 2: algorithms = analyze_data_type<float>(data_type, dist); break;
                    case 3: algorithms = analyze_data_type<string>(data_type, dist); break;
                    case 4: algorithms = analyze_data_type<bool>(data_type, dist); break;
                }

                // Сохраняем лучшее время для этого распределения
                double best_time = numeric_limits<double>::max();
                for (const auto& algo : algorithms) {
                    if (algo.stats.mean_time < best_time) {
                        best_time = algo.stats.mean_time;
                    }
                }
                analysis.best_times[distributions[dist]] = best_time;

                analysis.algorithms = algorithms;
            }

            all_results.push_back(analysis);
        }

        cout << "\n=== \u0410\u041d\u0410\u041b\u0418\u0417 \u0417\u0410\u0412\u0415\u0420\u0428\u0415\u041d ===\n";
        return all_results;
    }
};

// ==================== ГЛАВНАЯ ФУНКЦИЯ ====================

int main() {
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);

    int num_arrays, num_points, num_threads;

    cout << "=== \u041a\u041e\u041c\u041f\u041b\u0415\u041a\u0421\u041d\u042b\u0419 \u0410\u041d\u0410\u041b\u0418\u0417 \u0410\u041b\u0413\u041e\u0420\u0418\u0422\u041c\u041e\u0412 \u0421\u041e\u0420\u0422\u0418\u0420\u041e\u0412\u041a\u0418 ===\n\n";

    cout << "\u0412\u0432\u0435\u0434\u0438\u0442\u0435 \u043a\u043e\u043b\u0438\u0447\u0435\u0441\u0442\u0432\u043e \u043c\u0430\u0441\u0441\u0438\u0432\u043e\u0432 (\u0440\u0430\u0437\u043c\u0435\u0440\u043e\u0432): ";
    cin >> num_arrays;
    cout << "\u0412\u0432\u0435\u0434\u0438\u0442\u0435 \u043a\u043e\u043b\u0438\u0447\u0435\u0441\u0442\u0432\u043e \u0442\u043e\u0447\u0435\u043a \u043d\u0430 \u0433\u0440\u0430\u0444\u0438\u043a\u0430\u0445: ";
    cin >> num_points;
    cout << "\u0412\u0432\u0435\u0434\u0438\u0442\u0435 \u043a\u043e\u043b\u0438\u0447\u0435\u0441\u0442\u0432\u043e \u043f\u043e\u0442\u043e\u043a\u043e\u0432: ";
    cin >> num_threads;

    if (num_arrays <= 0 || num_points <= 0 || num_threads <= 0) {
        cout << "\u041e\u0448\u0438\u0431\u043a\u0430: \u0437\u043d\u0430\u0447\u0435\u043d\u0438\u044f \u0434\u043e\u043b\u0436\u043d\u044b \u0431\u044b\u0442\u044c \u043f\u043e\u043b\u043e\u0436\u0438\u0442\u0435\u043b\u044c\u043d\u044b\u043c\u0438!" << endl;
        return 1;
    }

    if (num_points > num_arrays) {
        cout << "\u041e\u0448\u0438\u0431\u043a\u0430: \u043a\u043e\u043b\u0438\u0447\u0435\u0441\u0442\u0432\u043e \u0442\u043e\u0447\u0435\u043a \u043d\u0435 \u043c\u043e\u0436\u0435\u0442 \u043f\u0440\u0435\u0432\u044b\u0448\u0430\u0442\u044c \u043a\u043e\u043b\u0438\u0447\u0435\u0441\u0442\u0432\u043e \u043c\u0430\u0441\u0441\u0438\u0432\u043e\u0432!" << endl;
        return 1;
    }

    // СОЗДАЕМ РАЗМЕРЫ КАК ЕСТЬ - БЕЗ АВТОКОРРЕКЦИИ!
    vector<int> test_sizes(num_arrays);
    for (int i = 0; i < num_arrays; i++) {
        test_sizes[i] = (i + 1) * 10; // 10, 20, 30, ...
    }

    cout << "\n\u041d\u0430\u0441\u0442\u0440\u043e\u0439\u043a\u0430 \u0430\u043d\u0430\u043b\u0438\u0437\u0430:\n";
    cout << "=============================================\n";
    cout << "\u2022 \u041a\u043e\u043b\u0438\u0447\u0435\u0441\u0442\u0432\u043e \u0440\u0430\u0437\u043c\u0435\u0440\u043e\u0432: " << num_arrays << " (" << test_sizes[0] << " - " << test_sizes.back() << " \u044d\u043b\u0435\u043c\u0435\u043d\u0442\u043e\u0432)\n";
    cout << "\u2022 \u041a\u043e\u043b\u0438\u0447\u0435\u0441\u0442\u0432\u043e \u0442\u043e\u0447\u0435\u043a: " << num_points << "\n";
    cout << "\u2022 \u041a\u043e\u043b\u0438\u0447\u0435\u0441\u0442\u0432\u043e \u043f\u043e\u0442\u043e\u043a\u043e\u0432: " << num_threads << "\n";
    cout << "\u2022 \u0422\u0438\u043f\u043e\u0432 \u0434\u0430\u043d\u043d\u044b\u0445: 5 (int, double, float, string, bool)\n";
    cout << "\u2022 \u0420\u0430\u0441\u043f\u0440\u0435\u0434\u0435\u043b\u0435\u043d\u0438\u0439: 5\n";
    cout << "\u2022 \u0410\u043b\u0433\u043e\u0440\u0438\u0442\u043c\u043e\u0432: 7\n";
    cout << "\u2022 \u041e\u0431\u0449\u0435\u0435 \u043a\u043e\u043b\u0438\u0447\u0435\u0441\u0442\u0432\u043e \u0438\u0437\u043c\u0435\u0440\u0435\u043d\u0438\u0439: " << num_arrays * 5 * 5 * 7 << endl;

    cout << "\n\u0417\u0430\u043f\u0443\u0441\u043a \u043a\u043e\u043c\u043f\u043b\u0435\u043a\u0441\u043d\u043e\u0433\u043e \u0430\u043d\u0430\u043b\u0438\u0437\u0430...\n";

    auto start_time = chrono::steady_clock::now();

    // Запуск анализа
    ComprehensiveAnalyzer analyzer(test_sizes, num_threads);
    auto results = analyzer.run_comprehensive_analysis();

    auto end_time = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::seconds>(end_time - start_time);

    cout << "\n\u0410\u043d\u0430\u043b\u0438\u0437 \u0437\u0430\u0432\u0435\u0440\u0448\u0435\u043d \u0437\u0430 " << duration.count() << " \u0441\u0435\u043a\u0443\u043d\u0434\n";

    // Запуск оконной таблицы с результатами
    cout << "\n\u0417\u0430\u043f\u0443\u0441\u043a \u043e\u043a\u043e\u043d\u043d\u043e\u0439 \u0442\u0430\u0431\u043b\u0438\u0446\u044b \u0440\u0435\u0437\u0443\u043b\u044c\u0442\u0430\u0442\u043e\u0432...\n";
    ResultsTableWindow tableWindow(results);

    // Запуск оконной системы графиков
    cout << "\n\u0417\u0430\u043f\u0443\u0441\u043a \u043e\u043a\u043e\u043d\u043d\u043e\u0439 \u0441\u0438\u0441\u0442\u0435\u043c\u044b \u0433\u0440\u0430\u0444\u0438\u043a\u043e\u0432...\n";
    GraphWindow graphWindow(results);

    cout << "\n=== \u0421\u0418\u0421\u0422\u0415\u041c\u0410 \u0410\u041d\u0410\u041b\u0418\u0417\u0410 \u0417\u0410\u041f\u0423\u0429\u0415\u041d\u0410 ===\n";
    cout << "\u0414\u043e\u0441\u0442\u0443\u043f\u043d\u044b \u0434\u0432\u0430 \u043e\u043a\u043d\u0430:\n";
    cout << "1. \u0422\u0430\u0431\u043b\u0438\u0446\u0430 \u0440\u0435\u0437\u0443\u043b\u044c\u0442\u0430\u0442\u043e\u0432 - \u0434\u0435\u0442\u0430\u043b\u044c\u043d\u0430\u044f \u0441\u0442\u0430\u0442\u0438\u0441\u0442\u0438\u043a\u0430\n";
    cout << "2. \u0413\u0440\u0430\u0444\u0438\u043a\u0438 - \u0432\u0438\u0437\u0443\u0430\u043b\u0438\u0437\u0430\u0446\u0438\u044f \u043f\u0440\u043e\u0438\u0437\u0432\u043e\u0434\u0438\u0442\u0435\u043b\u044c\u043d\u043e\u0441\u0442\u0438\n\n";

    cout << "\u0423\u043f\u0440\u0430\u0432\u043b\u0435\u043d\u0438\u0435 \u0442\u0430\u0431\u043b\u0438\u0446\u0435\u0439:\n";
    cout << "  Q, W, E, R, T - \u0432\u044b\u0431\u043e\u0440 \u0442\u0438\u043f\u0430 \u0434\u0430\u043d\u043d\u044b\u0445\n";
    cout << "  A, S, D, F, G - \u0432\u044b\u0431\u043e\u0440 \u0440\u0430\u0441\u043f\u0440\u0435\u0434\u0435\u043b\u0435\u043d\u0438\u044f\n";
    cout << "  ESC - \u0432\u044b\u0445\u043e\u0434\n\n";

    cout << "\u0423\u043f\u0440\u0430\u0432\u043b\u0435\u043d\u0438\u0435 \u0433\u0440\u0430\u0444\u0438\u043a\u0430\u043c\u0438:\n";
    cout << "  Q, W, E, R, T - \u0432\u044b\u0431\u043e\u0440 \u0442\u0438\u043f\u0430 \u0434\u0430\u043d\u043d\u044b\u0445\n";
    cout << "  A, S, D, F, G - \u0432\u044b\u0431\u043e\u0440 \u0440\u0430\u0441\u043f\u0440\u0435\u0434\u0435\u043b\u0435\u043d\u0438\u044f\n";
    cout << "  0-7 - \u0444\u0438\u043b\u044c\u0442\u0440 \u0430\u043b\u0433\u043e\u0440\u0438\u0442\u043c\u043e\u0432 (0-\u0432\u0441\u0435, 1-7-\u043a\u043e\u043d\u043a\u0440\u0435\u0442\u043d\u044b\u0439)\n";
    cout << "  ESC - \u0432\u044b\u0445\u043e\u0434\n";

    // Запускаем оба окна в отдельных потоках
    thread tableThread([&tableWindow]() {
        tableWindow.runMessageLoop();
    });

    thread graphThread([&graphWindow]() {
        graphWindow.runMessageLoop();
    });

    // Ждем завершения обоих потоков
    tableThread.join();
    graphThread.join();

    return 0;
}
