#include <iostream>
#include <vector>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <ctime>

using namespace std;

template<typename T>
class NiceBuffer final {
public:
    NiceBuffer() {
        idx = 0;
        buffer0 = new unordered_map<string, T>{};
        buffer1 = new unordered_map<string, T>{};
//        std::cout << "buffer0:" << buffer0 << ", buffer1:" << buffer1 << std::endl;
    }

    ~NiceBuffer() = default;

    NiceBuffer(const NiceBuffer &) = delete;

    NiceBuffer &operator=(const NiceBuffer &) = delete;

public:
//    explicit NiceBuffer(int x) : a(x) {}
//    NiceBuffer() : NiceBuffer(0)  {}

    int update(const string &key, const T &value);

    int finish_update();

    int get(const string &key, T &value);


private:
    volatile std::atomic<int> idx = {0};
    unordered_map<string, T> *buffer0;
    unordered_map<string, T> *buffer1;

    unordered_map<string, T> *get_current_map();

    unordered_map<string, T> *get_update_map();

};


template<typename T>
unordered_map<string, T> *NiceBuffer<T>::get_update_map() {
//    std::cout << "get_update_map idx: " << idx.load() << std::endl;
    if (idx.load() == 0) {
        return buffer1;
    } else {
        return buffer0;
    }
}

template<typename T>
unordered_map<string, T> *NiceBuffer<T>::get_current_map() {
//    std::cout << "get_current_map idx: " << idx.load() << std::endl;
    if (idx.load() == 0) {
        return buffer0;
    } else {
        return buffer1;
    }
}

template<typename T>
int NiceBuffer<T>::update(const string &key, const T &value) {
    auto buffer = get_update_map();
//    std::cout << "update " << buffer << " ,key " << key << ", value " << value << std::endl;
    (*buffer)[key] = value;
    return 0;
}

template<typename T>
int NiceBuffer<T>::get(const string &key, T &value) {
    auto buffer = get_current_map();
//    std::cout << "get " << buffer << std::endl;
    if (buffer->find(key) == buffer->end()) {
//        std::cout << "not find key " << key << std::endl;
        value = T{};
        return -1;
    } else {
        value = buffer->at(key);
        return 0;
    }
}

template<typename T>
int NiceBuffer<T>::finish_update() {
//    std::cout << "before idx=" << idx.load() << std::endl;
    if (idx.load() == 0) {
        idx.store(1);
    } else {
        idx.store(0);
    }
//    std::cout << "after idx=" << idx.load() << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}


void update_worker(NiceBuffer<double> *nb) {
    std::time_t pre_update_time = 0;
    int count = 0;
    int k = 1;
    while (1) {
        std::time_t now = std::time(0);
//        std::cout << now << "  " << pre_update_time << std::endl;
        if (now % 10 == 0 && now - pre_update_time > 9) {
            for (int i = 0; i < 1000; i++) {
                auto key = std::to_string(i);
                nb->update("k" + key, k * (count++));
            }
            k *= -1;
            nb->finish_update();
            pre_update_time = now;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }

}


int main() {
    auto *nb = new NiceBuffer<double>;
    double value_ = -10.0;

    thread t1(update_worker, nb);

    while (true) {
        for (int i = 0; i < 1000; i++) {
            auto key = std::to_string(i);
            nb->get("k" + key, value_);
            std::cout << value_ << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    t1.join();

    return 0;
}


