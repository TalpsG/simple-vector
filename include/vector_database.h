#pragma once

#include "index_factory.h"
#include "persistence.h"
#include "scalar_storage.h"
#include <rapidjson/document.h>
#include <string>
#include <vector>

class VectorDatabase {
public:
  VectorDatabase(const std::string &db_path, const std::string &wal_path);

  void upsert(uint64_t id, const rapidjson::Document &data,
              IndexFactory::IndexType index_type);
  rapidjson::Document query(uint64_t id);

  std::pair<std::vector<long>, std::vector<float>>
  search(const rapidjson::Document &json_request);

  void reloadDatabase();
  void writeWALLog(const std::string &operation_type,
                   const rapidjson::Document &json_data);
  IndexFactory::IndexType
  getIndexTypeFromRequest(const rapidjson::Document &json_request);

  void takeSnapshot();

private:
  ScalarStorage scalar_storage_;
  Persistence persistence_;
};
