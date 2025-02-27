#pragma once

#include "scalar_storage.h"
#include <cstdint>
#include <fstream>
#include <rapidjson/document.h>
#include <string>

class Persistence {
public:
  Persistence();
  ~Persistence();

  void init(const std::string &local_path);
  uint64_t increaseID();
  uint64_t getID() const;
  void writeWALLog(const std::string &operation_type,
                   const rapidjson::Document &json_data,
                   const std::string &version);
  void writeWALRawLog(uint64_t log_id, const std::string &operation_type,
                      const std::string &raw_data, const std::string &version);
  void readNextWALLog(std::string *operation_type,
                      rapidjson::Document *json_data);
  void takeSnapshot(ScalarStorage &scalar_storage);
  void loadSnapshot(ScalarStorage &scalar_storage);
  void saveLastSnapshotID();
  void loadLastSnapshotID();

private:
  uint64_t increaseID_;
  uint64_t lastSnapshotID_;

  std::fstream wal_log_file_;
};