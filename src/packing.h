#pragma once

#include "he_context.h"
#include <vector>

// Halevi-Shoup diagonal packing: pack a d x d matrix into d ciphertexts.
// Ciphertext k, slot j stores matrix[j][(k+j) % d].
std::vector<Ciphertext<DCRTPoly>> PackMatrixDiagonal(
    const HEContext &ctx,
    const std::vector<std::vector<double>> &matrix);

// Pack a plaintext vector into a single ciphertext.
Ciphertext<DCRTPoly> PackVector(
    const HEContext &ctx,
    const std::vector<double> &vec);

// Decrypt a ciphertext and extract the first `len` real values.
std::vector<double> UnpackVector(
    const HEContext &ctx,
    const Ciphertext<DCRTPoly> &ct,
    size_t len);
