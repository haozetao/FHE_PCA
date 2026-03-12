#include "pca.h"
#include "packing.h"
#include "he_linalg.h"
#include "utils.h"
#include <iostream>
#include <cmath>
#include <numeric>
#include <chrono>

// ---------------------------------------------------------------------------
// Plaintext helpers
// ---------------------------------------------------------------------------

std::vector<std::vector<double>> CenterData(
    const std::vector<std::vector<double>> &data) {

    int n   = static_cast<int>(data.size());
    int dim = static_cast<int>(data[0].size());

    // Compute column means.
    std::vector<double> mean(dim, 0.0);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < dim; j++)
            mean[j] += data[i][j];
    for (int j = 0; j < dim; j++)
        mean[j] /= n;

    // Subtract means.
    auto centered = data;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < dim; j++)
            centered[i][j] -= mean[j];

    return centered;
}

std::vector<std::vector<double>> ComputeCovarianceMatrix(
    const std::vector<std::vector<double>> &centeredData) {

    int n   = static_cast<int>(centeredData.size());
    int dim = static_cast<int>(centeredData[0].size());

    // C = X^T X / (n - 1)
    std::vector<std::vector<double>> cov(dim, std::vector<double>(dim, 0.0));
    for (int i = 0; i < n; i++)
        for (int r = 0; r < dim; r++)
            for (int c = r; c < dim; c++)
                cov[r][c] += centeredData[i][r] * centeredData[i][c];

    double denom = (n > 1) ? (n - 1.0) : 1.0;
    for (int r = 0; r < dim; r++) {
        for (int c = r; c < dim; c++) {
            cov[r][c] /= denom;
            cov[c][r] = cov[r][c];
        }
    }
    return cov;
}

// ---------------------------------------------------------------------------
// Encrypted PCA (power iteration + deflation)
// ---------------------------------------------------------------------------

PCAResult EncryptedPCA(
    const HEContext &ctx,
    const std::vector<std::vector<double>> &covMatrix,
    const PCAConfig &cfg) {

    PCAResult result;
    int d = static_cast<int>(covMatrix.size());

    // We perform deflation in plaintext on the covariance matrix.
    // This is valid when the cloud does not need to hide the deflated matrix
    // from the data owner (semi-honest model where the matrix itself may be
    // public or can be updated by the data owner between rounds).
    auto currentCov = covMatrix;

    for (int comp = 0; comp < cfg.numComponents; comp++) {
        std::cout << "\n=== Extracting principal component "
                  << (comp + 1) << "/" << cfg.numComponents << " ===\n";

        auto tComp = std::chrono::high_resolution_clock::now();

        // Pack the (possibly deflated) covariance matrix.
        auto encCovDiags = PackMatrixDiagonal(ctx, currentCov);

        // Create encrypted random initial vector.
        auto v0 = RandomUnitVector(d);
        auto encV = PackVector(ctx, v0);

        // Estimate spectral norm for scaling.
        PowerIterConfig pic = cfg.pic;
        pic.estNorm = EstimateSpectralNorm(currentCov);
        if (pic.estNorm < 1e-10) pic.estNorm = 1.0;

        // Choose initial guess for InvSqrt Newton iteration.
        // After power iteration, ||v||^2 should be close to some value
        // that depends on the scaling. A reasonable guess: 1/sqrt(1) = 1.
        pic.invSqrtInit = 1.0;

        std::cout << "[PCA] spectral norm estimate: " << pic.estNorm << "\n";

        // Run encrypted power iteration.
        auto encEigVec = EncryptedPowerIteration(ctx, encCovDiags, encV, pic);
        result.encEigenvectors.push_back(encEigVec);

        // Decrypt the eigenvector for eigenvalue computation and deflation.
        auto eigVec = UnpackVector(ctx, encEigVec, d);
        NormalizeVec(eigVec);

        // Compute eigenvalue in plaintext via Rayleigh quotient: λ = v^T C v.
        // This avoids a large level-gap issue with encrypted Rayleigh quotient
        // (fresh diags at top level vs eigenvector at a much lower level).
        auto Cv = MatVecMul(currentCov, eigVec);
        double lambda = DotProduct(eigVec, Cv);
        result.eigenvalues.push_back(lambda);

        std::cout << "[PCA] eigenvalue " << (comp + 1) << " = " << lambda << "\n";
        PrintVec("[PCA] eigenvector:", eigVec, 6);

        // C' = C - lambda * v * v^T
        for (int r = 0; r < d; r++)
            for (int c = 0; c < d; c++)
                currentCov[r][c] -= lambda * eigVec[r] * eigVec[c];

        auto tCompEnd = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(
                        tCompEnd - tComp).count();
        std::cout << "[PCA] component " << (comp + 1) << " took " << ms << " ms\n";
    }

    return result;
}

// ---------------------------------------------------------------------------
// Plaintext PCA (reference)
// ---------------------------------------------------------------------------

PlaintextPCAResult PlaintextPCA(
    const std::vector<std::vector<double>> &covMatrix,
    int numComponents, int numIter) {

    PlaintextPCAResult result;
    int d = static_cast<int>(covMatrix.size());

    auto currentCov = covMatrix;

    for (int comp = 0; comp < numComponents; comp++) {
        auto v = RandomUnitVector(d);

        for (int iter = 0; iter < numIter; iter++) {
            auto w = MatVecMul(currentCov, v);
            double norm = VecNorm(w);
            if (norm < 1e-15) break;
            for (auto &x : w) x /= norm;
            v = w;
        }

        // Eigenvalue via Rayleigh quotient.
        auto Cv = MatVecMul(currentCov, v);
        double lambda = DotProduct(v, Cv);

        result.eigenvectors.push_back(v);
        result.eigenvalues.push_back(lambda);

        // Deflation.
        for (int r = 0; r < d; r++)
            for (int c = 0; c < d; c++)
                currentCov[r][c] -= lambda * v[r] * v[c];
    }

    return result;
}
