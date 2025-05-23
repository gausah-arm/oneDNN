/*******************************************************************************
* Copyright 2022-2025 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#ifndef GPU_GENERIC_SYCL_SYCL_IO_HELPER_HPP
#define GPU_GENERIC_SYCL_SYCL_IO_HELPER_HPP

#include "common/c_types_map.hpp"
#include "common/dnnl_traits.hpp"

#include "gpu/generic/sycl/sycl_q10n.hpp"

namespace dnnl {
namespace impl {
namespace gpu {
namespace generic {
namespace sycl {

inline bool is_supported_type(data_type_t dt) {
    using namespace data_type;
    return utils::one_of(dt, f32, f16, bf16, s32, s8, u8);
}

inline int load_int_value(data_type_t dt, const void *ptr, dim_t idx) {
#define CASE(dt) \
    case dt: \
        return static_cast<int>(reinterpret_cast< \
                const typename xpu::sycl::prec_traits_t<dt>::type *>( \
                ptr)[idx]);
    using namespace data_type;
    switch (dt) {
        CASE(s32);
        CASE(s8);
        CASE(u8);
        default: return INT_MAX;
    }

#undef CASE
    return INT_MAX;
}

inline float load_float_value(data_type_t dt, const void *ptr, dim_t idx) {
#define CASE(dt) \
    case dt: \
        return static_cast<float>(reinterpret_cast< \
                const typename xpu::sycl::prec_traits_t<dt>::type *>( \
                ptr)[idx]);

    using namespace data_type;
    switch (dt) {
        CASE(bf16);
        CASE(f16);
        CASE(f32);
        CASE(s32);
        CASE(s8);
        CASE(u8);
        default: return ::sycl::nan(0u);
    }

#undef CASE
    return ::sycl::nan(0u);
}

inline void store_float_value(data_type_t dt, float val, void *ptr, dim_t idx) {
#define CASE(dt) \
    case dt: { \
        using type_ = typename xpu::sycl::prec_traits_t<dt>::type; \
        *(reinterpret_cast<type_ *>(ptr) + idx) \
                = gpu::generic::sycl::saturate_and_round<type_>(val); \
    } break;

    using namespace data_type;
    switch (dt) {
        CASE(bf16);
        CASE(f16);
        CASE(f32);
        CASE(s32);
        CASE(s8);
        CASE(u8);
        default: (void)0;
    }

#undef CASE
}

namespace {
template <typename T>
using global_ptr
        = ::sycl::multi_ptr<T, ::sycl::access::address_space::generic_space,
                ::sycl::access::decorated::yes>;

template <int width>
inline ::sycl::vec<float, width> handle_bf16_load(void *ptr, dim_t offset) {
    // Load 16 * width bits.
    global_ptr<uint16_t> gptr_u16(reinterpret_cast<uint16_t *>(ptr));
    ::sycl::vec<uint16_t, width> vec_u16;
    vec_u16.load(offset, gptr_u16);
    // Manually convert them as SYCL doesn't yet support bfloat16 conversion.
    ::sycl::vec<float, width> vec_f32;
    for (int i = 0; i < width; i++) {
        // Convert u16 value to bfloat16_t.
        const xpu::sycl::bfloat16_t bf16_val
                = static_cast<xpu::sycl::bfloat16_t>(vec_u16[i]);
        // Convert bfloat16_t to float.
        const float f32_val = static_cast<float>(bf16_val);
        // Write result to vector.
        vec_f32[i] = f32_val;
    }
    return vec_f32;
}

template <int width>
inline void handle_bf16_store(
        ::sycl::vec<float, width> vec_f32, void *ptr, dim_t offset) {
    global_ptr<uint16_t> gptr_u16(reinterpret_cast<uint16_t *>(ptr));
    ::sycl::vec<uint16_t, width> vec_u16;

    for (int i = 0; i < width; i++) {
        // Convert float value to bfloat16_t.
        const xpu::sycl::bfloat16_t bf16_val
                = static_cast<xpu::sycl::bfloat16_t>(vec_f32[i]);
        // Convert bfloat16_t to uint16_t.
        const uint16_t u16_val = bf16_val.raw_bits_;
        // Write result to vector.
        vec_u16[i] = u16_val;
    }
    vec_u16.store(offset, gptr_u16);
}
} // namespace

template <int width>
inline ::sycl::vec<float, width> load_float_vec(
        data_type_t dt, void *ptr, dim_t offset) {
#define CASE(dt) \
    case dt: { \
        using type = typename xpu::sycl::prec_traits_t<dt>::type; \
        global_ptr<type> gptr_dt(reinterpret_cast<type *>(ptr)); \
        ::sycl::vec<type, width> vec_dt; \
        vec_dt.load(offset, gptr_dt); \
        /* TODO: check rounding mode */ \
        return vec_dt.template convert<float>(); \
    } break;

    using namespace data_type;
    switch (dt) {
        CASE(f16);
        CASE(f32);
        CASE(s32);
        CASE(s8);
        CASE(u8);
        case bf16: return handle_bf16_load<width>(ptr, offset);
        default: return ::sycl::vec<float, width> {NAN};
    }
#undef CASE
}

template <int width>
inline void store_float_vec(data_type_t dt, ::sycl::vec<float, width> vec_f32,
        void *ptr, dim_t offset) {
#define CASE(dt) \
    case dt: { \
        using type = typename xpu::sycl::prec_traits_t<dt>::type; \
        global_ptr<type> gptr_dt(reinterpret_cast<type *>(ptr)); \
        auto vec_dt \
                = gpu::generic::sycl::saturate_and_round_vec<type>(vec_f32); \
        vec_dt.store(offset, gptr_dt); \
    } break;

    using namespace data_type;
    switch (dt) {
        CASE(f16);
        CASE(f32);
        CASE(s32);
        CASE(s8);
        CASE(u8);
        case bf16: handle_bf16_store(vec_f32, ptr, offset); break;
        default: (void)(0);
    }
#undef CASE
}

template <::sycl::access_mode mode>
struct memory_plain_t {
    memory_plain_t(xpu::sycl::memory_arg_t<mode> mem, data_type_t type)
        : mem_(mem), type_(type) {}

    inline float load(dim_t idx) const {
        return load_float_value(type_, mem_.get_pointer(), idx);
    }

    inline void store(float val, dim_t idx) {
        store_float_value(type_, val, mem_.get_pointer(), idx);
    }

private:
    xpu::sycl::memory_arg_t<mode> mem_;
    data_type_t type_;
};

using in_memory_plain_t = memory_plain_t<::sycl::access::mode::read>;
using out_memory_plain_t = memory_plain_t<::sycl::access::mode::write>;
using inout_memory_plain_t = memory_plain_t<::sycl::access::mode::read_write>;

template <::sycl::access_mode mode>
struct memory_tensor_t {
    memory_tensor_t(xpu::sycl::memory_arg_t<mode> mem, xpu::sycl::md_t md)
        : mem_(mem), md_(md) {}

    inline xpu::sycl::md_t md() const { return md_; }

    inline float load(dim_t idx) const {
        return load_float_value(md_.data_type(), mem_.get_pointer(), idx);
    }

    inline void store(float val, dim_t idx) {
        store_float_value(md_.data_type(), val, mem_.get_pointer(), idx);
    }

    template <int width>
    inline ::sycl::vec<float, width> load_vec(dim_t idx) const {
        return load_float_vec<width>(md_.data_type(), mem_.get_pointer(), idx);
    }

    template <int width>
    inline void store_vec(::sycl::vec<float, width> val, dim_t idx) {
        store_float_vec<width>(md_.data_type(), val, mem_.get_pointer(), idx);
    }

    inline float load_md(const dims_t &pos, bool padded = false) const {
        return load(md_.off_v(pos, padded));
    }

    inline float load_md_bc(dims_t offsets) const {
        dims_t offsets_masked;
        for (int i = 0; i < xpu::sycl::md_t::max_dims; i++) {
            offsets_masked[i] = md_.dims()[i] == 1 ? 0 : offsets[i];
        }
        return load(md_.off_v(offsets_masked));
    }

    inline void store_md(float val, const dims_t &pos, bool padded = false) {
        store(val, md_.off_v(pos, padded));
    }

    // flattened_index must be less than nelems
    inline void get_logical_index(dim_t flattened_index, dims_t &logical_index,
            bool use_padded_dims = false) {
        const auto &dims = use_padded_dims ? md_.padded_dims() : md_.dims();
        auto ndims = md_.ndims();
        for (auto i = ndims - 1; i >= 0; i--) {
            logical_index[i] = flattened_index % dims[i];
            flattened_index /= dims[i];
        }
    }

    inline void *ptr() const { return mem_.get_pointer(); }

private:
    xpu::sycl::memory_arg_t<mode> mem_;
    xpu::sycl::md_t md_;
};

using in_memory_tensor_t = memory_tensor_t<::sycl::access::mode::read>;
using out_memory_tensor_t = memory_tensor_t<::sycl::access::mode::write>;
using inout_memory_tensor_t = memory_tensor_t<::sycl::access::mode::read_write>;

} // namespace sycl
} // namespace generic
} // namespace gpu
} // namespace impl
} // namespace dnnl

#endif

// vim: et ts=4 sw=4 cindent cino+=l0,\:4,N-s
