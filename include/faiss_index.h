#pragma once

#include "faiss/impl/IDSelector.h"
#include "roaring/roaring.h"
#include <faiss/Index.h>
#include <faiss/utils/utils.h>
#include <vector>

struct RoaringBitmapIDSelector : faiss::IDSelector {
  RoaringBitmapIDSelector(const roaring_bitmap_t *bitmap) : bitmap_(bitmap) {}

  bool is_member(int64_t id) const final;

  ~RoaringBitmapIDSelector() override {}

  const roaring_bitmap_t *bitmap_;
};

class FaissIndex {
public:
  FaissIndex(faiss::Index *index);
  void insert_vectors(const std::vector<float> &data, uint64_t label);
  void remove_vectors(const std::vector<long> &ids);
  std::pair<std::vector<long>, std::vector<float>>
  search_vectors(const std::vector<float> &query, int k,
                 const roaring_bitmap_t *bitmap = nullptr);
  void saveIndex(const std::string &file_path);
  void loadIndex(const std::string &file_path);

private:
  faiss::Index *index;
};
