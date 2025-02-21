# filter
在最初我们可以指定某个field,为其创建filter,目前只支持int类型的equal/unequal filter.
后续插入数据时会查看插入的数据是否有带有filter的域，如果有，则需要将该数据id插入到对应的equal/unequal filter当中。

filter结构实际上是 `<fieldname,map<fieldValue,filter>>`的结构，fieldname 对应了一组位图，不同的fieldvalue对应了不同的filter,filter内部则是有filter_op和bitmap组成。
filter_op是该filter的类型，而bitmap则表示了对应位是否需要被过滤。

后续可以采取对标量数据建立索引的方式，比如btree索引，然后进行查询。

## mivlus的实现
mivlus内部可以对不同类型的标量数据建立不同的索引。

比如bool int等类型建立倒牌索引inverted，tantivy实现。

对于array类型则可以建立 复合索引。

对于json则可以建立json的索引，以便检索json内部数据。

filter的实质操作是根据不同的类型和数据条件，对标量数据处理得到对应的位图，然后把位图传给index，告知index 哪些数据是我不要的。

milvus内部对标量数据的存储是用parquet存的。







