#pragma once

#include "in_mem_state_mgr.h"
#include "log_state_machine.h"
#include "logger.h"
#include "vector_database.h"
#include <libnuraft/asio_service.hxx>

class RaftStuff {
public:
  RaftStuff(int node_id, const std::string &endpoint, int port,
            VectorDatabase *vector_database);

  void Init();
  ptr<cmd_result<ptr<buffer>>> addSrv(int srv_id,
                                      const std::string &srv_endpoint);
  void enableElectionTimeout(int lower_bound, int upper_bound);
  bool isLeader() const;
  std::vector<
      std::tuple<int, std::string, std::string, nuraft::ulong, nuraft::ulong>>
  getAllNodesInfo() const;
  std::tuple<int, std::string, std::string, nuraft::ulong, nuraft::ulong>
  getCurrentNodesInfo() const;
  std::string getNodeStatus(int node_id) const;
  ptr<cmd_result<ptr<buffer>>> appendEntries(const std::string &entry);

private:
  int node_id;
  std::string raft_address_;
  std::string endpoint;
  ptr<state_mgr> smgr_;
  ptr<state_machine> sm_;
  int port_;
  int http_port_;
  raft_launcher launcher_;
  ptr<raft_server> raft_instance_;
  VectorDatabase *vector_database_;
};
