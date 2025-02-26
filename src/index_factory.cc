#include "index_factory.h"
#include "filter_index.h"
#include "hnswlib_index.h"

#include <faiss/IndexFlat.h>
#include <faiss/IndexIDMap.h>

namespace {
IndexFactory globalIndexFactory;
}

IndexFactory *getGlobalIndexFactory() { return &globalIndexFactory; }

void IndexFactory::init(IndexFactory::IndexType type, int dim, int num_data,
                        IndexFactory::MetricType metric) {
  faiss::MetricType faiss_metric = (metric == IndexFactory::MetricType::L2)
                                       ? faiss::METRIC_L2
                                       : faiss::METRIC_INNER_PRODUCT;

  switch (type) {
  case IndexFactory::IndexType::FLAT:
    index_map[type] = new FaissIndex(
        new faiss::IndexIDMap(new faiss::IndexFlat(dim, faiss_metric)));
    break;
  case IndexFactory::IndexType::HNSW:
    index_map[type] = new HNSWLibIndex(dim, num_data, metric, 16, 200);
    break;
  case IndexFactory::IndexType::FILTER:
    index_map[type] = new FilterIndex();
    break;
  default:
    break;
  }
}

void *IndexFactory::getIndex(IndexType type) const {
  auto it = index_map.find(type);
  if (it != index_map.end()) {
    return it->second;
  }
  return nullptr;
}

void IndexFactory::saveIndex(const std::string &folder_path,
                             ScalarStorage &scalar_storage) {

  for (const auto &index_entry : index_map) {
    IndexType index_type = index_entry.first;
    void *index = index_entry.second;

    std::string file_path =
        folder_path + std::to_string(static_cast<int>(index_type)) + ".index";

    if (index_type == IndexType::FLAT) {
      static_cast<FaissIndex *>(index)->saveIndex(file_path);
    } else if (index_type == IndexType::HNSW) {
      static_cast<HNSWLibIndex *>(index)->saveIndex(file_path);
    } else if (index_type == IndexType::FILTER) {
      static_cast<FilterIndex *>(index)->saveIndex(scalar_storage, file_path);
    }
  }
}

void IndexFactory::loadIndex(const std::string &folder_path,
                             ScalarStorage &scalar_storage) {
  for (const auto &index_entry : index_map) {
    IndexType index_type = index_entry.first;
    void *index = index_entry.second;

    std::string file_path =
        folder_path + std::to_string(static_cast<int>(index_type)) + ".index";

    if (index_type == IndexType::FLAT) {
      static_cast<FaissIndex *>(index)->loadIndex(file_path);
    } else if (index_type == IndexType::HNSW) {
      static_cast<HNSWLibIndex *>(index)->loadIndex(file_path);
    } else if (index_type == IndexType::FILTER) {
      static_cast<FilterIndex *>(index)->loadIndex(scalar_storage, file_path);
    }
  }
}