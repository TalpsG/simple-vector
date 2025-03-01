#include "proxy_server.h"
#include "logger.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <iostream>
#include <sstream>
#include <unordered_set>
#include <unordered_map>
#include <future>
#include <functional>

ProxyServer::ProxyServer(const std::string& masterServerHost, int masterServerPort, const std::string& instanceId)
: masterServerHost_(masterServerHost), masterServerPort_(masterServerPort), instanceId_(instanceId), curlHandle_(nullptr), activeNodesIndex_(0) , nextNodeIndex_(0), running_(true), activePartitionIndex_(0) {
    initCurl();
    setupForwarding();

    readPaths_ = {"/search"};

    
    writePaths_ = {"/upsert"};

}


ProxyServer::~ProxyServer() {
    running_ = false; 
    cleanupCurl();
}

void ProxyServer::initCurl() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curlHandle_ = curl_easy_init();
    if (curlHandle_) {
        curl_easy_setopt(curlHandle_, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curlHandle_, CURLOPT_TCP_KEEPIDLE, 120L);
        curl_easy_setopt(curlHandle_, CURLOPT_TCP_KEEPINTVL, 60L);
    }
}

void ProxyServer::cleanupCurl() {
    if (curlHandle_) {
        curl_easy_cleanup(curlHandle_);
    }
    curl_global_cleanup();
}

void ProxyServer::start(int port) {
    GlobalLogger->info("Proxy server starting on port {}", port);

    fetchAndUpdateNodes(); 

    fetchAndUpdatePartitionConfig(); 

    startNodeUpdateTimer(); 
    startPartitionUpdateTimer(); 

    httpServer_.listen("0.0.0.0", port); 
}


void ProxyServer::setupForwarding() {

    
    httpServer_.Post("/upsert", [this](const httplib::Request& req, httplib::Response& res) {
        GlobalLogger->info("Forwarding POST /upsert");
        forwardRequest(req, res, "/upsert");
    });

    
    httpServer_.Post("/search", [this](const httplib::Request& req, httplib::Response& res) {
        GlobalLogger->info("Forwarding POST /search");
        forwardRequest(req, res, "/search");
    });

    
    httpServer_.Get("/topology", [this](const httplib::Request&, httplib::Response& res) {
        this->handleTopologyRequest(res);
    });

    
    
}

void ProxyServer::forwardRequest(const httplib::Request& req, httplib::Response& res, const std::string& path) {
    std::string partitionKeyValue;
    if (!extractPartitionKeyValue(req, partitionKeyValue)) {
        GlobalLogger->debug("Partition key value not found, broadcasting request to all partitions");
        broadcastRequestToAllPartitions(req, res, path);
        return;
    }

    int partitionId = calculatePartitionId(partitionKeyValue);
    
    NodeInfo targetNode;
    if (!selectTargetNode(req, partitionId, path, targetNode)) {
        res.status = 503;
        res.set_content("No suitable node found for forwarding", "text/plain");
        return;
    }

    forwardToTargetNode(req, res, path, targetNode);
}

void ProxyServer::broadcastRequestToAllPartitions(const httplib::Request& req, httplib::Response& res, const std::string& path) {
    
    rapidjson::Document doc;
    doc.Parse(req.body.c_str());
    if (doc.HasParseError() || !doc.HasMember("k") || !doc["k"].IsInt()) {
        res.status = 400;
        res.set_content("Invalid request: missing or invalid 'k'", "text/plain");
        return;
    }

    int k = doc["k"].GetInt();

    int activePartitionIndex = activePartitionIndex_.load();
    const auto& partitionConfig = nodePartitions_[activePartitionIndex];
    std::vector<std::future<httplib::Response>> futures;

    std::unordered_set<int> sentPartitionIds;

    for (const auto& partition : partitionConfig.nodesInfo) {
        int partitionId = partition.first; 
        if (sentPartitionIds.find(partitionId) != sentPartitionIds.end()) {
            
            continue;
        }

        futures.push_back(std::async(std::launch::async, &ProxyServer::sendRequestToPartition, this, req, path, partition.first));
        
        sentPartitionIds.insert(partitionId);
    }

    
    std::vector<httplib::Response> allResponses;
    for (auto& future : futures) {
        allResponses.push_back(future.get());
    }

    
    processAndRespondToBroadcast(res, allResponses, k);
}

httplib::Response ProxyServer::sendRequestToPartition(const httplib::Request& originalReq, const std::string& path, int partitionId) {
    NodeInfo targetNode;
    if (!selectTargetNode(originalReq, partitionId, path, targetNode)) {
        GlobalLogger->error("Failed to select target node for partition ID: {}", partitionId);
        
        return httplib::Response();
    }

    
    std::string targetUrl = targetNode.url + path;
    GlobalLogger->info("Forwarding request to partition node: {}", targetUrl);

    
    CURL *curl = curl_easy_init();
    if (!curl) {
        GlobalLogger->error("CURL initialization failed");
        return httplib::Response();
    }

    curl_easy_setopt(curl, CURLOPT_URL, targetUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);

    std::string response_data;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_data);

    if (originalReq.method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, originalReq.body.c_str());
    } else {
        
        
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    }

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        GlobalLogger->error("Curl request failed: {}", curl_easy_strerror(res));
        curl_easy_cleanup(curl);
        return httplib::Response();
    }

    
    httplib::Response httpResponse;
    httpResponse.status = 200; 
    httpResponse.set_content(response_data, "application/json");

    curl_easy_cleanup(curl);
    return httpResponse;
}


void ProxyServer::processAndRespondToBroadcast(httplib::Response& res, const std::vector<httplib::Response>& allResponses, uint k) {
    GlobalLogger->debug("Processing broadcast responses. Expected max results: {}", k);

    struct CombinedResult {
        double distance;
        double vector;
    };

    std::vector<CombinedResult> allResults;

    
    for (const auto& response : allResponses) {
        GlobalLogger->debug("Processing response from a partition");
        if (response.status == 200) {
            GlobalLogger->debug("Response parsed successfully");
            rapidjson::Document doc;
            doc.Parse(response.body.c_str());
            
            if (!doc.HasParseError() && doc.IsObject() && doc.HasMember("vectors") && doc.HasMember("distances")) {
                const auto& vectors = doc["vectors"].GetArray();
                const auto& distances = doc["distances"].GetArray();

                for (rapidjson::SizeType i = 0; i < vectors.Size(); ++i) {
                    CombinedResult result = {distances[i].GetDouble(), vectors[i].GetDouble()};
                    allResults.push_back(result);
                }
            }
        } else {
            GlobalLogger->debug("Response status: {}", response.status);
        }
    }

    
    GlobalLogger->debug("Sorting combined results");
    std::sort(allResults.begin(), allResults.end(), [](const CombinedResult& a, const CombinedResult& b) {
        return a.distance < b.distance; 
    });

    
    GlobalLogger->debug("Resizing results to max of {} from {}", k, allResults.size());
    if (allResults.size() > k) {
        allResults.resize(k);
    }

    
    GlobalLogger->debug("Building final response");
    rapidjson::Document finalDoc;
    finalDoc.SetObject();
    rapidjson::Document::AllocatorType& allocator = finalDoc.GetAllocator();

    rapidjson::Value finalVectors(rapidjson::kArrayType);
    rapidjson::Value finalDistances(rapidjson::kArrayType);

    for (const auto& result : allResults) {
        finalVectors.PushBack(result.vector, allocator);
        finalDistances.PushBack(result.distance, allocator);
    }

    finalDoc.AddMember("vectors", finalVectors, allocator);
    finalDoc.AddMember("distances", finalDistances, allocator);
    finalDoc.AddMember("retCode", 0, allocator);

    GlobalLogger->debug("Final response prepared");

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    finalDoc.Accept(writer);

    res.set_content(buffer.GetString(), "application/json");
}




size_t ProxyServer::writeCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void ProxyServer::fetchAndUpdateNodes() {
    GlobalLogger->info("Fetching nodes from Master Server");

    
    std::string url = "http://" + masterServerHost_ + ":" + std::to_string(masterServerPort_) + "/getInstance?instanceId=" + instanceId_;
    GlobalLogger->debug("Requesting URL: {}", url);

    
    curl_easy_setopt(curlHandle_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curlHandle_, CURLOPT_HTTPGET, 1L);
    std::string response_data;
    curl_easy_setopt(curlHandle_, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curlHandle_, CURLOPT_WRITEDATA, &response_data);

    
    CURLcode curl_res = curl_easy_perform(curlHandle_);
    if (curl_res != CURLE_OK) {
        GlobalLogger->error("curl_easy_perform() failed: {}", curl_easy_strerror(curl_res));
        return;
    }

    
    rapidjson::Document doc;
    if (doc.Parse(response_data.c_str()).HasParseError()) {
        GlobalLogger->error("Failed to parse JSON response");
        return;
    }

    
    if (doc["retCode"].GetInt() != 0) {
        GlobalLogger->error("Error from Master Server: {}", doc["msg"].GetString());
        return;
    }

    int inactiveIndex = activeNodesIndex_.load() ^ 1; 
    nodes_[inactiveIndex].clear();
    const auto& nodesArray = doc["data"]["nodes"].GetArray();
    for (const auto& nodeVal : nodesArray) {
        if (nodeVal["status"].GetInt() == 1) { 
            NodeInfo node;
            node.nodeId = nodeVal["nodeId"].GetString();
            node.url = nodeVal["url"].GetString();
            node.role = nodeVal["role"].GetInt();
            nodes_[inactiveIndex].push_back(node);
        } else {
            GlobalLogger->info("Skipping inactive node: {}", nodeVal["nodeId"].GetString());
        }
    }

    
    activeNodesIndex_.store(inactiveIndex);
    GlobalLogger->info("Nodes updated successfully");
}


void ProxyServer::handleTopologyRequest(httplib::Response& res) {
    rapidjson::Document doc;
    doc.SetObject();
    rapidjson::Document::AllocatorType& allocator = doc.GetAllocator();

    
    doc.AddMember("masterServer", rapidjson::Value(masterServerHost_.c_str(), allocator), allocator);
    doc.AddMember("masterServerPort", masterServerPort_, allocator);

    
    doc.AddMember("instanceId", rapidjson::Value(instanceId_.c_str(), allocator), allocator);

    
    rapidjson::Value nodesArray(rapidjson::kArrayType);
    int activeNodeIndex = activeNodesIndex_.load();
    for (const auto& node : nodes_[activeNodeIndex]) {
        rapidjson::Value nodeObj(rapidjson::kObjectType);
        nodeObj.AddMember("nodeId", rapidjson::Value(node.nodeId.c_str(), allocator), allocator);
        nodeObj.AddMember("url", rapidjson::Value(node.url.c_str(), allocator), allocator);
        nodeObj.AddMember("role", node.role, allocator);
        nodesArray.PushBack(nodeObj, allocator);
    }
    doc.AddMember("nodes", nodesArray, allocator);

    
    int activePartitionIndex = activePartitionIndex_.load();
    const auto& partitionConfig = nodePartitions_[activePartitionIndex];
    
    rapidjson::Value partitionConfigObj(rapidjson::kObjectType);
    partitionConfigObj.AddMember("partitionKey", rapidjson::Value(partitionConfig.partitionKey_.c_str(), allocator), allocator);
    partitionConfigObj.AddMember("numberOfPartitions", partitionConfig.numberOfPartitions_, allocator);

    
    rapidjson::Value partitionNodes(rapidjson::kArrayType);
    for (const auto& partition : partitionConfig.nodesInfo) {
        rapidjson::Value partitionInfo(rapidjson::kObjectType);
        partitionInfo.AddMember("partitionId", partition.first, allocator);

        rapidjson::Value nodesInPartition(rapidjson::kArrayType);
        for (const auto& node : partition.second.nodes) {
            rapidjson::Value nodeObj(rapidjson::kObjectType);
            nodeObj.AddMember("nodeId", rapidjson::Value(node.nodeId.c_str(), allocator), allocator);
            nodesInPartition.PushBack(nodeObj, allocator);
        }

        partitionInfo.AddMember("nodes", nodesInPartition, allocator);
        partitionNodes.PushBack(partitionInfo, allocator);
    }
    partitionConfigObj.AddMember("partitions", partitionNodes, allocator);
    doc.AddMember("partitionConfig", partitionConfigObj, allocator);

    
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    
    res.set_content(buffer.GetString(), "application/json");
}


void ProxyServer::fetchAndUpdatePartitionConfig() {
    GlobalLogger->info("Fetching Partition Config from Master Server");
    
    std::string url = "http://" + masterServerHost_ + ":" + std::to_string(masterServerPort_) + "/getPartitionConfig?instanceId=" + instanceId_;
    curl_easy_setopt(curlHandle_, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curlHandle_, CURLOPT_HTTPGET, 1L);

    std::string response_data;
    curl_easy_setopt(curlHandle_, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curlHandle_, CURLOPT_WRITEDATA, &response_data);

    CURLcode curl_res = curl_easy_perform(curlHandle_);
    if (curl_res != CURLE_OK) {
        GlobalLogger->error("curl_easy_perform() failed: {}", curl_easy_strerror(curl_res));
        return;
    }

    
    rapidjson::Document doc;
    if (doc.Parse(response_data.c_str()).HasParseError()) {
        GlobalLogger->error("Failed to parse JSON response");
        return;
    }

    
    if (doc["retCode"].GetInt() != 0) {
        GlobalLogger->error("Error from Master Server: {}", doc["msg"].GetString());
        return;
    }

    int inactiveIndex = activePartitionIndex_.load() ^ 1; 
    nodePartitions_[inactiveIndex].nodesInfo.clear();

        
    nodePartitions_[inactiveIndex].partitionKey_ = doc["data"]["partitionKey"].GetString();
    nodePartitions_[inactiveIndex].numberOfPartitions_ = doc["data"]["numberOfPartitions"].GetInt();


    
    const auto& partitionsArray = doc["data"]["partitions"].GetArray();
    for (const auto& partitionVal : partitionsArray) {
        int partitionId = partitionVal["partitionId"].GetInt();
        std::string nodeId = partitionVal["nodeId"].GetString();

        

        auto it = nodePartitions_[inactiveIndex].nodesInfo.find(partitionId);

        if (it == nodePartitions_[inactiveIndex].nodesInfo.end()) {
            
            NodePartitionInfo newPartition;
            newPartition.partitionId = partitionId;
            nodePartitions_[inactiveIndex].nodesInfo[partitionId]= newPartition;
            it = std::prev(nodePartitions_[inactiveIndex].nodesInfo.end());
        }

        
        NodeInfo nodeInfo;
        nodeInfo.nodeId = nodeId;
        
        it->second.nodes.push_back(nodeInfo);
    }

    
    activePartitionIndex_.store(inactiveIndex);

    GlobalLogger->info("Partition Config updated successfully");

}


bool ProxyServer::extractPartitionKeyValue(const httplib::Request& req, std::string& partitionKeyValue) {
    GlobalLogger->debug("Extracting partition key value from request");

    rapidjson::Document doc;
    if (doc.Parse(req.body.c_str()).HasParseError()) {
        GlobalLogger->debug("Failed to parse request body as JSON");
        return false;
    }

    int activePartitionIndex = activePartitionIndex_.load();
    const auto& partitionConfig = nodePartitions_[activePartitionIndex];

    if (!doc.HasMember(partitionConfig.partitionKey_.c_str())) {
        GlobalLogger->debug("Partition key not found in request");
        return false;
    }

    const rapidjson::Value& keyVal = doc[partitionConfig.partitionKey_.c_str()];
    if (keyVal.IsString()) {
        partitionKeyValue = keyVal.GetString();
    } else if (keyVal.IsInt()) {
        partitionKeyValue = std::to_string(keyVal.GetInt());
    } else {
        GlobalLogger->debug("Unsupported type for partition key");
        return false;
    }

    GlobalLogger->debug("Partition key value extracted: {}", partitionKeyValue);
    return true;
}



int ProxyServer::calculatePartitionId(const std::string& partitionKeyValue) {
    GlobalLogger->debug("Calculating partition ID for key value: {}", partitionKeyValue);

    int activePartitionIndex = activePartitionIndex_.load();
    const auto& partitionConfig = nodePartitions_[activePartitionIndex];

    
    std::hash<std::string> hasher;
    size_t hashValue = hasher(partitionKeyValue);

    
    int partitionId = static_cast<int>(hashValue % partitionConfig.numberOfPartitions_);
    GlobalLogger->debug("Calculated partition ID: {}", partitionId);

    return partitionId;
}



bool ProxyServer::selectTargetNode(const httplib::Request& req, int partitionId, const std::string& path, NodeInfo& targetNode) {
    GlobalLogger->debug("Selecting target node for partition ID: {}", partitionId);

    bool forceMaster = (req.has_param("forceMaster") && req.get_param_value("forceMaster") == "true")  ;
    int activeNodeIndex = activeNodesIndex_.load();
    const auto& partitionConfig = nodePartitions_[activePartitionIndex_.load()];
    const auto& partitionNodes = partitionConfig.nodesInfo.find(partitionId);
    
    if (partitionNodes == partitionConfig.nodesInfo.end()) {
        GlobalLogger->error("No nodes found for partition ID: {}", partitionId);
        return false;
    }

    
    std::vector<NodeInfo> availableNodes;
    for (const auto& partitionNode : partitionNodes->second.nodes) {
        auto it = std::find_if(nodes_[activeNodeIndex].begin(), nodes_[activeNodeIndex].end(),
                               [&partitionNode](const NodeInfo& n) { return n.nodeId == partitionNode.nodeId; });
        if (it != nodes_[activeNodeIndex].end()) {
            availableNodes.push_back(*it);
        }
    }

    if (availableNodes.empty()) {
        GlobalLogger->error("No available nodes for partition ID: {}", partitionId);
        return false;
    }

    if (forceMaster || writePaths_.find(path) != writePaths_.end()) {
        
        for (const auto& node : availableNodes) {
            if (node.role == 1) {
                targetNode = node;
                return true;
            }
        }
        GlobalLogger->error("No master node available for partition ID: {}", partitionId);
        return false;
    } else {
        
        size_t nodeIndex = nextNodeIndex_.fetch_add(1) % availableNodes.size();
        targetNode = availableNodes[nodeIndex];
        return true;
    }
}



void ProxyServer::forwardToTargetNode(const httplib::Request& req, httplib::Response& res, const std::string& path, const NodeInfo& targetNode) {
    GlobalLogger->debug("Forwarding request to target node: {}", targetNode.nodeId);
    
    std::string targetUrl = targetNode.url + path;
    GlobalLogger->info("Forwarding request to: {}", targetUrl);

    
    curl_easy_setopt(curlHandle_, CURLOPT_URL, targetUrl.c_str());
    if (req.method == "POST") {
        curl_easy_setopt(curlHandle_, CURLOPT_POSTFIELDS, req.body.c_str());
    } else {
        
        curl_easy_setopt(curlHandle_, CURLOPT_HTTPGET, 1L);
    }
    curl_easy_setopt(curlHandle_, CURLOPT_WRITEFUNCTION, writeCallback);

    
    std::string response_data;
    curl_easy_setopt(curlHandle_, CURLOPT_WRITEDATA, &response_data);

    
    CURLcode curl_res = curl_easy_perform(curlHandle_);
    if (curl_res != CURLE_OK) {
        GlobalLogger->error("curl_easy_perform() failed: {}", curl_easy_strerror(curl_res));
        res.status = 500;
        res.set_content("Internal Server Error", "text/plain");
    } else {
        GlobalLogger->info("Received response from server");
        
        if (response_data.empty()) {
            GlobalLogger->error("Received empty response from server");
            res.status = 500;
            res.set_content("Internal Server Error", "text/plain");
        } else {
            res.set_content(response_data, "application/json");
        }
    }
}


void ProxyServer::startNodeUpdateTimer() {
    std::thread([this]() {
        while (running_) {
            std::this_thread::sleep_for(std::chrono::seconds(30));
            fetchAndUpdateNodes();
        }
    }).detach();
}

void ProxyServer::startPartitionUpdateTimer() {
    std::thread([this]() {
        while (running_) {
            std::this_thread::sleep_for(std::chrono::minutes(5)); 
            fetchAndUpdatePartitionConfig();
        }
    }).detach();
}
