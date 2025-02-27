curl -X POST -H "Content-Type: application/json" -d '{"nodeId":2,"endpoint":"127.0.0.1:8082"}'  http://localhost:9091/admin/addFollower

curl -X GET -H "Content-Type: application/json" -d '{}'  http://localhost:9091/admin/listNode
