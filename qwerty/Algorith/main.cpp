#include <fstream>
#include <chrono>
#include <random>
#include <iostream>
#include <windows.h>
#include <vector>
#include <string>
#include <sstream>
#include <thread>
#include <algorithm>
#include <functional>
#include <cmath>
#include <queue>
#include <stack>
#include <mutex>
using namespace std;

int rand_uns(int min, int max) {
    unsigned seed = std::chrono::steady_clock::now().time_since_epoch().count();
    static std::default_random_engine e(seed);
    std::uniform_int_distribution<int> d(min, max);
    return d(e);
}

double get_time() {
    return std::chrono::duration_cast<std::chrono::microseconds>
    (std::chrono::steady_clock::now().time_since_epoch()).count()/1e6;
}

void bubble_sort(int arr[], int n) {
    bool swapped;
    for (int i = 0; i < n-1; i++) {
        swapped = false;
        for (int j = 0; j < n-i-1; j++) {
            if (arr[j] > arr[j+1]) {
                swap(arr[j], arr[j+1]);
                swapped = true;
            }
        }
        if (!swapped) break;
    }
}

void selection_sort(int arr[], int n) {
    for (int i = 0; i < n-1; i++) {
        int min_idx = i;
        for (int j = i+1; j < n; j++) {
            if (arr[j] < arr[min_idx]) {
                min_idx = j;
            }
        }
        swap(arr[i], arr[min_idx]);
    }
}

void insertion_sort(int arr[], int n) {
    for (int i = 1; i < n; i++) {
        int key = arr[i];
        int j = i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j+1] = arr[j];
            j--;
        }
        arr[j+1] = key;
    }
}

void quick_sort(int arr[], int low, int high) {
    if (low < high) {
        int pivot = arr[(low + high) / 2];
        int i = low - 1;
        int j = high + 1;

        while (true) {
            do { i++; } while (arr[i] < pivot);
            do { j--; } while (arr[j] > pivot);

            if (i >= j) break;

            swap(arr[i], arr[j]);
        }

        quick_sort(arr, low, j);
        quick_sort(arr, j + 1, high);
    }
}

void quick_sort(int arr[], int n) {
    quick_sort(arr, 0, n - 1);
}

void merge(int arr[], int left, int mid, int right) {
    int n1 = mid - left + 1;
    int n2 = right - mid;

    vector<int> L(n1), R(n2);

    for (int i = 0; i < n1; i++)
        L[i] = arr[left + i];
    for (int j = 0; j < n2; j++)
        R[j] = arr[mid + 1 + j];

    int i = 0, j = 0, k = left;

    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        } else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }
}

void merge_sort(int arr[], int left, int right) {
    if (left < right) {
        int mid = left + (right - left) / 2;

        merge_sort(arr, left, mid);
        merge_sort(arr, mid + 1, right);

        merge(arr, left, mid, right);
    }
}

void merge_sort(int arr[], int n) {
    merge_sort(arr, 0, n - 1);
}

void heapify(int arr[], int n, int i) {
    int largest = i;
    int left = 2 * i + 1;
    int right = 2 * i + 2;

    if (left < n && arr[left] > arr[largest])
        largest = left;

    if (right < n && arr[right] > arr[largest])
        largest = right;

    if (largest != i) {
        swap(arr[i], arr[largest]);
        heapify(arr, n, largest);
    }
}

void heap_sort(int arr[], int n) {
    for (int i = n / 2 - 1; i >= 0; i--)
        heapify(arr, n, i);

    for (int i = n - 1; i > 0; i--) {
        swap(arr[0], arr[i]);
        heapify(arr, i, 0);
    }
}

int* copy_array(int source[], int n) {
    int* dest = new int[n];
    for (int i = 0; i < n; i++) {
        dest[i] = source[i];
    }
    return dest;
}

double measure_sort_time(int arr[], int n, void (*sort_func)(int[], int), int repetitions = 1) {
    if (repetitions <= 0) repetitions = 1;

    double total_time = 0;
    for (int rep = 0; rep < repetitions; rep++) {
        int* test_arr = copy_array(arr, n);
        double start = get_time();
        sort_func(test_arr, n);
        double end = get_time();
        total_time += (end - start);
        delete[] test_arr;
    }
    return total_time / repetitions;
}

int* create_array(int n, int type) {
    int* arr = new int[n];
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
    }
    return arr;
}

class AnimationWindow {
private:
    HWND hwnd;
    HDC hdc;
    HDC hdcBuffer;
    HBITMAP hBitmap;
    vector<int> data;
    vector<int> original_data;
    bool animation_running;
    int current_algorithm;
    vector<int> highlighted_indices;
    vector<pair<int, int>> compared_indices;
    mutex animation_mutex;

public:
    AnimationWindow(const vector<int>& input_data) : data(input_data), original_data(input_data) {
        animation_running = false;
        current_algorithm = 0;
        hdcBuffer = NULL;
        hBitmap = NULL;
        createWindow();
    }

    ~AnimationWindow() {
        if (hBitmap) DeleteObject(hBitmap);
        if (hdcBuffer) DeleteDC(hdcBuffer);
    }

    void createWindow() {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = AnimationWindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = L"AnimationWindow";
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);

        RegisterClassW(&wc);

        hwnd = CreateWindowW(
            L"AnimationWindow", L"Sorting Animation",
            WS_OVERLAPPEDWINDOW, 900, 100,
            800, 650, NULL, NULL, GetModuleHandle(NULL), this
        );

        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);

        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);
    }

    void createDoubleBuffer(HDC hdc, int width, int height) {
        if (hdcBuffer) DeleteDC(hdcBuffer);
        if (hBitmap) DeleteObject(hBitmap);

        hdcBuffer = CreateCompatibleDC(hdc);
        hBitmap = CreateCompatibleBitmap(hdc, width, height);
        SelectObject(hdcBuffer, hBitmap);
    }

    void drawToBuffer() {
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        int width = clientRect.right;
        int height = clientRect.bottom;

        if (!hdcBuffer || !hBitmap) {
            createDoubleBuffer(GetDC(hwnd), width, height);
        }

        HBRUSH backgroundBrush = (HBRUSH)(COLOR_WINDOW + 1);
        FillRect(hdcBuffer, &clientRect, backgroundBrush);

        int infoHeight = 120;
        int margin = 60;
        int graphWidth = clientRect.right - 2 * margin;
        int graphHeight = clientRect.bottom - 2 * margin - infoHeight;

        if (graphWidth <= 0 || graphHeight <= 0) return;

        int max_val = *max_element(data.begin(), data.end());
        if (max_val == 0) max_val = 1;

        HPEN axisPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
        HPEN oldPen = (HPEN)SelectObject(hdcBuffer, axisPen);

        int graphTop = margin + infoHeight;
        MoveToEx(hdcBuffer, margin, graphTop + graphHeight, NULL);
        LineTo(hdcBuffer, margin + graphWidth, graphTop + graphHeight);

        MoveToEx(hdcBuffer, margin, graphTop, NULL);
        LineTo(hdcBuffer, margin, graphTop + graphHeight);

        SetTextColor(hdcBuffer, RGB(0, 0, 0));
        SetBkMode(hdcBuffer, TRANSPARENT);

        TextOutW(hdcBuffer, margin + graphWidth/2 - 40, graphTop + graphHeight + 20, L"Index", 5);
        TextOutW(hdcBuffer, margin - 50, graphTop + graphHeight/2 - 10, L"Value", 5);

        int barWidth = graphWidth / max(1, (int)data.size());
        barWidth = max(2, barWidth);

        for (size_t i = 0; i < data.size(); i++) {
            int barHeight = (data[i] * graphHeight) / max_val;
            int x = margin + i * barWidth;
            int y = graphTop + graphHeight - barHeight;

            HBRUSH barBrush;

            bool is_highlighted = false;
            for (int idx : highlighted_indices) {
                if (i == idx) {
                    is_highlighted = true;
                    break;
                }
            }

            bool is_compared = false;
            for (auto& pair : compared_indices) {
                if (i == pair.first || i == pair.second) {
                    is_compared = true;
                    break;
                }
            }

            if (is_highlighted) {
                barBrush = CreateSolidBrush(RGB(255, 0, 0));
            } else if (is_compared) {
                barBrush = CreateSolidBrush(RGB(255, 255, 0));
            } else {
                barBrush = CreateSolidBrush(RGB(100, 100, 255));
            }

            HBRUSH oldBrush = (HBRUSH)SelectObject(hdcBuffer, barBrush);
            Rectangle(hdcBuffer, x, y, x + barWidth - 1, graphTop + graphHeight);
            SelectObject(hdcBuffer, oldBrush);
            DeleteObject(barBrush);

            if (data.size() <= 20) {
                wchar_t val[10];
                swprintf_s(val, L"%d", data[i]);
                TextOutW(hdcBuffer, x, y - 15, val, wcslen(val));
            }
        }

        const wchar_t* algo_names[] = {
            L"Bubble Sort",
            L"Selection Sort",
            L"Insertion Sort",
            L"Quick Sort",
            L"Merge Sort",
            L"Heap Sort"
        };

        wchar_t info[150];
        swprintf_s(info, L"Algorithm: %s | Elements: %d",
                  algo_names[current_algorithm], data.size());
        TextOutW(hdcBuffer, margin, 20, info, wcslen(info));

        if (animation_running) {
            TextOutW(hdcBuffer, margin, 45, L"Status: Sorting...", 17);
        } else {
            TextOutW(hdcBuffer, margin, 45, L"Status: Ready", 13);
        }

        TextOutW(hdcBuffer, margin, 70, L"1: Bubble Sort  2: Selection Sort  3: Insertion Sort", 50);
        TextOutW(hdcBuffer, margin, 95, L"4: Quick Sort    5: Merge Sort      6: Heap Sort", 45);

        SelectObject(hdcBuffer, oldPen);
        DeleteObject(axisPen);
    }

    void drawAnimation() {
        PAINTSTRUCT ps;
        hdc = BeginPaint(hwnd, &ps);

        drawToBuffer();

        RECT clientRect;
        GetClientRect(hwnd, &clientRect);
        BitBlt(hdc, 0, 0, clientRect.right, clientRect.bottom, hdcBuffer, 0, 0, SRCCOPY);

        EndPaint(hwnd, &ps);
    }

    void updateAnimation() {
        RECT clientRect;
        GetClientRect(hwnd, &clientRect);

        drawToBuffer();

        HDC hdcScreen = GetDC(hwnd);
        BitBlt(hdcScreen, 0, 0, clientRect.right, clientRect.bottom, hdcBuffer, 0, 0, SRCCOPY);
        ReleaseDC(hwnd, hdcScreen);

        this_thread::sleep_for(chrono::milliseconds(50));
    }

    void animateBubbleSort() {
        animation_running = true;
        int n = data.size();
        bool swapped;

        for (int i = 0; i < n-1; i++) {
            swapped = false;
            for (int j = 0; j < n-i-1; j++) {
                highlighted_indices = {j, j+1};
                compared_indices = {{j, j+1}};

                if (data[j] > data[j+1]) {
                    swap(data[j], data[j+1]);
                    swapped = true;
                }

                updateAnimation();
            }
            if (!swapped) break;
        }
        highlighted_indices.clear();
        compared_indices.clear();
        animation_running = false;
        updateAnimation();
    }

    void animateSelectionSort() {
        animation_running = true;
        int n = data.size();

        for (int i = 0; i < n-1; i++) {
            int min_idx = i;
            highlighted_indices = {i};

            for (int j = i+1; j < n; j++) {
                compared_indices = {{min_idx, j}};

                if (data[j] < data[min_idx]) {
                    min_idx = j;
                    highlighted_indices = {min_idx};
                }

                updateAnimation();
            }

            swap(data[i], data[min_idx]);
            updateAnimation();
        }
        highlighted_indices.clear();
        compared_indices.clear();
        animation_running = false;
        updateAnimation();
    }

    void animateInsertionSort() {
        animation_running = true;
        int n = data.size();

        for (int i = 1; i < n; i++) {
            int key = data[i];
            int j = i - 1;
            highlighted_indices = {i};

            while (j >= 0 && data[j] > key) {
                compared_indices = {{j, i}};
                data[j + 1] = data[j];
                j = j - 1;

                updateAnimation();
            }
            data[j + 1] = key;

            updateAnimation();
        }
        highlighted_indices.clear();
        compared_indices.clear();
        animation_running = false;
        updateAnimation();
    }

    void animateQuickSort(int low, int high) {
        if (low < high) {
            int pivot = data[(low + high) / 2];
            int i = low - 1;
            int j = high + 1;

            highlighted_indices = {(low + high) / 2};

            while (true) {
                do {
                    i++;
                    compared_indices = {{i, (low + high) / 2}};
                    updateAnimation();
                } while (data[i] < pivot);

                do {
                    j--;
                    compared_indices = {{j, (low + high) / 2}};
                    updateAnimation();
                } while (data[j] > pivot);

                if (i >= j) break;

                swap(data[i], data[j]);
                updateAnimation();
            }

            animateQuickSort(low, j);
            animateQuickSort(j + 1, high);
        }
    }

    void animateQuickSort() {
        animation_running = true;
        animateQuickSort(0, data.size() - 1);
        highlighted_indices.clear();
        compared_indices.clear();
        animation_running = false;
        updateAnimation();
    }

    void animateMerge(int left, int mid, int right) {
        vector<int> temp(right - left + 1);
        int i = left, j = mid + 1, k = 0;

        while (i <= mid && j <= right) {
            compared_indices = {{i, j}};
            updateAnimation();

            if (data[i] <= data[j]) {
                temp[k++] = data[i++];
            } else {
                temp[k++] = data[j++];
            }
        }

        while (i <= mid) {
            temp[k++] = data[i++];
        }

        while (j <= right) {
            temp[k++] = data[j++];
        }

        for (int i = left; i <= right; i++) {
            data[i] = temp[i - left];
            highlighted_indices = {i};
            updateAnimation();
        }
    }

    void animateMergeSort(int left, int right) {
        if (left < right) {
            int mid = left + (right - left) / 2;

            animateMergeSort(left, mid);
            animateMergeSort(mid + 1, right);

            highlighted_indices.clear();
            for (int i = left; i <= right; i++) {
                highlighted_indices.push_back(i);
            }
            updateAnimation();

            animateMerge(left, mid, right);
        }
    }

    void animateMergeSort() {
        animation_running = true;
        animateMergeSort(0, data.size() - 1);
        highlighted_indices.clear();
        compared_indices.clear();
        animation_running = false;
        updateAnimation();
    }

    void animateHeapify(int n, int i) {
        int largest = i;
        int left = 2 * i + 1;
        int right = 2 * i + 2;

        if (left < n) {
            compared_indices = {{largest, left}};
            updateAnimation();
            if (data[left] > data[largest])
                largest = left;
        }

        if (right < n) {
            compared_indices = {{largest, right}};
            updateAnimation();
            if (data[right] > data[largest])
                largest = right;
        }

        if (largest != i) {
            highlighted_indices = {i, largest};
            swap(data[i], data[largest]);
            updateAnimation();

            animateHeapify(n, largest);
        }
    }

    void animateHeapSort() {
        animation_running = true;
        int n = data.size();

        for (int i = n / 2 - 1; i >= 0; i--) {
            animateHeapify(n, i);
        }

        for (int i = n - 1; i > 0; i--) {
            highlighted_indices = {0, i};
            swap(data[0], data[i]);
            updateAnimation();

            animateHeapify(i, 0);
        }

        highlighted_indices.clear();
        compared_indices.clear();
        animation_running = false;
        updateAnimation();
    }

    void startAnimation(int algorithm) {
        lock_guard<mutex> lock(animation_mutex);

        if (animation_running) {
            return;
        }

        current_algorithm = algorithm;
        data = original_data;
        highlighted_indices.clear();
        compared_indices.clear();

        thread animation_thread([this, algorithm]() {
            switch (algorithm) {
                case 0: animateBubbleSort(); break;
                case 1: animateSelectionSort(); break;
                case 2: animateInsertionSort(); break;
                case 3: animateQuickSort(); break;
                case 4: animateMergeSort(); break;
                case 5: animateHeapSort(); break;
            }
        });
        animation_thread.detach();
    }

    static LRESULT CALLBACK AnimationWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        AnimationWindow* pThis = (AnimationWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

        switch (uMsg) {
            case WM_PAINT:
                if (pThis) pThis->drawAnimation();
                break;

            case WM_SIZE:
                if (pThis) {
                    if (pThis->hBitmap) DeleteObject(pThis->hBitmap);
                    if (pThis->hdcBuffer) DeleteDC(pThis->hdcBuffer);
                    pThis->hdcBuffer = NULL;
                    pThis->hBitmap = NULL;
                }
                break;

            case WM_KEYDOWN:
                if (pThis && wParam >= '1' && wParam <= '6') {
                    pThis->startAnimation(wParam - '1');
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

class GraphWindow {
private:
    HWND hwnd;
    HDC hdc;
    vector<vector<double>> times;
    vector<int> sizes;
    int width, height;
    int current_display;
    int graph_type;

public:
    GraphWindow(const vector<vector<double>>& t, const vector<int>& sz, int type = 0)
        : times(t), sizes(sz), graph_type(type) {
        width = 1000;
        height = 700;
        current_display = 0;
        createWindow();
    }

    void createWindow() {
        WNDCLASSW wc = {};
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = L"GraphWindow";
        wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);

        RegisterClassW(&wc);

        hwnd = CreateWindowW(
            L"GraphWindow", L"Sorting Algorithms Time Graph",
            WS_OVERLAPPEDWINDOW, 100, 100,
            width, height, NULL, NULL, GetModuleHandle(NULL), this
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

        int margin = 100;
        int graphWidth = clientRect.right - 2 * margin;
        int graphHeight = clientRect.bottom - 2 * margin;

        if (graphWidth <= 0 || graphHeight <= 0) return;

        double max_val = 0;
        int max_index = max(1, (int)sizes.size() - 1);

        for (size_t algo = 0; algo < times.size(); algo++) {
            if (current_display == 0 || current_display == algo + 1) {
                for (size_t i = 0; i < sizes.size(); i++) {
                    double value = times[algo][i];
                    if (graph_type == 1 && sizes[i] > 1) {
                        value = value / (sizes[i] * log2(sizes[i]));
                    }
                    max_val = max(max_val, value);
                }
            }
        }
        max_val *= 1.1;

        HPEN axisPen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
        SelectObject(hdc, axisPen);

        MoveToEx(hdc, margin, margin + graphHeight, NULL);
        LineTo(hdc, margin + graphWidth, margin + graphHeight);
        MoveToEx(hdc, margin, margin, NULL);
        LineTo(hdc, margin, margin + graphHeight);

        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, TRANSPARENT);

        TextOutW(hdc, margin + graphWidth/2 - 70, margin + graphHeight + 30, L"Array Size", 10);

        for (size_t i = 0; i < sizes.size(); i++) {
            if (i % 2 == 0) {
                int x = margin + i * graphWidth / max_index;
                wchar_t size_label[20];
                swprintf_s(size_label, L"%d", sizes[i]);
                TextOutW(hdc, x - 15, margin + graphHeight + 10, size_label, wcslen(size_label));
            }
        }

        for (int i = 0; i <= 5; i++) {
            double time_val = (max_val * i) / 5;
            int y = margin + graphHeight - (time_val * graphHeight) / max_val;
            wchar_t time_label[20];
            swprintf_s(time_label, L"%.3f", time_val);
            TextOutW(hdc, margin - 50, y - 8, time_label, wcslen(time_label));
        }

        vector<COLORREF> colors = {
            RGB(255, 0, 0),
            RGB(0, 0, 255),
            RGB(0, 128, 0),
            RGB(255, 0, 255),
            RGB(0, 128, 128),
            RGB(128, 0, 128)
        };

        for (int algo = 0; algo < 6; algo++) {
            if (current_display == 0 || current_display == algo + 1) {
                HPEN linePen = CreatePen(PS_SOLID, 2, colors[algo]);
                SelectObject(hdc, linePen);
                HBRUSH pointBrush = CreateSolidBrush(colors[algo]);

                for (size_t i = 1; i < sizes.size(); i++) {
                    double value1 = times[algo][i-1];
                    double value2 = times[algo][i];

                    if (graph_type == 1 && sizes[i-1] > 1 && sizes[i] > 1) {
                        value1 = value1 / (sizes[i-1] * log2(sizes[i-1]));
                        value2 = value2 / (sizes[i] * log2(sizes[i]));
                    }

                    int x1 = margin + (i-1) * graphWidth / max_index;
                    int y1 = margin + graphHeight - (value1 * graphHeight) / max_val;
                    int x2 = margin + i * graphWidth / max_index;
                    int y2 = margin + graphHeight - (value2 * graphHeight) / max_val;

                    MoveToEx(hdc, x1, y1, NULL);
                    LineTo(hdc, x2, y2);

                    SelectObject(hdc, pointBrush);
                    if (algo < 3) {
                        Ellipse(hdc, x1 - 3, y1 - 3, x1 + 3, y1 + 3);
                    } else {
                        Rectangle(hdc, x1 - 3, y1 - 3, x1 + 3, y1 + 3);
                    }
                }

                DeleteObject(linePen);
                DeleteObject(pointBrush);
            }
        }

        int legendX = margin + graphWidth - 250;
        int legendY = margin + 20;

        TextOutW(hdc, legendX, legendY, L"Legend:", 7);

        const wchar_t* algo_names[] = {
            L"Bubble Sort",
            L"Selection Sort",
            L"Insertion Sort",
            L"Quick Sort",
            L"Merge Sort",
            L"Heap Sort"
        };

        for (int i = 0; i < 6; i++) {
            if (current_display == 0 || current_display == i + 1) {
                HBRUSH legendBrush = CreateSolidBrush(colors[i]);
                SelectObject(hdc, legendBrush);

                if (i < 3) {
                    Ellipse(hdc, legendX, legendY + 25 + i * 20, legendX + 10, legendY + 35 + i * 20);
                } else {
                    Rectangle(hdc, legendX, legendY + 25 + i * 20, legendX + 10, legendY + 35 + i * 20);
                }

                SetTextColor(hdc, colors[i]);
                TextOutW(hdc, legendX + 15, legendY + 28 + i * 20, algo_names[i], wcslen(algo_names[i]));
                DeleteObject(legendBrush);
            }
        }

        SetTextColor(hdc, RGB(0, 0, 0));
        const wchar_t* titles[] = {
            L"All Algorithms Comparison",
            L"O(N log N) Proof",
            L"O(N^2) vs O(N log N)"
        };

        wchar_t title[150];
        swprintf_s(title, L"%s", titles[graph_type]);
        TextOutW(hdc, margin + graphWidth/2 - 200, margin - 40, title, wcslen(title));

        TextOutW(hdc, margin, margin - 70,
                L"0-6: Select algorithm (0-all, 1-bubble, 2-selection, 3-insertion, 4-quick, 5-merge, 6-heap)",
                80);

        DeleteObject(axisPen);
        EndPaint(hwnd, &ps);
    }

    void setDisplay(int display) {
        current_display = display;
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
    }

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        GraphWindow* pThis = (GraphWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

        switch (uMsg) {
            case WM_PAINT:
                if (pThis) pThis->drawGraph();
                break;

            case WM_KEYDOWN:
                if (pThis && wParam >= '0' && wParam <= '6') {
                    pThis->setDisplay(wParam - '0');
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

int main() {
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);

    int data_points;
    cout << "Enter number of data points for measurements: ";
    cin >> data_points;

    if (data_points <= 0) {
        cout << "Number of points must be positive!" << endl;
        return 1;
    }

    vector<int> sizes;
    vector<vector<double>> times(6);

    cout << "Measuring sorting time for " << data_points << " data points...\n";
    cout << "=============================================\n";

    int step_size = 100;
    int max_size = step_size * data_points;

    for (int i = 1; i <= data_points; i++) {
        int size = i * step_size;
        sizes.push_back(size);

        cout << "Array size: " << size << endl;

        int* arr = create_array(size, 0);

        for (int algo = 0; algo < 6; algo++) {
            double time_val = 0;

            switch (algo) {
                case 0: time_val = measure_sort_time(arr, size, bubble_sort); break;
                case 1: time_val = measure_sort_time(arr, size, selection_sort); break;
                case 2: time_val = measure_sort_time(arr, size, insertion_sort); break;
                case 3: time_val = measure_sort_time(arr, size, quick_sort); break;
                case 4: time_val = measure_sort_time(arr, size, merge_sort); break;
                case 5: time_val = measure_sort_time(arr, size, heap_sort); break;
            }

            times[algo].push_back(time_val);
            cout << "  Algorithm " << algo+1 << ": " << time_val << " s" << endl;
        }

        cout << "---------------------------------------------\n";
        delete[] arr;
    }

    ofstream f("sorting_times.csv");
    f << "Size,Bubble,Selection,Insertion,Quick,Merge,Heap" << endl;
    for (int i = 0; i < data_points; i++) {
        f << sizes[i];
        for (int algo = 0; algo < 6; algo++) {
            f << "," << times[algo][i];
        }
        f << endl;
    }
    f.close();

    cout << "Data saved to sorting_times.csv\n";

    vector<int> animation_data;
    int anim_size = 30;
    for (int i = 0; i < anim_size; i++) {
        animation_data.push_back(rand_uns(10, 100));
    }

    GraphWindow graphWindow(times, sizes, 0);
    AnimationWindow animWindow(animation_data);

    cout << "\nBoth windows are open!\n";
    cout << "Left window: Time graphs - Press 0-6 to filter algorithms\n";
    cout << "Right window: Sorting animation - Press 1-6 to run algorithms\n";
    cout << "Close both windows to exit the program.\n";

    graphWindow.runMessageLoop();

    return 0;
}
