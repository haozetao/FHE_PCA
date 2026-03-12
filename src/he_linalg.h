#pragma once

#include "he_context.h"
#include <vector>

// Homomorphic matrix-vector multiplication using Halevi-Shoup diagonal method.
// encDiags: d ciphertexts from diagonal packing of the matrix.
// encVec:   encrypted vector.
Ciphertext<DCRTPoly> HE_MatVecMul(
    const HEContext &ctx,
    const std::vector<Ciphertext<DCRTPoly>> &encDiags,
    const Ciphertext<DCRTPoly> &encVec);

// Homomorphic inner product: <a, b> replicated across all slots.
Ciphertext<DCRTPoly> HE_InnerProduct(
    const HEContext &ctx,
    const Ciphertext<DCRTPoly> &a,
    const Ciphertext<DCRTPoly> &b);

// Polynomial approximation of inverse square root 1/sqrt(x).
// Uses Newton iteration: y_{n+1} = y_n * (3 - x * y_n^2) / 2
// `initGuess` is the plaintext initial guess for the Newton method.
// `numIter` is the number of Newton iterations (typically 3-5).
Ciphertext<DCRTPoly> HE_InvSqrt(
    const HEContext &ctx,
    const Ciphertext<DCRTPoly> &x,
    double initGuess = 1.0,
    int numIter = 4);

// Homomorphic scalar-vector multiplication (plaintext scalar * ciphertext vec).
Ciphertext<DCRTPoly> HE_ScalarMul(
    const HEContext &ctx,
    const Ciphertext<DCRTPoly> &ct,
    double scalar);
