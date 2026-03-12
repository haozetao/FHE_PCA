#pragma once

#include "he_context.h"
#include <vector>

struct PowerIterConfig {
    int numIter        = 20;
    int bootstrapEvery = 5;
    double estNorm     = 1.0;
    double invSqrtInit = 1.0;
    int invSqrtIter    = 4;
};

// Encrypted power iteration: compute the top eigenvector of the covariance
// matrix whose diagonals are packed in `encCovDiags`.
// Returns the encrypted (normalized) eigenvector.
Ciphertext<DCRTPoly> EncryptedPowerIteration(
    const HEContext &ctx,
    const std::vector<Ciphertext<DCRTPoly>> &encCovDiags,
    Ciphertext<DCRTPoly> encV,
    const PowerIterConfig &cfg);

// Compute the approximate eigenvalue lambda = v^T C v (encrypted).
Ciphertext<DCRTPoly> EncryptedRayleighQuotient(
    const HEContext &ctx,
    const std::vector<Ciphertext<DCRTPoly>> &encCovDiags,
    const Ciphertext<DCRTPoly> &encV);
