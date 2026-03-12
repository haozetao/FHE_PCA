#include "he_context.h"
#include <iostream>
#include <set>

HEContext InitCKKSContext(uint32_t dim, uint32_t multDepth,
                          uint32_t scalingModSize) {
    CCParams<CryptoContextCKKSRNS> params;
    params.SetMultiplicativeDepth(multDepth);
    params.SetScalingModSize(scalingModSize);
    params.SetScalingTechnique(FLEXIBLEAUTO);
    params.SetSecurityLevel(HEStd_128_classic);
    // Deeper CKKS circuits need a larger ring dimension to satisfy
    // OpenFHE's parameter generation under 128-bit security.
    uint32_t ringDim = (multDepth > 26) ? (1 << 17) : (1 << 16);
    params.SetRingDim(ringDim);
    params.SetBatchSize(ringDim >> 1);

    auto cc = GenCryptoContext(params);
    cc->Enable(PKE);
    cc->Enable(KEYSWITCH);
    cc->Enable(LEVELEDSHE);
    cc->Enable(ADVANCEDSHE);

    auto keys = cc->KeyGen();
    cc->EvalMultKeyGen(keys.secretKey);

    // Collect all rotation indices needed:
    //   - [1..dim-1] for Halevi-Shoup diagonal mat-vec multiply
    //   - power-of-2 shifts for rotate-and-sum (inner product)
    std::set<int32_t> rotSet;
    for (uint32_t i = 1; i < dim; i++)
        rotSet.insert(static_cast<int32_t>(i));
    for (uint32_t i = 1; i < dim; i *= 2)
        rotSet.insert(static_cast<int32_t>(i));
    // Negative rotations may be needed; OpenFHE handles them via index wrapping.

    std::vector<int32_t> rotations(rotSet.begin(), rotSet.end());
    cc->EvalRotateKeyGen(keys.secretKey, rotations);

    std::cout << "[HEContext] CKKS initialized: dim=" << dim
              << " multDepth=" << multDepth
              << " scalingModSize=" << scalingModSize
              << " ringDim=" << ringDim
              << " #rotKeys=" << rotations.size() << "\n";

    HEContext ctx;
    ctx.cc        = cc;
    ctx.keys      = keys;
    ctx.batchSize = params.GetBatchSize();
    ctx.dim       = dim;
    return ctx;
}
