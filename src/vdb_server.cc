#include "http_server.h"
#include "index_factory.h"
#include "logger.h"
#include "vector_database.h"

int main() {
  init_global_logger();
  set_log_level(spdlog::level::debug);

  GlobalLogger->info("Global logger initialized");

  int dim = 1;
  int num_data = 1000;
  IndexFactory *globalIndexFactory = getGlobalIndexFactory();
  globalIndexFactory->init(IndexFactory::IndexType::FLAT, dim);
  globalIndexFactory->init(IndexFactory::IndexType::HNSW, dim, num_data);
  globalIndexFactory->init(IndexFactory::IndexType::FILTER);
  GlobalLogger->info("Global IndexFactory initialized");

  std::string db_path = "ScalarStorage";
  VectorDatabase vector_database(db_path);
  GlobalLogger->info("VectorDatabase initialized");

  HttpServer server("localhost", 8080, &vector_database);
  GlobalLogger->info("HttpServer created");
  server.start();

  return 0;
}
