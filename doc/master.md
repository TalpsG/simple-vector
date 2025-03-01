# master server
记录多集群的配置信息。

比如集群中的节点个数，每个节点的端口号，状态等信息。

使用etcd在本地当作元数据的存储，也可以etcd部署到多节点，master请求etcd集群的leader来获取元数据。

可以请求masterserver 获取节点或者集群的信息。

我们使用`/instance/instanceid/nodes/nodeid`来当作节点元数据的key, value则是一个json，格式与`vdb`的`getNode`请求返回的json一样。

获取集群信息与获取节点元数据的方式类似，etcdcppapi提供了ls接口，可以获取前缀为`/instance/instanceid/nodes`的key对应的所有value，据此我们可以找到该集群的全部节点的元数据。


# update
后台有线程定期请求etcd获取节点url,然后curl到对应节点，获取该节点的信息，并更新到etcd中。

如果多次获取curl节点都失败，则视为节点异常，设置etcd中对应的status为异常。


