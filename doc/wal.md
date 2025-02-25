# wal
wal日志，在update前将日志写入walstore文件内。

log的格式为 
`logsize(8byte) "logid|version|optype|json_str`

数据在重启时会将walstore当中的所有日志都重放一遍.

# 问题
每次重启都重放，没有必要，数据库应该有一个持久化的状态，重放其状态包含之外的日志即可。


# todo:
1. log 二进制化，现在除了logsize以外，后面存放的是字符串，读取太慢，占用空间也大。
2. snapshot
3. 切换log，每一次snapshot后重写一个新的log文件，定期删除其他log文件。
