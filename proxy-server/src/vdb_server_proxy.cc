#include "proxy_server.h"
#include "logger.h"

int main() {
    init_global_logger();
    set_log_level(spdlog::level::debug);

    std::string master_host = "127.0.0.1"; 
    int master_port = 6060;                
    std::string instance_id = "1"; 
    int proxy_port = 7070;                   

    GlobalLogger->info("Starting ProxyServer...");
    ProxyServer proxy(master_host, master_port, instance_id);
    proxy.start(proxy_port);

    return 0;
}
