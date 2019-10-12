#include "SimpleLRU.h"

namespace Afina {
namespace Backend {

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Put(const std::string &key, const std::string &value) { 
	if(key.size() + value.size() > _max_size)
		return false;
	if(_lru_index.find(key) != _lru_index.end())
		return put_old(key, value);
	return put_new(key,value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {
	if (key.size() + value.size() > _max_size)
		return false;
	if (_lru_index.find(key) != _lru_index.end())
		return false;
	return put_new(key,value);
 
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Set(const std::string &key, const std::string &value) {
	if (key.size() + value.size() > _max_size)
		return false;
	if (_lru_index.find(key) == _lru_index.end())
		return false;
	return put_old(key, value);
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Delete(const std::string &key) { 
	auto it = _lru_index.find(key);
	if (it == _lru_index.end())
		return false;

	lru_node &delete_element = it->second.get();
	_current_size -= delete_element.value.size() + delete_element.key.size();
	_lru_index.erase(key);

	if (delete_element.prev == nullptr) {
		delete_element.next->prev = nullptr;
		_lru_head.swap(delete_element.next);
		delete_element.next.reset();
		return true;
	}
	else if (delete_element.next == nullptr) {
		delete_element.prev->next.swap(delete_element.next);
		_lru_tail = delete_element.prev;
		delete_element.next.reset();
		return true;
	}
	else {
		delete_element.next->prev = delete_element.prev;
		delete_element.prev->next.swap(delete_element.next);
		delete_element.next.reset();
		return true;
	}
}

// See MapBasedGlobalLockImpl.h
bool SimpleLRU::Get(const std::string &key, std::string &value) { 
	if (_lru_head == nullptr)
		return false;
	auto get_element = _lru_index.find(key);
	if (get_element == _lru_index.end())
		return false;
	value = get_element->second.get().value;
	RefreshList(get_element->second.get());
	return true;
}

bool SimpleLRU::put_new(const std::string &key, const std::string &value) {
	while (_max_size - _current_size < key.size() + value.size()) {
		if (_lru_head == nullptr)
			return false;
		//DeleteLRU();
		const std::string key_h = _lru_head->key;
		Delete(key_h);
	}

	auto new_element = std::unique_ptr<lru_node>(new lru_node(key, value));
	if (_lru_tail != nullptr) {
		new_element->prev = _lru_tail;
		_lru_tail->next.swap(new_element);
        	_lru_tail = _lru_tail->next.get();
	} 
	else {
		_lru_tail = new_element.get();
       		_lru_head.swap(new_element);
	}

	_lru_index.insert(std::make_pair(std::ref(_lru_tail->key), std::ref(*_lru_tail)));

	_current_size += key.size() + value.size();
	//new_element.reset(); 
	return true;
}

bool SimpleLRU::put_old(const std::string &key, const std::string &value) {
	auto iterator = _lru_index.find(key);
	auto old_value = iterator->second.get().value;
	iterator->second.get().value = value;
	RefreshList(iterator->second.get());
	_current_size += value.size() - old_value.size();
	return true;
}

bool SimpleLRU::RefreshList(lru_node &curr_node) {
	if (curr_node.next == nullptr)
		return true;

	if(&curr_node == _lru_head.get()) {
		_lru_head.swap(curr_node.next);
		_lru_head->prev = nullptr;
	}
	else {
		curr_node.next->prev = curr_node.prev;
		curr_node.prev->next.swap(curr_node.next);
	}
	_lru_tail->next.swap(curr_node.next);
	curr_node.prev = _lru_tail;
	_lru_tail = &curr_node;
	return true;
}

/*bool SimpleLRU::DeleteLRU() {
	if (_lru_head == nullptr)
		return false;
	auto value = _lru_head->value;
	auto key = _lru_head->key;
	_lru_index.erase(_lru_head->key);
	if (_lru_head->next == nullptr) {
		_lru_head.reset();
		_lru_tail = nullptr;
		_lru_head = nullptr;
	} 
	else {
		_lru_head.swap(_lru_head->next);
		_lru_head->prev = nullptr;
	}
	_current_size -= key.size() + value.size();
	return true;
}*/

} // namespace Backend
} // namespace Afina
