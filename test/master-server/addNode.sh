curl -X POST localhost:6060/addNode \
  -H "Content-Type: application/json" \
  -d '{"instanceId":"1","nodeId":"1","url":"0.0.0.0:9091","status":0,"role":1}' \

curl -X POST localhost:6060/addNode \
  -H "Content-Type: application/json" \
  -d '{"instanceId":"1","nodeId":"2","url":"0.0.0.0:9092","status":0,"role":0}' \

curl -X POST localhost:6060/addNode \
  -H "Content-Type: application/json" \
  -d '{"instanceId":"1","nodeId":"3","url":"0.0.0.0:9093","status":0,"role":0}' \

curl -X GET localhost:6060/getInstance?instanceId=instance1
