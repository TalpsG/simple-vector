# 持久化
索引的持久化是使用faiss/hnswlib 库自带的持久化接口实现的。
我们显式的对外提供一个http服务 snapshot，接收到这个请求时，会将索引和filter持久化，并且将lastsnapshotid写入文件。
vdb重启后，会读取lastsnapshotid，之后在重放log时，小于这个id的log不会被重放。


# todo
1. 后台定期自动持久化
2. 显式持久化时检查是否需要持久化，如果数据没变，持久化只会浪费时间。
