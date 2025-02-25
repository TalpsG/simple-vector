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
  void put(const std::string &key, const std::string &value);
  std::string get(const std::string &key);

private:
  rocksdb::DB *db_;
};
