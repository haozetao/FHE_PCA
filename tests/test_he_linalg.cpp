#include "../src/he_context.h"
#include "../src/packing.h"
#include "../src/he_linalg.h"
#include "../src/utils.h"
#include <iostream>
#include <cmath>
#include <cassert>

bool ApproxEq(double a, double b, double tol = 1e-2) {
    return std::abs(a - b) < tol;
}

void TestInnerProduct() {
    std::cout << "=== TestInnerProduct ===\n";
    const int dim = 4;
    auto ctx = InitCKKSContext(dim, 15, 50);

    std::vector<double> a = {1, 2, 3, 4};
    std::vector<double> b = {2, 3, 4, 5};
    double expected = DotProduct(a, b);

    auto encA = PackVector(ctx, a);
    auto encB = PackVector(ctx, b);
    auto encDot = HE_InnerProduct(ctx, encA, encB);
    auto decDot = UnpackVector(ctx, encDot, 1);

    std::cout << "  expected=" << expected << " got=" << decDot[0] << "\n";
    assert(ApproxEq(expected, decDot[0]));
    std::cout << "PASSED\n\n";
}

void TestInvSqrt() {
    std::cout << "=== TestInvSqrt ===\n";
    const int dim = 4;
    auto ctx = InitCKKSContext(dim, 20, 50);

    // Test: 1/sqrt(4) = 0.5
    double val = 4.0;
    double expected = 1.0 / std::sqrt(val);

    std::vector<double> xvec(ctx.batchSize, val);
    auto ptX = ctx.cc->MakeCKKSPackedPlaintext(xvec);
    auto encX = ctx.cc->Encrypt(ctx.keys.publicKey, ptX);

    // Initial guess: for x=4, 1/sqrt(4) ~ 0.5. Start with y0=0.6.
    auto encInv = HE_InvSqrt(ctx, encX, 0.6, 4);
    auto dec = UnpackVector(ctx, encInv, 1);

    std::cout << "  1/sqrt(" << val << ") expected=" << expected << " got=" << dec[0] << "\n";
    assert(ApproxEq(expected, dec[0], 0.05));
    std::cout << "PASSED\n\n";
}

void TestScalarMul() {
    std::cout << "=== TestScalarMul ===\n";
    const int dim = 4;
    auto ctx = InitCKKSContext(dim, 10, 50);

    std::vector<double> v = {1, 2, 3, 4};
    auto encV = PackVector(ctx, v);
    auto encScaled = HE_ScalarMul(ctx, encV, 2.5);
    auto dec = UnpackVector(ctx, encScaled, dim);

    for (int i = 0; i < dim; i++) {
        double expected = v[i] * 2.5;
        std::cout << "  [" << i << "] expected=" << expected << " got=" << dec[i] << "\n";
        assert(ApproxEq(expected, dec[i]));
    }
    std::cout << "PASSED\n\n";
}

int main() {
    std::cout << "========== HE Linear Algebra Tests ==========\n\n";
    TestInnerProduct();
    TestInvSqrt();
    TestScalarMul();
    std::cout << "All HE linalg tests passed!\n";
    return 0;
}
