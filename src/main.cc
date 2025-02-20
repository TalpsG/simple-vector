#include "HttpServer.h"
#include "IndexFactory.h"
#include "logger.h"

int main() {
  init_global_logger();
  set_log_level(spdlog::level::debug);

  GlobalLogger->info("Global logger initialized");

  int dim = 1;
  IndexFactory *globalIndexFactory = getGlobalIndexFactory();
  globalIndexFactory->init(IndexFactory::IndexType::FLAT, dim);
  GlobalLogger->info("Global IndexFactory initialized");

  HttpServer server("localhost", 8080);
  GlobalLogger->info("HttpServer created");
  server.start();

  return 0;
}
