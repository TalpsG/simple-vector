#include "log_state_machine.h"
#include "constants.h"
#include "logger.h"
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

using namespace nuraft;

void log_state_machine::setVectorDatabase(VectorDatabase *vector_database) {
  vector_database_ = vector_database;
  last_committed_idx_ = vector_database->getStartIndexID();
}

ptr<buffer> log_state_machine::commit(const ulong log_idx, buffer &data) {
  std::string content(
      reinterpret_cast<const char *>(data.data() + data.pos() + sizeof(int)),
      data.size() - sizeof(int));
  GlobalLogger->debug("Commit log_idx: {}, content: {}", log_idx, content);

  rapidjson::Document json_request;
  json_request.Parse(content.c_str());
  uint64_t label = json_request[REQUEST_ID].GetUint64();

  // Update last committed index number.
  last_committed_idx_ = log_idx;

  IndexFactory::IndexType indexType =
      vector_database_->getIndexTypeFromRequest(json_request);

  vector_database_->upsert(label, json_request, indexType);
  // Return Raft log number as a return result.
  ptr<buffer> ret = buffer::alloc(sizeof(log_idx));
  buffer_serializer bs(ret);
  bs.put_u64(log_idx);
  return ret;
}

ptr<buffer> log_state_machine::pre_commit(const ulong log_idx, buffer &data) {
  std::string content(
      reinterpret_cast<const char *>(data.data() + data.pos() + sizeof(int)),
      data.size() - sizeof(int));
  GlobalLogger->debug("Pre Commit log_idx: {}, content: {}", log_idx, content);
  return nullptr;
}