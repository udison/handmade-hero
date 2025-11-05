pushd build
cl -Zi /std:c++20 ../src/win32_main.cpp user32.lib gdi32.lib
popd
