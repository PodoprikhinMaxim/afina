#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) {
    if (key.size() + value.size() > _max_size) {
        return false;
    }
    auto it = _lru_index.find(key);
    if (it != _lru_index.end()) {
        return put_old(it->second.get(), value);
    }
    return put_new(key, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
    if (key.size() + value.size() > _max_size) {
        return false;
    }
    auto it = _lru_index.find(key);
    if (it != _lru_index.end()) {
        return false;
    }
    return put_new(key, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
    if (key.size() + value.size() > _max_size) {
        return false;
    }
    auto it = _lru_index.find(key);
    if (it == _lru_index.end()) {
        return false;
    }
    return put_old(it->second.get(), value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) {
    auto it = _lru_index.find(key);
    if (it == _lru_index.end()) {
        return false;
    }
    lru_node &delete_element = it->second.get();
    _current_size -= delete_element.value.size() + delete_element.key.size();
    _lru_index.erase(key);

    if (delete_element.prev == nullptr) {
        delete_element.next->prev = nullptr;
        _lru_head.swap(delete_element.next);
        delete_element.next.reset();
        return true;
    } else if (delete_element.next == nullptr) {
        delete_element.prev->next.swap(delete_element.next);
        _lru_tail = delete_element.prev;
        delete_element.next.reset();
        return true;
    } else {
        delete_element.next->prev = delete_element.prev;
        delete_element.prev->next.swap(delete_element.next);
        delete_element.next.reset();
        return true;
    }
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) {
    if (_lru_head == nullptr) {
        return false;
    }
    auto get_element = _lru_index.find(key);
    if (get_element == _lru_index.end()) {
        return false;
    }
    value = get_element->second.get().value;
    RefreshList(get_element->second.get());
    return true;
}

bool SimpleLRU::put_new(const std::string &key, const std::string &value) {
    while (_max_size - _current_size < key.size() + value.size()) {
        if (_lru_head == nullptr) {
            return false;
        }
        DeleteLRU();
    }

    auto new_element = std::unique_ptr<lru_node>(new lru_node(key, value));
    if (_lru_tail != nullptr) {
        new_element->prev = _lru_tail;
        _lru_tail->next.swap(new_element);
        _lru_tail = _lru_tail->next.get();
    } else {
        _lru_tail = new_element.get();
        _lru_head.swap(new_element);
    }

    _lru_index.insert(std::make_pair(std::ref(_lru_tail->key), std::ref(*_lru_tail)));

    _current_size += key.size() + value.size();
    return true;
}

bool SimpleLRU::put_old(lru_node &curr_node, const std::string &value) {
    auto old_value = curr_node.value;
    RefreshList(curr_node);
    if (value.size() >= old_value.size()) {
        while (value.size() - old_value.size() > _max_size - _current_size) {
            if (_lru_head == nullptr) {
                break;
            }
            DeleteLRU();
        }
        curr_node.value = value;
        _current_size += value.size() - old_value.size();
        return true;
    } else {
        while (old_value.size() - value.size() > _max_size - _current_size) {
            if (_lru_head == nullptr) {
                break;
            }
            DeleteLRU();
        }
        curr_node.value = value;
        _current_size -= old_value.size() - value.size();
        return true;
    }
}

bool SimpleLRU::RefreshList(lru_node &curr_node) {
    if (curr_node.next == nullptr) {
        return true;
    }
    if (&curr_node == _lru_head.get()) {
        _lru_head.swap(curr_node.next);
        _lru_head->prev = nullptr;
    } else {
        curr_node.next->prev = curr_node.prev;
        curr_node.prev->next.swap(curr_node.next);
    }
    _lru_tail->next.swap(curr_node.next);
    curr_node.prev = _lru_tail;
    _lru_tail = &curr_node;
    return true;
}

void SimpleLRU::DeleteLRU() {
    _current_size -= _lru_head->key.size() + _lru_head->value.size();

    _lru_index.erase(_lru_head->key);

    if (_lru_head->next != nullptr) {
        _lru_head->next->prev = _lru_head->prev;
        std::unique_ptr<lru_node> tmp(std::move(_lru_head));
        _lru_head = std::move(tmp->next);
    } else {
        _lru_tail = nullptr;
        _lru_head.reset();
    }
}

} // namespace Backend
} // namespace Afina
