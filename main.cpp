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
        idx_ = 0;
        buffer0_ = new unordered_map<string, T>{};
        buffer1_ = new unordered_map<string, T>{};
        std::cout << idx_.is_lock_free() << std::endl;
//        std::cout << "buffer0_:" << buffer0_ << ", buffer1_:" << buffer1_ << std::endl;
    }

    ~NiceBuffer() = default;

    NiceBuffer(const NiceBuffer &) = delete;

    NiceBuffer &operator=(const NiceBuffer &) = delete;

public:
//    explicit NiceBuffer(int x) : a(x) {}
//    NiceBuffer() : NiceBuffer(0)  {}
    int get(const string &key, T &value);

    int update(const string &key, const T &value);

    bool can_update();

    int start_update();

    int finish_update();

private:
    volatile std::atomic<int> idx_ = {0};
    volatile std::atomic<bool> is_updating_ = {false};
    unordered_map<string, T> *buffer0_;
    unordered_map<string, T> *buffer1_;

    unordered_map<string, T> *get_current_map();

    unordered_map<string, T> *get_update_map();
};


template<typename T>
unordered_map<string, T> *NiceBuffer<T>::get_update_map() {
//    std::cout << "get_update_map idx_: " << idx_.load() << std::endl;
    if (idx_.load() == 0) {
        return buffer1_;
    } else {
        return buffer0_;
    }
}

template<typename T>
unordered_map<string, T> *NiceBuffer<T>::get_current_map() {
//    std::cout << "get_current_map idx_: " << idx_.load() << std::endl;
    if (idx_.load() == 0) {
        return buffer0_;
    } else {
        return buffer1_;
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
//    std::cout << "before idx_=" << idx_.load() << std::endl;
    if (idx_.load() == 0) {
        idx_.store(1);
    } else {
        idx_.store(0);
    }
//    std::cout << "after idx_=" << idx_.load() << std::endl;
    is_updating_.store(false);
    std::this_thread::sleep_for(std::chrono::seconds(5));
    return 0;
}

template<typename T>
bool NiceBuffer<T>::can_update() {
    return !is_updating_.load();
}

template<typename T>
int NiceBuffer<T>::start_update() {
    is_updating_.store(true);
    return 0;
}


void update_worker(NiceBuffer<double> *nb) {
    std::time_t pre_update_time = 0;
    int count = 0;
    int k = 1;
    while (1) {
        std::time_t now = std::time(0);
//        std::cout << now << "  " << pre_update_time << std::endl;
        if ((now % 10 == 0) && (now - pre_update_time > 9) && (nb->can_update())) {
            nb->start_update();
            for (int i = 0; i < 1000; i++) {
                auto key = std::to_string(i);
                nb->update("k" + key, k * (count++));
            }
            k *= -1;
            pre_update_time = now;
            nb->finish_update();
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        if (now < 0) {
            break;
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


