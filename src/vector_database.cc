#include "vector_database.h"
#include "constants.h"
#include "faiss_index.h"
#include "filter_index.h"
#include "hnswlib_index.h"
#include "index_factory.h"
#include "logger.h"
#include "scalar_storage.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <vector>

VectorDatabase::VectorDatabase(const std::string &db_path)
    : scalar_storage_(db_path) {}

void VectorDatabase::upsert(uint64_t id, const rapidjson::Document &data,
                            IndexFactory::IndexType index_type) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  data.Accept(writer);
  GlobalLogger->info("Upsert data: {}", buffer.GetString());

  rapidjson::Document existingData;
  try {
    existingData = scalar_storage_.get_scalar(id);
  } catch (const std::runtime_error &e) {
  }

  if (existingData.IsObject()) {
    GlobalLogger->debug("try remove old index");
    std::vector<float> existingVector(existingData["vectors"].Size());
    for (rapidjson::SizeType i = 0; i < existingData["vectors"].Size(); ++i) {
      existingVector[i] = existingData["vectors"][i].GetFloat();
    }

    void *index = getGlobalIndexFactory()->getIndex(index_type);
    switch (index_type) {
    case IndexFactory::IndexType::FLAT: {
      FaissIndex *faiss_index = static_cast<FaissIndex *>(index);
      faiss_index->remove_vectors({static_cast<long>(id)});
      break;
    }
    case IndexFactory::IndexType::HNSW: {
      HNSWLibIndex *hnsw_index = static_cast<HNSWLibIndex *>(index);
      break;
    }
    default:
      break;
    }
  }

  std::vector<float> newVector(data["vectors"].Size());
  for (rapidjson::SizeType i = 0; i < data["vectors"].Size(); ++i) {
    newVector[i] = data["vectors"][i].GetFloat();
  }

  GlobalLogger->debug("try add new index");

  void *index = getGlobalIndexFactory()->getIndex(index_type);
  switch (index_type) {
  case IndexFactory::IndexType::FLAT: {
    FaissIndex *faiss_index = static_cast<FaissIndex *>(index);
    faiss_index->insert_vectors(newVector, id);
    break;
  }
  case IndexFactory::IndexType::HNSW: {
    HNSWLibIndex *hnsw_index = static_cast<HNSWLibIndex *>(index);
    hnsw_index->insert_vectors(newVector, id);
    break;
  }
  default:
    break;
  }

  GlobalLogger->debug("try add new filter");
  FilterIndex *filter_index = static_cast<FilterIndex *>(
      getGlobalIndexFactory()->getIndex(IndexFactory::IndexType::FILTER));
  for (auto it = data.MemberBegin(); it != data.MemberEnd(); ++it) {
    std::string field_name = it->name.GetString();
    GlobalLogger->debug("try filter member {} {}", it->value.IsInt(),
                        field_name);
    if (it->value.IsInt() && field_name != "id") {
      int64_t field_value = it->value.GetInt64();

      int64_t *old_field_value_p = nullptr;
      if (existingData.IsObject()) {
        old_field_value_p = (int64_t *)malloc(sizeof(int64_t));
        *old_field_value_p = existingData[field_name.c_str()].GetInt64();
      }

      filter_index->updateIntFieldFilter(field_name, old_field_value_p,
                                         field_value, id);
      if (old_field_value_p) {
        delete old_field_value_p;
      }
    }
  }

  scalar_storage_.insert_scalar(id, data);
}

rapidjson::Document VectorDatabase::query(uint64_t id) {
  return scalar_storage_.get_scalar(id);
}

std::pair<std::vector<long>, std::vector<float>>
VectorDatabase::search(const rapidjson::Document &json_request) {
  std::vector<float> query;
  for (const auto &q : json_request[REQUEST_VECTORS].GetArray()) {
    query.push_back(q.GetFloat());
  }
  int k = json_request[REQUEST_K].GetInt();

  IndexFactory::IndexType indexType = IndexFactory::IndexType::UNKNOWN;
  if (json_request.HasMember(REQUEST_INDEX_TYPE) &&
      json_request[REQUEST_INDEX_TYPE].IsString()) {
    std::string index_type_str = json_request[REQUEST_INDEX_TYPE].GetString();
    if (index_type_str == INDEX_TYPE_FLAT) {
      indexType = IndexFactory::IndexType::FLAT;
    } else if (index_type_str == INDEX_TYPE_HNSW) {
      indexType = IndexFactory::IndexType::HNSW;
    }
  }

  roaring_bitmap_t *filter_bitmap = nullptr;
  if (json_request.HasMember(REQUEST_FILTER) &&
      json_request[REQUEST_FILTER].IsObject()) {
    const auto &filter = json_request[REQUEST_FILTER];
    std::string fieldName = filter[REQUEST_FILTER_FIELD].GetString();
    std::string op_str = filter[REQUEST_FILTER_OP].GetString();
    int64_t value = filter[REQUEST_FILTER_FIELD_VALUE].GetInt64();

    FilterIndex::Operation op = (op_str == "=")
                                    ? FilterIndex::Operation::EQUAL
                                    : FilterIndex::Operation::NOT_EQUAL;

    FilterIndex *filter_index = static_cast<FilterIndex *>(
        getGlobalIndexFactory()->getIndex(IndexFactory::IndexType::FILTER));

    filter_bitmap = roaring_bitmap_create();
    filter_index->getIntFieldFilterBitmap(fieldName, op, value, filter_bitmap);
  }

  void *index = getGlobalIndexFactory()->getIndex(indexType);

  std::pair<std::vector<long>, std::vector<float>> results;
  switch (indexType) {
  case IndexFactory::IndexType::FLAT: {
    FaissIndex *faissIndex = static_cast<FaissIndex *>(index);
    results = faissIndex->search_vectors(query, k, filter_bitmap);
    break;
  }
  case IndexFactory::IndexType::HNSW: {
    HNSWLibIndex *hnswIndex = static_cast<HNSWLibIndex *>(index);
    results = hnswIndex->search_vectors(query, k, filter_bitmap);
    break;
  }
  default:
    break;
  }
  if (filter_bitmap != nullptr) {
    delete filter_bitmap;
  }
  return results;
}
