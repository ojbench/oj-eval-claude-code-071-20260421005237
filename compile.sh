WARNING_FLAGS="-Wall -Wextra"

g++ -std=c++20 $1.cpp -O2 -o $1 $WARNING_FLAGS

echo "Compile $1.cpp done"
