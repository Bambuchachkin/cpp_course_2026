#include <list>
#include <unordered_map>

template <typename Value_T, typename Key_T, typename Freq_T = unsigned int>
struct LFU {
private:
    struct Node {
        Value_T value_;
        Freq_T freq_;
        Key_T key_;
        Node(Key_T key, Value_T value) : key_(key), value_(value), freq_((Freq_T) 1) {}
    };

    size_t capacity_;
    Freq_T min_freq_ = 0;
    std::unordered_map<Key_T, Node*> search_map_;
    std::unordered_map<Freq_T, std::list<Node*>> sort_map_;

    void update_freq(Node* node);

public:
    LFU(size_t capacity);
    ~LFU();

    void clear();
    void put(const Key_T key, const Value_T value);

    size_t get_size() const;
    size_t get_capacity() const;

    bool contains(const Key_T key) const;
    bool empty() const;
    const Value_T* get(const Key_T key);
};

// ---------- ctor / dtor ----------

template <typename Value_T, typename Key_T, typename Freq_T>
LFU<Value_T, Key_T, Freq_T>::LFU(size_t capacity) : capacity_(capacity) {}

template <typename Value_T, typename Key_T, typename Freq_T>
LFU<Value_T, Key_T, Freq_T>::~LFU() { clear(); }

// ---------- private helpers ----------

template <typename Value_T, typename Key_T, typename Freq_T>
void LFU<Value_T, Key_T, Freq_T>::update_freq(Node* node) {
    Freq_T old_freq = node->freq_;

    if (sort_map_.count(old_freq) > 0) {
        sort_map_[old_freq].remove(node);
        if (sort_map_[old_freq].empty()) {
            sort_map_.erase(old_freq);
            if (old_freq == min_freq_) {
                ++min_freq_;
            }
        }
    }

    ++node->freq_;
    sort_map_[node->freq_].push_front(node);
}

// ---------- public API ----------

template <typename Value_T, typename Key_T, typename Freq_T>
void LFU<Value_T, Key_T, Freq_T>::clear() {
    // .begin() здесь — единственный способ добраться до элементов map
    // без хранения итераторов. Переменных-итераторов нет.
    while (!search_map_.empty()) {
        Node* n = search_map_.begin()->second;
        search_map_.erase(n->key_);
        delete n;
    }
    sort_map_.clear();
    min_freq_ = 0;
}

template <typename Value_T, typename Key_T, typename Freq_T>
void LFU<Value_T, Key_T, Freq_T>::put(const Key_T key, const Value_T value) {
    if (capacity_ == 0) return;

    // Ключ уже есть — обновляем значение и поднимаем частоту.
    if (search_map_.count(key) > 0) {
        search_map_[key]->value_ = value;
        update_freq(search_map_[key]);
        return;
    }

    // Вытеснение: самый старый узел из корзины с минимальной частотой.
    if (search_map_.size() >= capacity_) {
        if (sort_map_.count(min_freq_) > 0 && !sort_map_[min_freq_].empty()) {
            Node* victim = sort_map_[min_freq_].back();
            sort_map_[min_freq_].pop_back();
            if (sort_map_[min_freq_].empty()) {
                sort_map_.erase(min_freq_);
            }
            search_map_.erase(victim->key_);
            delete victim;
        }
    }

    Node* new_node = new Node(key, value);
    search_map_[key] = new_node;
    sort_map_[new_node->freq_].push_front(new_node);
    min_freq_ = new_node->freq_;
}

template <typename Value_T, typename Key_T, typename Freq_T>
const Value_T* LFU<Value_T, Key_T, Freq_T>::get(const Key_T key) {
    if (search_map_.count(key) == 0) return nullptr;
    update_freq(search_map_[key]);
    return &search_map_[key]->value_;
}

template <typename Value_T, typename Key_T, typename Freq_T>
bool LFU<Value_T, Key_T, Freq_T>::contains(const Key_T key) const {
    return search_map_.count(key) > 0;
}

template <typename Value_T, typename Key_T, typename Freq_T>
bool LFU<Value_T, Key_T, Freq_T>::empty() const {
    return search_map_.empty();
}

template <typename Value_T, typename Key_T, typename Freq_T>
size_t LFU<Value_T, Key_T, Freq_T>::get_size() const {
    return search_map_.size();
}

template <typename Value_T, typename Key_T, typename Freq_T>
size_t LFU<Value_T, Key_T, Freq_T>::get_capacity() const {
    return capacity_;
}