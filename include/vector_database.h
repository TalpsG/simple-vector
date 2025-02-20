#pragma once

#include "index_factory.h"
#include "scalar_storage.h"
#include <rapidjson/document.h>
#include <string>
#include <vector>

class VectorDatabase {
public:
  VectorDatabase(const std::string &db_path);

  void upsert(uint64_t id, const rapidjson::Document &data,
              IndexFactory::IndexType index_type);
  rapidjson::Document query(uint64_t id);

  std::pair<std::vector<long>, std::vector<float>>
  search(const rapidjson::Document &json_request);

private:
  ScalarStorage scalar_storage_;
};
