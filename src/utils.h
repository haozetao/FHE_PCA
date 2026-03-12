#pragma once

#include <vector>
#include <string>

// Read CSV data file: each row is a data sample, columns are features.
std::vector<std::vector<double>> ReadCSV(const std::string &filepath);

// Generate a random d-dimensional unit vector.
std::vector<double> RandomUnitVector(int d);

// Compute L2 norm of a vector.
double VecNorm(const std::vector<double> &v);

// Normalize a vector in place.
void NormalizeVec(std::vector<double> &v);

// Compute dot product of two vectors.
double DotProduct(const std::vector<double> &a, const std::vector<double> &b);

// Compute cosine similarity |<a,b>| / (||a|| * ||b||).
double CosineSimilarity(const std::vector<double> &a,
                        const std::vector<double> &b);

// Compute matrix-vector product (plaintext).
std::vector<double> MatVecMul(
    const std::vector<std::vector<double>> &mat,
    const std::vector<double> &vec);

// Compute spectral norm estimate (largest singular value) of a matrix.
double EstimateSpectralNorm(
    const std::vector<std::vector<double>> &mat,
    int numIter = 50);

// Generate synthetic PCA test data with known structure.
std::vector<std::vector<double>> GenerateSyntheticData(
    int numSamples, int dim, int seed = 42);

// Print a vector.
void PrintVec(const std::string &label, const std::vector<double> &v,
              int maxElems = 10);
