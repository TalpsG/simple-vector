curl -X POST localhost:8080/upsert \
  -H "Content-Type: application/json" \
  -d @upsert.json

curl -X POST localhost:8080/upsert \
  -H "Content-Type: application/json" \
  -d @upsert_1.json

curl -X POST localhost:8080/upsert \
  -H "Content-Type: application/json" \
  -d @upsert_2.json

curl -X POST localhost:8080/upsert \
  -H "Content-Type: application/json" \
  -d @upsert_3.json

curl -X POST localhost:8080/search \
  -H "Content-Type: application/json" \
  -d @search_equal.json

curl -X POST localhost:8080/search \
  -H "Content-Type: application/json" \
  -d @search_unequal.json

curl -X POST localhost:8080/search \
  -H "Content-Type: application/json" \
  -d @search_normal.json
