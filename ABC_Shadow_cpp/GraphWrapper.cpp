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
    nodeList = vector<Node>();
    this->inSize = inSize;
    this->outSize = outSize;
    
    for (int i{0}; i < inSize; i++) {
        adj[i] = vector<Node>(inSize + outSize);

        for (int j{i + 1}; j < inSize; j++) {
            adj[i][j] = Node(i,j, NodeType::INTRA);
        }

        for(int j{inSize}; j < inSize + outSize; j ++) {
            adj[i][j] = Node(i,j, NodeType::INTER);
        }
    }
    /*
     Create the neighbourhood for every node labelled as INTRA : (i,j) such as  0 <= i, j < inSize and i < j
     */
    for(int i{0}; i < inSize; i++) {
        for(int j{i+1}; j < inSize; j++) {
            //std::cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n";
            //std::cout << i << ","<< j << " voisin de :\n";
            for(int k{0}; k < inSize; k ++) {
                if (k < i) {
                    //std::cout << " 1 "<< k << "," << i << " | ";
                    adj[i][j].addNeighbour(adj[k][i].getLabel());
                }else if (k > i && k != j) {
                    //std::cout << " 2 " << i << "," << k << " | ";
                    adj[i][j].addNeighbour(adj[i][k].getLabel());
                }
                
                if (k < j && k != i) {
                    //std::cout << " 3 " << k << "," << j << " | ";
                    adj[i][j].addNeighbour(adj[k][j].getLabel());
                } else if (k > j) {
                    //std::cout << " 4 " << j << "," << k << " | ";
                    adj[i][j].addNeighbour(adj[j][k].getLabel());
                }
            }
            
            for(int k{inSize}; k < inSize + outSize; k++) {
                adj[i][j].addNeighbour(adj[i][k].getLabel());
                adj[i][k].addNeighbour(adj[i][j].getLabel());
                
                adj[i][j].addNeighbour(adj[j][k].getLabel());
                adj[j][k].addNeighbour(adj[i][j].getLabel());
            }
            nodeList.push_back(adj[i][j]);
            //std::cout << "\n";
        }
    }
    
    /*
     Create the neighbourhood for every node labelled as INTER  (i,j) such as  0 <= i < inSize, inSize <= j < inSize + outSize and i < j
     Assumption :
     - idx of INTRA nodes < idx of INTER nodes
     - No interaction between two nodes labelled as INTER
    */
    for (int i{0}; i < inSize; i++) {
        for(int j{inSize}; j < inSize + outSize; j++) {
            for(int k{inSize}; k < inSize + outSize; k++) { //INTRA (i,j) and INTER (k,l) such as i<j and k<l are neighbours only if i = k
                if(k != j) {
                    adj[i][j].addNeighbour(adj[i][k].getLabel());
                }
            }
        }
    }
    for (int i{0}; i < inSize; i++) {
        for(int j{inSize}; j < inSize + outSize; j ++) {
           nodeList.push_back(adj[i][j]);
        }
    }
    
}

GraphWrapper::~GraphWrapper() {}

void GraphWrapper::empty() {
    for(auto &n : nodeList) {
        n.updateNodeState(State::DISABLED);
    }
}

vector<vector<Node>> &GraphWrapper::getAdj() {
    return adj;
}

Node &GraphWrapper::getNode(int i,  int j) {
    return adj[i][j];
}

vector<Node> &GraphWrapper::getNodes() {
    return nodeList;
}

Stats GraphWrapper::getInteractionStats() {
    Stats stats{};
    for(auto n : getNodes()) {
        stats +=  n.getInteractionStats();
    }
    return stats / 2;
}

int GraphWrapper::getInSize() {
    return inSize;
}

int GraphWrapper::getOutSize() {
    return outSize;
}
 
