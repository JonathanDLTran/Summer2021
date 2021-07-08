cd build
make 
cd ..
/usr/local/opt/llvm/bin/clang -Xclang -load -Xclang build/skeleton/libSkeletonPass.* example.c 
echo 2 | ./a.out 