#include "raft_stuff.h"
#include "raft_logger_wrapper.h"
#include <libnuraft/raft_params.hxx>

RaftStuff::RaftStuff(int node_id, const std::string &endpoint, int port,
                     VectorDatabase *vector_database)
    : node_id(node_id), endpoint(endpoint), port_(port),
      vector_database_(vector_database) {
  Init();
}

void RaftStuff::Init() {
  smgr_ = cs_new<inmem_state_mgr>(node_id, endpoint, vector_database_);
  sm_ = cs_new<log_state_machine>();

  ptr<log_state_machine> log_sm =
      std::dynamic_pointer_cast<log_state_machine>(sm_);

  log_sm->setVectorDatabase(vector_database_);

  asio_service::options asio_opt;
  asio_opt.thread_pool_size_ = 1;

  raft_params params;
  // params.election_timeout_lower_bound_ = 100000000;
  // params.election_timeout_upper_bound_ = 200000000;
  params.election_timeout_lower_bound_ = 200;
  params.election_timeout_upper_bound_ = 400;
  params.heart_beat_interval_ = 100;
  params.client_req_timeout_ = 3000;
  params.return_method_ = nuraft::raft_params::blocking;
  params.snapshot_block_size_ = 5;

  // Logger.
  std::string log_file_name = "./srv.log";
  ptr<logger_wrapper> log_wrap = cs_new<logger_wrapper>(log_file_name);

  raft_instance_ =
      launcher_.init(sm_, smgr_, log_wrap, port_, asio_opt, params);
  GlobalLogger->debug(
      "RaftStuff initialized with node_id: {}, endpoint: {}, port: {}", node_id,
      endpoint, port_);
}

ptr<cmd_result<ptr<buffer>>>
RaftStuff::addSrv(int srv_id, const std::string &srv_endpoint) {
  ptr<srv_config> peer_srv_conf = cs_new<srv_config>(srv_id, srv_endpoint);
  GlobalLogger->debug("Adding server with srv_id: {}, srv_endpoint: {}", srv_id,
                      srv_endpoint);
  return raft_instance_->add_srv(*peer_srv_conf);
}

ptr<cmd_result<ptr<buffer>>>
RaftStuff::appendEntries(const std::string &entry) {
  if (!raft_instance_ || !raft_instance_->is_leader()) {
    if (!raft_instance_) {
      GlobalLogger->debug(
          "Cannot append entries: Raft instance is not available");
    } else {
      GlobalLogger->debug(
          "Cannot append entries: Current node is not the leader");
    }
    return nullptr;
  }

  size_t total_size = sizeof(int) + entry.size();

  GlobalLogger->debug("Total size of entry: {}", total_size);

  ptr<buffer> log_entry_buffer = buffer::alloc(total_size);
  buffer_serializer bs_log(log_entry_buffer);

  bs_log.put_str(entry);

  GlobalLogger->debug("Created log_entry_buffer at address: {}",
                      static_cast<const void *>(log_entry_buffer.get()));

  GlobalLogger->debug("Appending entry to Raft instance");

  return raft_instance_->append_entries({log_entry_buffer});
}

void RaftStuff::enableElectionTimeout(int lower_bound, int upper_bound) {
  if (raft_instance_) {
    raft_params params = raft_instance_->get_current_params();
    params.election_timeout_lower_bound_ = lower_bound;
    params.election_timeout_upper_bound_ = upper_bound;
    raft_instance_->update_params(params);
  }
}

bool RaftStuff::isLeader() const {
  if (!raft_instance_) {
    return false;
  }
  return raft_instance_->is_leader();
}

std::vector<
    std::tuple<int, std::string, std::string, nuraft::ulong, nuraft::ulong>>
RaftStuff::getAllNodesInfo() const {
  std::vector<
      std::tuple<int, std::string, std::string, nuraft::ulong, nuraft::ulong>>
      nodes_info;

  if (!raft_instance_) {
    return nodes_info;
  }

  auto config = raft_instance_->get_config();
  if (!config) {
    return nodes_info;
  }

  auto servers = config->get_servers();
  for (const auto &srv : servers) {
    if (srv) {
      std::string node_state;
      if (srv->get_id() == raft_instance_->get_leader()) {
        node_state = "leader";
      } else {
        node_state = "follower";
      }

      nuraft::raft_server::peer_info node_info =
          raft_instance_->get_peer_info(srv->get_id());
      nuraft::ulong last_log_idx = node_info.last_log_idx_;
      nuraft::ulong last_succ_resp_us = node_info.last_succ_resp_us_;

      nodes_info.push_back(std::make_tuple(srv->get_id(), srv->get_endpoint(),
                                           node_state, last_log_idx,
                                           last_succ_resp_us));
    }
  }

  return nodes_info;
}

std::tuple<int, std::string, std::string, nuraft::ulong, nuraft::ulong>
RaftStuff::getCurrentNodesInfo() const {
  std::tuple<int, std::string, std::string, nuraft::ulong, nuraft::ulong>
      nodes_info;

  if (!raft_instance_) {
    return nodes_info;
  }

  auto config = raft_instance_->get_config();
  if (!config) {
    return nodes_info;
  }

  auto servers = config->get_servers();

  for (const auto &srv : servers) {
    if (srv && srv->get_id() == node_id) {
      std::string node_state;
      if (srv->get_id() == raft_instance_->get_leader()) {
        node_state = "leader";
      } else {
        node_state = "follower";
      }

      nuraft::raft_server::peer_info node_info =
          raft_instance_->get_peer_info(srv->get_id());
      nuraft::ulong last_log_idx = node_info.last_log_idx_;
      nuraft::ulong last_succ_resp_us = node_info.last_succ_resp_us_;
      nodes_info = std::make_tuple(srv->get_id(), srv->get_endpoint(),
                                   node_state, last_log_idx, last_succ_resp_us);
      break;
    }
  }

  return nodes_info;
}
