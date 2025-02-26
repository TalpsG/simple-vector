rm -rf node1 node2 node3
mkdir node1 node2 node3

cd node1
echo -e "cp ../../../build/simple_vector .\n
rm -rf ScalarStorage WalStore\n
./simple_vector 1" > test.sh
cd ..


cd node2
echo -e "cp ../../../build/simple_vector .\n
rm -rf ScalarStorage WalStore\n
./simple_vector 2" > test.sh
cd ..

cd node3
echo -e "cp ../../../build/simple_vector .\n
rm -rf ScalarStorage WalStore\n
./simple_vector 3" > test.sh
cd ..
