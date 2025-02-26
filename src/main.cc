#include "http_server.h"
#include "index_factory.h"
#include "logger.h"
#include "raft_stuff.h"
#include "vector_database.h"

struct Node {
  int node_id;
  std::string endpoint;
  int raft_port;
  int vdb_port;
};
const Node nodes[] = {
    {},
    {1, "127.0.0.1:8081", 8081, 9091},
    {2, "127.0.0.1:8082", 8082, 9092},
    {3, "127.0.0.1:8083", 8083, 9093},
};

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <node_id>" << std::endl;
    return 1;
  }
  int node_id = std::stoi(argv[1]);
  if (node_id < 1 || node_id > 3) {
    std::cerr << "Invalid node_id, 1 <= node_id <=3" << std::endl;
    return 1;
  }
  auto [_, endpoint, raft_port, vdb_port] = nodes[node_id];

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
  std::string wal_path = "WalStore";
  VectorDatabase vector_database(db_path, wal_path);
  RaftStuff raftStuff(node_id, endpoint, raft_port);
  GlobalLogger->debug(
      "RaftStuff object created with node_id: {}, endpoint: {}, port: {}",
      node_id, endpoint, raft_port);
  vector_database.reloadDatabase();
  GlobalLogger->info("VectorDatabase initialized port: {}", vdb_port);

  HttpServer server("localhost", vdb_port, &vector_database, &raftStuff);
  GlobalLogger->info("HttpServer created");
  server.start();

  return 0;
}
