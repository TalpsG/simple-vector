# 分布式
多节点通过raft协议实现数据的一致性，当前实现了raft的主从关系。

每个节点有两个端口，raft_port是nuraft用来同步或选主使用的端口，vdb_port就是我们http的端口。

# setleader
setleader 通过设置raft_server的选举超时来进行，每个节点启动时选举超时都很大，设置其选举超时为几百毫秒。
这样该节点后续会开始选举，从而当上leader.

# listNode
调用raft_server的add_srv即可，注意必须在leader节点调用该函数。