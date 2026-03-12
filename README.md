# HE-PCA: Homomorphic Encryption PCA via Power Iteration

Privacy-preserving Principal Component Analysis using the CKKS homomorphic encryption scheme and the power iteration (modified Krasulina) algorithm.

## Architecture

```
src/
├── he_context.h/cpp       -- CKKS context initialization, key generation
├── packing.h/cpp          -- Halevi-Shoup diagonal matrix packing
├── he_linalg.h/cpp        -- Encrypted linear algebra primitives
├── power_iteration.h/cpp  -- Power iteration core algorithm
├── pca.h/cpp              -- PCA orchestration with deflation
├── utils.h/cpp            -- Data I/O and plaintext utilities
└── main.cpp               -- Entry point
```

## Algorithm Overview

1. **Data preprocessing** (plaintext): center data, compute covariance matrix `C = X^T X / (n-1)`
2. **Matrix packing**: encode `C` using Halevi-Shoup diagonal packing into `d` CKKS ciphertexts
3. **Encrypted power iteration**: repeatedly compute `v = C * v` (normalized by estimated spectral norm) entirely in the encrypted domain
4. **Final normalization**: use polynomial-approximated inverse square root (`1/sqrt(||v||^2)`) via Newton iteration
5. **Deflation**: subtract extracted component `C' = C - lambda * v * v^T`, repeat for next component

Key innovations from [Cheon et al., ePrint 2023/1544]:
- **Delayed normalization**: only one `InvSqrt` call at the end instead of per-iteration
- **Arithmetic-only operations**: all intermediate steps use only addition and multiplication

## Prerequisites

- C++ compiler with C++17 support (GCC 9+ or Clang 10+)
- CMake 3.16+
- [OpenFHE](https://github.com/openfheorg/openfhe-development) v1.2+

## Build

```bash
# Option 1: Automated (installs OpenFHE if needed)
chmod +x build.sh
./build.sh

# Option 2: Manual (OpenFHE already installed)
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/path/to/openfhe
make -j$(nproc)
```

## Usage

```bash
# Run with synthetic data (default: dim=8, 200 samples, 3 components)
./build/he_pca

# Custom parameters
./build/he_pca --dim 16 --n 500 --k 4 --iter 20 --depth 25

# With CSV data file
./build/he_pca --data data/sample_data.csv --k 3 --iter 15
```

### Parameters

| Flag      | Description                     | Default |
|-----------|---------------------------------|---------|
| `--dim`   | Feature dimension               | 8       |
| `--n`     | Number of samples               | 200     |
| `--k`     | Number of principal components  | 3       |
| `--iter`  | Power iteration count           | 15      |
| `--depth` | CKKS multiplicative depth       | 25      |
| `--data`  | Path to CSV data file           | (none)  |

## Tests

```bash
cd build
./test_packing      # Diagonal packing + encrypted mat-vec
./test_he_linalg    # Inner product, InvSqrt, scalar multiply
./test_pca          # End-to-end PCA: encrypted vs plaintext comparison
```

## References

- Cheon et al., "Arithmetic PCA for Encrypted Data", ePrint 2023/1544
- Panda, "Polynomial Approximation of Inverse sqrt Function for FHE", 2022
- Halevi & Shoup, "Algorithms in HElib", ePrint 2014/106
- OpenFHE documentation: https://openfhe-development.readthedocs.io/



