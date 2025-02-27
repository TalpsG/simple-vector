#include "http_server.h"
#include "index_factory.h"
#include "logger.h"
#include "vector_database.h"

std::map<std::string, std::string> readConfigFile(const std::string &filename) {
  std::ifstream file(filename);
  std::map<std::string, std::string> config;
  if (file.is_open()) {
    std::string line;
    while (std::getline(file, line)) {
      std::stringstream ss(line);
      std::string key, value;
      std::getline(ss, key, '=');
      std::getline(ss, value);
      config[key] = value;
    }
    file.close();
  } else {
    GlobalLogger->error("Failed to open config file: {}", filename);
    throw std::runtime_error("Failed to open config file: " + filename);
  }
  return config;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <config_file>" << std::endl;
    return 1;
  }

  std::string config_file_path = argv[1];

  auto config = readConfigFile(config_file_path);

  init_global_logger();
  set_log_level(spdlog::level::debug);

  GlobalLogger->info("Global logger initialized");

  int dim = 1;
  int num_data = 100000;
  IndexFactory *globalIndexFactory = getGlobalIndexFactory();
  globalIndexFactory->init(IndexFactory::IndexType::FLAT, dim);
  globalIndexFactory->init(IndexFactory::IndexType::HNSW, dim, num_data);
  globalIndexFactory->init(IndexFactory::IndexType::FILTER);
  GlobalLogger->info("Global IndexFactory initialized");

  std::string db_path = config["db_path"];
  std::string wal_path = config["wal_path"];
  int node_id = std::stoi(config["node_id"]);
  std::string endpoint = config["endpoint"];
  int port = std::stoi(config["port"]);

  VectorDatabase vector_database(db_path, wal_path);
  vector_database.reloadDatabase();
  GlobalLogger->info("VectorDatabase initialized");

  RaftStuff raftStuff(node_id, endpoint, port, &vector_database);
  GlobalLogger->debug(
      "RaftStuff object created with node_id: {}, endpoint: {}, port: {}",
      node_id, endpoint, port);

  std::string http_server_address = config["http_server_address"];
  int http_server_port = std::stoi(config["http_server_port"]);
  HttpServer server(http_server_address, http_server_port, &vector_database,
                    &raftStuff);
  GlobalLogger->info("HttpServer created");
  server.start();

  return 0;
}
