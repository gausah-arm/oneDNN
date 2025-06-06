Inner Product {#dev_guide_inner_product}
========================================

>
> [API Reference](@ref dnnl_api_inner_product)
>

## General

The inner product primitive (sometimes called fully connected) treats each
activation in the minibatch as a vector and computes its product with a
weights 2D tensor producing a 2D tensor as an output.

### Forward

More precisely, let \src, \weights, \bias and \dst be \f$N \times IC\f$,
\f$OC \times IC\f$, \f$OC\f$, and \f$N \times OC\f$ tensors, respectively
(variable names follow the standard @ref dev_guide_conventions). Then:

\f[\dst(n, oc) = \bias(oc) + \sum_{ic=0}^{IC-1} \src(n, ic) \cdot \weights(oc, ic)\f]

In cases where the \src and \weights tensors have spatial dimensions, they are
flattened to 2D. For example, if they are 4D
\f$N \times IC' \times IH \times IW\f$ and
\f$OC \times IC' \times KH \times KW\f$ tensors, then the formula above is
applied with \f$IC = IC' \cdot IH \cdot IW\f$. In such cases, the \src and
\weights tensors must have equal spatial dimensions (e.g. \f$KH = IH\f$ and
\f$KW = IW\f$ for 4D tensors).

#### Difference Between Forward Training and Forward Inference

There is no difference between the @ref dnnl::prop_kind::forward_training
and @ref dnnl::prop_kind::forward_inference propagation kinds.

### Backward

The backward propagation computes \diffsrc based on \diffdst and
\weights.

The weights update computes \diffweights and \diffbias based on
\diffdst and \src.

@note The *optimized* memory formats \src and \weights might be
different on forward propagation, backward propagation, and weights
update.

## Execution Arguments

When executed, the inputs and outputs should be mapped to an execution
argument index as specified by the following table.

| Primitive input/output      | Execution argument index                                                   |
|-----------------------------|----------------------------------------------------------------------------|
| \src                        | DNNL_ARG_SRC                                                               |
| \weights                    | DNNL_ARG_WEIGHTS                                                           |
| \bias                       | DNNL_ARG_BIAS                                                              |
| \dst                        | DNNL_ARG_DST                                                               |
| \diffsrc                    | DNNL_ARG_DIFF_SRC                                                          |
| \diffweights                | DNNL_ARG_DIFF_WEIGHTS                                                      |
| \diffbias                   | DNNL_ARG_DIFF_BIAS                                                         |
| \diffdst                    | DNNL_ARG_DIFF_DST                                                          |
| \f$\text{binary post-op}\f$ | DNNL_ARG_ATTR_MULTIPLE_POST_OP(binary_post_op_position) \| DNNL_ARG_SRC_1, |
|                             | DNNL_ARG_ATTR_MULTIPLE_POST_OP(binary_post_op_position) \| DNNL_ARG_SRC_2  |
| \f$\text{prelu post-op}\f$  | DNNL_ARG_ATTR_MULTIPLE_POST_OP(prelu_post_op_position) \| DNNL_ARG_WEIGHTS |

## Implementation Details

### General Notes

N/A.

### Data Types

Inner product primitive supports the following combination of data types for
source, destination, weights, and bias:

| Propagation        | Source    | Weights   | Destination            | Bias                   |
|:-------------------|:----------|:----------|:-----------------------|:-----------------------|
| forward / backward | f32       | f32       | f32                    | f32                    |
| forward            | f16       | f16       | f32, f16, u8, s8       | f16, f32               |
| forward            | u8, s8    | s8        | u8, s8, s32, bf16, f32 | u8, s8, s32, bf16, f32 |
| forward            | bf16      | bf16      | f32, bf16              | f32, bf16              |
| backward           | f32, bf16 | bf16      | bf16                   |                        |
| backward           | f32, f16  | f16       | f16                    |                        |
| weights update     | bf16      | f32, bf16 | bf16                   | f32, bf16              |
| weights update     | f16       | f32, f16  | f16                    | f32, f16               |

### Data Representation

Like other CNN primitives, the inner product primitive expects the following
tensors:

| Spatial | Source                                      | Destination      | Weights                                          |
|:--------|:--------------------------------------------|:-----------------|:-------------------------------------------------|
| 1D      | \f$N \times C \times W\f$                   | \f$N \times C\f$ | \f$OC \times IC \times KW\f$                     |
| 2D      | \f$N \times C \times H \times W\f$          | \f$N \times C\f$ | \f$OC \times IC \times KH \times KW\f$           |
| 3D      | \f$N \times C \times D \times H \times W\f$ | \f$N \times C\f$ | \f$OC \times IC \times KD \times KH \times KW\f$ |

Memory format of data and weights memory objects is critical for inner
product primitive performance. In the oneDNN programming model, inner
product primitive is one of the few primitives that support the placeholder
format #dnnl::memory::format_tag::any (shortened to `any` from
now on) and can define data and weight memory objects formats based on the
primitive parameters. When using `any` it is necessary to first create an
inner product primitive descriptor and then query it for the actual data and
weight memory objects formats.

The table below shows the combinations of memory formats the
inner product primitive is optimized for. For the destination tensor (which is
always \f$N \times C\f$) the memory format is always
#dnnl::memory::format_tag::nc (#dnnl::memory::format_tag::ab).

| Source / Destination                         | Weights                                      | Limitations |
|:---------------------------------------------|:---------------------------------------------|:------------|
| `any`                                        | `any`                                        | N/A         |
| #dnnl_nc, #dnnl_nwc, #dnnl_nhwc, #dnnl_ndhwc | `any`                                        | N/A         |
| #dnnl_nc, #dnnl_ncw, #dnnl_nchw, #dnnl_ncdhw | `any`                                        | N/A         |
| #dnnl_nc, #dnnl_nwc, #dnnl_nhwc, #dnnl_ndhwc | #dnnl_io, #dnnl_wio, #dnnl_hwio, #dnnl_dhwio | N/A         |
| #dnnl_nc, #dnnl_ncw, #dnnl_nchw, #dnnl_ncdhw | #dnnl_oi, #dnnl_oiw, #dnnl_oihw, #dnnl_oidhw | N/A         |

### Post-Ops and Attributes

Post-ops and attributes enable you to modify the behavior of the inner product
primitive by chaining certain operations after the inner product operation.
The following post-ops are supported by inner product primitives:

| Propagation | Type      | Operation                                            | Description                                                                   | Restrictions                        |
|:------------|:----------|:-----------------------------------------------------|:------------------------------------------------------------------------------|:------------------------------------|
| forward     | attribute | [Scales](@ref dnnl::primitive_attr::set_scales_mask) | Scales the result of inner product by given scale factor(s)                   | int8 inner products only            |
| forward     | post-op   | [Eltwise](@ref dnnl::post_ops::append_eltwise)       | Applies an @ref dnnl_api_eltwise operation to the result                      |                                     |
| forward     | post-op   | [Sum](@ref dnnl::post_ops::append_sum)               | Adds the operation result to the destination tensor instead of overwriting it |                                     |
| forward     | post-op   | [Binary](@ref dnnl::post_ops::append_binary)         | Applies a @ref dnnl_api_binary operation to the result                        | General binary post-op restrictions |
| forward     | post-op   | [Prelu](@ref dnnl::post_ops::append_prelu)           | Applies an @ref dnnl_api_prelu operation to the result                        |                                     |

The following masks are supported by the primitive:
- 0, which applies one scale value to an entire tensor, and
- 1, which applies a scale value per output channel for
  `DNNL_ARG_WEIGHTS` argument.

When scales masks are specified, the user must provide the
corresponding scales as additional input memory objects with argument
`DNNL_ARG_ATTR_SCALES | DNNL_ARG_${MEMORY_INDEX}` during the execution
stage.


## Implementation Limitations

1. Check @ref dev_guide_data_types.

2. The CPU engine does not support `u8` or `s8` data type for `dst` with `f16` `src` and `weights`.

## Performance Tips

- Use #dnnl::memory::format_tag::any for source, weights,
  and destinations memory format tags when create an inner product primitive
  to allow the library to choose the most appropriate memory format.

## Example

[Inner Product Primitive Example](@ref inner_product_example_cpp)

@copydetails inner_product_example_cpp_short
