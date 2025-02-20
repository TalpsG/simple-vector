
#include "HttpServer.h"
#include "FaissIndex.h"
#include "IndexFactory.h"
#include "constants.h"
#include "logger.h"
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

HttpServer::HttpServer(const std::string &host, int port)
    : host(host), port(port) {
  server.Post("/search",
              [this](const httplib::Request &req, httplib::Response &res) {
                searchHandler(req, res);
              });

  server.Post("/insert",
              [this](const httplib::Request &req, httplib::Response &res) {
                insertHandler(req, res);
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
  default:
    return false;
  }
}

IndexFactory::IndexType
HttpServer::getIndexTypeFromRequest(const rapidjson::Document &json_request) {
  // get index type
  if (json_request.HasMember(REQUEST_INDEX_TYPE)) {
    std::string index_type_str = json_request[REQUEST_INDEX_TYPE].GetString();
    if (index_type_str == "FLAT") {
      return IndexFactory::IndexType::FLAT;
    }
  }
  return IndexFactory::IndexType::UNKNOWN;
}

void HttpServer::searchHandler(const httplib::Request &req,
                               httplib::Response &res) {
  GlobalLogger->debug("Received search request");

  // parse json request
  rapidjson::Document json_request;
  json_request.Parse(req.body.c_str());

  // out user params to log
  GlobalLogger->info("Search request parameters: {}", req.body);

  // is json request valid?
  if (!json_request.IsObject()) {
    GlobalLogger->error("Invalid JSON request");
    res.status = 400;
    setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR, "Invalid JSON request");
    return;
  }

  // check json request have params we need
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

  // indextype
  IndexFactory::IndexType indexType = getIndexTypeFromRequest(json_request);

  if (indexType == IndexFactory::IndexType::UNKNOWN) {
    GlobalLogger->error("Invalid indexType parameter in the request");
    res.status = 400;
    setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR,
                         "Invalid indexType parameter in the request");
    return;
  }

  // get index
  void *index = getGlobalIndexFactory()->getIndex(indexType);

  std::pair<std::vector<long>, std::vector<float>> results;
  switch (indexType) {
  case IndexFactory::IndexType::FLAT: {
    FaissIndex *faissIndex = static_cast<FaissIndex *>(index);
    results = faissIndex->search_vectors(query, k);
    break;
  }
  default:
    break;
  }

  rapidjson::Document json_response;
  json_response.SetObject();
  rapidjson::Document::AllocatorType &allocator = json_response.GetAllocator();

  // check result
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

  // parse json request
  rapidjson::Document json_request;
  json_request.Parse(req.body.c_str());

  GlobalLogger->info("Insert request parameters: {}", req.body);

  if (!json_request.IsObject()) {
    GlobalLogger->error("Invalid JSON request");
    res.status = 400;
    setErrorJsonResponse(res, RESPONSE_RETCODE_ERROR, "Invalid JSON request");
    return;
  }

  if (!isRequestValid(json_request,
                      CheckType::INSERT)) { // 添加对isRequestValid的调用
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
  uint64_t label = json_request[REQUEST_ID].GetUint64(); // 使用宏定义

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

void HttpServer::setJsonResponse(const rapidjson::Document &json_response,
                                 httplib::Response &res) {
  rapidjson::StringBuffer buffer;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
  json_response.Accept(writer);
  res.set_content(buffer.GetString(), RESPONSE_CONTENT_TYPE_JSON); // 使用宏定义
}

void HttpServer::setErrorJsonResponse(httplib::Response &res, int error_code,
                                      const std::string &errorMsg) {
  rapidjson::Document json_response;
  json_response.SetObject();
  rapidjson::Document::AllocatorType &allocator = json_response.GetAllocator();
  json_response.AddMember(RESPONSE_RETCODE, error_code, allocator);
  json_response.AddMember(RESPONSE_ERROR_MSG,
                          rapidjson::StringRef(errorMsg.c_str()),
                          allocator); // 使用宏定义
  setJsonResponse(json_response, res);
}
