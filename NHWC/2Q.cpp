#include <list>
#include <unordered_map>
#include <cstddef>

template <typename Value_T, typename Key_T>
struct TwoQCache {
private:
    struct Node {
        Value_T value_;
        Key_T  key_;
        Node(Key_T k, Value_T v) : key_(k), value_(v) {}
    };

    size_t capacity_;
    size_t kin_;    // максимум элементов в A1in
    size_t kmax_;   // максимум элементов в Am
    size_t kout_;   // максимум элементов в A1out (ghost)

    std::list<Node*> a1in_;    // FIFO: front — самый новый
    std::list<Node*> am_;      // LRU:  front — самый свежий
    std::list<Key_T> a1out_;   // ghost: только ключи

    std::unordered_map<Key_T, Node*> in_map_;   // key -> Node* в a1in_
    std::unordered_map<Key_T, Node*> am_map_;   // key -> Node* в am_
    std::unordered_map<Key_T, int>   out_map_;  // key -> заглушка (важно только наличие)

    void promote_to_am(Node* node);
    void evict_a1in_to_ghost();
    void evict_ghost();
    void evict_am();

public:
    TwoQCache(size_t capacity);
    ~TwoQCache();

    void clear();
    void put(const Key_T key, const Value_T value);
    const Value_T* get(const Key_T key);

    bool contains(const Key_T key) const;
    bool empty() const;
    size_t get_size() const;
    size_t get_capacity() const;
};

// ---------- ctor / dtor ----------

template <typename Value_T, typename Key_T>
TwoQCache<Value_T, Key_T>::TwoQCache(size_t capacity)
    : capacity_(capacity)
{
    // Классические доли: Kin = 25%, Kout = 50%, Km = 75% от capacity.
    // Для маленьких capacity поднимаем минимум до 1, чтобы не сломаться.
    kin_ = capacity / 4;
    if (kin_ == 0 && capacity > 0) kin_ = 1;

    kout_ = capacity / 2;
    if (kout_ == 0 && capacity > 0) kout_ = 1;

    kmax_ = (capacity > kin_) ? (capacity - kin_) : 1;
}

template <typename Value_T, typename Key_T>
TwoQCache<Value_T, Key_T>::~TwoQCache() { clear(); }

// ---------- private helpers ----------

template <typename Value_T, typename Key_T>
void TwoQCache<Value_T, Key_T>::evict_am() {
    if (am_.empty()) return;
    Node* victim = am_.back();
    am_.pop_back();
    am_map_.erase(victim->key_);
    delete victim;
}

template <typename Value_T, typename Key_T>
void TwoQCache<Value_T, Key_T>::evict_ghost() {
    if (a1out_.empty()) return;
    Key_T key = a1out_.back();
    a1out_.pop_back();
    out_map_.erase(key);
}

template <typename Value_T, typename Key_T>
void TwoQCache<Value_T, Key_T>::evict_a1in_to_ghost() {
    if (a1in_.empty()) return;

    Node* victim = a1in_.back();
    a1in_.pop_back();
    in_map_.erase(victim->key_);

    if (a1out_.size() >= kout_) {
        evict_ghost();
    }
    a1out_.push_front(victim->key_);
    out_map_[victim->key_] = 1;

    delete victim;
}

template <typename Value_T, typename Key_T>
void TwoQCache<Value_T, Key_T>::promote_to_am(Node* node) {
    in_map_.erase(node->key_);
    a1in_.remove(node);

    if (am_.size() >= kmax_) {
        evict_am();
    }
    am_.push_front(node);
    am_map_[node->key_] = node;
}

// ---------- public API ----------

template <typename Value_T, typename Key_T>
void TwoQCache<Value_T, Key_T>::clear() {
    while (!a1in_.empty()) {
        Node* n = a1in_.front();
        a1in_.pop_front();
        delete n;
    }
    while (!am_.empty()) {
        Node* n = am_.front();
        am_.pop_front();
        delete n;
    }
    a1out_.clear();
    in_map_.clear();
    am_map_.clear();
    out_map_.clear();
}

template <typename Value_T, typename Key_T>
void TwoQCache<Value_T, Key_T>::put(const Key_T key, const Value_T value) {
    if (capacity_ == 0) return;

    // 1) Ключ уже в Am — обновляем значение и поднимаем в MRU.
    if (am_map_.count(key) > 0) {
        Node* node = am_map_[key];
        node->value_ = value;
        am_.remove(node);
        am_.push_front(node);
        return;
    }

    // 2) Ключ в A1in — обновляем значение и повышаем до Am.
    if (in_map_.count(key) > 0) {
        Node* node = in_map_[key];
        node->value_ = value;
        promote_to_am(node);
        return;
    }

    // 3) Ключ в ghost — это «повторное обращение», сразу в Am.
    if (out_map_.count(key) > 0) {
        out_map_.erase(key);
        a1out_.remove(key);

        Node* node = new Node(key, value);
        if (am_.size() >= kmax_) {
            evict_am();
        }
        am_.push_front(node);
        am_map_[key] = node;
        return;
    }

    // 4) Новый ключ — в A1in; при переполнении хвост уходит в ghost.
    Node* node = new Node(key, value);
    a1in_.push_front(node);
    in_map_[key] = node;
    if (a1in_.size() > kin_) {
        evict_a1in_to_ghost();
    }
}

template <typename Value_T, typename Key_T>
const Value_T* TwoQCache<Value_T, Key_T>::get(const Key_T key) {
    // 1) Ключ в Am — LRU-подъём.
    if (am_map_.count(key) > 0) {
        Node* node = am_map_[key];
        am_.remove(node);
        am_.push_front(node);
        return &node->value_;
    }

    // 2) Ключ в A1in — повышаем до Am (второе обращение).
    if (in_map_.count(key) > 0) {
        Node* node = in_map_[key];
        promote_to_am(node);
        return &node->value_;
    }

    // 3) Ключ в ghost — это всё равно промах (значения нет).
    //    Продвижение в Am произойдёт при следующем put.
    return nullptr;
}

template <typename Value_T, typename Key_T>
bool TwoQCache<Value_T, Key_T>::contains(const Key_T key) const {
    return in_map_.count(key) > 0 || am_map_.count(key) > 0;
}

template <typename Value_T, typename Key_T>
bool TwoQCache<Value_T, Key_T>::empty() const {
    return in_map_.empty() && am_map_.empty();
}

template <typename Value_T, typename Key_T>
size_t TwoQCache<Value_T, Key_T>::get_size() const {
    return in_map_.size() + am_map_.size();
}

template <typename Value_T, typename Key_T>
size_t TwoQCache<Value_T, Key_T>::get_capacity() const {
    return capacity_;
}