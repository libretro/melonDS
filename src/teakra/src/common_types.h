#pragma once

#include <cstdint>

#ifdef HAVE_THREADS
#include <atomic>
#include <mutex>
#else
namespace std
{
    class mutex {};
    class recursive_mutex {};

    template<typename _Mutex>
    class lock_guard
    {
    public:
        lock_guard(_Mutex& _) { }
    };

    template <typename _Tp>
    struct atomic
    {
    public:
        atomic() {}
        atomic(_Tp new_value) { this->value = new_value; }

        _Tp exchange(_Tp new_value) 
        {
            _Tp old_value = this->value;
            this->value = new_value; 
            return old_value;
        }

        operator bool() { return this->value; }

    private:
        _Tp value;
    };
}
#endif

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

using s8 = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;

template <typename T>
constexpr unsigned BitSize() {
    return sizeof(T) * 8; // yeah I know I shouldn't use 8 here.
}

template <typename T>
constexpr T SignExtend(const T value, unsigned bit_count) {
    const T mask = static_cast<T>(1ULL << bit_count) - 1;
    const bool sign_bit = ((value >> (bit_count - 1)) & 1) != 0;
    if (sign_bit) {
        return value | ~mask;
    }
    return value & mask;
}

template <unsigned bit_count, typename T>
constexpr T SignExtend(const T value) {
    static_assert(bit_count <= BitSize<T>(), "bit_count larger than bitsize of T");
    return SignExtend(value, bit_count);
}

inline constexpr u16 BitReverse(u16 value) {
    u16 result = 0;
    for (u32 i = 0; i < 16; ++i) {
        result |= ((value >> i) & 1) << (15 - i);
    }
    return result;
}
