#pragma once

constexpr char LOGGER_NAME[] = "GlobalLogger";

constexpr char RESPONSE_VECTORS[] = "vectors";
constexpr char RESPONSE_DISTANCES[] = "distances";

constexpr char REQUEST_VECTORS[] = "vectors";
constexpr char REQUEST_K[] = "k";
constexpr char REQUEST_ID[] = "id";
constexpr char REQUEST_INDEX_TYPE[] = "indexType";

constexpr char RESPONSE_RETCODE[] = "retCode";
constexpr char RESPONSE_RETCODE_SUCCESS = 0;
constexpr char RESPONSE_RETCODE_ERROR = -1;

constexpr char RESPONSE_ERROR_MSG[] = "errorMsg";

constexpr char RESPONSE_CONTENT_TYPE_JSON[] = "application/json";

constexpr char INDEX_TYPE_FLAT[] = "FLAT";
constexpr char INDEX_TYPE_HNSW[] = "HNSW";

constexpr char REQUEST_FILTER[] = "filter";
constexpr char REQUEST_FILTER_FIELD[] = "fieldName";
constexpr char REQUEST_FILTER_FIELD_VALUE[] = "fieldValue";
constexpr char REQUEST_FILTER_OP[] = "op";

constexpr char VERSION[] = "1.0";