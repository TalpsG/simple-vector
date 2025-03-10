#include "faiss_index.h"
#include "constants.h"
#include "logger.h"
#include <faiss/IndexFlat.h>
#include <faiss/IndexIDMap.h>
#include <faiss/index_io.h>
#include <fstream>
#include <vector>

bool RoaringBitmapIDSelector::is_member(int64_t id) const {
  return roaring_bitmap_contains(bitmap_, static_cast<uint32_t>(id));
}

FaissIndex::FaissIndex(faiss::Index *index) : index(index) {}

void FaissIndex::insert_vectors(const std::vector<float> &data,
                                uint64_t label) {
  long id = static_cast<long>(label);
  index->add_with_ids(1, data.data(), &id);
}

void FaissIndex::remove_vectors(const std::vector<long> &ids) {
  faiss::IndexIDMap *id_map = dynamic_cast<faiss::IndexIDMap *>(index);
  if (id_map) {
    faiss::IDSelectorBatch selector(ids.size(), ids.data());
    id_map->remove_ids(selector);
  } else {
    throw std::runtime_error("Underlying Faiss index is not an IndexIDMap");
  }
}

std::pair<std::vector<long>, std::vector<float>>
FaissIndex::search_vectors(const std::vector<float> &query, int k,
                           const roaring_bitmap_t *bitmap) {
  int dim = index->d;
  int num_queries = query.size() / dim;
  std::vector<long> indices(num_queries * k);
  std::vector<float> distances(num_queries * k);

  faiss::SearchParameters search_params;
  RoaringBitmapIDSelector selector(bitmap);
  if (bitmap != nullptr) {
    search_params.sel = &selector;
  }

  index->search(num_queries, query.data(), k, distances.data(), indices.data(),
                &search_params);

  GlobalLogger->debug("Retrieved values:");
  for (size_t i = 0; i < indices.size(); ++i) {
    if (indices[i] != -1) {
      GlobalLogger->debug("ID: {}, Distance: {}", indices[i], distances[i]);
    } else {
      GlobalLogger->debug("No specific value found");
    }
  }
  return {indices, distances};
}
void FaissIndex::saveIndex(const std::string &file_path) {
  faiss::write_index(index, file_path.c_str());
}

void FaissIndex::loadIndex(const std::string &file_path) {
  std::ifstream file(file_path);
  if (file.good()) {
    file.close();
    if (index != nullptr) {
      delete index;
    }
    index = faiss::read_index(file_path.c_str());
  } else {
    GlobalLogger->warn("File not found: {}. Skipping loading index.",
                       file_path);
  }
}
