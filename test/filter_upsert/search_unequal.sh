echo -e "\n search uneuqal \n"
curl -X POST localhost:8080/search \
  -H "Content-Type: application/json" \
  -d @search_unequal.json
