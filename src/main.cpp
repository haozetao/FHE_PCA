#include "he_context.h"
#include "packing.h"
#include "pca.h"
#include "utils.h"
#include <iostream>
#include <chrono>
#include <string>

int main(int argc, char *argv[]) {
    // Defaults
    int dim          = 8;
    int numSamples   = 200;
    int numComponents = 3;
    int numIter      = 15;
    int multDepth    = 25;
    std::string dataFile;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--dim"  && i + 1 < argc) dim          = std::stoi(argv[++i]);
        if (arg == "--n"    && i + 1 < argc) numSamples   = std::stoi(argv[++i]);
        if (arg == "--k"    && i + 1 < argc) numComponents = std::stoi(argv[++i]);
        if (arg == "--iter" && i + 1 < argc) numIter      = std::stoi(argv[++i]);
        if (arg == "--depth"&& i + 1 < argc) multDepth    = std::stoi(argv[++i]);
        if (arg == "--data" && i + 1 < argc) dataFile     = argv[++i];
    }

    std::cout << "======================================\n";
    std::cout << " HE-PCA: Encrypted Power Iteration PCA\n";
    std::cout << "======================================\n";
    std::cout << "dim=" << dim << " samples=" << numSamples
              << " components=" << numComponents
              << " iterations=" << numIter
              << " multDepth=" << multDepth << "\n\n";

    // 1. Load or generate data
    std::vector<std::vector<double>> data;
    if (!dataFile.empty()) {
        data = ReadCSV(dataFile);
        if (data.empty()) {
            std::cerr << "Failed to read data from " << dataFile << "\n";
            return 1;
        }
        dim = static_cast<int>(data[0].size());
        numSamples = static_cast<int>(data.size());
        std::cout << "Loaded " << numSamples << " x " << dim << " from " << dataFile << "\n";
    } else {
        std::cout << "Generating synthetic data...\n";
        data = GenerateSyntheticData(numSamples, dim);
    }

    // 2. Preprocess: center and compute covariance
    auto centered = CenterData(data);
    auto covMatrix = ComputeCovarianceMatrix(centered);
    std::cout << "Covariance matrix computed (" << dim << "x" << dim << ")\n\n";

    // 3. Plaintext PCA (reference)
    std::cout << "--- Plaintext PCA (reference) ---\n";
    auto tPlain = std::chrono::high_resolution_clock::now();
    auto plainResult = PlaintextPCA(covMatrix, numComponents);
    auto tPlainEnd = std::chrono::high_resolution_clock::now();
    double plainMs = std::chrono::duration<double, std::milli>(tPlainEnd - tPlain).count();

    for (int i = 0; i < numComponents; i++) {
        std::cout << "  eigenvalue[" << i << "] = " << plainResult.eigenvalues[i] << "\n";
        PrintVec("  eigenvector:", plainResult.eigenvectors[i], 6);
    }
    std::cout << "Plaintext PCA took " << plainMs << " ms\n\n";

    // 4. Encrypted PCA
    std::cout << "--- Encrypted PCA (CKKS) ---\n";
    auto tInit = std::chrono::high_resolution_clock::now();
    auto ctx = InitCKKSContext(dim, multDepth);
    auto tInitEnd = std::chrono::high_resolution_clock::now();
    std::cout << "Context init: "
              << std::chrono::duration<double, std::milli>(tInitEnd - tInit).count()
              << " ms\n\n";

    PCAConfig pcaCfg;
    pcaCfg.numComponents     = numComponents;
    pcaCfg.pic.numIter       = numIter;
    pcaCfg.pic.bootstrapEvery = 0;
    pcaCfg.pic.invSqrtIter   = 4;

    auto tEnc = std::chrono::high_resolution_clock::now();
    auto encResult = EncryptedPCA(ctx, covMatrix, pcaCfg);
    auto tEncEnd = std::chrono::high_resolution_clock::now();
    double encMs = std::chrono::duration<double, std::milli>(tEncEnd - tEnc).count();

    std::cout << "\n--- Encrypted PCA completed in " << encMs << " ms ---\n\n";

    // 5. Validate: compare encrypted vs plaintext results
    std::cout << "--- Validation ---\n";
    for (int i = 0; i < numComponents; i++) {
        auto decVec = UnpackVector(ctx, encResult.encEigenvectors[i], dim);

        double cosSim = CosineSimilarity(decVec, plainResult.eigenvectors[i]);
        double lambda = encResult.eigenvalues[i];
        double lambdaErr = std::abs(lambda - plainResult.eigenvalues[i])
                         / std::abs(plainResult.eigenvalues[i] + 1e-15);

        std::cout << "Component " << (i + 1) << ":\n";
        std::cout << "  cosine similarity = " << cosSim << "\n";
        std::cout << "  eigenvalue (enc)  = " << lambda
                  << "  (plain) = " << plainResult.eigenvalues[i]
                  << "  rel_err = " << lambdaErr << "\n";
    }

    std::cout << "\nDone.\n";
    return 0;
}
