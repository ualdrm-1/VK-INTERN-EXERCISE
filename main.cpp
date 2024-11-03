#include <iostream>
#include <functional>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <ctime>
#include <vector>
#include <stdexcept>

using namespace std;

class Task {
public:
    function<void()> func;
    time_t timestamp;
    Task(function<void()> f, time_t t) : func(move(f)), timestamp(t) {}
    
    bool operator>(const Task& other) const {
        return timestamp > other.timestamp;
    }
    
    bool operator<(const Task& other) const {
        return timestamp < other.timestamp;
    }
};

class BinaryHeap {
private:
    vector<Task> heap;

    void siftUp(int index) {
        while (index > 0) {
            int parent = (index - 1) / 2;
            if (heap[index] > heap[parent]) break;
            swap(heap[index], heap[parent]);
            index = parent;
        }
    }

    void siftDown(int index) {
        int size = heap.size();
        while (index < size) {
            int left = 2 * index + 1;
            int right = 2 * index + 2;
            int smallest = index;

            if (left < size && heap[left] < heap[smallest]) {
                smallest = left;
            }
            if (right < size && heap[right] < heap[smallest]) {
                smallest = right;
            }
            if (smallest == index) break;
            swap(heap[index], heap[smallest]);
            index = smallest;
        }
    }

public:
    void push(const Task& task) {
        heap.push_back(task);
        siftUp(heap.size() - 1);
    }

    Task pop() {
        if (heap.empty()) throw runtime_error("Heap is empty");
        Task top = heap[0];
        heap[0] = heap.back();
        heap.pop_back();
        if (!heap.empty()) {
            siftDown(0);
        }
        return top;
    }

    bool empty() const {
        return heap.empty();
    }

    size_t size() const {
        return heap.size();
    }
};

class TaskScheduler {
private:
    BinaryHeap taskQueue;
    mutex mtx;
    condition_variable cv;
    bool running = false;
    thread worker;
    bool inputInProgress = false;
    int taskCount = 0;
    int completedTasks = 0;

    void workerThread() {
    while (running) {
        unique_lock<mutex> lock(mtx);
        if (taskQueue.empty() || inputInProgress) {
            cv.wait(lock);
        } else {
            auto now = time(nullptr);
            Task task = taskQueue.pop();
            if (task.timestamp <= now) {
                lock.unlock();
                try {
                    task.func();
                } catch (const exception& e) {
                    cerr << "Ошибка при выполнении задачи: " << e.what() << endl;
                }
                lock.lock();
                completedTasks++;
                if (completedTasks >= taskCount) {
                    cv.notify_all();
                }
            } else {
                taskQueue.push(task);
                cv.wait_until(lock, chrono::system_clock::from_time_t(task.timestamp));
            }
        }
    }
}

public:
    TaskScheduler() : running(true), worker(&TaskScheduler::workerThread, this) {}

    ~TaskScheduler() {
        Stop();
    }

    void Add(function<void()> task, time_t timestamp) {
        if (timestamp < time(nullptr)) {
            throw invalid_argument("Timestamp cannot be in the past.");
        }
        lock_guard<mutex> lock(mtx);
        taskQueue.push(Task(move(task), timestamp));
        taskCount++;
        cv.notify_one();
    }

    void Start() {
        if (!running) {
            running = true;
            worker = thread(&TaskScheduler::workerThread, this);
        }
    }

    void Stop() {
        running = false;
        cv.notify_all();
        if (worker.joinable()) {
            worker.join();
        }
    }

    void WaitForCompletion() {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this]() { return completedTasks >= taskCount; });
    }

    void StartInput() {
        lock_guard<mutex> lock(mtx);
        inputInProgress = true;
    }

    void StopInput() {
        lock_guard<mutex> lock(mtx);
        inputInProgress = false;
        cv.notify_all();
    }
};

int main() {
    TaskScheduler scheduler;
    scheduler.Start();
    int taskCount;
    cout << "Введите количество задач: ";
    cin >> taskCount;
    scheduler.StartInput();
    for (int i = 0; i < taskCount; ++i) {
        int delay;
        cout << "Введите время выполнения для задачи " << i + 1 << " (в секундах): ";
        cin >> delay;
        time_t timestamp = time(nullptr) + delay;
        scheduler.Add([i]() { cout << "Задача " << i + 1 << " выполнена!" << endl; }, timestamp);
    }
    scheduler.StopInput();
    scheduler.WaitForCompletion();
    scheduler.Stop();
    return 0;
}