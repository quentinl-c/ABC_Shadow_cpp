//
//  GraphWrapper.cpp
//  ABC_Shadow_cpp
//
//  Created by Quentin on 02/08/2019.
//  Copyright © 2019 Quentin. All rights reserved.
//

#include "GraphWrapper.hpp"

GraphWrapper::GraphWrapper(const int inSize, const int outSize) {
    adj = vector<vector<Node>>(inSize);
    nodeList = vector<NodeName>();
    dimsIn = vector<int>(inSize);
    dimsOut = vector<int>(inSize);
    
    this->inSize = inSize;
    this->outSize = outSize;
    int inCounter = 0;
    for (int i{0}; i < inSize; i++) {
        adj[i] = vector<Node>(inSize + outSize);

        for (int j{i + 1}; j < inSize; j++) {
            adj[i][j] = Node(i,j, NodeType::INTRA);
            inCounter ++;
        }

        for(int j{inSize}; j < inSize + outSize; j ++) {
            adj[i][j] = Node(i,j, NodeType::INTER);
        }
    }
    std::cout << "Nodes in " << inCounter << std::endl;
    
    
    for(int i{0}; i < inSize; i++) {
        for(int j{i+1}; j < inSize; j++) {
            nodeList.push_back(adj[i][j].getName());
        }
    }
    for (int i{0}; i < inSize; i++) {
        for(int j{inSize}; j < inSize + outSize; j ++) {
            nodeList.push_back(adj[i][j].getName());
        }
    }
    
}

GraphWrapper::~GraphWrapper() {}

void GraphWrapper::empty() {
    for(auto &n : nodeList) {
        
        adj[n.first][n.second].updateNodeState(State::DISABLED);
    }
    dimsIn = vector<int>(inSize);
    dimsOut = vector<int>(inSize);
}

vector<vector<Node>> &GraphWrapper::getAdj() {
    return adj;
}

Node &GraphWrapper::getNode(int i,  int j) {
    return adj[i][j];
}


const vector<NodeName> & GraphWrapper::getNodes() const {
    return nodeList;
}

Stats GraphWrapper::getInteractionStats() {
    Stats stats{};
    for(auto n : getNodes()) {
        Node node = adj[n.first][n.second];
        if (node.getNodeState() == State::ENABLED) {
            Stats res{};
            computeChangeStatistics(n.first, n.second, res);
            stats += res;
        }
    }
    return stats / 2;
}

int GraphWrapper::getInSize() {
    return inSize;
}

int GraphWrapper::getOutSize() {
    return outSize;
}
 
/*
 ================================================================
 == Experimental, don't worry the situation is under control ! ==
 ================================================================
 */
void GraphWrapper::enableNode(int i, int j) {
    const int left = i < j ? i : j;
    const int right = i < j ? j :i;
   
    if (right < inSize) {
        dimsIn[left] ++;
        dimsIn[right] ++;
    } else {
        dimsOut[left] ++;
    }
}

void GraphWrapper::disableNode(int i, int j) {
    const int left = i < j ? i : j;
    const int right = i < j ? j :i;

    if (right < inSize) {
        dimsIn[left] --;
        dimsIn[right] --;
    } else {
        dimsOut[left] --;
    }
}

int GraphWrapper::getDimsIn(const int & idx) {
    return dimsIn[idx];
}

int GraphWrapper::getDimsOut(const int & idx) {
    return dimsOut[idx];
}


void GraphWrapper::computeChangeStatistics(const int &i, const int & j, Stats & s) {
    const int left = i < j ? i : j;
    const int right = i < j ? j :i;
    Node & node = getNode(left, right);
    State state = node.getNodeState();
    NodeType activation = node.getActivationVal();
    int activationVal = static_cast<int>(activation);

    
    int inCount = getDimsIn(left);
    if (state == State::ENABLED && activation == NodeType::INTRA) inCount --;
    
    int outCount = getDimsOut(left);
    if (state == State::ENABLED && activation == NodeType::INTER) outCount --;

    if (right < getInSize()) {
        inCount += getDimsIn(right);
        if (state == State::ENABLED) inCount --;
        
        outCount += getDimsOut(right);
     }
    
     int idx = activationVal + static_cast<int>(NodeType::INTRA) - 2;
    
     s.set(idx, inCount);
     idx = activationVal + static_cast<int>(NodeType::INTER) - 2 ;
     s.set(idx, outCount);
}

Stats GraphWrapper::fastGetInteractionStats() {
    Stats stats{};
       for(const auto & n : getNodes()) {
           Node node = adj[n.first][n.second];
           if (node.getNodeState() == State::ENABLED) {
               Stats res{};
               computeChangeStatistics(n.first, n.second, res);
               stats += res;
           }
           
       }
       return stats / 2;
}
