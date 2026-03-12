#include "utils.h"
#include <cmath>
#include <fstream>
#include <sstream>
#include <random>
#include <iostream>
#include <algorithm>
#include <numeric>
#include <iomanip>

std::vector<std::vector<double>> ReadCSV(const std::string &filepath) {
    std::vector<std::vector<double>> data;
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error: cannot open " << filepath << "\n";
        return data;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::vector<double> row;
        std::stringstream ss(line);
        std::string cell;
        while (std::getline(ss, cell, ',')) {
            try { row.push_back(std::stod(cell)); }
            catch (...) { /* skip header or bad value */ }
        }
        if (!row.empty()) data.push_back(row);
    }
    return data;
}

std::vector<double> RandomUnitVector(int d) {
    static std::mt19937 rng(42);
    std::normal_distribution<double> dist(0.0, 1.0);
    std::vector<double> v(d);
    for (auto &x : v) x = dist(rng);
    NormalizeVec(v);
    return v;
}

double VecNorm(const std::vector<double> &v) {
    double s = 0;
    for (auto x : v) s += x * x;
    return std::sqrt(s);
}

void NormalizeVec(std::vector<double> &v) {
    double n = VecNorm(v);
    if (n > 1e-15)
        for (auto &x : v) x /= n;
}

double DotProduct(const std::vector<double> &a,
                  const std::vector<double> &b) {
    double s = 0;
    for (size_t i = 0; i < a.size(); i++) s += a[i] * b[i];
    return s;
}

double CosineSimilarity(const std::vector<double> &a,
                        const std::vector<double> &b) {
    double na = VecNorm(a), nb = VecNorm(b);
    if (na < 1e-15 || nb < 1e-15) return 0;
    return std::abs(DotProduct(a, b)) / (na * nb);
}

std::vector<double> MatVecMul(
    const std::vector<std::vector<double>> &mat,
    const std::vector<double> &vec) {
    int rows = static_cast<int>(mat.size());
    int cols = static_cast<int>(vec.size());
    std::vector<double> result(rows, 0.0);
    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++)
            result[i] += mat[i][j] * vec[j];
    return result;
}

double EstimateSpectralNorm(const std::vector<std::vector<double>> &mat,
                            int numIter) {
    int d = static_cast<int>(mat.size());
    auto v = RandomUnitVector(d);
    for (int iter = 0; iter < numIter; iter++) {
        auto w = MatVecMul(mat, v);
        double norm = VecNorm(w);
        if (norm < 1e-15) break;
        for (auto &x : w) x /= norm;
        v = w;
    }
    auto Av = MatVecMul(mat, v);
    return VecNorm(Av);
}

std::vector<std::vector<double>> GenerateSyntheticData(
    int numSamples, int dim, int seed) {
    std::mt19937 rng(seed);
    std::normal_distribution<double> dist(0.0, 1.0);

    // Create data with a dominant direction along the first axis
    // and decreasing variance along subsequent axes.
    std::vector<std::vector<double>> data(numSamples,
                                          std::vector<double>(dim, 0.0));
    for (int i = 0; i < numSamples; i++) {
        for (int j = 0; j < dim; j++) {
            double scale = 10.0 / (j + 1);
            data[i][j] = dist(rng) * scale;
        }
    }
    return data;
}

void PrintVec(const std::string &label, const std::vector<double> &v,
              int maxElems) {
    std::cout << label << " [";
    int n = std::min(static_cast<int>(v.size()), maxElems);
    for (int i = 0; i < n; i++) {
        std::cout << std::fixed << std::setprecision(6) << v[i];
        if (i + 1 < n) std::cout << ", ";
    }
    if (static_cast<int>(v.size()) > maxElems) std::cout << ", ...";
    std::cout << "] (len=" << v.size() << ")\n";
}
