echo -e "\n search euqal \n"
curl -X POST localhost:9091/search \
  -H "Content-Type: application/json" \
  -d @search_equal.json
