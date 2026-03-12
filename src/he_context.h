#pragma once

#include "openfhe.h"

using namespace lbcrypto;

struct HEContext {
    CryptoContext<DCRTPoly> cc;
    KeyPair<DCRTPoly> keys;
    uint32_t batchSize;
    uint32_t dim;
};

HEContext InitCKKSContext(uint32_t dim,
                         uint32_t multDepth = 20,
                         uint32_t scalingModSize = 50);
