# 基于同态加密的幂迭代 PCA 实现报告

## 1. 项目概述

本项目使用 C++ 和 OpenFHE 库实现了基于 CKKS 同态加密方案的 PCA（主成分分析）算法。核心思想是：在加密状态下执行幂迭代，使得云服务器能够帮助计算特征向量，但无法获知计算结果。

### 威胁模型

- **协方差矩阵**：明文（数据拥有者自己计算的聚合统计量，可公开）
- **特征向量**：全程加密（只有数据拥有者能解密）
- **特征值**：在明文中通过 Rayleigh quotient 计算（协方差矩阵和解密后的特征向量均已可见）

## 2. 算法流程

```
输入: 数据矩阵 X (n × d)
输出: 前 k 个主成分 (特征向量 + 特征值)

1. 数据预处理 (明文)
   - 中心化: X_centered = X - mean(X)
   - 协方差矩阵: C = X^T X / (n-1)

2. 对每个主成分 i = 1, ..., k:
   a. Halevi-Shoup 对角线打包: 将 C 按对角线编码为 d 个密文
   b. 加密随机初始向量: v₀ ← Encrypt(random_unit_vector)
   c. 幂迭代 (密文运算):
      for t = 1 to T:
          v_t = (C × v_{t-1}) / ||C||   ← 矩阵-向量乘 + 标量缩放
   d. 归一化 (密文运算):
      - ||v||² = InnerProduct(v, v)      ← 旋转求和
      - inv_norm = InvSqrt(||v||²)       ← Newton 迭代近似
      - v_norm = v × inv_norm
   e. 解密特征向量, 明文计算特征值: λ = v^T C v
   f. Deflation (明文): C ← C - λ·v·v^T
```

## 3. 项目结构

```
PCA/
├── CMakeLists.txt          # 构建配置
├── build.sh                # 一键编译脚本 (含 OpenFHE 安装)
├── src/
│   ├── he_context.h/cpp    # CKKS 上下文初始化、密钥生成
│   ├── packing.h/cpp       # Halevi-Shoup 对角线矩阵打包
│   ├── he_linalg.h/cpp     # 加密线性代数 (矩阵乘、内积、InvSqrt)
│   ├── power_iteration.h/cpp  # 加密幂迭代核心算法
│   ├── pca.h/cpp           # PCA 封装 (多主成分 deflation)
│   ├── utils.h/cpp         # 辅助函数 (CSV 读写、向量运算)
│   └── main.cpp            # 主程序入口
└── tests/
    ├── test_packing.cpp    # 打包/解包单元测试
    ├── test_he_linalg.cpp  # 加密线性代数单元测试
    └── test_pca.cpp        # 端到端 PCA 测试
```

## 4. 关键模块实现

### 4.1 CKKS 上下文 (`he_context.cpp`)

初始化 OpenFHE 的 CKKS 加密上下文，包括：

- **加密方案**: CKKS with FLEXIBLEAUTO 自动缩放
- **安全等级**: HEStd_128_classic (128-bit 经典安全)
- **乘法深度**: 32 (支持 6 次幂迭代 + 归一化)
- **环维度**: 根据深度自动选择 N=65536 或 N=131072
- **旋转密钥**: 生成 `[1, 2, ..., d-1]` 的旋转密钥，用于 Halevi-Shoup 乘法和 rotate-and-sum

### 4.2 Halevi-Shoup 矩阵打包 (`packing.cpp`)

将 d×d 矩阵编码为 d 个密文，每个密文存储一条对角线：

- **对角线 k** 的第 j 个 slot 存储 `matrix[j][(k+j) % d]`
- 数据以周期 d **复制填充**整个 batchSize，确保 `EvalRotate` 的循环移位正确性
- 向量同样以周期 d 复制填充

矩阵-向量乘法公式：

$$\text{result} = \sum_{k=0}^{d-1} \text{diag}_k \odot \text{Rotate}(\vec{v}, k)$$

仅消耗 **1 个乘法深度**（所有 d 次乘法并行执行在同一层）。

### 4.3 加密线性代数 (`he_linalg.cpp`)

| 操作 | 实现方式 | 深度消耗 |
|------|---------|---------|
| **矩阵-向量乘** | Halevi-Shoup 对角线法 | 1 level |
| **内积** | 逐元素乘 + rotate-and-sum | 1 level |
| **逆平方根** | Newton 迭代: $y_{n+1} = y_n(3 - xy_n^2)/2$ | ~4 levels/iteration |
| **标量乘** | `EvalMult(ct, scalar)` | 1 level |

**关键设计**：内积不使用 mask，因为数据已以周期 d 复制。rotate-and-sum 恰好覆盖一个完整周期，每个 slot 都得到正确的内积值。

### 4.4 幂迭代 (`power_iteration.cpp`)

每次迭代执行：
1. `w = C × v` (Halevi-Shoup 矩阵-向量乘, 1 level)
2. `v = w / estNorm` (标量缩放防止数值溢出, 1 level)

迭代结束后执行归一化：
1. `||v||² = <v, v>` (加密内积, 1 level)
2. `1/||v|| = InvSqrt(||v||²)` (Newton 迭代, ~12 levels)
3. `v_norm = v × (1/||v||)` (1 level)

**总深度消耗**: 6×2 + 1 + 12 + 1 = **26 levels** (深度预算 32，余量 6)

### 4.5 PCA 封装 (`pca.cpp`)

对每个主成分：
1. 打包协方差矩阵为密文对角线
2. 加密随机初始向量
3. 执行幂迭代得到加密特征向量
4. 解密特征向量，在明文中计算特征值 $\lambda = v^T C v$
5. Deflation: $C \leftarrow C - \lambda \cdot v \cdot v^T$，然后提取下一个主成分

## 5. 测试方法

### 5.1 单元测试

#### test_packing — 打包正确性

| 测试 | 内容 | 验证方式 |
|------|------|---------|
| TestPackUnpackVector | 向量加密→解密 | 解密值与原值误差 < 0.001 |
| TestDiagonalPacking | 单位矩阵 × 向量 | 结果 = 原向量 |
| TestGeneralMatVec | 一般矩阵 × 向量 | 与明文矩阵乘法对比，误差 < 0.001 |

#### test_he_linalg — 加密运算正确性

| 测试 | 内容 | 验证方式 |
|------|------|---------|
| TestInnerProduct | `<[1,2,3,4], [2,3,4,5]>` | 期望 40，误差 < 0.01 |
| TestInvSqrt | `1/sqrt(4)` | 期望 0.5，误差 < 0.05 |
| TestScalarMul | 向量 × 2.5 | 逐元素误差 < 0.01 |

### 5.2 端到端测试

#### test_pca — PCA 精度验证

**测试数据**: 200 个样本、8 维，沿各轴方差递减的合成数据（`seed=42` 可复现）。

**测试流程**:
1. 明文 PCA（200 次幂迭代）作为 ground truth
2. 加密 PCA（6 次幂迭代）在密文上执行
3. 对比两者的特征向量方向和特征值

**验证指标**:
- 特征向量：`CosineSimilarity(decrypted, plaintext) > 0.8`
- 特征值：`相对误差 < 50%`

### 5.3 运行方式

```bash
# 编译
cd /Users/bytedance/PCA
cmake -S . -B build && cmake --build build

# 运行三组测试
./build/test_packing
./build/test_he_linalg
./build/test_pca

# 运行主程序
./build/he_pca --dim 8 --n 200 --k 2 --iter 6 --depth 32
```

## 6. 实验结果

### 6.1 加密参数

| 参数 | 值 |
|------|---|
| 加密方案 | CKKS (FLEXIBLEAUTO) |
| 安全等级 | HEStd_128_classic |
| Ring Dimension | 131072 (2¹⁷) |
| Batch Size | 65536 |
| Multiplicative Depth | 32 |
| Scaling Mod Size | 50 bits (~15 位十进制精度) |

### 6.2 算法参数

| 参数 | 值 |
|------|---|
| 数据维度 | 8 |
| 样本数 | 200 |
| 提取主成分数 | 2 |
| 幂迭代次数 | 6 |
| InvSqrt Newton 迭代次数 | 3 |
| Bootstrapping | 未启用 |

### 6.3 精度结果

| 主成分 | 余弦相似度 | 加密特征值 | 明文特征值 | 相对误差 |
|--------|-----------|-----------|-----------|---------|
| PC1 | **1.000000** | 104.049019 | 104.049038 | 0.00002% |
| PC2 | **0.999972** | 23.284367 | 23.284820 | 0.0019% |

### 6.4 性能指标

| 指标 | 值 |
|------|---|
| 单次幂迭代耗时 | ~2.2 – 2.7 秒 |
| 单个主成分总耗时 | ~18 秒 |
| 2 个主成分总耗时 | ~36 秒 |
| Level 消耗 | 26 / 32 |

## 7. 技术要点与踩坑记录

### 7.1 数据复制 (Replication)

CKKS 的 `EvalRotate` 是全局循环移位（在整个 batchSize 上），而非在逻辑 d 维空间内移位。当 `batchSize >> d` 时，移位后数据会"溢出"到错误位置。

**解决方案**：将向量和矩阵对角线以周期 d 复制填满所有 slot，使全局循环移位等价于逻辑空间内的循环移位。

### 7.2 Inner Product 不能加 Mask

对于复制数据，rotate-and-sum 天然正确（每 d 个连续 slot 的和 = 一个完整周期之和 = 内积）。如果先 mask 再求和，会破坏复制结构，导致不同 slot 得到不同值，进而使后续 InvSqrt 和归一化产生错误。

### 7.3 乘法深度预算

CKKS 中每次乘法消耗 1 个 level。总深度 = 迭代次数 × 每次迭代消耗 + 归一化消耗。超出预算会导致 `DropLastElement` 异常。需要在迭代次数和精度之间权衡。

### 7.4 特征值计算

在 CKKS FLEXIBLEAUTO 模式下，将全新加密的密文（高 level）与经过多层运算的密文（低 level）直接做乘法，会因缩放因子不匹配导致解密值严重失真。因此特征值在明文中计算，这在安全性上是合理的（协方差矩阵和特征向量此时均已可见）。

## 8. 依赖

- **OpenFHE** ≥ 1.1.4 — C++ 全同态加密库
- **CMake** ≥ 3.16
- **C++17** 编译器
- **libomp** (macOS 需通过 Homebrew 安装: `brew install libomp`)
