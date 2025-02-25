#include "http_server.h"
#include "constants.h"
#include "faiss_index.h"
#include "hnswlib_index.h"
#include "index_factory.h"
#include "logger.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

HttpServer::HttpServer(const std::string &host, int port,
                       VectorDatabase *vector_database)
    : host(host), port(port), vector_database_(vector_database) {
  server.Post("/search",
              [this](const httplib::Request &req, httplib::Response &res) {
                searchHandler(req, res);
              });

  server.Post("/insert",
              [this](const httplib::Request &req, httplib::Response &res) {
                insertHandler(req, res);
              });

  server.Post("/upsert",
              [this](const httplib::Request &req, httplib::Response &res) {
                upsertHandler(req, res);
              });

  server.Post("/query",
              [this](const httplib::Request &req, httplib::Response &res) {
                queryHandler(req, res);
              });
  server.Post(
      "/admin/snapshot",
      [this](const httplib::Request &req,
             httplib::Response &res) { 
        snapshotHandler(req, res);
      });
}

void HttpServer::start() { server.listen(host.c_str(), port); }

bool HttpServer::isRequestValid(const rapidjson::Document &json_request,
                                CheckType check_type) {
  switch (check_type) {
  case CheckType::SEARCH:
    return json_request.HasMember(REQUEST_VECTORS) &&
           json_request.HasMember(REQUEST_K) &&
           (!json_request.HasMember(REQUEST_INDEX_TYPE) ||
            json_request[REQUEST_INDEX_TYPE].IsString());
  case CheckType::INSERT:
    return json_request.HasMember(REQUEST_VECTORS) &&
           json_request.HasMember(REQUEST_ID) &&
           (!json_request.HasMember(REQUEST_INDEX_TYPE) ||
            json_request[REQUEST_INDEX_TYPE].IsString());
  case CheckType::UPSERT:
    return json_request.HasMember(REQUEST_VECTORS) &&
           json_request.HasMember(REQUEST_ID) &&
           (!json_request.HasMember(REQUEST_INDEX_TYPE) ||
            json_request[REQUEST_INDEX_TYPE].IsString());
  default:
    return false;
  }
}

IndexFactory::IndexType
HttpServer::getIndexTypeFromRequest(const rapidjson::Document &json_request) {
  if (json_request.HasMember(REQUEST_INDEX_TYPE) &&
      json_request[REQUEST_INDEX_TYPE].IsString()) {
    std::string index_type_str = json_request[REQUEST_INDEX_TYPE].GetString();
    if (index_type_str == INDEX_TYPE_FLAT) {
      return IndexFactory::IndexType::FLAT;
    } else if (index_type_str == INDEX_TYPE_HNSW) {
      return IndexFactory::IndexType::HNSW;
    }
  }
  return IndexFactory::IndexType::UNKNOWN;
}

void HttpServer::searchHandler(const httplib::Request &req,
                               httplib::Response &res) {
  GlobalLogger->debug("Received search request");

  rapidjson::Document json_request;
  json_request.Parse(req.body.c_str());

  GlobalLogger->info("Search request parameters: {}", req.body);

  if (!json_request.IsObject()) {
    GlobalLogger->error("Invalid JSON request");
    res.status = 400;
    setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR, "Invalid JSON request");
    return;
  }

  if (!isRequestValid(json_request, CheckType::SEARCH)) {
    GlobalLogger->error("Missing vectors or k parameter in the request");
    res.status = 400;
    setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR,
                         "Missing vectors or k parameter in the request");
    return;
  }

  std::vector<float> query;
  for (const auto &q : json_request[REQUEST_VECTORS].GetArray()) {
    query.push_back(q.GetFloat());
  }
  int k = json_request[REQUEST_K].GetInt();

  GlobalLogger->debug("Query parameters: k = {}", k);

  IndexFactory::IndexType indexType = getIndexTypeFromRequest(json_request);

  if (indexType == IndexFactory::IndexType::UNKNOWN) {
    GlobalLogger->error("Invalid indexType parameter in the request");
    res.status = 400;
    setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR,
                         "Invalid indexType parameter in the request");
    return;
  }

  std::pair<std::vector<long>, std::vector<float>> results =
      vector_database_->search(json_request);

  rapidjson::Document json_response;
  json_response.SetObject();
  rapidjson::Document::AllocatorType &allocator = json_response.GetAllocator();

  bool valid_results = false;
  rapidjson::Value vectors(rapidjson::kArrayType);
  rapidjson::Value distances(rapidjson::kArrayType);
  for (size_t i = 0; i < results.first.size(); ++i) {
    if (results.first[i] != -1) {
      valid_results = true;
      vectors.PushBack(results.first[i], allocator);
      distances.PushBack(results.second[i], allocator);
    }
  }

  if (valid_results) {
    json_response.AddMember(RESPONSE_VECTORS, vectors, allocator);
    json_response.AddMember(RESPONSE_DISTANCES, distances, allocator);
  }

  json_response.AddMember(RESPONSE_RETCODE, RESPONSE_RETCODE_SUCCESS,
                          allocator);
  setJsonResponse(json_response, res);
}

void HttpServer::insertHandler(const httplib::Request &req,
                               httplib::Response &res) {
  GlobalLogger->debug("Received insert request");

  rapidjson::Document json_request;
  json_request.Parse(req.body.c_str());

  GlobalLogger->info("Insert request parameters: {}", req.body);

  if (!json_request.IsObject()) {
    GlobalLogger->error("Invalid JSON request");
    res.status = 400;
    setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR, "Invalid JSON request");
    return;
  }

  if (!isRequestValid(json_request, CheckType::INSERT)) {
    GlobalLogger->error("Missing vectors or id parameter in the request");
    res.status = 400;
    setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR,
                         "Missing vectors or k parameter in the request");
    return;
  }

  std::vector<float> data;
  for (const auto &d : json_request[REQUEST_VECTORS].GetArray()) {
    data.push_back(d.GetFloat());
  }
  uint64_t label = json_request[REQUEST_ID].GetUint64();

  GlobalLogger->debug("Insert parameters: label = {}", label);

  IndexFactory::IndexType indexType = getIndexTypeFromRequest(json_request);

  if (indexType == IndexFactory::IndexType::UNKNOWN) {
    GlobalLogger->error("Invalid indexType parameter in the request");
    res.status = 400;
    setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR,
                         "Invalid indexType parameter in the request");
    return;
  }

  void *index = getGlobalIndexFactory()->getIndex(indexType);

  switch (indexType) {
  case IndexFactory::IndexType::FLAT: {
    FaissIndex *faissIndex = static_cast<FaissIndex *>(index);
    faissIndex->insert_vectors(data, label);
    break;
  }
  case IndexFactory::IndexType::HNSW: {
    HNSWLibIndex *hnswIndex = static_cast<HNSWLibIndex *>(index);
    hnswIndex->insert_vectors(data, label);
    break;
  }

  default:
    break;
  }

  rapidjson::Document json_response;
  json_response.SetObject();
  rapidjson::Document::AllocatorType &allocator = json_response.GetAllocator();

  json_response.AddMember(RESPONSE_RETCODE, RESPONSE_RETCODE_SUCCESS,
                          allocator);

  setJsonResponse(json_response, res);
}

void HttpServer::upsertHandler(const httplib::Request &req,
                               httplib::Response &res) {
  GlobalLogger->debug("Received upsert request");

  rapidjson::Document json_request;
  json_request.Parse(req.body.c_str());

  if (!json_request.IsObject()) {
    GlobalLogger->error("Invalid JSON request");
    res.status = 400;
    setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR, "Invalid JSON request");
    return;
  }

  if (!isRequestValid(json_request, CheckType::UPSERT)) {
    GlobalLogger->error("Missing vectors or id parameter in the request");
    res.status = 400;
    setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR,
                         "Missing vectors or id parameter in the request");
    return;
  }

  uint64_t label = json_request[REQUEST_ID].GetUint64();

  IndexFactory::IndexType indexType = getIndexTypeFromRequest(json_request);
  // wal
  vector_database_->writeWALLog("upsert", json_request);

  vector_database_->upsert(label, json_request, indexType);

  rapidjson::Document json_response;
  json_response.SetObject();
  rapidjson::Document::AllocatorType &response_allocator =
      json_response.GetAllocator();

  json_response.AddMember(RESPONSE_RETCODE, RESPONSE_RETCODE_SUCCESS,
                          response_allocator);
  setJsonResponse(json_response, res);
}

void HttpServer::queryHandler(const httplib::Request &req,
                              httplib::Response &res) {
  GlobalLogger->debug("Received query request");

  rapidjson::Document json_request;
  json_request.Parse(req.body.c_str());

  if (!json_request.IsObject()) {
    GlobalLogger->error("Invalid JSON request");
    res.status = 400;
    setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR, "Invalid JSON request");
    return;
  }

  uint64_t id = json_request[REQUEST_ID].GetUint64();

  rapidjson::Document json_data = vector_database_->query(id);

  rapidjson::Document json_response;
  json_response.SetObject();
  rapidjson::Document::AllocatorType &allocator = json_response.GetAllocator();

  if (!json_data.IsNull()) {
    for (auto it = json_data.MemberBegin(); it != json_data.MemberEnd(); ++it) {
      json_response.AddMember(it->name, it->value, allocator);
    }
  }

  json_response.AddMember(RESPONSE_RETCODE, RESPONSE_RETCODE_SUCCESS,
                          allocator);
  setJsonResponse(json_response, res);
}

void HttpServer::setJsonResponse(const rapidjson::Document &json_response,
                                 httplib::Response &res) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  json_response.Accept(writer);
  res.set_content(buffer.GetString(), RESPONSE_CONTENT_TYPE_JSON);
}

void HttpServer::setErrorJsonResponse(httplib::Response &res, int error_code,
                                      const std::string &errorMsg) {
  rapidjson::Document json_response;
  json_response.SetObject();
  rapidjson::Document::AllocatorType &allocator = json_response.GetAllocator();
  json_response.AddMember(RESPONSE_RETCODE, error_code, allocator);
  json_response.AddMember(RESPONSE_ERROR_MSG,
                          rapidjson::StringRef(errorMsg.c_str()), allocator);
  setJsonResponse(json_response, res);
}

void HttpServer::snapshotHandler(const httplib::Request &req,
                                 httplib::Response &res) {
  GlobalLogger->debug("Received snapshot request");

  vector_database_->takeSnapshot(); 

  rapidjson::Document json_response;
  json_response.SetObject();
  rapidjson::Document::AllocatorType &allocator = json_response.GetAllocator();

  json_response.AddMember(RESPONSE_RETCODE, RESPONSE_RETCODE_SUCCESS,
                          allocator);
  setJsonResponse(json_response, res);
}
