#include "httplib.h"
#include <curl/curl.h>
#include <string>
#include <sstream>
#include <vector>
#include <mutex>


struct NodeInfo {
    std::string nodeId;
    std::string url;
    int role; 
};

struct NodePartitionInfo {
    int partitionId;
    std::vector<NodeInfo> nodes; 
};

struct NodePartitionConfig {
    std::string partitionKey_; 
    int numberOfPartitions_;   
    std::map<int, NodePartitionInfo> nodesInfo;
};

class ProxyServer {
public:
    ProxyServer(const std::string& masterServerHost, int masterServerPort, const std::string& instanceId);
    ~ProxyServer();
    void start(int port);

private:
    std::string masterServerHost_; 
    int masterServerPort_;         
    std::string instanceId_;       
    CURL* curlHandle_;
    httplib::Server httpServer_;
    std::vector<NodeInfo> nodes_[2]; 
    std::atomic<int> activeNodesIndex_; 
    std::atomic<size_t> nextNodeIndex_; 
    std::mutex nodesMutex_; 
    bool running_; 

    NodePartitionConfig nodePartitions_[2]; 
    std::atomic<int> activePartitionIndex_; 

    
    std::set<std::string> readPaths_;  
    std::set<std::string> writePaths_; 

    void setupForwarding();
    void forwardRequest(const httplib::Request& req, httplib::Response& res, const std::string& path);
    void handleTopologyRequest(httplib::Response& res);
    void initCurl();
    void cleanupCurl();
    static size_t writeCallback(void *contents, size_t size, size_t nmemb, void *userp);
    void fetchAndUpdateNodes(); 
    void startNodeUpdateTimer(); 
    bool extractPartitionKeyValue(const httplib::Request& req, std::string& partitionKeyValue);
    int calculatePartitionId(const std::string& partitionKeyValue);
    bool selectTargetNode(const httplib::Request& req, int partitionId, const std::string& path, NodeInfo& targetNode);
    void forwardToTargetNode(const httplib::Request& req, httplib::Response& res, const std::string& path, const NodeInfo& targetNode);
    void broadcastRequestToAllPartitions(const httplib::Request& req, httplib::Response& res, const std::string& path);
    httplib::Response sendRequestToPartition(const httplib::Request& originalReq, const std::string& path, int partitionId);
    void processAndRespondToBroadcast(httplib::Response& res, const std::vector<httplib::Response>& allResponses, uint k);
    
    void fetchAndUpdatePartitionConfig();
    
    
    void startPartitionUpdateTimer();

};
