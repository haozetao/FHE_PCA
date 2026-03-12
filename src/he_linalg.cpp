#include "he_linalg.h"
#include <cmath>
#include <iostream>

Ciphertext<DCRTPoly> HE_MatVecMul(
    const HEContext &ctx,
    const std::vector<Ciphertext<DCRTPoly>> &encDiags,
    const Ciphertext<DCRTPoly> &encVec) {
    // Halevi-Shoup diagonal method:
    //   result = sum_{k=0}^{d-1} encDiags[k] * Rotate(encVec, -k)
    // Multiplicative depth: 1 (all d multiplications are parallel).
    uint32_t d = ctx.dim;

    // k=0: no rotation needed
    auto result = ctx.cc->EvalMult(encDiags[0], encVec);

    for (uint32_t k = 1; k < d; k++) {
        // Rotate input vector by -k (which is d-k in positive terms).
        // OpenFHE uses positive rotation index for left-cyclic shift.
        auto rotated = ctx.cc->EvalRotate(encVec, static_cast<int32_t>(k));
        auto term    = ctx.cc->EvalMult(encDiags[k], rotated);
        result       = ctx.cc->EvalAdd(result, term);
    }

    return result;
}

Ciphertext<DCRTPoly> HE_InnerProduct(
    const HEContext &ctx,
    const Ciphertext<DCRTPoly> &a,
    const Ciphertext<DCRTPoly> &b) {
    // Element-wise multiply, then rotate-and-sum to accumulate all slots.
    // Both a and b must be replicated with period dim across all batchSize slots.
    // With replicated data, each d-consecutive-slot window covers exactly one
    // full period, so rotate-and-sum naturally produces the correct inner product
    // in EVERY slot — no masking needed (and masking would break it).
    auto product = ctx.cc->EvalMult(a, b);

    uint32_t shift = 1;
    while (shift < ctx.dim) {
        auto rotated = ctx.cc->EvalRotate(product, static_cast<int32_t>(shift));
        product      = ctx.cc->EvalAdd(product, rotated);
        shift *= 2;
    }
    return product;
}

Ciphertext<DCRTPoly> HE_InvSqrt(
    const HEContext &ctx,
    const Ciphertext<DCRTPoly> &x,
    double initGuess,
    int numIter) {
    // Newton iteration for f(y) = 1/sqrt(x):
    //   y_{n+1} = y_n * (3 - x * y_n^2) / 2
    //
    // Each iteration costs 2 multiplicative levels
    // (one for y_n^2, one for x * y_n^2 and the final y_n * (...)).

    // Start with a plaintext initial guess replicated in all slots.
    std::vector<double> guessVec(ctx.batchSize, initGuess);
    auto ptGuess = ctx.cc->MakeCKKSPackedPlaintext(guessVec);
    auto y       = ctx.cc->Encrypt(ctx.keys.publicKey, ptGuess);

    std::vector<double> threeVec(ctx.batchSize, 3.0);
    auto ptThree = ctx.cc->MakeCKKSPackedPlaintext(threeVec);

    for (int i = 0; i < numIter; i++) {
        // y^2
        auto y2 = ctx.cc->EvalMult(y, y);
        // x * y^2
        auto xy2 = ctx.cc->EvalMult(x, y2);
        // 3 - x * y^2
        auto diff = ctx.cc->EvalSub(ptThree, xy2);
        // y * (3 - x * y^2)
        auto num = ctx.cc->EvalMult(y, diff);
        // Divide by 2 (multiply by 0.5 — cheap plaintext-ciphertext op)
        y = ctx.cc->EvalMult(num, 0.5);
    }

    return y;
}

Ciphertext<DCRTPoly> HE_ScalarMul(
    const HEContext &ctx,
    const Ciphertext<DCRTPoly> &ct,
    double scalar) {
    return ctx.cc->EvalMult(ct, scalar);
}
