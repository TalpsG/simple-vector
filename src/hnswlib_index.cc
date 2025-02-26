#include "hnswlib_index.h"
#include "constants.h"
#include "logger.h"
#include <iostream>
#include <vector>

HNSWLibIndex::HNSWLibIndex(int dim, int num_data,
                           IndexFactory::MetricType metric, int M,
                           int ef_construction)
    : max_elements(num_data) {
  hnswlib::SpaceInterface<float> *space;
  if (metric == IndexFactory::MetricType::L2) {
    space = new hnswlib::L2Space(dim);
  } else {
    space = new hnswlib::InnerProductSpace(dim);
  }
  this->space = space;
  index =
      new hnswlib::HierarchicalNSW<float>(space, num_data, M, ef_construction);
}

void HNSWLibIndex::insert_vectors(const std::vector<float> &data,
                                  uint64_t label) {
  index->addPoint(data.data(), static_cast<hnswlib::labeltype>(label));
}

std::pair<std::vector<long>, std::vector<float>>
HNSWLibIndex::search_vectors(const std::vector<float> &query, int k,
                             const roaring_bitmap_t *bitmap, int ef_search) {
  index->setEf(ef_search);

  RoaringBitmapIDFilter *selector = nullptr;
  if (bitmap != nullptr) {
    selector = new RoaringBitmapIDFilter(bitmap);
  }

  auto result = index->searchKnn(query.data(), k, selector);

  std::vector<long> indices;
  std::vector<float> distances;
  while (!result.empty()) {
    auto item = result.top();
    indices.push_back(item.second);
    distances.push_back(item.first);
    result.pop();
  }

  if (bitmap != nullptr) {
    delete selector;
  }

  return {indices, distances};
}

void HNSWLibIndex::saveIndex(const std::string &file_path) {
  index->saveIndex(file_path);
}

void HNSWLibIndex::loadIndex(const std::string &file_path) {
  std::ifstream file(file_path);
  if (file.good()) {
    file.close();
    index->loadIndex(file_path, space, max_elements);
  } else {
    GlobalLogger->warn("File not found: {}. Skipping loading index.",
                       file_path);
  }
}
