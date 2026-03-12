#include "../src/he_context.h"
#include "../src/packing.h"
#include "../src/he_linalg.h"
#include "../src/utils.h"
#include <iostream>
#include <cmath>
#include <cassert>

bool ApproxEqual(double a, double b, double tol = 1e-3) {
    return std::abs(a - b) < tol;
}

void TestPackUnpackVector() {
    std::cout << "=== TestPackUnpackVector ===\n";
    const int dim = 4;
    auto ctx = InitCKKSContext(dim, 10, 50);

    std::vector<double> v = {1.0, 2.0, 3.0, 4.0};
    auto enc = PackVector(ctx, v);
    auto dec = UnpackVector(ctx, enc, dim);

    for (int i = 0; i < dim; i++) {
        std::cout << "  v[" << i << "] = " << v[i] << " -> " << dec[i] << "\n";
        assert(ApproxEqual(v[i], dec[i]));
    }
    std::cout << "PASSED\n\n";
}

void TestDiagonalPacking() {
    std::cout << "=== TestDiagonalPacking ===\n";
    const int dim = 4;
    auto ctx = InitCKKSContext(dim, 10, 50);

    // Identity matrix: mat-vec should return the same vector.
    std::vector<std::vector<double>> identity(dim, std::vector<double>(dim, 0.0));
    for (int i = 0; i < dim; i++) identity[i][i] = 1.0;

    auto encDiags = PackMatrixDiagonal(ctx, identity);
    std::vector<double> v = {1.0, 2.0, 3.0, 4.0};
    auto encV = PackVector(ctx, v);
    auto encResult = HE_MatVecMul(ctx, encDiags, encV);
    auto result = UnpackVector(ctx, encResult, dim);

    std::cout << "  I * v:\n";
    for (int i = 0; i < dim; i++) {
        std::cout << "    [" << i << "] expected=" << v[i] << " got=" << result[i] << "\n";
        assert(ApproxEqual(v[i], result[i]));
    }
    std::cout << "PASSED\n\n";
}

void TestGeneralMatVec() {
    std::cout << "=== TestGeneralMatVec ===\n";
    const int dim = 4;
    auto ctx = InitCKKSContext(dim, 10, 50);

    std::vector<std::vector<double>> mat = {
        {2, 1, 0, 0},
        {1, 3, 1, 0},
        {0, 1, 4, 1},
        {0, 0, 1, 5}
    };
    std::vector<double> v = {1, 1, 1, 1};

    // Plaintext expected result
    auto expected = MatVecMul(mat, v);

    auto encDiags = PackMatrixDiagonal(ctx, mat);
    auto encV = PackVector(ctx, v);
    auto encResult = HE_MatVecMul(ctx, encDiags, encV);
    auto result = UnpackVector(ctx, encResult, dim);

    std::cout << "  M * v:\n";
    for (int i = 0; i < dim; i++) {
        std::cout << "    [" << i << "] expected=" << expected[i]
                  << " got=" << result[i] << "\n";
        assert(ApproxEqual(expected[i], result[i]));
    }
    std::cout << "PASSED\n\n";
}

int main() {
    std::cout << "========== Packing Tests ==========\n\n";
    TestPackUnpackVector();
    TestDiagonalPacking();
    TestGeneralMatVec();
    std::cout << "All packing tests passed!\n";
    return 0;
}
