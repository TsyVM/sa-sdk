// include/sasdk/core/global.hpp
//
// Typed accessors for the game's global variables, singletons, and fixed
// arrays -- the "typed globals" layer the audit (06_GLOBALS_MEMORY_POOLS) found
// missing. A global lives at a fixed VA; these wrappers resolve the slide and
// hand back a real reference/pointer with the correct type, so a mod reads and
// writes game state through named, typed handles instead of raw casts.
#pragma once
#include <sasdk/core/config.hpp>
#include <sasdk/core/address.hpp>
#include <cstdint>
#include <cstddef>

namespace sasdk::rt {

// A single global object of type T at a fixed VA (e.g. a counter, a flag, a
// singleton struct laid out by sa10us_db.inl).
template <class T>
class Global {
public:
    constexpr explicit Global(std::uint32_t va) : at_(va) {}
    constexpr explicit Global(Address a) : at_(a) {}

    T*  operator->() const { return at_.as<T>(); }
    T&  operator*()  const { return *at_.as<T>(); }
    T&  get()        const { return *at_.as<T>(); }
    T*  ptr()        const { return at_.as<T>(); }
    Address address() const { return at_; }
private:
    Address at_;
};

// A global POINTER to T (very common in GTA:SA: e.g. `CPools *` or a manager
// singleton stored as a pointer). Dereferences twice: *(T**)va.
template <class T>
class GlobalPtr {
public:
    constexpr explicit GlobalPtr(std::uint32_t va) : at_(va) {}
    T*  get()        const { return *at_.as<T*>(); }
    T*  operator->() const { return get(); }
    T&  operator*()  const { return *get(); }
    explicit operator bool() const { return get() != nullptr; }
    Address address() const { return at_; }
private:
    Address at_;
};

// A fixed-size global array at a VA with a compile-time element count. Bounds
// are checked in debug via at(); operator[] is unchecked for hot paths.
template <class T, std::size_t N>
class Array {
public:
    constexpr explicit Array(std::uint32_t va) : at_(va) {}

    T&  operator[](std::size_t i) const { return data()[i]; }
    T*  data()   const { return at_.as<T>(); }
    constexpr std::size_t size() const { return N; }
    T*  begin()  const { return data(); }
    T*  end()    const { return data() + N; }
    Address address() const { return at_; }
private:
    Address at_;
};

// A runtime-sized global array: a VA, an element stride, and a count discovered
// at runtime (e.g. the streaming-info array: base 0x8E4CC0, stride 20,
// 26,316 entries). Stride may differ from sizeof(T) when T is a partial layout.
template <class T>
class ArrayView {
public:
    ArrayView(std::uint32_t va, std::size_t count, std::size_t stride = sizeof(T))
        : at_(va), count_(count), stride_(stride) {}

    T& operator[](std::size_t i) const {
        auto* p = reinterpret_cast<std::uint8_t*>(at_.ptr()) + i * stride_;
        return *reinterpret_cast<T*>(p);
    }
    std::size_t size()   const { return count_; }
    std::size_t stride() const { return stride_; }
    Address address()    const { return at_; }
private:
    Address     at_;
    std::size_t count_;
    std::size_t stride_;
};

} // namespace sasdk::rt
