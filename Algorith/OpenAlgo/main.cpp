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

// ==================== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ====================

/**
 * Конвертирует string в wstring для совместимости с Windows API
 * @param str - входная строка в UTF-8
 * @return wstring в UTF-16 для Windows
 */
wstring string_to_wstring(const string& str) {
    if (str.empty()) return wstring();
    // Определяем необходимый размер буфера
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    wstring wstrTo(size_needed, 0);
    // Выполняем конвертацию
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// ==================== СТРУКТУРЫ ДАННЫХ ====================

/**
 * Структура для подсчета операций сортировки
 * Содержит счетчики сравнений, обменов, доступа к памяти и использования памяти
 */
struct OperationCounts {
    long long comparisons = 0;    // Количество сравнений элементов
    long long swaps = 0;          // Количество обменов элементов
    long long memory_access = 0;  // Количество обращений к памяти
    size_t extra_memory = 0;      // Пиковое использование дополнительной памяти
    size_t current_memory = 0;    // Текущее использование памяти
    vector<int> accessed_indices; // Индексы accessed элементов (для анализа кэша)

    // Сброс всех счетчиков
    void reset() {
        comparisons = swaps = memory_access = 0;
        extra_memory = current_memory = 0;
        accessed_indices.clear();
        accessed_indices.shrink_to_fit();
    }

    // Обновление пикового использования памяти
    void update_peak_memory() {
        if (current_memory > extra_memory) {
            extra_memory = current_memory;
        }
    }

    // Добавление используемой памяти
    void add_memory(size_t bytes) {
        current_memory += bytes;
        update_peak_memory();
    }

    // Освобождение памяти
    void remove_memory(size_t bytes) {
        if (current_memory >= bytes) {
            current_memory -= bytes;
        }
    }

    // Суммирование операций
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

/**
 * Детальные метрики для одного запуска алгоритма
 */
struct DetailedMetrics {
    double time;                // Время выполнения в секундах
    OperationCounts operations; // Операции выполненные алгоритмом
    size_t memory_used;         // Использованная память в байтах
    bool stable;               // Флаг стабильности алгоритма

    DetailedMetrics() : time(0.0), memory_used(0), stable(false) {}
};

/**
 * Статистические результаты по множеству запусков
 */
struct StatisticalResults {
    double mean_time;           // Среднее время выполнения
    double std_dev;             // Стандартное отклонение
    double min_time;            // Минимальное время
    double max_time;            // Максимальное время
    double confidence_interval; // Доверительный интервал (95%)
    vector<double> all_measurements; // Все измерения времени

    // Расчет статистических показателей
    void calculate() {
        if (all_measurements.empty()) return;

        // Расчет среднего значения
        mean_time = accumulate(all_measurements.begin(), all_measurements.end(), 0.0) / all_measurements.size();

        // Расчет стандартного отклонения
        double sq_sum = 0.0;
        for (double t : all_measurements) {
            sq_sum += (t - mean_time) * (t - mean_time);
        }
        std_dev = sqrt(sq_sum / all_measurements.size());

        // Поиск минимального и максимального времени
        min_time = *min_element(all_measurements.begin(), all_measurements.end());
        max_time = *max_element(all_measurements.begin(), all_measurements.end());

        // Расчет доверительного интервала (95%)
        confidence_interval = 1.96 * std_dev / sqrt(all_measurements.size());
    }
};

/**
 * Результаты работы алгоритма сортировки
 */
struct AlgorithmResult {
    string name;                // Название алгоритма
    vector<DetailedMetrics> metrics; // Метрики для каждого размера данных
    StatisticalResults stats;   // Статистические результаты
    double cache_efficiency;    // Эффективность кэша (0-1)
    bool stable;               // Стабильность алгоритма
    string complexity;         // Вычислительная сложность
    vector<double> times_by_size; // Времена для каждого размера массива
    OperationCounts avg_operations; // Средние операции

    AlgorithmResult(const string& n) : name(n), cache_efficiency(0), stable(false) {}

    // Расчет средних операций по всем метрикам
    void calculateAverageOperations() {
        if (metrics.empty()) return;

        avg_operations.reset();
        for (const auto& metric : metrics) {
            avg_operations += metric.operations;
        }

        // Усреднение значений
        avg_operations.comparisons /= metrics.size();
        avg_operations.swaps /= metrics.size();
        avg_operations.memory_access /= metrics.size();
        avg_operations.extra_memory /= metrics.size();
    }
};

/**
 * Анализ производительности для конкретного типа данных
 */
struct DataTypeAnalysis {
    string type_name;           // Название типа данных
    vector<AlgorithmResult> algorithms; // Результаты алгоритмов
    map<string, double> best_times;    // Лучшие времена для каждого размера
    vector<int> test_sizes;     // Размеры тестовых данных
    map<string, vector<AlgorithmResult>> algorithms_by_distribution; // Результаты по распределениям

    // Получение алгоритмов для конкретного распределения данных
    vector<AlgorithmResult> getAlgorithmsForDistribution(const string& distribution) const {
        auto it = algorithms_by_distribution.find(distribution);
        if (it != algorithms_by_distribution.end() && !it->second.empty()) {
            return it->second;
        }

        // Если для распределения нет данных, используем основные алгоритмы
        if (!algorithms_by_distribution.empty()) {
            cout << "Warning: No data for distribution '" << distribution
                 << "'. Using default algorithms." << endl;
        }

        return algorithms;
    }
};

// ==================== СТРУКТУРЫ ДЛЯ JSON СЕРИАЛИЗАЦИИ ====================

/**
 * Упрощенная структура OperationCounts для сохранения в JSON
 */
struct SavedOperationCounts {
    long long comparisons = 0;
    long long swaps = 0;
    long long memory_access = 0;
    size_t extra_memory = 0;
};

/**
 * Упрощенная структура StatisticalResults для сохранения в JSON
 */
struct SavedStatisticalResults {
    double mean_time;
    double std_dev;
    double min_time;
    double max_time;
    double confidence_interval;
    vector<double> all_measurements;
};

/**
 * Упрощенная структура AlgorithmResult для сохранения в JSON
 */
struct SavedAlgorithmResult {
    string name;
    SavedStatisticalResults stats;
    double cache_efficiency;
    bool stable;
    string complexity;
    vector<double> times_by_size;
    SavedOperationCounts avg_operations;
};

/**
 * Упрощенная структура DataTypeAnalysis для сохранения в JSON
 */
struct SavedDataTypeAnalysis {
    string type_name;
    vector<SavedAlgorithmResult> algorithms;
    map<string, double> best_times;
    vector<int> test_sizes;
    map<string, vector<SavedAlgorithmResult>> algorithms_by_distribution;
};

/**
 * Структура для сохранения всей сессии анализа
 */
struct AnalysisSession {
    string timestamp;           // Временная метка создания
    string version = "1.0";     // Версия формата данных
    vector<SavedDataTypeAnalysis> results; // Все результаты
    vector<string> distributions; // Использованные распределения
    vector<int> original_test_sizes; // Исходные размеры тестов
    int num_threads;            // Количество потоков
    double total_duration_seconds; // Общее время выполнения
};

// ==================== СИСТЕМА ЗАГРУЗКИ РЕЗУЛЬТАТОВ ====================

/**
 * Класс для загрузки результатов из JSON файлов
 * Конвертирует сохраненные данные обратно в рабочие структуры
 */
class ResultsLoader {
public:
    /**
     * Конвертирует SavedOperationCounts в OperationCounts
     */
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

    /**
     * Конвертирует SavedStatisticalResults в StatisticalResults
     */
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

    /**
     * Конвертирует SavedAlgorithmResult в AlgorithmResult
     */
    static AlgorithmResult convert(const SavedAlgorithmResult& saved) {
        AlgorithmResult algo(saved.name);
        algo.stats = convert(saved.stats);
        algo.cache_efficiency = saved.cache_efficiency;
        algo.stable = saved.stable;
        algo.complexity = saved.complexity;
        algo.times_by_size = saved.times_by_size;
        algo.avg_operations = convert(saved.avg_operations);

        // Восстановление metrics из times_by_size
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

    /**
     * Конвертирует SavedDataTypeAnalysis в DataTypeAnalysis
     */
    static DataTypeAnalysis convert(const SavedDataTypeAnalysis& saved) {
        DataTypeAnalysis analysis;
        analysis.type_name = saved.type_name;
        analysis.test_sizes = saved.test_sizes;
        analysis.best_times = saved.best_times;

        // Восстановление основных алгоритмов
        for (const auto& saved_algo : saved.algorithms) {
            analysis.algorithms.push_back(convert(saved_algo));
        }

        // Восстановление алгоритмов по распределениям
        for (const auto& [dist, saved_algos] : saved.algorithms_by_distribution) {
            vector<AlgorithmResult> algos;
            for (const auto& saved_algo : saved_algos) {
                algos.push_back(convert(saved_algo));
            }
            analysis.algorithms_by_distribution[dist] = algos;
        }

        return analysis;
    }

    /**
     * Основная функция загрузки данных из JSON файла
     * @param filename - имя файла для загрузки
     * @return вектор с результатами анализа
     */
    static vector<DataTypeAnalysis> loadFromJSON(const string& filename) {
        vector<DataTypeAnalysis> results;

        ifstream file(filename);
        if (!file.is_open()) {
            cerr << "Error: Cannot open file " << filename << " for reading" << endl;
            return results;
        }

        try {
            cout << "Parsing JSON file: " << filename << endl;

            // Парсинг JSON
            json j;
            file >> j;

            // Проверка валидности JSON
            if (!validateJSON(j)) {
                cerr << "Error: Invalid JSON format or missing required fields" << endl;
                return results;
            }

            cout << "JSON version: " << j["version"] << ", timestamp: " << j["timestamp"] << endl;
            cout << "Test sizes: " << j["test_sizes"].size() << ", distributions: " << j["distributions"].size() << endl;

            // Конвертация каждой DataTypeAnalysis
            for (const auto& json_data_type : j["results"]) {
                SavedDataTypeAnalysis saved = parseDataTypeAnalysis(json_data_type);
                DataTypeAnalysis analysis = convert(saved);
                results.push_back(analysis);

                cout << "Loaded data type: " << analysis.type_name
                     << " with " << analysis.algorithms.size() << " main algorithms"
                     << " and " << analysis.algorithms_by_distribution.size() << " distributions" << endl;

                // Вывод информации о распределениях
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
    /**
     * Проверка валидности JSON структуры
     */
    static bool validateJSON(const json& j) {
        // Проверка обязательных полей
        if (!j.contains("timestamp") || !j.contains("version") ||
            !j.contains("results") || !j.contains("test_sizes")) {
            return false;
        }

        // Проверка что results является массивом
        if (!j["results"].is_array()) {
            return false;
        }

        return true;
    }

    /**
     * Парсинг DataTypeAnalysis из JSON
     */
    static SavedDataTypeAnalysis parseDataTypeAnalysis(const json& j) {
        SavedDataTypeAnalysis saved;

        saved.type_name = j["type_name"];
        saved.test_sizes = j["test_sizes"].get<vector<int>>();

        // Парсинг best_times
        if (j.contains("best_times") && j["best_times"].is_object()) {
            for (auto& [key, value] : j["best_times"].items()) {
                saved.best_times[key] = value.get<double>();
            }
        }

        // Парсинг основных алгоритмов
        if (j.contains("algorithms") && j["algorithms"].is_array()) {
            for (const auto& json_algo : j["algorithms"]) {
                saved.algorithms.push_back(parseAlgorithmResult(json_algo));
            }
        }

        // Парсинг алгоритмов по распределениям
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

    /**
     * Парсинг AlgorithmResult из JSON
     */
    static SavedAlgorithmResult parseAlgorithmResult(const json& j) {
        SavedAlgorithmResult saved;

        saved.name = j["name"];
        saved.cache_efficiency = j["cache_efficiency"];
        saved.stable = j["stable"];
        saved.complexity = j["complexity"];

        // Парсинг times_by_size
        if (j.contains("times_by_size") && j["times_by_size"].is_array()) {
            saved.times_by_size = j["times_by_size"].get<vector<double>>();
        }

        // Парсинг статистики
        if (j.contains("stats") && j["stats"].is_object()) {
            saved.stats = parseStatisticalResults(j["stats"]);
        }

        // Парсинг операций
        if (j.contains("operations") && j["operations"].is_object()) {
            saved.avg_operations = parseOperationCounts(j["operations"]);
        }

        return saved;
    }

    /**
     * Парсинг StatisticalResults из JSON
     */
    static SavedStatisticalResults parseStatisticalResults(const json& j) {
        SavedStatisticalResults saved;

        saved.mean_time = j["mean_time"];
        saved.std_dev = j["std_dev"];
        saved.min_time = j["min_time"];
        saved.max_time = j["max_time"];
        saved.confidence_interval = j["confidence_interval"];

        // Парсинг all_measurements
        if (j.contains("all_measurements") && j["all_measurements"].is_array()) {
            saved.all_measurements = j["all_measurements"].get<vector<double>>();
        }

        return saved;
    }

    /**
     * Парсинг OperationCounts из JSON
     */
    static SavedOperationCounts parseOperationCounts(const json& j) {
        SavedOperationCounts saved;

        saved.comparisons = j["comparisons"];
        saved.swaps = j["swaps"];
        saved.memory_access = j["memory_access"];
        saved.extra_memory = j["extra_memory"];

        return saved;
    }
};

// ==================== ОКНО ГРАФИКОВ (УЛУЧШЕННАЯ ВЕРСИЯ) ====================

/**
 * Класс для отображения графиков производительности
 * с улучшенным интерфейсом и адаптивным размером
 */
class GraphWindow {
private:
    HWND hwnd;                          // Handle окна
    HDC hdc;                            // Device context
    vector<DataTypeAnalysis> results;   // Загруженные результаты
    vector<string> distributions;       // Доступные распределения данных
    size_t current_data_type;           // Текущий тип данных
    size_t current_distribution;        // Текущее распределение
    int current_display;                // Текущий режим отображения (0-все, 1-7-конкретные)
    bool log_scale_x;                   // Логарифмическая шкала по X
    bool log_scale_y;                   // Логарифмическая шкала по Y
    bool normalized_view;               // Нормализованное отображение
    COLORREF background_color;          // Цвет фона
    COLORREF grid_color;                // Цвет сетки
    COLORREF text_color;                // Цвет текста

public:
    GraphWindow(const vector<DataTypeAnalysis>& res) : results(res) {
        // Инициализация распределений данных
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

        // Установка приятных цветов
        background_color = RGB(245, 245, 245);  // Светло-серый фон
        grid_color = RGB(220, 220, 220);        // Светло-серая сетка
        text_color = RGB(50, 50, 50);           // Темно-серый текст

        createWindow();
    }

    /**
     * Создание графического окна
     */
    void createWindow() {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = GraphWindowProc;    // Обработчик сообщений
        wc.hInstance = GetModuleHandle(NULL); // Handle приложения
        wc.lpszClassName = L"GraphWindow";   // Имя класса
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // Цвет фона
        wc.hCursor = LoadCursor(NULL, IDC_ARROW); // Курсор

        RegisterClassW(&wc);

        // Создание окна с увеличенным размером для лучшего отображения
        hwnd = CreateWindowW(
            L"GraphWindow",
            L"Performance Analysis - Advanced Viewer", // Красивое название
            WS_OVERLAPPEDWINDOW | WS_MAXIMIZEBOX, // С возможностью максимизации
            100, 100, 1400, 900,  // Увеличенный начальный размер
            NULL, NULL, GetModuleHandle(NULL), this
        );

        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);
    }

    /**
     * Отрисовка графика с улучшенным дизайном
     */
    void drawGraph() {
        PAINTSTRUCT ps;
        hdc = BeginPaint(hwnd, &ps);

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);

        // Заполнение фона приятным цветом
        HBRUSH background_brush = CreateSolidBrush(background_color);
        FillRect(hdc, &clientRect, background_brush);
        DeleteObject(background_brush);

        // Создание улучшенного заголовка
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

        // Улучшенный шрифт для заголовка
        HFONT hFont = CreateFontW(18, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Segoe UI");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        SetTextColor(hdc, text_color);
        SetBkMode(hdc, TRANSPARENT);
        TextOutW(hdc, 50, 25, title, wcslen(title));
        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);

        // Расчет размеров графика с адаптацией под размер окна и учетом отступов для оси X
        int margin_top = 100;    // Верхний отступ
        int margin_bottom = 120; // Нижний отступ (увеличили для оси X)
        int margin_left = 100;   // Левый отступ
        int margin_right = 250;  // Правый отступ (для легенды)

        int graphWidth = clientRect.right - margin_left - margin_right;
        int graphHeight = clientRect.bottom - margin_top - margin_bottom;
        int graphTop = margin_top;
        int graphBottom = graphTop + graphHeight;
        int graphLeft = margin_left;
        int graphRight = graphLeft + graphWidth;

        // Проверка валидности размеров
        if (graphWidth <= 100 || graphHeight <= 100) {
            EndPaint(hwnd, &ps);
            return;
        }

        // Получение текущих алгоритмов и размеров
        auto current_algorithms = results[current_data_type].getAlgorithmsForDistribution(distributions[current_distribution]);
        auto& sizes = results[current_data_type].test_sizes;

        // Подготовка данных для отображения
        vector<double> x_values;
        vector<vector<double>> y_values(current_algorithms.size());
        vector<vector<double>> original_y_values(current_algorithms.size());

        // Расчет значений по X
        for (int size : sizes) {
            if (log_scale_x) {
                x_values.push_back(log10(max(1, size)));
            } else {
                x_values.push_back(size);
            }
        }

        double min_x = *min_element(x_values.begin(), x_values.end());
        double max_x = *max_element(x_values.begin(), x_values.end());

        // Расчет диапазона по Y
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

                // Нормализация относительно std::sort
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

                // Логарифмическое преобразование
                if (log_scale_y) {
                    display_val = log10(max(1e-10, display_val));
                }

                y_values[algo_idx].push_back(display_val);

                // Обновление диапазона
                if (current_display == 0 || current_display == static_cast<int>(algo_idx) + 1) {
                    min_time = min(min_time, display_val);
                    max_time = max(max_time, display_val);
                }
            }
        }

        // Настройка диапазона по Y если данные отсутствуют
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
            // Добавление небольших отступов
            double range = max_time - min_time;
            min_time -= range * 0.05;
            max_time += range * 0.05;
        }

        // Отрисовка осей с улучшенным дизайном
        HPEN axisPen = CreatePen(PS_SOLID, 3, RGB(80, 80, 80)); // Темно-серые оси
        HPEN oldPen = (HPEN)SelectObject(hdc, axisPen);

        // Ось X (оставляем место снизу для подписей)
        MoveToEx(hdc, graphLeft, graphBottom, NULL);
        LineTo(hdc, graphRight, graphBottom);

        // Ось Y
        MoveToEx(hdc, graphLeft, graphTop, NULL);
        LineTo(hdc, graphLeft, graphBottom);

        // Настройка текста
        SetTextColor(hdc, text_color);
        SetBkMode(hdc, TRANSPARENT);

        // Подписи осей
        wchar_t x_label[50];
        if (log_scale_x) {
            swprintf(x_label, 50, L"log₁₀(Array Size)");
        } else {
            swprintf(x_label, 50, L"Array Size");
        }

        HFONT labelFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                    DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                                    CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Segoe UI");
        HFONT hOldLabelFont = (HFONT)SelectObject(hdc, labelFont);

        // Подпись оси X (центрируем по горизонтали)
        int x_label_width = wcslen(x_label) * 8; // Примерная ширина текста
        TextOutW(hdc, graphLeft + (graphWidth - x_label_width) / 2, graphBottom + 40, x_label, wcslen(x_label));

        wchar_t y_label[50];
        if (normalized_view) {
            swprintf(y_label, 50, L"Normalized Time");
        } else if (log_scale_y) {
            swprintf(y_label, 50, L"log₁₀(Time (s))");
        } else {
            swprintf(y_label, 50, L"Time (s)");
        }

        // Вертикальная подпись оси Y
        LOGFONTW lf;
        GetObjectW(GetCurrentObject(hdc, OBJ_FONT), sizeof(LOGFONTW), &lf);
        lf.lfEscapement = 900; // Поворот на 90 градусов
        HFONT hFontVert = CreateFontIndirectW(&lf);
        HFONT hOldFontVert = (HFONT)SelectObject(hdc, hFontVert);

        // Центрируем подпись оси Y по вертикали
        int y_label_width = wcslen(y_label) * 8;
        TextOutW(hdc, graphLeft - 70, graphTop + (graphHeight - y_label_width) / 2, y_label, wcslen(y_label));

        SelectObject(hdc, hOldFontVert);
        DeleteObject(hFontVert);

        // Отрисовка сетки и подписей значений по оси Y
        HPEN gridPen = CreatePen(PS_SOLID, 1, grid_color);
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

            // Подписи значений по оси Y
            TextOutW(hdc, graphLeft - 40, y_pos - 52, label, wcslen(label));

            // Горизонтальные линии сетки
            SelectObject(hdc, gridPen);
            MoveToEx(hdc, graphLeft, y_pos, NULL);
            LineTo(hdc, graphRight, y_pos);
            SelectObject(hdc, axisPen);
        }

        // Подписи по оси X (убедимся, что они помещаются)
        if (!sizes.empty()) {
            int step = max(1, static_cast<int>(sizes.size()) / 8);
            for (size_t i = 0; i < sizes.size(); i += step) {
                double x_val = x_values[i];
                int x = graphLeft + static_cast<int>((graphWidth * (x_val - min_x)) / (max_x - min_x));

                // Проверяем, чтобы подпись не выходила за границы
                if (x >= graphLeft && x <= graphRight) {
                    wchar_t size_label[20];
                    if (log_scale_x) {
                        swprintf(size_label, 20, L"10^%.0f", x_val);
                    } else if (sizes[i] >= 1000) {
                        swprintf(size_label, 20, L"%dk", sizes[i] / 1000);
                    } else {
                        swprintf(size_label, 20, L"%d", sizes[i]);
                    }
                    TextOutW(hdc, x - 15, graphBottom + 10, size_label, wcslen(size_label));
                }
            }
        }

        DeleteObject(gridPen);
        SelectObject(hdc, hOldLabelFont);
        DeleteObject(labelFont);

        // Цвета для алгоритмов (приятная палитра)
        vector<COLORREF> colors = {
            RGB(231, 76, 60),    // Красный - Bubble
            RGB(52, 152, 219),   // Синий - Selection
            RGB(46, 204, 113),   // Зеленый - Insertion
            RGB(155, 89, 182),   // Фиолетовый - Quick
            RGB(241, 196, 15),   // Желтый - Merge
            RGB(230, 126, 34),   // Оранжевый - Heap
            RGB(44, 62, 80)      // Темно-синий - std::sort
        };

        // Отрисовка точек данных (БЕЗ ЛИНИЙ и БЕЗ КОНТУРА)
        if (!sizes.empty() && !current_algorithms.empty()) {
            for (size_t algo_idx = 0; algo_idx < current_algorithms.size(); algo_idx++) {
                if (current_display != 0 && current_display != static_cast<int>(algo_idx) + 1) {
                    continue;
                }

                if (y_values[algo_idx].size() != sizes.size()) continue;

                COLORREF algo_color = colors[algo_idx % colors.size()];

                // Отрисовка только точек (без линий и без контура)
                for (size_t i = 0; i < sizes.size(); i++) {
                    if (original_y_values[algo_idx][i] <= 0) {
                        continue;
                    }

                    int x = graphLeft + static_cast<int>((graphWidth * (x_values[i] - min_x)) / (max_x - min_x));
                    int y = graphBottom - static_cast<int>((graphHeight * (y_values[algo_idx][i] - min_time)) / (max_time - min_time));

                    // Ограничение координат в пределах графика
                    y = max(graphTop, min(graphBottom, y));
                    x = max(graphLeft, min(graphRight, x));

                    // Создание кисти для точки (БЕЗ КОНТУРА)
                    HBRUSH pointBrush = CreateSolidBrush(algo_color);
                    HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, pointBrush);

                    // Отрисовка точки большего размера для лучшей видимости
                    // Используем Ellipse но БЕЗ обводки
                    SelectObject(hdc, GetStockObject(NULL_PEN)); // Убираем контур
                    Ellipse(hdc, x - 4, y - 4, x + 4, y + 4);

                    // Восстанавливаем перо
                    SelectObject(hdc, oldPen);

                    SelectObject(hdc, oldBrush);
                    DeleteObject(pointBrush);
                }
            }
        }

        // Улучшенная легенда (размещаем в правой части с учетом новых отступов)
        int legendX = graphRight + 20;
        int legendY = graphTop + 20;

        HFONT legendFont = CreateFontW(14, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                     DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                                     CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Segoe UI");
        HFONT hOldLegendFont = (HFONT)SelectObject(hdc, legendFont);

        // Фон легенды
        HBRUSH legendBgBrush = CreateSolidBrush(RGB(255, 255, 255));
        HBRUSH hOldLegendBrush = (HBRUSH)SelectObject(hdc, legendBgBrush);
        Rectangle(hdc, legendX - 10, legendY - 10, legendX + 210, legendY + 180);
        SelectObject(hdc, hOldLegendBrush);
        DeleteObject(legendBgBrush);

        // Обводка легенды
        HPEN legendBorderPen = CreatePen(PS_SOLID, 1, RGB(200, 200, 200));
        HPEN hOldLegendPen = (HPEN)SelectObject(hdc, legendBorderPen);
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, legendX - 10, legendY - 10, legendX + 210, legendY + 180);
        SelectObject(hdc, hOldLegendPen);
        DeleteObject(legendBorderPen);

        SetTextColor(hdc, RGB(60, 60, 60));
        TextOutW(hdc, legendX, legendY, L"Algorithms Legend", 20);

        const wchar_t* algo_names[] = {
            L"● Bubble Sort", L"● Selection Sort", L"● Insertion Sort",
            L"● Quick Sort", L"● Merge Sort", L"● Heap Sort", L"● std::sort"
        };

        for (int i = 0; i < 7; i++) {
            if (current_display == 0 || current_display == i + 1) {
                // Цветной квадратик (БЕЗ КОНТУРА)
                HBRUSH legendBrush = CreateSolidBrush(colors[i]);
                HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, legendBrush);
                SelectObject(hdc, GetStockObject(NULL_PEN)); // Убираем контур
                Rectangle(hdc, legendX, legendY + 30 + i * 22, legendX + 15, legendY + 45 + i * 22);
                SelectObject(hdc, oldPen); // Восстанавливаем перо
                SelectObject(hdc, oldBrush);
                DeleteObject(legendBrush);

                // Название алгоритма
                SetTextColor(hdc, colors[i]);
                TextOutW(hdc, legendX + 20, legendY + 28 + i * 22, algo_names[i], wcslen(algo_names[i]));
            }
        }

        // Восстановление оригинальных объектов
        SelectObject(hdc, hOldLegendFont);
        DeleteObject(legendFont);
        SelectObject(hdc, oldPen);
        DeleteObject(axisPen);

        // Улучшенная панель управления (размещаем внизу с учетом новых отступов)
        int controlsY = graphBottom + 60;
        HFONT controlsFont = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                       DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                                       CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Segoe UI");
        HFONT hOldControlsFont = (HFONT)SelectObject(hdc, controlsFont);

        SetTextColor(hdc, RGB(70, 70, 70));
        TextOutW(hdc, graphLeft, controlsY,
                L"Controls: Q/W/E/R/T - Data Types | A/S/D/F/G - Distributions | 0-7 - Algorithms", 85);
        TextOutW(hdc, graphLeft, controlsY + 25,
                L"L - Toggle Log X | K - Toggle Log Y | N - Toggle Normalized View | ESC - Exit", 75);

        SelectObject(hdc, hOldControlsFont);
        DeleteObject(controlsFont);

        EndPaint(hwnd, &ps);
    }

    // Методы управления отображением
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

    /**
     * Обработчик сообщений окна
     */
    static LRESULT CALLBACK GraphWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        GraphWindow* pThis = (GraphWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

        switch (uMsg) {
            case WM_PAINT:
                if (pThis) pThis->drawGraph();
                break;

            case WM_SIZE:
                // Перерисовка при изменении размера окна
                InvalidateRect(hwnd, NULL, TRUE);
                break;

            case WM_KEYDOWN:
                if (pThis) {
                    // Управление типами данных
                    if (wParam == 'Q') pThis->setDataType(0);
                    else if (wParam == 'W') pThis->setDataType(1);
                    else if (wParam == 'E') pThis->setDataType(2);
                    else if (wParam == 'R') pThis->setDataType(3);
                    else if (wParam == 'T') pThis->setDataType(4);
                    // Управление распределениями
                    else if (wParam == 'A') pThis->setDistribution(0);
                    else if (wParam == 'S') pThis->setDistribution(1);
                    else if (wParam == 'D') pThis->setDistribution(2);
                    else if (wParam == 'F') pThis->setDistribution(3);
                    else if (wParam == 'G') pThis->setDistribution(4);
                    // Управление алгоритмами
                    else if (wParam >= '0' && wParam <= '7') pThis->setDisplay(wParam - '0');
                    // Управление отображением
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

    /**
     * Запуск цикла сообщений
     */
    void runMessageLoop() {
        MSG msg = {};
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
};

// ==================== ОКНО ТАБЛИЦЫ РЕЗУЛЬТАТОВ (УЛУЧШЕННАЯ ВЕРСИЯ) ====================

/**
 * Класс для отображения таблицы с результатами
 * с улучшенным дизайном и читаемостью
 */
class ResultsTableWindow {
private:
    HWND hwnd;                          // Handle окна
    HDC hdc;                            // Device context
    vector<DataTypeAnalysis> results;   // Загруженные результаты
    vector<string> distributions;       // Доступные распределения
    size_t current_data_type;           // Текущий тип данных
    size_t current_distribution;        // Текущее распределение
    COLORREF header_color;              // Цвет заголовка
    COLORREF row_color1;                // Цвет четных строк
    COLORREF row_color2;                // Цвет нечетных строк
    COLORREF text_color;                // Цвет текста

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

        // Приятная цветовая схема
        header_color = RGB(52, 152, 219);    // Синий заголовок
        row_color1 = RGB(245, 245, 245);     // Светло-серые четные строки
        row_color2 = RGB(255, 255, 255);     // Белые нечетные строки
        text_color = RGB(50, 50, 50);        // Темно-серый текст

        createWindow();
    }

    /**
     * Создание окна таблицы
     */
    void createWindow() {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = TableWindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = L"ResultsTable";

        RegisterClassW(&wc);

        hwnd = CreateWindowW(
            L"ResultsTable",
            L"Analysis Results - Detailed Metrics", // Красивое название
            WS_OVERLAPPEDWINDOW | WS_MAXIMIZEBOX, // С возможностью максимизации
            400, 200, 1300, 850,  // Увеличенный размер для лучшего отображения
            NULL, NULL, GetModuleHandle(NULL), this
        );

        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);
    }

    /**
     * Отрисовка таблицы с улучшенным дизайном
     */
    void drawTable() {
        PAINTSTRUCT ps;
        hdc = BeginPaint(hwnd, &ps);

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);

        // Заполнение фона
        HBRUSH background_brush = CreateSolidBrush(RGB(240, 240, 240));
        FillRect(hdc, &clientRect, background_brush);
        DeleteObject(background_brush);

        // Создание заголовка
        wchar_t title[300];
        wstring data_type_w = string_to_wstring(results[current_data_type].type_name);
        wstring distribution_w = string_to_wstring(distributions[current_distribution]);

        swprintf(title, 300, L"Data Type: %ls | Distribution: %ls",
                data_type_w.c_str(),
                distribution_w.c_str());

        // Красивый заголовок
        HFONT hFont = CreateFontW(20, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Segoe UI");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        SetTextColor(hdc, text_color);
        SetBkMode(hdc, TRANSPARENT);
        TextOutW(hdc, 50, 25, title, wcslen(title));
        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);

        // Позиции колонок
        int y = 80;
        int x_algorithm = 50;
        int x_time = 250;
        int x_stddev = 400;
        int x_comparisons = 550;
        int x_swaps = 700;
        int x_memory = 850;
        int x_efficiency = 1000;
        int x_stable = 1150;

        // Заголовок таблицы
        HFONT headerFont = CreateFontW(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                     DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                                     CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Segoe UI");
        HFONT hOldHeaderFont = (HFONT)SelectObject(hdc, headerFont);

        // Фон заголовка
        HBRUSH header_brush = CreateSolidBrush(header_color);
        RECT header_rect = {x_algorithm - 10, y - 5, x_stable + 100, y + 30};
        FillRect(hdc, &header_rect, header_brush);
        DeleteObject(header_brush);

        // Текст заголовка (белый)
        SetTextColor(hdc, RGB(255, 255, 255));
        TextOutW(hdc, x_algorithm, y, L"Algorithm", 9);
        TextOutW(hdc, x_time, y, L"Avg Time (s)", 12);
        TextOutW(hdc, x_stddev, y, L"Std Dev", 7);
        TextOutW(hdc, x_comparisons, y, L"Comparisons", 11);
        TextOutW(hdc, x_swaps, y, L"Swaps", 5);
        TextOutW(hdc, x_memory, y, L"Memory (KB)", 11);
        TextOutW(hdc, x_efficiency, y, L"Efficiency %", 12);
        TextOutW(hdc, x_stable, y, L"Stable", 6);

        SelectObject(hdc, hOldHeaderFont);
        DeleteObject(headerFont);

        y += 40;

        // Данные таблицы
        auto current_algorithms = results[current_data_type].getAlgorithmsForDistribution(distributions[current_distribution]);

        HFONT dataFont = CreateFontW(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                                   CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Segoe UI");
        HFONT hOldDataFont = (HFONT)SelectObject(hdc, dataFont);
        SetTextColor(hdc, text_color);

        for (size_t i = 0; i < current_algorithms.size(); i++) {
            const auto& algo = current_algorithms[i];

            // Чередование цветов строк
            HBRUSH row_brush = CreateSolidBrush((i % 2 == 0) ? row_color1 : row_color2);
            RECT row_rect = {x_algorithm - 10, y - 5, x_stable + 100, y + 20};
            FillRect(hdc, &row_rect, row_brush);
            DeleteObject(row_brush);

            wstring algo_name_w = string_to_wstring(algo.name);
            TextOutW(hdc, x_algorithm, y, algo_name_w.c_str(), algo_name_w.length());

            // Форматирование числовых значений
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

            TextOutW(hdc, x_stable, y, algo.stable ? L"Yes" : L"No", algo.stable ? 6 : 6);

            y += 25;
        }

        SelectObject(hdc, hOldDataFont);
        DeleteObject(dataFont);

        // Дополнительная информация
        y += 30;
        HFONT infoFont = CreateFontW(13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
                                   CLEARTYPE_QUALITY, VARIABLE_PITCH, L"Segoe UI");
        HFONT hOldInfoFont = (HFONT)SelectObject(hdc, infoFont);

        SetTextColor(hdc, RGB(70, 70, 70));
        TextOutW(hdc, 50, y, L"Memory Usage Notes:", 21);
        y += 25;
        TextOutW(hdc, 50, y, L"• Bubble/Selection/Insertion: O(1) extra space", 45);
        y += 20;
        TextOutW(hdc, 50, y, L"• Quick Sort: O(log n) stack space", 35);
        y += 20;
        TextOutW(hdc, 50, y, L"• Merge Sort: O(n) temporary arrays", 35);
        y += 20;
        TextOutW(hdc, 50, y, L"• Heap Sort: O(1) extra space", 30);

        y += 35;
        TextOutW(hdc, 50, y, L"Controls:", 12);
        y += 25;
        TextOutW(hdc, 50, y, L"Q/W/E/R/T - Data Types | A/S/D/F/G - Distributions | ESC - Exit", 64);

        SelectObject(hdc, hOldInfoFont);
        DeleteObject(infoFont);

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

    /**
     * Обработчик сообщений для окна таблицы
     */
    static LRESULT CALLBACK TableWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        ResultsTableWindow* pThis = (ResultsTableWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

        switch (uMsg) {
            case WM_PAINT:
                if (pThis) pThis->drawTable();
                break;

            case WM_SIZE:
                // Перерисовка при изменении размера
                InvalidateRect(hwnd, NULL, TRUE);
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

    /**
     * Запуск цикла сообщений
     */
    void runMessageLoop() {
        MSG msg = {};
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
};

// ==================== ГЛАВНАЯ ФУНКЦИЯ ПРОСМОТРЩИКА ====================

/**
 * Главная функция программы
 * Загружает результаты и запускает интерфейс просмотра
 */
int main() {
    // Настройка консоли для поддержки UTF-8
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);

    cout << "=== SORTING RESULTS VIEWER ===\n\n";

    string filename;
    cout << "Enter JSON filename to load: ";
    cin >> filename;

    // Загрузка результатов из JSON файла
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

    cout << "\=== VIEWER SYSTEM LAUNCHED ===\n";
    cout << " Use controls in windows to navigate results\n";
    cout << "   - Q/W/E/R/T: Switch data types\n";
    cout << "   - A/S/D/F/G: Switch distributions\n";
    cout << "   - 0-7: Switch algorithms (0=all)\n";
    cout << "   - L/K: Toggle log scales\n";
    cout << "   - N: Toggle normalized view\n";
    cout << "   - ESC: Exit\n";

    // Запуск message loop в отдельных потоках
    thread tableThread([&tableWindow]() {
        tableWindow.runMessageLoop();
    });

    graphWindow.runMessageLoop();
    tableThread.join();

    return 0;
}
