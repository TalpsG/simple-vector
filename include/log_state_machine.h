#pragma once

#include <libnuraft/nuraft.hxx>

using namespace nuraft;

class log_state_machine : public state_machine {
public:
    ptr<buffer> commit(const ulong log_idx, buffer& data);

    void pre_commit(const ulong log_idx);
    void rollback(const ulong log_idx);
    void snapshot_save(const ptr<snapshot>& s, const ulong log_idx);
    int32_t snapshot_load(const ptr<snapshot>& s);
    bool apply_snapshot(snapshot& s);
    ptr<snapshot> last_snapshot();
    ulong last_commit_index();
    void create_snapshot(snapshot& s, cmd_result<bool, std::shared_ptr<std::exception>>::handler_type& h);
};

