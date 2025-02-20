#pragma once

#include "faiss_index.h"
#include <map>

class IndexFactory {
public:
  enum class IndexType { FLAT, HNSW, FILTER, UNKNOWN = -1 };

  enum class MetricType { L2, IP };

  void init(IndexFactory::IndexType type, int dim = 1, int num_data = 0,
            IndexFactory::MetricType metric = IndexFactory::MetricType::L2);
  void *getIndex(IndexType type) const;

private:
  std::map<IndexType, void *> index_map;
};

IndexFactory *getGlobalIndexFactory();
