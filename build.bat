@echo off
cd /d %~dp0
if not exist build-release mkdir build-release
cd build-release
cmake .. -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ^
  -DVCPKG_TARGET_TRIPLET=x64-mingw-dynamic ^
  -DVCPKG_HOST_TRIPLET=x64-mingw-dynamic ^
  -DCMAKE_C_COMPILER=D:/mingw64/bin/gcc.exe ^
  -DCMAKE_CXX_COMPILER=D:/mingw64/bin/g++.exe
cmake --build . -j8
echo Готово: build-release\Remont_Uchet.exe
