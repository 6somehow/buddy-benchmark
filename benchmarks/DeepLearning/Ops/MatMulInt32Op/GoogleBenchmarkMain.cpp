//===----------GoogleBenchmarkMain.cpp----------===//
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//===----------------------------------------------------------------------===//
//
// This file implements the benchmark for Matmul on Risc-V vector devices.
//
//===----------------------------------------------------------------------===//

#include <benchmark/benchmark.h>
#include <buddy/Core/Container.h>
#include <iostream>
#include <random>

// Define target layout.
#define M (128 * 4)
#define N (128 * 4)
#define K (128 * 4)

// Helper functions and variables.
namespace {
const std::string PASS = "\033[32mPASS\033[0m";
const std::string FAIL = "\033[31mFAIL\033[0m";

bool areArraysEqual(int array1[], int array2[], int size) {
  for (int i = 0; i < size; ++i) {
    if (array1[i] != array2[i]) {
      return false;
    }
  }
  return true;
}
} // namespace

namespace {
// Declare the matmul C interface.
extern "C" {
void _mlir_ciface_matmul_scalar(MemRef<int, 2> *A, MemRef<int, 2> *B,
                                MemRef<int, 2> *C);
void _mlir_ciface_matmul_rvv(MemRef<int, 2> *A, MemRef<int, 2> *B,
                             MemRef<int, 2> *C);
void _mlir_ciface_matmul_vec(
    MemRef<int, 2> *A, MemRef<int, 2> *B,
    MemRef<int, 2> *C); // Add the interface for matmul_vec
}

template <typename Func>
void DL_OPS_MATMUL(benchmark::State &state, Func func) {

  intptr_t sizesA[2] = {M, K};
  intptr_t sizesB[2] = {K, N};
  intptr_t sizesC[2] = {M, N};

  MemRef<int, 2> A(sizesA, 1.0);
  MemRef<int, 2> B(sizesB, 1.0);
  MemRef<int, 2> C(sizesC, 0.0);

  for (auto _ : state) {
    func(&A, &B, &C);
  }
}
} // namespace

// Register benchmarking function with different arguments.
BENCHMARK_CAPTURE(DL_OPS_MATMUL, Scalar, _mlir_ciface_matmul_scalar)
    ->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(
    DL_OPS_MATMUL, VectorBroadcast,
    _mlir_ciface_matmul_vec) // Register matmul_vec for benchmarking
    ->Unit(benchmark::kMillisecond);
BENCHMARK_CAPTURE(DL_OPS_MATMUL, VectorRVV, _mlir_ciface_matmul_rvv)
    ->Unit(benchmark::kMillisecond);

/// Correctness Verification
/// The verification does not affect the performance.
/// - Set the scalar case as the criteria.
/// - Input elements are random numbers.
/// - Output elements are initialized to zero.
/// - Compare the output of various optimizations with the scalar version to
///   verify correctness.
void verification() {
  // Set the random number generator.
  unsigned int seed = time(NULL);

  // Set the layout sizes of input and output memref container.
  intptr_t sizesA[2] = {M, K};
  intptr_t sizesB[2] = {K, N};
  intptr_t sizesC[2] = {M, N};

  // Generate input A and input B memref container with random numbers.
  const int inputASize = M * K;
  int inputARand[inputASize];
  for (int i = 0; i < inputASize; ++i) {
    inputARand[i] = rand_r(&seed) / 1000 - 500;
  }
  MemRef<int, 2> inputAMemRef(inputARand, sizesA);

  const int inputBSize = K * N;
  int inputBRand[inputBSize];
  for (int i = 0; i < inputBSize; ++i) {
    inputBRand[i] = rand_r(&seed) / 1000 - 500;
  }
  MemRef<int, 2> inputBMemRef(inputBRand, sizesB);

  // Generate output memref container with zero.
  const int outputSize = M * N;
  MemRef<int, 2> outputScalar(sizesC, 0);
  MemRef<int, 2> outputRVV(sizesC, 0);
  MemRef<int, 2> outputVec(sizesC, 0); // Add output for matmul_vec

  // Perform all the matmul implementation.
  _mlir_ciface_matmul_scalar(&inputAMemRef, &inputBMemRef, &outputScalar);
  _mlir_ciface_matmul_rvv(&inputAMemRef, &inputBMemRef, &outputRVV);
  _mlir_ciface_matmul_vec(&inputAMemRef, &inputBMemRef,
                          &outputVec); // Call matmul_vec

  // Get the result array.
  auto resultScalar = outputScalar.getData();
  auto resultRVV = outputRVV.getData();
  auto resultVec = outputVec.getData(); // Get the result for matmul_vec

  // Print the verification result.
  std::cout << "-----------------------------------------------------------"
            << std::endl;
  std::cout << "Correctness Verification:" << std::endl;
  std::cout << "RVV case: "
            << (areArraysEqual(resultScalar, resultRVV, outputSize) ? PASS
                                                                    : FAIL)
            << std::endl;
  std::cout << "Vec case: "
            << (areArraysEqual(resultScalar, resultVec, outputSize) ? PASS
                                                                    : FAIL)
            << std::endl;
  std::cout << "-----------------------------------------------------------"
            << std::endl;
}

int main(int argc, char **argv) {
  // Run benchmark.
  ::benchmark::Initialize(&argc, argv);
  ::benchmark::RunSpecifiedBenchmarks();
  // Run correctness verification.
  verification();
  return 0;
}
