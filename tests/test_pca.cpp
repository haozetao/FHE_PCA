#include "../src/he_context.h"
#include "../src/packing.h"
#include "../src/pca.h"
#include "../src/utils.h"
#include <iostream>
#include <cmath>
#include <cassert>

void TestPlaintextPCA() {
    std::cout << "=== TestPlaintextPCA ===\n";
    const int dim = 8;
    const int numSamples = 200;
    const int numComp = 3;

    auto data = GenerateSyntheticData(numSamples, dim);
    auto centered = CenterData(data);
    auto cov = ComputeCovarianceMatrix(centered);

    auto result = PlaintextPCA(cov, numComp);

    std::cout << "  Eigenvalues: ";
    for (int i = 0; i < numComp; i++)
        std::cout << result.eigenvalues[i] << " ";
    std::cout << "\n";

    // Check eigenvalues are in decreasing order.
    for (int i = 0; i + 1 < numComp; i++) {
        assert(result.eigenvalues[i] >= result.eigenvalues[i + 1] - 1e-6);
    }

    // Check eigenvectors are approximately orthonormal.
    for (int i = 0; i < numComp; i++) {
        double norm = VecNorm(result.eigenvectors[i]);
        std::cout << "  ||v" << i << "|| = " << norm << "\n";
        assert(std::abs(norm - 1.0) < 1e-6);
    }
    for (int i = 0; i < numComp; i++) {
        for (int j = i + 1; j < numComp; j++) {
            double dot = std::abs(DotProduct(result.eigenvectors[i],
                                             result.eigenvectors[j]));
            std::cout << "  |<v" << i << ", v" << j << ">| = " << dot << "\n";
            assert(dot < 1e-3);
        }
    }
    std::cout << "PASSED\n\n";
}

void TestEncryptedPCA() {
    std::cout << "=== TestEncryptedPCA ===\n";
    const int dim = 8;
    const int numSamples = 200;
    const int numComp = 2;

    auto data = GenerateSyntheticData(numSamples, dim);
    auto centered = CenterData(data);
    auto cov = ComputeCovarianceMatrix(centered);

    // Plaintext reference.
    auto plainResult = PlaintextPCA(cov, numComp);

    // Encrypted PCA.
    auto ctx = InitCKKSContext(dim, 32, 50);

    PCAConfig pcaCfg;
    pcaCfg.numComponents       = numComp;
    // Keep the encrypted circuit shallow enough for the current CKKS depth.
    pcaCfg.pic.numIter         = 6;
    pcaCfg.pic.bootstrapEvery  = 0;
    pcaCfg.pic.invSqrtIter     = 3;

    auto encResult = EncryptedPCA(ctx, cov, pcaCfg);

    std::cout << "\n--- Comparison ---\n";
    for (int i = 0; i < numComp; i++) {
        auto decVec = UnpackVector(ctx, encResult.encEigenvectors[i], dim);

        double cosSim = CosineSimilarity(decVec, plainResult.eigenvectors[i]);
        double lambda = encResult.eigenvalues[i];
        double lambdaRelErr = std::abs(lambda - plainResult.eigenvalues[i])
                            / (std::abs(plainResult.eigenvalues[i]) + 1e-15);

        std::cout << "  Component " << (i + 1) << ":\n";
        std::cout << "    cosine similarity = " << cosSim << "\n";
        std::cout << "    eigenvalue enc=" << lambda
                  << " plain=" << plainResult.eigenvalues[i]
                  << " rel_err=" << lambdaRelErr << "\n";

        assert(cosSim > 0.8);
        assert(lambdaRelErr < 0.5);
    }
    std::cout << "PASSED\n\n";
}

int main() {
    std::cout << "========== PCA Tests ==========\n\n";
    TestPlaintextPCA();
    TestEncryptedPCA();
    std::cout << "All PCA tests passed!\n";
    return 0;
}
