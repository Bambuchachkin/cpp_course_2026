#include <iostream>
#include <random>
#include <vector>

#include "LFU.cpp"
#include "2Q.cpp"
// #include "ARC.cpp"
// #include "LIRS.cpp"
// #include "ideal_cache.cpp"
// #include "two_level_cache.cpp"
// #include "three_level_cache.cpp"

// Генерируем последовательность int-ов в диапазоне [0, key_space).
static std::vector<int> generate_sequence(size_t length, int key_space, unsigned seed) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(0, key_space - 1);

    std::vector<int> seq;
    seq.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        seq.push_back(dist(rng));
    }
    return seq;
}

// Прогоняем одну последовательность через LFU. Возвращаем число хитов.
static size_t run_lfu(const std::vector<int>& seq, size_t capacity) {
    LFU<int, int> cache(capacity);
    size_t hits = 0;

    for (size_t i = 0; i < seq.size(); ++i) {
        int key = seq[i];

        if (cache.contains(key)) {
            ++hits;
            const int* val = cache.get(key);   // поднимаем частоту
            (void) val;                        // значение не важно, важен факт попадания
        } else {
            cache.put(key, key);               // value = key (нам всё равно)
        }
    }
    return hits;
}

// Прогоняем одну последовательность через 2Q. Возвращаем число хитов.
static size_t run_two_q(const std::vector<int>& seq, size_t capacity) {
    TwoQCache<int, int> cache(capacity);
    size_t hits = 0;

    for (size_t i = 0; i < seq.size(); ++i) {
        int key = seq[i];

        const int* val = cache.get(key);
        if (val != nullptr) {
            ++hits;
        } else {
            cache.put(key, key);
        }
    }
    return hits;
}

int main() {
    const size_t capacity    = 64;
    const size_t seq_length  = 100000;
    const int    key_space   = 512;    // ключей в 8 раз больше ёмкости
    const unsigned seed      = 42;

    std::vector<int> seq = generate_sequence(seq_length, key_space, seed);

    size_t hits_lfu = run_lfu(seq, capacity);
    size_t hits_2q  = run_two_q(seq, capacity);

    std::cout << "Sequence: length=" << seq_length
              << ", key_space=" << key_space
              << ", capacity=" << capacity << "\n";
    std::cout << "----------------------------------------\n";
    std::cout << "LFU hits: " << hits_lfu
              << "  (" << (100.0 * hits_lfu / seq_length) << "%)\n";
    std::cout << "2Q  hits: " << hits_2q
              << "  (" << (100.0 * hits_2q  / seq_length) << "%)\n";

    return 0;
}