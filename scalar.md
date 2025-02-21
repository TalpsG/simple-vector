# scalar 
当前采取的方式是直接存放rapidjson的Document对象的二进制数据，不太好。

后续可以将scalar数据存放到pg或者一些列存数据库当中.

scalar之上可以构建索引，比如tantivy之类的。

对文本使用tantivy的索引，对数字则可以添加btree之类的索引。


