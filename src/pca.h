#pragma once

#include "he_context.h"
#include "power_iteration.h"
#include <vector>

struct PCAResult {
    std::vector<Ciphertext<DCRTPoly>> encEigenvectors;
    std::vector<double> eigenvalues;
};

struct PCAConfig {
    int numComponents   = 3;
    PowerIterConfig pic;
};

// Center the data matrix (subtract column means).
std::vector<std::vector<double>> CenterData(
    const std::vector<std::vector<double>> &data);

// Compute covariance matrix C = X^T X / (n-1) from centered data.
std::vector<std::vector<double>> ComputeCovarianceMatrix(
    const std::vector<std::vector<double>> &centeredData);

// Perform PCA on encrypted covariance matrix.
// Extracts `cfg.numComponents` principal components via power iteration + deflation.
PCAResult EncryptedPCA(
    const HEContext &ctx,
    const std::vector<std::vector<double>> &covMatrix,
    const PCAConfig &cfg);

// Plaintext PCA (reference implementation for validation).
struct PlaintextPCAResult {
    std::vector<std::vector<double>> eigenvectors;
    std::vector<double> eigenvalues;
};

PlaintextPCAResult PlaintextPCA(
    const std::vector<std::vector<double>> &covMatrix,
    int numComponents,
    int numIter = 200);
