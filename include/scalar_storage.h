#pragma once

#include <rapidjson/document.h>
#include <rocksdb/db.h>
#include <string>
#include <vector>

class ScalarStorage {
public:
  ScalarStorage(const std::string &db_path);

  ~ScalarStorage();

  void insert_scalar(uint64_t id, const rapidjson::Document &data);

  rapidjson::Document get_scalar(uint64_t id);

private:
  rocksdb::DB *db_;
};
