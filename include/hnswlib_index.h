#pragma once

#include "hnswlib/hnswlib.h"
#include "index_factory.h"
#include "roaring/roaring.h"
#include <vector>

class HNSWLibIndex {
public:
  HNSWLibIndex(int dim, int num_data, IndexFactory::MetricType metric,
               int M = 16, int ef_construction = 200);
  void insert_vectors(const std::vector<float> &data, uint64_t label);

  std::pair<std::vector<long>, std::vector<float>>
  search_vectors(const std::vector<float> &query, int k,
                 const roaring_bitmap_t *bitmap = nullptr, int ef_search = 50);
  void saveIndex(const std::string &file_path);
  void loadIndex(const std::string &file_path);

  class RoaringBitmapIDFilter : public hnswlib::BaseFilterFunctor {
  public:
    RoaringBitmapIDFilter(const roaring_bitmap_t *bitmap) : bitmap_(bitmap) {}

    bool operator()(hnswlib::labeltype label) {
      return roaring_bitmap_contains(bitmap_, static_cast<uint32_t>(label));
    }

  private:
    const roaring_bitmap_t *bitmap_;
  };

private:
  hnswlib::HierarchicalNSW<float> *index;
  hnswlib::SpaceInterface<float> *space;
  size_t max_elements;
};
