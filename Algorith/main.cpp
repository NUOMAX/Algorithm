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
#include <atomic>
using namespace std;

// ���������� ��������� ��������� �����
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

// ��������� ����������
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

// ���������������� ����������� �������
int* copy_array(int source[], int n) {
    int* dest = new int[n];
    memcpy(dest, source, n * sizeof(int));
    return dest;
}

// ���������������� ��������� ������� � ��������������� ���������
double measure_sort_time(int arr[], int n, void (*sort_func)(int[], int), int repetitions = 1) {
    if (repetitions <= 0) repetitions = 1;

    // ��������������� ������ ��� "��������" ����
    int* warmup_arr = copy_array(arr, n);
    sort_func(warmup_arr, n);
    delete[] warmup_arr;

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

// ���������������� �������� �������
int* create_array(int n, int type) {
    int* arr = new int[n];
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
    }
    return arr;
}

// ����� ����������� ���������� ����������
int determine_repetitions(int size) {
    if (size <= 100) return 100;
    if (size <= 1000) return 50;
    if (size <= 5000) return 20;
    if (size <= 10000) return 10;
    if (size <= 50000) return 5;
    if (size <= 100000) return 3;
    return 1;
}

// ������ AnimationWindow � GraphWindow
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
            int x = margin + (int)i * barWidth;
            int y = graphTop + graphHeight - barHeight;

            HBRUSH barBrush;

            bool is_highlighted = false;
            for (int idx : highlighted_indices) {
                if ((int)i == idx) {
                    is_highlighted = true;
                    break;
                }
            }

            bool is_compared = false;
            for (auto& pair : compared_indices) {
                if ((int)i == pair.first || (int)i == pair.second) {
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
                  algo_names[current_algorithm], (int)data.size());
        TextOutW(hdcBuffer, margin, 20, info, wcslen(info));

        if (animation_running) {
            TextOutW(hdcBuffer, margin, 45, L"Status: Sorting...", 17);
        } else {
            TextOutW(hdcBuffer, margin, 45, L"Status: Ready", 13);
        }

        TextOutW(hdcBuffer, margin, 70, L"1: Bubble Sort  2: Selection Sort  3: Insertion Sort", 55);
        TextOutW(hdcBuffer, margin, 95, L"4: Quick Sort    5: Merge Sort      6: Heap Sort", 35);

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
        int n = (int)data.size();
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
        int n = (int)data.size();

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
        int n = (int)data.size();

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
        animateQuickSort(0, (int)data.size() - 1);
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

        for (int idx = left; idx <= right; idx++) {
            data[idx] = temp[idx - left];
            highlighted_indices = {idx};
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
        animateMergeSort(0, (int)data.size() - 1);
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
        int n = (int)data.size();

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
            if (current_display == 0 || current_display == (int)algo + 1) {
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

        // ����� �������� �������� ��� ������ 1-� ����� (���� �����)
        for (size_t i = 0; i < sizes.size(); i++) {
            int x = margin + (int)i * graphWidth / max_index;
            wchar_t size_label[20];
            swprintf_s(size_label, L"%d", sizes[i]);

            SIZE textSize;
            GetTextExtentPoint32W(hdc, size_label, wcslen(size_label), &textSize);
            TextOutW(hdc, x - textSize.cx/2, margin + graphHeight + 10, size_label, wcslen(size_label));
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

        // ������ ����� - ��� �����!
       // ������ ����� - ��� ����� � ��� �������!
for (int algo = 0; algo < 6; algo++) {
    if (current_display == 0 || current_display == algo + 1) {
        HBRUSH pointBrush = CreateSolidBrush(colors[algo]);
        HPEN transparentPen = CreatePen(PS_NULL, 1, RGB(0, 0, 0)); // ���������� ����

        // ��������� ������ �������
        HPEN oldPen = (HPEN)SelectObject(hdc, transparentPen);
        HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, pointBrush);

        for (size_t i = 0; i < sizes.size(); i++) {
            double value = times[algo][i];
            if (graph_type == 1 && sizes[i] > 1) {
                value = value / (sizes[i] * log2(sizes[i]));
            }

            int x = margin + (int)i * graphWidth / max_index;
            int y = margin + graphHeight - (value * graphHeight) / max_val;

            // ������ ����� ��� �������
            if (algo < 3) {
                Ellipse(hdc, x - 3, y - 3, x + 3, y + 3);
            } else {
                Rectangle(hdc, x - 3, y - 3, x + 3, y + 3);
            }
        }

        // ��������������� ������ ������� � ������� ���������
        SelectObject(hdc, oldPen);
        SelectObject(hdc, oldBrush);
        DeleteObject(transparentPen);
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
                92);

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

    int num_arrays;
    int num_points;

    cout << "Enter number of arrays: ";
    cin >> num_arrays;
    cout << "Enter number of points on graph: ";
    cin >> num_points;

    if (num_arrays <= 0 || num_points <= 0) {
        cout << "Values must be positive!" << endl;
        return 1;
    }

    if (num_points > num_arrays) {
        cout << "Number of points cannot exceed number of arrays!" << endl;
        return 1;
    }

    vector<int> all_sizes(num_arrays);
    vector<vector<double>> all_times(6, vector<double>(num_arrays, 0.0));

    cout << "Measuring sorting time for " << num_arrays << " arrays...\n";
    cout << "=============================================\n";

    // ������� ������� ���� �������� �� 1 �� num_arrays
    for (int i = 0; i < num_arrays; i++) {
        all_sizes[i] = i + 1;  // �������: 1, 2, 3, ..., num_arrays
    }

    // ��������������� ��������� ���� �������� ������
    vector<int*> test_arrays;
    for (int size : all_sizes) {
        test_arrays.push_back(create_array(size, 0));
    }

    auto start_time = chrono::steady_clock::now();

    // ���������������� ���������������
    unsigned int num_threads = thread::hardware_concurrency();
    if (num_threads == 0) num_threads = 4;

    cout << "Using " << num_threads << " threads\n";

    vector<thread> threads;
    atomic<int> next_task{0};
    atomic<int> completed_tasks{0};
    int total_tasks = num_arrays;

    // ������� ������� ������
    for (unsigned int t = 0; t < num_threads; t++) {
        threads.emplace_back([&]() {
            while (true) {
                int i = next_task++;
                if (i >= num_arrays) break;

                int size = all_sizes[i];
                int* arr = test_arrays[i];

                // ��������������� �������� ��� 6 ���������� ��� ����� �������
                for (int algo = 0; algo < 6; algo++) {
                    int reps = determine_repetitions(size);
                    double time_val = 0;

                    switch (algo) {
                        case 0: time_val = measure_sort_time(arr, size, bubble_sort, reps); break;
                        case 1: time_val = measure_sort_time(arr, size, selection_sort, reps); break;
                        case 2: time_val = measure_sort_time(arr, size, insertion_sort, reps); break;
                        case 3: time_val = measure_sort_time(arr, size, quick_sort, reps); break;
                        case 4: time_val = measure_sort_time(arr, size, merge_sort, reps); break;
                        case 5: time_val = measure_sort_time(arr, size, heap_sort, reps); break;
                    }

                    all_times[algo][i] = time_val;
                }

                completed_tasks++;

                // ����� ���������
                if (completed_tasks % max(1, num_arrays / 10) == 0) {
                    double percent = (completed_tasks * 100.0) / total_tasks;
                    cout << "Progress: " << completed_tasks << "/" << total_tasks << " (" << percent << "%)" << endl;
                }
            }
        });
    }

    // ���� ���������� ���� �������
    for (auto& thread : threads) {
        thread.join();
    }

    auto end_time = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::seconds>(end_time - start_time);

    cout << "Measurements completed in " << duration.count() << " seconds\n";

    // �������� ����� ��� �������
    vector<int> graph_sizes(num_points);
    vector<vector<double>> graph_times(6, vector<double>(num_points, 0.0));

    int step = max(1, num_arrays / num_points);
    for (int i = 0; i < num_points; i++) {
        int idx = min(i * step, num_arrays - 1);
        graph_sizes[i] = all_sizes[idx];
        for (int algo = 0; algo < 6; algo++) {
            graph_times[algo][i] = all_times[algo][idx];
        }
    }

    // ����� ����������� ��� ��������� �����
    for (int i = 0; i < num_points; i++) {
        cout << "Array size: " << graph_sizes[i] << endl;
        for (int algo = 0; algo < 6; algo++) {
            cout << "  Algorithm " << algo+1 << ": " << graph_times[algo][i] << " s" << endl;
        }
        if (i < num_points - 1) {
            cout << "---------------------------------------------\n";
        }
    }

    // ������� �������� ������
    for (int* arr : test_arrays) {
        delete[] arr;
    }

    // ���������� �����������
    ofstream f("sorting_times.csv");
    f << "Size,Bubble,Selection,Insertion,Quick,Merge,Heap" << endl;
    for (int i = 0; i < num_points; i++) {
        f << graph_sizes[i];
        for (int algo = 0; algo < 6; algo++) {
            f << "," << graph_times[algo][i];
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

    GraphWindow graphWindow(graph_times, graph_sizes, 0);
    AnimationWindow animWindow(animation_data);

    cout << "\nBoth windows are open!\n";
    cout << "Left window: Time graphs - Press 0-6 to filter algorithms\n";
    cout << "Right window: Sorting animation - Press 1-6 to run algorithms\n";
    cout << "Close both windows to exit the program.\n";

    graphWindow.runMessageLoop();

    return 0;
}
