/*******************************************************************************
* Copyright 2024-2025 Intel Corporation
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

#ifndef COMMON_SDPA_TYPES_HPP
#define COMMON_SDPA_TYPES_HPP

#include "oneapi/dnnl/dnnl_types.h"

#include "common/c_types_map.hpp"
#include "common/memory_desc.hpp"
#include "common/primitive_attr_quant.hpp"

#include <assert.h>

namespace dnnl {
namespace impl {

#define DNNL_ARG_QUERIES DNNL_ARG_SRC_0
#define DNNL_ARG_KEYS DNNL_ARG_SRC_1
#define DNNL_ARG_VALUES DNNL_ARG_SRC_2
#define DNNL_ARG_ATTN_MASK DNNL_ARG_SHIFT

// NOLINTBEGIN(modernize-use-using)
/// Types of attention mask
typedef enum {
    dnnl_attn_mask_undef = 0,
    /// explicit attention masks defined in a buffer
    dnnl_attn_mask_buffer = 1,

    /// causal mask with the diagonal starting from the top left hand side of
    /// the mask tensor
    dnnl_attn_mask_top_left = 2,

    /// causal mask with the diagonal starting from the bottom right hand side
    /// of the mask tensor
    dnnl_attn_mask_bottom_right = 3,
} dnnl_attn_mask_type_t;
// NOLINTEND(modernize-use-using)

using attn_mask_type_t = dnnl_attn_mask_type_t;
namespace attn_mask_type {
const attn_mask_type_t undef = dnnl_attn_mask_undef;
const attn_mask_type_t buffer = dnnl_attn_mask_buffer;
const attn_mask_type_t top_left = dnnl_attn_mask_top_left;
const attn_mask_type_t bottom_right = dnnl_attn_mask_bottom_right;
} // namespace attn_mask_type

// A descriptor for a scaled dot product attention (SDPA) operation.
struct sdpa_desc_t : public op_desc_t {
    sdpa_desc_t() : op_desc_t(primitive_kind::sdpa) {}

    std::unique_ptr<op_desc_t> clone() const override {
        return utils::make_unique<sdpa_desc_t>(*this);
    }

    memory_desc_t q_desc; /* queries */
    memory_desc_t k_desc; /* keys */
    memory_desc_t v_desc; /* values */

    // primitive_attr_t can't be used because of deleted copy-ctor, but desc_t
    // must be copyable.
    quant_entry_t kq_scales;
    quant_entry_t kq_zero_points;
    quant_entry_t vs_scales;
    quant_entry_t vs_zero_points;

    memory_desc_t dst_desc;
    memory_desc_t attn_mask_desc;
    data_type_t scale_dt {};
    // invert_scale = false: multiply by scale
    // invert_scale = true:  divide by scale
    bool invert_scale {};
    dim_t kv_head_number {};

    attn_mask_type_t mask_type = attn_mask_type::undef;
    alg_kind_t softmax_alg = alg_kind::softmax_accurate;

    // Number of queries.
    dnnl_dim_t queries() const { return q_desc.dims[q_desc.ndims - 2]; }
    // Head size.
    dnnl_dim_t head_size() const { return q_desc.dims[q_desc.ndims - 1]; }
    // Number of keys.
    dnnl_dim_t keys() const { return k_desc.dims[k_desc.ndims - 1]; }
    // Number of values.
    dnnl_dim_t values() const { return v_desc.dims[v_desc.ndims - 1]; }
    // Total batch size.
    dnnl_dim_t batch_size() const {
        dnnl_dim_t batch = 1;
        for (int i = 0; i < dst_desc.ndims - 2; i++)
            batch *= dst_desc.dims[i];
        return batch;
    }
};

} // namespace impl
} // namespace dnnl

#endif // COMMON_SDPA_TYPES_HPP
