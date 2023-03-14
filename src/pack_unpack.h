#pragma once
#include <cstring>
#include <type_traits>
#include <unordered_map>

#include "logging.h"
#include "typedefs.h"

enum pack_dir
{
    PACK_DIR_IN,
    PACK_DIR_OUT
};

namespace pack_va_flags
{
enum : u64
{
    FIXED_ARRAY_CUSTOM_SIZE = 1 // Specify that there is a custom sizet specified by the void* to pack/unpack the array
                                // to
};
}

template<class T>
struct is_binary_archive
{
    static constexpr bool value = false;
};

struct binary_buffer_archive
{
    u8 *data;
    int dir;
    sizet cur_offset{0};
};

// Enable type trait
template<>
struct is_binary_archive<binary_buffer_archive>
{
    static constexpr bool value = true;
};

template<sizet N>
struct binary_fixed_buffer_archive
{
    static constexpr sizet size = N;
    int dir;
    sizet cur_offset{0};
    u8 data[size];
};

// Enable type trait
template<sizet N>
struct is_binary_archive<binary_fixed_buffer_archive<N>>
{
    static constexpr bool value = true;
};

struct pack_var_meta
{
    u64 flags{0};
    void *data{nullptr};
};

struct pack_var_info
{
    const char *name{};
    pack_var_meta meta{};
};

template<class T>
typename std::enable_if<std::is_floating_point_v<T>, const char *>::type get_flag_for_type(T &var)
{
    return "%f";
}

template<class T>
typename std::enable_if<std::is_integral_v<T>, const char *>::type get_flag_for_type(T &var)
{
    return "%d";
}

template<class ArchiveT, class T>
typename std::enable_if<std::is_arithmetic_v<T> && is_binary_archive<ArchiveT>::value, void>::type pack_unpack(
    ArchiveT &ar,
    T &val,
    const pack_var_info &vinfo)
{
    char logging_statement[]{"Packing %s %d bytes for %s with value xx"};
    const char *flag = get_flag_for_type(val);
    memcpy(strstr(logging_statement, "xx"), flag, 2);

    sizet sz = sizeof(T);
    if (ar.dir == PACK_DIR_IN)
        memcpy(&val, ar.data + ar.cur_offset, sz);
    else
        memcpy(ar.data + ar.cur_offset, &val, sz);
    tlog(logging_statement, (ar.dir) ? "out" : "in", sz, vinfo.name, val);
    ar.cur_offset += sz;
}

template<class ArchiveT, class T, sizet N>
typename std::enable_if<std::is_arithmetic_v<T>, void>::type pack_unpack(ArchiveT &ar,
                                                                         T (&val)[N],
                                                                         const pack_var_info &vinfo)
{
    sizet sz = sizeof(T);
    if (test_flags(vinfo.meta.flags, pack_va_flags::FIXED_ARRAY_CUSTOM_SIZE))
        sz *= *((u32 *)vinfo.meta.data);
    else
        sz *= N;

    if (ar.dir == PACK_DIR_IN)
        memcpy(val, ar.data + ar.cur_offset, sz);
    else
        memcpy(ar.data + ar.cur_offset, val, sz);

    tlog("Packing %s %d bytes for %s (array)", (ar.dir) ? "out" : "in", sz, vinfo.name);
    ar.cur_offset += sz;
}

template<class ArchiveT, class T, sizet N>
typename std::enable_if<!std::is_arithmetic_v<T>, void>::type pack_unpack(ArchiveT &ar,
                                                                          T (&val)[N],
                                                                          const pack_var_info &vinfo)
{
    sizet size = 0;
    if (test_flags(vinfo.meta.flags, pack_va_flags::FIXED_ARRAY_CUSTOM_SIZE))
        size = *((u32 *)vinfo.meta.data);
    else
        size = N;

    for (int i = 0; i < size; ++i) {
        pack_unpack(ar, val[i], vinfo);
    }
}

#define pup_func(type)                                                                                                 \
    template<class ArchiveT>                                                                                           \
    void pack_unpack(ArchiveT &ar, type &val, const pack_var_info &vinfo)

#define pup_member(mem_name) pack_unpack(ar, val.mem_name, {#mem_name, {}})
#define pup_member_meta(mem_name, ...) pack_unpack(ar, val.mem_name, {#mem_name, {__VA_ARGS__}})
#define pup_member_name(mem_name, name) pack_unpack(ar, val.mem_name, {name, {}})
#define pup_member_info(mem_name, info) pack_unpack(ar, val.mem_name, info)

template<class T>
sizet packed_sizeof()
{
    constexpr sizet max_size = sizeof(T);
    static T inst{};
    static binary_fixed_buffer_archive<max_size> buf{};
    pack_unpack(buf, inst, {});
    sizet ret = buf.cur_offset;
    buf.cur_offset = 0;
    return ret;
}
