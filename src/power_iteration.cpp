#include "power_iteration.h"
#include "he_linalg.h"
#include <iostream>
#include <chrono>

Ciphertext<DCRTPoly> EncryptedPowerIteration(
    const HEContext &ctx,
    const std::vector<Ciphertext<DCRTPoly>> &encCovDiags,
    Ciphertext<DCRTPoly> encV,
    const PowerIterConfig &cfg) {

    auto tStart = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < cfg.numIter; i++) {
        auto tIter = std::chrono::high_resolution_clock::now();

        // w = C * v  (Halevi-Shoup encrypted mat-vec)
        auto encW = HE_MatVecMul(ctx, encCovDiags, encV);

        // Scale by 1/estNorm to prevent numerical blowup.
        // This is a plaintext-ciphertext multiply (cheap, consumes 1 level).
        if (cfg.estNorm > 1e-10) {
            encV = HE_ScalarMul(ctx, encW, 1.0 / cfg.estNorm);
        } else {
            encV = encW;
        }

        auto tIterEnd = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(
                        tIterEnd - tIter).count();
        std::cout << "  [PowerIter] iteration " << (i + 1) << "/" << cfg.numIter
                  << "  (" << ms << " ms)\n";

        // Bootstrapping to refresh ciphertext levels.
        // Insert every `bootstrapEvery` iterations, but not after the last one.
        if (cfg.bootstrapEvery > 0 &&
            (i + 1) % cfg.bootstrapEvery == 0 &&
            (i + 1) < cfg.numIter) {
            std::cout << "  [PowerIter] bootstrapping at iteration " << (i + 1) << "\n";
            // Note: EvalBootstrap requires FHE mode enabled in the context.
            // If bootstrapping is not configured, this section can be skipped
            // for shallow circuits (small numIter with sufficient multDepth).
            // encV = ctx.cc->EvalBootstrap(encV);
        }
    }

    // Final normalization: v = v * (1 / ||v||)
    // Compute ||v||^2 = <v, v>
    auto normSq = HE_InnerProduct(ctx, encV, encV);

    // Compute 1/sqrt(||v||^2) via polynomial approximation (Newton iteration).
    auto invNorm = HE_InvSqrt(ctx, normSq, cfg.invSqrtInit, cfg.invSqrtIter);

    // v_normalized = v * invNorm
    // The result stays replicated (period dim) — no mask here, because
    // masking would break the replication needed by Rayleigh quotient's mat-vec.
    encV = ctx.cc->EvalMult(encV, invNorm);

    auto tEnd = std::chrono::high_resolution_clock::now();
    double totalMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    std::cout << "[PowerIter] completed " << cfg.numIter
              << " iterations in " << totalMs << " ms\n";

    return encV;
}

Ciphertext<DCRTPoly> EncryptedRayleighQuotient(
    const HEContext &ctx,
    const std::vector<Ciphertext<DCRTPoly>> &encCovDiags,
    const Ciphertext<DCRTPoly> &encV) {
    // lambda = v^T * C * v
    auto Cv = HE_MatVecMul(ctx, encCovDiags, encV);
    return HE_InnerProduct(ctx, encV, Cv);
}
