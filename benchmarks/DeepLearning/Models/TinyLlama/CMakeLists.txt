add_custom_command(
  OUTPUT 
    ${CMAKE_CURRENT_SOURCE_DIR}/forward.mlir 
    ${CMAKE_CURRENT_SOURCE_DIR}/subgraph0.mlir
  COMMAND
    python3 ${CMAKE_CURRENT_SOURCE_DIR}/buddy_tinyllama_import.py
  COMMENT "Generating forward.mlir, subgraph0.mlir..."
)

add_custom_command(
  OUTPUT forward_auto_vectorization.o
  COMMAND
    cat ${CMAKE_CURRENT_SOURCE_DIR}/forward.mlir |
    sed -e {s/@forward/@forward_auto_vectorization/}
        -e {s/@subgraph0/@subgraph0_auto_vectorization/} |
    ${BUDDY_MLIR_BINARY_DIR}/buddy-opt 
      -pass-pipeline 
      "builtin.module(func.func(tosa-to-linalg-named),func.func(tosa-to-linalg),func.func(tosa-to-tensor),func.func(tosa-to-arith))" |
    ${BUDDY_MLIR_BINARY_DIR}/buddy-opt
      -arith-expand
      -eliminate-empty-tensors
      -empty-tensor-to-alloc-tensor
      -one-shot-bufferize
      -convert-linalg-to-affine-loops
      -lower-affine
      -func-bufferize
      -arith-bufferize
      -tensor-bufferize
      -buffer-deallocation
      -finalizing-bufferize
      -convert-vector-to-scf
      -expand-strided-metadata
      -convert-vector-to-llvm
      -memref-expand
      -arith-expand
      -convert-arith-to-llvm
      -finalize-memref-to-llvm
      -convert-scf-to-cf
      -llvm-request-c-wrappers
      -convert-openmp-to-llvm
      -convert-arith-to-llvm
      -convert-math-to-llvm
      -convert-math-to-libm 
      -convert-func-to-llvm
      -reconcile-unrealized-casts |
    ${LLVM_MLIR_BINARY_DIR}/mlir-translate -mlir-to-llvmir |
    ${LLVM_MLIR_BINARY_DIR}/llc -O3
      -mtriple=${BUDDY_OPT_TRIPLE} -mattr=${BUDDY_OPT_ATTR} -filetype=obj
      -o ${CMAKE_CURRENT_BINARY_DIR}/forward_auto_vectorization.o
  DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/forward.mlir
    ${BUDDY_MLIR_BINARY_DIR}/buddy-opt
  COMMENT "Building forward_auto_vectorization.o"
  VERBATIM)

add_custom_command(
  OUTPUT subgraph0_auto_vectorization.o
  COMMAND
    cat ${CMAKE_CURRENT_SOURCE_DIR}/subgraph0.mlir |
    sed -e {s/@subgraph0/@subgraph0_auto_vectorization/} |
    ${BUDDY_MLIR_BINARY_DIR}/buddy-opt 
      -pass-pipeline 
      "builtin.module(func.func(tosa-to-linalg-named),func.func(tosa-to-linalg),func.func(tosa-to-tensor),func.func(tosa-to-arith))" |
    ${BUDDY_MLIR_BINARY_DIR}/buddy-opt
      -arith-expand
      -eliminate-empty-tensors
      -empty-tensor-to-alloc-tensor
      -one-shot-bufferize
      -convert-linalg-to-affine-loops
      -affine-loop-fusion
      -lower-affine
      -func-bufferize-dynamic-offset
      -tensor-bufferize
      -arith-bufferize
      -buffer-deallocation
      -finalizing-bufferize
      -convert-vector-to-scf
      -expand-strided-metadata
      -cse
      -convert-vector-to-llvm
      -memref-expand
      -arith-expand
      -convert-arith-to-llvm
      -finalize-memref-to-llvm
      -convert-scf-to-cf
      -llvm-request-c-wrappers
      -convert-openmp-to-llvm
      -convert-arith-to-llvm
      -convert-math-to-llvm
      -convert-math-to-libm 
      -convert-func-to-llvm
      -reconcile-unrealized-casts |
    ${LLVM_MLIR_BINARY_DIR}/mlir-translate -mlir-to-llvmir |
    ${LLVM_MLIR_BINARY_DIR}/llc -O3
      -mtriple=${BUDDY_OPT_TRIPLE} -mattr=${BUDDY_OPT_ATTR} -filetype=obj
      -o ${CMAKE_CURRENT_BINARY_DIR}/subgraph0_auto_vectorization.o
  DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/subgraph0.mlir
    ${BUDDY_MLIR_BINARY_DIR}/buddy-opt
  COMMENT "Building subgraph0_auto_vectorization.o"
  VERBATIM)

add_custom_command(
  OUTPUT forward_buddy_vectorization.o
  COMMAND 
    cat ${CMAKE_CURRENT_SOURCE_DIR}/forward.mlir |
    sed -e {s/@forward/@forward_buddy_vectorization/}
        -e {s/@subgraph0/@subgraph0_buddy_vectorization/} |
    ${BUDDY_MLIR_BINARY_DIR}/buddy-opt
      -pass-pipeline 
      "builtin.module(func.func(tosa-to-linalg-named),func.func(tosa-to-linalg),func.func(tosa-to-tensor),func.func(tosa-to-arith))" |
    ${BUDDY_MLIR_BINARY_DIR}/buddy-opt
      -arith-expand
      -eliminate-empty-tensors
      -empty-tensor-to-alloc-tensor
      -one-shot-bufferize
      -batchmatmul-optimize
      -convert-linalg-to-affine-loops
      -affine-loop-fusion
      -lower-affine
      -func-bufferize
      -arith-bufferize
      -tensor-bufferize
      -buffer-deallocation
      -finalizing-bufferize
      -convert-vector-to-scf
      -expand-strided-metadata
      -convert-vector-to-llvm
      -memref-expand
      -arith-expand
      -convert-arith-to-llvm
      -finalize-memref-to-llvm
      -convert-scf-to-cf
      -llvm-request-c-wrappers
      -convert-openmp-to-llvm
      -convert-arith-to-llvm
      -convert-math-to-llvm
      -convert-math-to-libm 
      -convert-func-to-llvm
      -reconcile-unrealized-casts |
    ${LLVM_MLIR_BINARY_DIR}/mlir-translate -mlir-to-llvmir |
    ${LLVM_MLIR_BINARY_DIR}/llc -O3
      -mtriple=${BUDDY_OPT_TRIPLE} -mattr=${BUDDY_OPT_ATTR} -filetype=obj
      -o ${CMAKE_CURRENT_BINARY_DIR}/forward_buddy_vectorization.o
  DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/forward.mlir
    ${BUDDY_MLIR_BINARY_DIR}/buddy-opt
  COMMENT "Building forward_buddy_vectorization.o"
  VERBATIM)

add_custom_command(
  OUTPUT subgraph0_buddy_vectorization.o
  COMMAND
    cat ${CMAKE_CURRENT_SOURCE_DIR}/subgraph0.mlir |
    sed -e {s/@subgraph0/@subgraph0_buddy_vectorization/} |
    ${BUDDY_MLIR_BINARY_DIR}/buddy-opt
      -pass-pipeline 
      "builtin.module(func.func(tosa-to-linalg-named),func.func(tosa-to-linalg),func.func(tosa-to-tensor),func.func(tosa-to-arith))" |
    ${BUDDY_MLIR_BINARY_DIR}/buddy-opt
      -arith-expand
      -eliminate-empty-tensors
      -empty-tensor-to-alloc-tensor
      -one-shot-bufferize
      -batchmatmul-optimize
      -convert-linalg-to-affine-loops
      -affine-loop-fusion
      -lower-affine
      -func-bufferize-dynamic-offset
      -tensor-bufferize
      -arith-bufferize
      -buffer-deallocation
      -finalizing-bufferize
      -convert-vector-to-scf
      -expand-strided-metadata
      -cse
      -convert-vector-to-llvm
      -memref-expand
      -arith-expand
      -convert-arith-to-llvm
      -finalize-memref-to-llvm
      -convert-scf-to-cf
      -llvm-request-c-wrappers
      -convert-openmp-to-llvm
      -convert-arith-to-llvm
      -convert-math-to-llvm
      -convert-math-to-libm 
      -convert-func-to-llvm
      -reconcile-unrealized-casts |
    ${LLVM_MLIR_BINARY_DIR}/mlir-translate -mlir-to-llvmir |
    ${LLVM_MLIR_BINARY_DIR}/llc -O3
      -mtriple=${BUDDY_OPT_TRIPLE} -mattr=${BUDDY_OPT_ATTR} -filetype=obj
      -o ${CMAKE_CURRENT_BINARY_DIR}/subgraph0_buddy_vectorization.o
  DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/subgraph0.mlir
    ${BUDDY_MLIR_BINARY_DIR}/buddy-opt
  COMMENT "Building subgraph0_buddy_vectorization.o"
  VERBATIM)

add_library(TINYLLAMA_AUTO_VECTORIZATION STATIC subgraph0_auto_vectorization.o forward_auto_vectorization.o)
set_target_properties(TINYLLAMA_AUTO_VECTORIZATION PROPERTIES LINKER_LANGUAGE CXX)

add_library(TINYLLAMA_BUDDY_VECTORIZATION STATIC subgraph0_buddy_vectorization.o forward_buddy_vectorization.o)
set_target_properties(TINYLLAMA_BUDDY_VECTORIZATION PROPERTIES LINKER_LANGUAGE CXX)

add_executable(dl-model-tinyllama-benchmark
  GoogleBenchmarkMain.cpp
)

set_target_properties(dl-model-tinyllama-benchmark PROPERTIES
  LINK_FLAGS "-static"
)

set(BenchmarkTool GoogleBenchmark)

if(CROSS_COMPILE_RVV)
  set(BUDDY_LIB_DIR ${BUDDY_MLIR_CROSS_LIB_DIR})
else()
  set(BUDDY_LIB_DIR ${BUDDY_MLIR_LIB_DIR})
endif()

target_link_libraries(dl-model-tinyllama-benchmark
  ${BenchmarkTool}
  TINYLLAMA_AUTO_VECTORIZATION
  TINYLLAMA_BUDDY_VECTORIZATION
  ${BUDDY_LIB_DIR}/libStaticMLIRCRunnerUtils.a
)
