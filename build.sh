if command -v ninja >/dev/null 2>&1; then
  echo "ninja is available."
  cmake -GNinja -Bbuild . -DCMAKE_BUILD_TYPE=Release
else
  echo "ninja is not available."
  cmake -Bbuild . -DCMAKE_BUILD_TYPE=Release
fi
cmake --build build
cp build/rimeac.lua ./
cp build/librimeac.so ./
