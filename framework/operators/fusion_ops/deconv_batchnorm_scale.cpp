#include "framework/operators/fusion_ops/deconv_batchnorm_scale.h"

namespace anakin {

namespace ops {

#define INSTANCE_DECONVBATCHNORMSCALE(Ttype, Ptype) \
template<> \
void DeconvBatchnormScale<Ttype, Ptype>::operator()(\
    OpContext<Ttype>& ctx,\
    const std::vector<Tensor4dPtr<Ttype> >& ins,\
    std::vector<Tensor4dPtr<Ttype> >& outs) {\
    auto* impl =\
        static_cast<DeconvBatchnormScaleHelper<Ttype, Ptype>*>(this->_helper);\
    auto& param = impl->_param_deconv_batchnorm_scale;\
    impl->_funcs_deconv_batchnorm_scale(ins, outs, param, ctx);\
}

/// TODO ... specialization other type of operator

/// set helper
template<typename Ttype, Precision Ptype>
DeconvBatchnormScaleHelper<Ttype, Ptype>::~DeconvBatchnormScaleHelper() {
}

template<typename Ttype, Precision Ptype>
Status DeconvBatchnormScaleHelper<Ttype, Ptype>::InitParam() {
    DLOG(WARNING) << "Parsing DeconvBatchnormScale op parameter.";

    // get conv param
    auto group = GET_PARAMETER(int, group);
    auto bias_term = GET_PARAMETER(bool, bias_term);
    auto padding = GET_PARAMETER(PTuple<int>, padding);
    auto strides = GET_PARAMETER(PTuple<int>, strides);
    auto dilation_rate = GET_PARAMETER(PTuple<int>, dilation_rate);
    auto filter_num = GET_PARAMETER(int, filter_num);
    auto kernel_size = GET_PARAMETER(PTuple<int>, kernel_size);
    auto axis = GET_PARAMETER(int, axis);

	using pblock_type = PBlock<Ttype>;
    auto weights = GET_PARAMETER(pblock_type, weight_1);
    auto weights_shape = weights.shape();
    auto weights_dtype = weights.h_tensor().get_dtype();
    // resize weights scale
    auto& w = weights.h_tensor();
    if (w.get_scale().size() == 1){
        float scale_tmp = w.get_scale()[0];
        std::vector<float> w_scale(filter_num, scale_tmp);
        w.set_scale(w_scale);
    }

    // get batchnorm param
    auto epsilon = GET_PARAMETER(float, batchnorm_0_epsilon);
    auto momentum = GET_PARAMETER(float, batchnorm_0_momentum);
    auto batch_norm_weight_1 = GET_PARAMETER(pblock_type, batchnorm_0_weight_1);
    auto batch_norm_weight_1_vector = batch_norm_weight_1.vector();
    auto batch_norm_weight_2 = GET_PARAMETER(pblock_type, batchnorm_0_weight_2);
    auto batch_norm_weight_2_vector = batch_norm_weight_2.vector();
    auto batch_norm_weight_3 = GET_PARAMETER(pblock_type, batchnorm_0_weight_3);
    auto batch_norm_weight_3_vector = batch_norm_weight_3.vector();

    // get scale param
    auto scale_num_axes = GET_PARAMETER(int, scale_0_num_axes);
    auto scale_bias_term = GET_PARAMETER(bool, scale_0_bias_term);
    auto scale_axis = GET_PARAMETER(int, scale_0_axis);
    auto scale_weight_1 = GET_PARAMETER(pblock_type, scale_0_weight_1);
    auto scale_weight_1_vector = scale_weight_1.vector();
    auto scale_weight_2 = GET_PARAMETER(pblock_type, scale_0_weight_2);
    auto scale_weight_2_vector = scale_weight_2.vector();

    if(bias_term) {
        auto bias = GET_PARAMETER(pblock_type, weight_2);
        if (weights_dtype == AK_FLOAT) {
            graph::GraphGlobalMem<Ttype>::Global().template apply<Level_0>(
                    WeightsFusion<float, Ttype>::update_deconv_weights,
                    weights, bias,
                    weights_shape[0], weights_shape[1], weights_shape[2], weights_shape[3],
                    true,
                    batch_norm_weight_3_vector[0], epsilon,
                    batch_norm_weight_1_vector,
                    batch_norm_weight_2_vector,
                    scale_weight_1_vector,
                    scale_weight_2_vector,
                    scale_bias_term);
        }else {
            graph::GraphGlobalMem<Ttype>::Global().template apply<Level_0>(
                    WeightsFusion<char, Ttype>::update_deconv_weights,
                    weights, bias,
                    weights_shape[0], weights_shape[1], weights_shape[2], weights_shape[3],
                    true,
                    batch_norm_weight_3_vector[0], epsilon,
                    batch_norm_weight_1_vector,
                    batch_norm_weight_2_vector,
                    scale_weight_1_vector,
                    scale_weight_2_vector,
                    scale_bias_term);
        }
        saber::ConvParam<Ttype> conv_param(group, padding[0], padding[1],
                                           strides[0], strides[1],
                                           dilation_rate[0], dilation_rate[1],
                                           &(weights.d_tensor()), &(bias.d_tensor()));
        _param_deconv_batchnorm_scale = conv_param;
    } else {
        pblock_type* bias = new pblock_type();
        SET_PARAMETER(bias_term, true, bool); // set attr bias_term true
        SET_PARAMETER(weight_2, *bias, pblock_type); // gen new bias
        if (weights_dtype == AK_FLOAT) {
            graph::GraphGlobalMem<Ttype>::Global().template apply<Level_0>(
                    WeightsFusion<float, Ttype>::update_deconv_weights,
                    weights, *bias,
                    weights_shape[0], weights_shape[1], weights_shape[2], weights_shape[3],
                    false,
                    batch_norm_weight_3_vector[0], epsilon,
                    batch_norm_weight_1_vector,
                    batch_norm_weight_2_vector,
                    scale_weight_1_vector,
                    scale_weight_2_vector,
                    scale_bias_term);
        } else{
            graph::GraphGlobalMem<Ttype>::Global().template apply<Level_0>(
                    WeightsFusion<char, Ttype>::update_deconv_weights,
                    weights, *bias,
                    weights_shape[0], weights_shape[1], weights_shape[2], weights_shape[3],
                    false,
                    batch_norm_weight_3_vector[0], epsilon,
                    batch_norm_weight_1_vector,
                    batch_norm_weight_2_vector,
                    scale_weight_1_vector,
                    scale_weight_2_vector,
                    scale_bias_term);
        }
        saber::ConvParam<Ttype> conv_param(group, padding[0], padding[1],
                                           strides[0], strides[1],
                                           dilation_rate[0], dilation_rate[1],
                                           &(weights.d_tensor()), &(bias->d_tensor()));
        _param_deconv_batchnorm_scale = conv_param;
    }

    return Status::OK();
}

template<typename Ttype, Precision Ptype>
Status DeconvBatchnormScaleHelper<Ttype, Ptype>::Init(OpContext<Ttype>& ctx,
        const std::vector<Tensor4dPtr<Ttype> >& ins,
        std::vector<Tensor4dPtr<Ttype> >& outs) {
    if (_param_deconv_batchnorm_scale.group == ins[0]->channel() && \
            _param_deconv_batchnorm_scale.group == outs[0]->channel()) {
        _funcs_deconv_batchnorm_scale.init(ins, outs, _param_deconv_batchnorm_scale, SPECIFY,
                                              SABER_IMPL, ctx);
    } else {
        _funcs_deconv_batchnorm_scale.init(ins, outs, _param_deconv_batchnorm_scale, SPECIFY,
                                              SABER_IMPL, ctx);
    }

    //_funcs_deconv_batchnorm_scale.init(ins, outs, _param_deconv_batchnorm_scale, SPECIFY, VENDER_IMPL, ctx);
    return Status::OK();
}

template<typename Ttype, Precision Ptype>
Status DeconvBatchnormScaleHelper<Ttype, Ptype>::InferShape(const
        std::vector<Tensor4dPtr<Ttype> >& ins,
        std::vector<Tensor4dPtr<Ttype> >& outs) {
    _funcs_deconv_batchnorm_scale.compute_output_shape(ins, outs, _param_deconv_batchnorm_scale);
    return Status::OK();
}

#ifdef USE_CUDA
template class DeconvBatchnormScaleHelper<NV, Precision::FP32>;
template class DeconvBatchnormScaleHelper<NV, Precision::FP16>;
template class DeconvBatchnormScaleHelper<NV, Precision::INT8>;
#endif

#ifdef USE_ARM_PLACE
template class DeconvBatchnormScaleHelper<ARM, Precision::FP32>;
template class DeconvBatchnormScaleHelper<ARM, Precision::FP16>;
template class DeconvBatchnormScaleHelper<ARM, Precision::INT8>;
#endif

#if defined USE_X86_PLACE || defined BUILD_LITE
template class DeconvBatchnormScaleHelper<X86, Precision::FP32>;
template class DeconvBatchnormScaleHelper<X86, Precision::FP16>;
template class DeconvBatchnormScaleHelper<X86, Precision::INT8>;
#endif

#if defined USE_X86_PLACE || defined BUILD_LITE
INSTANCE_DECONVBATCHNORMSCALE(X86, Precision::FP32);
ANAKIN_REGISTER_OP_HELPER(DeconvBatchnormScale, DeconvBatchnormScaleHelper, X86, Precision::FP32);
#endif

// register helper
#ifdef USE_CUDA
INSTANCE_DECONVBATCHNORMSCALE(NV, Precision::FP32);
ANAKIN_REGISTER_OP_HELPER(DeconvBatchnormScale, DeconvBatchnormScaleHelper, NV, Precision::FP32);
#endif

#ifdef USE_ARM_PLACE
INSTANCE_DECONVBATCHNORMSCALE(ARM, Precision::FP32);
ANAKIN_REGISTER_OP_HELPER(DeconvBatchnormScale, DeconvBatchnormScaleHelper, ARM, Precision::FP32);
#endif

//! register op
ANAKIN_REGISTER_OP(DeconvBatchnormScale)
.Doc("DeconvBatchnormScale fusion operator")
#ifdef USE_CUDA
.__alias__<NV, Precision::FP32>("convolution_batchnorm_scale_")
#endif
#ifdef USE_ARM_PLACE
.__alias__<ARM, Precision::FP32>("deconvolution_batchnorm_scale_")
#endif
#if defined USE_X86_PLACE || defined BUILD_LITE
.__alias__<X86, Precision::FP32>("deconvolution_batchnorm_scale_")
#endif
.num_in(1)
.num_out(1)
.Args<int>("group", " group of conv ")
.Args<bool>("bias_term", " whether conv weights have bias")
.Args<PTuple<int>>("padding", "padding of conv (x, y)")
.Args<PTuple<int>>("strides", "strides of conv (x)")
.Args<PTuple<int>>("dilation_rate", "dilation rate of conv (x)")
.Args<int>("filter_num", "filter(kernel) number of weights")
.Args<PTuple<int>>("kernel_size", "kernel size of kernel (x, y)")
.Args<int>("axis", "axis of conv")
.Args<int>("scale_0_num_axes", " num axes for scale")
.Args<bool>("scale_0_bias_term", "whether scale has bias")
.Args<int>("scale_0_axis", "axis for scale")
.Args<float>("batchnorm_0_epsilon", "epsilon for batchnorm")
.Args<float>("batchnorm_0_momentum", "momentum for batchnorm");

} /* namespace ops */

} /* namespace anakin */


