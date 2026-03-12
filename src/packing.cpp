#include "packing.h"
#include <cassert>
#include <iostream>

std::vector<Ciphertext<DCRTPoly>> PackMatrixDiagonal(
    const HEContext &ctx,
    const std::vector<std::vector<double>> &matrix) {

    uint32_t d = ctx.dim;
    assert(matrix.size() == d && matrix[0].size() == d);

    std::vector<Ciphertext<DCRTPoly>> encDiags;
    encDiags.reserve(d);

    for (uint32_t k = 0; k < d; k++) {
        // Build diagonal k, replicated with period d across all slots.
        // Slot j stores matrix[j % d][(k + j % d) % d].
        // Replication ensures cyclic rotation correctness when batchSize > d.
        std::vector<double> diag(ctx.batchSize, 0.0);
        for (uint32_t j = 0; j < ctx.batchSize; j++) {
            uint32_t jj = j % d;
            diag[j] = matrix[jj][(k + jj) % d];
        }
        auto pt = ctx.cc->MakeCKKSPackedPlaintext(diag);
        encDiags.push_back(ctx.cc->Encrypt(ctx.keys.publicKey, pt));
    }

    std::cout << "[Packing] Packed " << d << "x" << d
              << " matrix into " << d << " diagonal ciphertexts\n";
    return encDiags;
}

Ciphertext<DCRTPoly> PackVector(
    const HEContext &ctx,
    const std::vector<double> &vec) {

    // Replicate the vector with period d to fill all slots.
    // This ensures EvalRotate wraps correctly for Halevi-Shoup mat-vec.
    std::vector<double> padded(ctx.batchSize, 0.0);
    for (size_t i = 0; i < ctx.batchSize; i++)
        padded[i] = vec[i % vec.size()];

    auto pt = ctx.cc->MakeCKKSPackedPlaintext(padded);
    return ctx.cc->Encrypt(ctx.keys.publicKey, pt);
}

std::vector<double> UnpackVector(
    const HEContext &ctx,
    const Ciphertext<DCRTPoly> &ct,
    size_t len) {

    Plaintext pt;
    ctx.cc->Decrypt(ctx.keys.secretKey, ct, &pt);
    pt->SetLength(len);
    auto vals = pt->GetRealPackedValue();
    vals.resize(len);
    return vals;
}
