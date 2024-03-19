#pragma once

#include <utility>
#include <type_traits>
#include <new>

namespace smart_ptr {

namespace details {

struct super_linked_ptr {
    super_linked_ptr() : next(this), prev(this) {}

    mutable const super_linked_ptr *next;
    mutable const super_linked_ptr *prev;

    void insert(const super_linked_ptr &other) const { // insert this after other
        other.next->prev = this;
        next = other.next;
        prev = &other;
        other.next = this;
    }

    void remove() {
        next->prev = prev;
        prev->next = next;
        next = prev = this;
    }
};

}

template<typename T>
class linked_ptr : public details::super_linked_ptr {
public:
    explicit linked_ptr(T *ptr = nullptr): super_linked_ptr(), data(ptr) {}

    linked_ptr(const linked_ptr<T> &other): super_linked_ptr(), data(other.data) {
        insert(other);
    }

    linked_ptr(linked_ptr<T> &&other) noexcept {
        insert(other);
        data = other.data;
        other.remove();
        other.data = nullptr;
    }

    linked_ptr<T>& operator=(const linked_ptr<T>& other) {
        if (&other != this) {
            this->~linked_ptr();
            new (this) linked_ptr(other);
        }
        return *this;
    }

    linked_ptr<T>& operator=(linked_ptr<T>&& other) noexcept {
        this->~linked_ptr();
        new (this) linked_ptr(other);
        other.remove();
        other.data = nullptr;
        return *this;
    }

    template<typename U>
    linked_ptr<T>& operator=(linked_ptr<U> const &other) {
        static_assert(std::is_convertible_v<U*, T*>);
        if (next != other.next && prev != other.prev) {
            this->~linked_ptr();
            new (this) linked_ptr(other);
        }
        return *this;
    }

    template<typename U>
    explicit linked_ptr(U *other): super_linked_ptr(), data(other) {
        static_assert(std::is_convertible_v<U *, T *>);
    }

    template<typename U>
    linked_ptr(const linked_ptr<U> &other): super_linked_ptr() {
        static_assert(std::is_convertible_v<U *, T *>);
        data = static_cast<T*>(other.data);
        insert(other);
    }

    void reset(T* ptr = nullptr) {
        delete_if_unique();
        remove();
        data = ptr;
    }

    explicit operator bool() const noexcept {
        return data != nullptr;
    }

    void swap(linked_ptr<T>& other) noexcept {
        if (other.data == data) return;
        using std::swap;
        swap(data, other.data);
        if (is_unique()) {
            if (other.is_unique()) {
                return;
            }
            other.next->prev = this;
            this->next = other.next;
            other.next = this;
            this->prev = &other;
            other.remove();
            return;
        }
        if (other.is_unique()) {
            swap(data, other.data);
            other.swap(*this);
            return;
        }
        swap(other.next->prev, next->prev);
        swap(other.prev->next, prev->next);
        swap(other.next, next);
        swap(other.prev, prev);
    }

    bool unique() const {
        return next == this && prev == this && data != nullptr;
    }

    [[nodiscard]] T* get() const noexcept {
        return data;
    }

    T& operator*() const noexcept {
        return *data;
    }

    T* operator->() const noexcept {
        return data;
    }

    template<typename U>
    bool operator==(const linked_ptr<U> &rhs) const {
        return data == rhs.data;
    }

    template<typename U>
    bool operator<(const linked_ptr<U> &rhs) const {
        return data < rhs.data;
    }

    template<typename U>
    bool operator!=(const linked_ptr<U> &rhs) const {
        return data != rhs.data;
    }

    ~linked_ptr() {
        delete_if_unique();
        remove();
    }

    T *data;

private:
    void delete_if_unique() {
        if (unique()) {
            static_assert(sizeof(T) != 0);
            delete data;
            data = nullptr;
        }
    }

    [[nodiscard]] bool is_unique() const {
        return next == this && prev == this;
    }
};

template<class T, class... Args>
linked_ptr<T> make_linked(Args&&... args) {
    return linked_ptr<T>(new T(std::forward<Args>(args)...));
}

}