"""Generate a sample CSV dataset for testing HE-PCA."""
import numpy as np
import os

def main():
    np.random.seed(42)
    n_samples = 200
    dim = 8

    # Create data with clear principal components:
    # - First axis has highest variance (scale 10)
    # - Each subsequent axis has decreasing variance
    scales = [10.0 / (i + 1) for i in range(dim)]
    data = np.random.randn(n_samples, dim) * scales

    # Apply a random rotation to make it non-trivial.
    Q, _ = np.linalg.qr(np.random.randn(dim, dim))
    data = data @ Q.T

    out_path = os.path.join(os.path.dirname(__file__), "sample_data.csv")
    np.savetxt(out_path, data, delimiter=",", fmt="%.8f")
    print(f"Generated {n_samples} x {dim} dataset at {out_path}")

if __name__ == "__main__":
    main()
