#pragma once

#include "in_mem_state_mgr.h"
#include "log_state_machine.h"
#include <libnuraft/asio_service.hxx>
#include "logger.h" 

class RaftStuff {
public:
    RaftStuff(int node_id, const std::string& endpoint, int port);

    void Init();
    ptr< cmd_result< ptr<buffer> > > addSrv(int srv_id, const std::string& srv_endpoint);
    void enableElectionTimeout(int lower_bound, int upper_bound); 
    bool isLeader() const; 
    std::vector<std::tuple<int, std::string, std::string, nuraft::ulong, nuraft::ulong>> getAllNodesInfo() const;
    std::string getNodeStatus(int node_id) const; 

private:
    int node_id;
    std::string endpoint;
    ptr<state_mgr> smgr_;
    ptr<state_machine> sm_;
    int port_;
    raft_launcher launcher_;
    ptr<raft_server> raft_instance_;
};
