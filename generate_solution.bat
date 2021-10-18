mkdir build
cd build

conan install .. -s build_type=Debug
cmake .. -G "Visual Studio 16" -DCMAKE_BUILD_TYPE=Debug

cd ../