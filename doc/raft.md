# 分布式
多节点通过raft协议实现数据的一致性，当前实现了raft的主从关系。

每个节点有两个端口，raft_port是nuraft用来同步或选主使用的端口，http_port就是我们http的端口。

upsert会产生log，由raft_stuff调用下面的raft_server来添加日志，这个操作只能leader进行。

addNode则会修改raft集群的config，这个由nuraft库内进行同步。

## nuraft
nuraft使用需要有三个对象
1. logger记录信息的 不必须
2. logstore,存储raft 产生的log的
3. statemachine 这个是有关上层应用的状态机，当提交log时，我们需要在statemachine中对上层的数据进行修改，比如commit log.
4. state mgr 节点config相关


由于基类已经规定好了一些接口，我们直接实现这些接口就可以了。
有precommit,commit等一些接口。
比如commit,我们在commit的时候需要vdb应用commit来的wal日志就可以。

