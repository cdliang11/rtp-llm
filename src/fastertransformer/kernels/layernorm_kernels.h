/*
 * Copyright (c) 2019-2023, NVIDIA CORPORATION.  All rights reserved.
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
 */

#pragma once


#include "src/fastertransformer/cuda/cuda_utils.h"
#include <assert.h>
#include <cuda_fp16.h>
#include <cuda_runtime.h>

namespace fastertransformer {

enum class LayerNormType {
    pre_layernorm,
    post_layernorm,
    InvalidType
};

enum class NormType {
    layernorm,
    rmsnorm,
    alphanorm,
    InvalidType
};

inline LayerNormType getLayerNormType(std::string layernorm_type_str) {
    if (layernorm_type_str == "pre_layernorm") {
        return LayerNormType::pre_layernorm;
    } else if (layernorm_type_str == "post_layernorm") {
        return LayerNormType::post_layernorm;
    } else {
        FT_CHECK_WITH_INFO(false, "Layernorm Type: " + layernorm_type_str + " not supported !");
    }
    return LayerNormType::InvalidType;
}

inline NormType getNormType(std::string norm_type_str) {
    if (norm_type_str == "layernorm") {
        return NormType::layernorm;
    } else if (norm_type_str == "rmsnorm") {
        return NormType::rmsnorm;
    } else if (norm_type_str == "alphanorm") {
        return NormType::alphanorm;
    } else {
        FT_CHECK_WITH_INFO(false, "Norm Type: " + norm_type_str + " not supported !");
    }
    return NormType::InvalidType;
}

template<typename T>
struct LayerNormWeight {
    const T* gamma = nullptr;
    const T* beta  = nullptr;
};

template<typename T>
void invokeAddBiasResidualLayerNorm(T*           out,
                                    const T*     input,
                                    const T*     bias,
                                    const T*     gamma,
                                    const T*     beta,
                                    const float  layernorm_eps,
                                    const int    m,
                                    const int    n,
                                    cudaStream_t stream);

template<typename T>
void invokeGeneralAddBiasResidualPreLayerNorm(T*           output,
                                              T*           norm_output,
                                              const T*     input,
                                              const T*     residual1,
                                              const T*     gamma,
                                              const T*     beta,
                                              const T*     bias,
                                              const float  layernorm_eps,
                                              int          m,
                                              int          n,
                                              const float* scale_inter,
                                              const float* scale_out,
                                              float*       scale,
                                              float*       dynamic_scale,
                                              const int    int8_mode,
                                              cudaStream_t stream,
                                              int          opt_version = 2);

template<typename T>
void invokeGeneralLayerNorm(T*           out,
                            const T*     input,
                            const T*     gamma,
                            const T*     beta,
                            const float  layernorm_eps,
                            const int    m,
                            const int    n,
                            float*       scale,
                            float*       dynamic_scale,
                            const int    int8_mode,
                            cudaStream_t stream,
                            int          opt_version = 2);

template<typename T>
void invokeGeneralLayerNorm(T*           out,
                            const T*     input,
                            const T*     gamma,
                            const T*     beta,
                            const float  layernorm_eps,
                            const int    m,
                            const int    n,
                            float*       scale,
                            const int    int8_mode,
                            cudaStream_t stream,
                            int          opt_version = 2) {
    invokeGeneralLayerNorm(
        out, input, gamma, beta, layernorm_eps, m, n, scale, (float*)nullptr, int8_mode, stream, opt_version);
}

template<typename T>
void invokeGeneralLayerNormWithPadding(T*           out,
                                       const T*     input,
                                       const T*     gamma,
                                       const T*     beta,
                                       const float  layernorm_eps,
                                       const int    m,
                                       const int    real_n,
                                       const int    padding_n,
                                       float*       scale,
                                       float*       dynamic_scale,
                                       const int    int8_mode,
                                       cudaStream_t stream,
                                       int          opt_version = 2);

}  // namespace fastertransformer
