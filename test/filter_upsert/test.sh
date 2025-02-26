curl -X POST localhost:9091/upsert \
  -H "Content-Type: application/json" \
  -d @upsert.json
echo -e "\n upsert \n"

curl -X POST localhost:9091/upsert \
  -H "Content-Type: application/json" \
  -d @upsert_1.json

echo -e "\n upsert 1 \n"

curl -X POST localhost:9091/upsert \
  -H "Content-Type: application/json" \
  -d @upsert_2.json

echo -e "\n upsert 2 \n"

curl -X POST localhost:9091/upsert \
  -H "Content-Type: application/json" \
  -d @upsert_3.json

echo -e "\n upsert 3 \n"

curl -X POST localhost:9091/upsert \
  -H "Content-Type: application/json" \
  -d @upsert_4.json

echo -e "\n upsert 4 \n"

curl -X POST localhost:9091/search \
  -H "Content-Type: application/json" \
  -d @search_equal.json

echo -e "\n search euqal \n"

curl -X POST localhost:9091/search \
  -H "Content-Type: application/json" \
  -d @search_unequal.json

echo -e "\n search uneuqal \n"

curl -X POST localhost:9091/search \
  -H "Content-Type: application/json" \
  -d @search_normal.json

echo -e "\n search normal \n"
