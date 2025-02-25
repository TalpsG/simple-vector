#include "persistence.h"
#include "logger.h"
#include <cassert>
#include <cerrno>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <unistd.h>

Persistence::Persistence() : increaseID_(1) {}

Persistence::~Persistence() {
  if (wal_fd_ != -1) {
    fsync(wal_fd_);
    ::close(wal_fd_);
  }
}

void Persistence::init(const std::string &local_path, bool flush) {
  need_flush_ = flush;
  wal_fd_ = ::open(local_path.c_str(), O_RDWR | O_APPEND | O_CREAT,
                   S_IRUSR | S_IWUSR);
  if (wal_fd_ == -1) {
    GlobalLogger->error(
        "an error occurred while writing the wal log entry. reason: {}",
        std::strerror(errno));
    exit(-1);
  }
}

uint64_t Persistence::increaseID() {
  increaseID_++;
  return increaseID_;
}

uint64_t Persistence::getID() const { return increaseID_; }

void Persistence::writeWALLog(const std::string &operation_type,
                              const rapidjson::Document &json_data,
                              const std::string &version) {
  uint64_t log_id = increaseID();

  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  json_data.Accept(writer);

  auto log = std::to_string(log_id) + "|" + version + "|" + operation_type +
             "|" + buffer.GetString() + "\n";
  uint64_t log_size = log.size();
  auto write_size = ::write(wal_fd_, &log_size, sizeof(uint64_t));
  if (write_size == -1) {
    GlobalLogger->error(
        "An error occurred while writing the WAL log entry. Reason: {}",
        std::strerror(errno));
  }
  write_size = ::write(wal_fd_, log.c_str(), log.size());

  if (write_size == -1) {
    GlobalLogger->error(
        "An error occurred while writing the WAL log entry. Reason: {}",
        std::strerror(errno));
  } else {
    GlobalLogger->debug("Wrote WAL log entry: log_id={}, version={}, "
                        "operation_type={}, json_data_str={}",
                        log_id, version, operation_type, buffer.GetString());
    if (need_flush_) {
      ::fsync(wal_fd_);
    }
  }
}

void Persistence::readNextWALLog(std::string *operation_type,
                                 rapidjson::Document *json_data) {
  GlobalLogger->debug("Reading next WAL log entry");

  char buf[sizeof(uint64_t)];
  ::lseek(wal_fd_, read_offset_, SEEK_SET);
  auto read_size = read(wal_fd_, buf, sizeof(uint64_t));
  if (read_size != 0 && read_size != -1) {
    read_offset_ += read_size;
    assert(read_size == sizeof(uint64_t));
    uint64_t log_size;
    memcpy(&log_size, buf, sizeof(uint64_t));
    auto log_buf = std::unique_ptr<char>(new char[log_size + 1]);
    read_size = read(wal_fd_, log_buf.get(), log_size);
    assert(read_size == log_size);
    log_buf.get()[log_size] = '\0';
    if (read_size == -1) {
      GlobalLogger->error(
          "An error occurred while reading the WAL log entry. Reason: {}",
          std::strerror(errno));
      ::lseek(wal_fd_, 0, SEEK_END);
      return;
    }

    read_offset_ += read_size;
    std::istringstream iss(log_buf.get());
    std::string log_id_str, version, json_data_str;

    std::getline(iss, log_id_str, '|');
    std::getline(iss, version, '|');
    std::getline(iss, *operation_type, '|');
    std::getline(iss, json_data_str, '|');

    uint64_t log_id = std::stoull(log_id_str);
    if (log_id > increaseID_) {
      increaseID_ = log_id;
    }

    json_data->Parse(json_data_str.c_str());

    GlobalLogger->debug(
        "Read WAL log entry: log_id={}, operation_type={}, json_data_str={}",
        log_id_str, *operation_type, json_data_str);
  } else {
    GlobalLogger->debug("No more WAL log entries to read");
  }
  ::lseek(wal_fd_, 0, SEEK_END);
}
