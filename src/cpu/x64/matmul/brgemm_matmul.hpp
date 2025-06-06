/*******************************************************************************
* Copyright 2021-2025 Intel Corporation
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

#ifndef CPU_X64_MATMUL_BRGEMM_MATMUL_HPP
#define CPU_X64_MATMUL_BRGEMM_MATMUL_HPP

#include "common/c_types_map.hpp"
#include "common/primitive.hpp"
#include "common/type_helpers.hpp"

#include "cpu/matmul/cpu_matmul_pd.hpp"

#include "cpu/x64/brgemm/brgemm.hpp"
#include "cpu/x64/brgemm/brgemm_containers.hpp"
#include "cpu/x64/brgemm/brgemm_utils.hpp"
#include "cpu/x64/cpu_reducer.hpp"
#include "cpu/x64/jit_avx512_core_scale_precompute.hpp"
#include "cpu/x64/jit_avx512_sparse_decompress_kernel.hpp"
#include "cpu/x64/jit_brgemm_post_ops.hpp"
#include "cpu/x64/matmul/brgemm_matmul_copy_utils.hpp"
#include "cpu/x64/matmul/brgemm_matmul_utils.hpp"

namespace dnnl {
namespace impl {
namespace cpu {
namespace x64 {
namespace matmul {

constexpr int dynamic_m_tails[] = {32, 16, 8, 1};
constexpr int max_num_dynamic_m_tails
        = sizeof(dynamic_m_tails) / sizeof(dynamic_m_tails[0]);
constexpr int dynamic_n_tails[] = {32, 16, 8, 1};
constexpr int max_num_dynamic_n_tails
        = sizeof(dynamic_n_tails) / sizeof(dynamic_n_tails[0]);
constexpr int max_num_brg_kernels_matmul = 2 * 2 * 2
        * (max_num_dynamic_n_tails + 1 /* main kernel size */)
        * (max_num_dynamic_m_tails + 1 /* main kernel size */);

template <cpu_isa_t isa>
struct brgemm_matmul_t : public primitive_t {
    struct pd_t : public ::dnnl::impl::cpu::matmul::cpu_matmul_pd_t {
        using ::dnnl::impl::cpu::matmul::cpu_matmul_pd_t::cpu_matmul_pd_t;

        DECLARE_COMMON_PD_T(
                JIT_IMPL_NAME_HELPER("brg_matmul:", isa, ""), brgemm_matmul_t);

        status_t init(engine_t *engine);
        int get_brg_kernel_idx(bool is_bs_tail, bool do_initialization,
                int m_ker_idx, int n_ker_idx, bool is_K_tail) const;
        const brgemm_desc_t &get_brg_desc(int idx) const {
            return brg_descs_[idx];
        }
        const brgemm_matmul_conf_t &get_brgemm_matmul_conf() const {
            return bgmmc_;
        }

        void maybe_set_LDB2();

    private:
        brgemm_desc_t brg_descs_[max_num_brg_kernels_matmul];
        brgemm_matmul_conf_t bgmmc_;
    };

    brgemm_matmul_t(const pd_t *apd) : primitive_t(apd) {}

    status_t init(engine_t *engine) override;
    static constexpr data_type_t acc_type = data_type::s32;

    status_t execute(const exec_ctx_t &ctx) const override {
        return execute_body(ctx);
    }

private:
    struct brg_matmul_exec_ctx_t;

    const pd_t *pd() const { return (const pd_t *)primitive_t::pd().get(); }
    status_t execute_body(const exec_ctx_t &ctx) const;
    void compute_kernel(const brg_matmul_exec_ctx_t &brgmm_ctx,
            const char *A_data_batch_ptr, const char *B_data_batch_ptr,
            int ithr, int b_idx, int m_blk_idx, int n_blk_idx, int k_blk_idx,
            bool do_init, int &prev_ker_idx) const;
    void copy_a_chunk_in_buffer(const brg_matmul_exec_ctx_t &brgmm_ctx,
            const char *A_data_batch_ptr, int ithr, int m_blk_idx,
            int k_blk_idx) const;
    void copy_b_chunk_in_buffer(const brg_matmul_exec_ctx_t &brgmm_ctx,
            const char *B_data_batch_ptr, int ithr, int b_idx, int n_blk_idx,
            int k_blk_idx) const;
    void maybe_reduce_partial_results_and_apply_postops(
            const brg_matmul_exec_ctx_t &brgmm_ctx) const;
    void maybe_reduce_A(const brg_matmul_exec_ctx_t &brgmm_ctx, int ithr,
            int gemm_batch, int m_blk_idx, int n_blk_idx, int k_chunk_idx,
            bool do_init, bool has_K_tail, bool do_K_tail) const;
    void maybe_reduce_and_convert_partial_results_A(
            const brg_matmul_exec_ctx_t &brgmm_ctx) const;
    void accumulate(
            char *result_ptr, const char *reduce_ptr, size_t size) const;

    std::unique_ptr<brgemm_kernel_t> brg_kernels_[max_num_brg_kernels_matmul];
    brgemm_containers::brgemm_palette_container_t brgemm_palettes_ {
            max_num_brg_kernels_matmul};

    std::unique_ptr<jit_brgemm_matmul_copy_b_t> copy_B_kernel_;
    std::unique_ptr<jit_brgemm_matmul_copy_a_t> copy_A_kernel_;
    std::unique_ptr<cpu_accumulator_1d_t<data_type::f32>> acc_ker_f32_;
    std::unique_ptr<cpu_accumulator_1d_t<data_type::s32>> acc_ker_s32_;
    std::unique_ptr<jit_avx512_sparse_decompress_kernel_t>
            sparse_decompress_kernel_;
    std::unique_ptr<jit_avx512_core_scale_precompute_t> jit_scale_precompute_;

    using reducer_t = x64::jit_brgemm_kernel_diff_bias_t<
            typename cpu_isa_traits_t<isa>::Vmm>;
    std::unique_ptr<reducer_t> reducers_[2][2];
};

} // namespace matmul
} // namespace x64
} // namespace cpu
} // namespace impl
} // namespace dnnl

#endif
