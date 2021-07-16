cd build
LLVM_DIR=/usr/local/opt/llvm/lib/cmake/llvm cmake ..
make 
cd ..
cc -c rtlib.c
/usr/local/opt/llvm/bin/clang -Xclang -load -Xclang build/skeleton/libSkeletonPass.so -c example.c
cc example.o rtlib.o
echo 12 | ./a.out 