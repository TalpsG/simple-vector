rm -rf node1 node2 node3
mkdir node1 node2 node3

cd node1
echo -e "cp ../../../build/simple_vector .\n
rm -rf ScalarStorage WalStore\n
./simple_vector ./conf.ini" > test.sh
echo -e "db_path=ScalarStorage
wal_path=WALStorage
node_id=1
endpoint=127.0.0.1:8081
port=8081
http_server_address=0.0.0.0
http_server_port=9091" > conf.ini

cd ..


cd node2
echo -e "cp ../../../build/simple_vector .\n
rm -rf ScalarStorage WalStore\n
./simple_vector ./conf.ini" > test.sh
echo -e "db_path=ScalarStorage
wal_path=WALStorage
node_id=2
endpoint=127.0.0.1:8082
port=8082
http_server_address=0.0.0.0
http_server_port=9092" > conf.ini
cd ..

cd node3
echo -e "cp ../../../build/simple_vector .\n
rm -rf ScalarStorage WalStore\n
./simple_vector ./conf.ini" > test.sh
echo -e "db_path=ScalarStorage
wal_path=WALStorage
node_id=3
endpoint=127.0.0.1:8083
port=8083
http_server_address=0.0.0.0
http_server_port=9093" > conf.ini
cd ..
