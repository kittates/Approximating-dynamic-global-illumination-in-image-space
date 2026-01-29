cd build
rm -rf ./*
cmake .. && make
cd ..
./build/app
cd ..
