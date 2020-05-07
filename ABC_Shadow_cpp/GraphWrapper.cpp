//
//  GraphWrapper.cpp
//  ABC_Shadow_cpp
//
//  Created by Quentin on 02/08/2019.
//  Copyright © 2019 Quentin. All rights reserved.
//

#include "GraphWrapper.hpp"

GraphWrapper::GraphWrapper(const vector<size_t>& teamSizes) {
    int totalSize = 0;
    vector<size_t> affiliations = vector<size_t>();
    for(size_t i{0}; i < teamSizes.size(); i++) {
        totalSize += teamSizes[i];
        
        for(int j{0}; j < teamSizes[i]; j++) {
            affiliations.push_back(i);
        }
    
    }
    
    vector<vector<Node>> mat = vector<vector<Node>>(totalSize);
    nodeList = vector<Node>();

    this->teamSizes = teamSizes;
    
    for (int i{0}; i < totalSize; i++) {
        mat[i] = vector<Node>(totalSize);
        for (int j{0}; j < totalSize; j++) {
            NodeType t;
            if(affiliations[i] == affiliations[j]) {
                t = NodeType::INTRA;
            } else {
                t = NodeType::INTER;
            }
            mat[i][j] = Node(i,j, t);
        }
    }
    

    for(int i{0}; i < totalSize; i++) {
        for(int j{i+1}; j < totalSize; j++) {
            //std::cout << "<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n";
            //std::cout << i << ","<< j << " voisin de :\n";
            for(int k{0}; k < totalSize; k ++) {
                if (k < i) {
                    //std::cout << " 1 "<< k << "," << i << " | ";
                    mat[i][j].addNeighbour(mat[k][i].getLabel());
                }else if (k > i && k != j) {
                    //std::cout << " 2 " << i << "," << k << " | ";
                    mat[i][j].addNeighbour(mat[i][k].getLabel());
                }
                
                if (k < j && k != i) {
                    //std::cout << " 3 " << k << "," << j << " | ";
                    mat[i][j].addNeighbour(mat[k][j].getLabel());
                } else if (k > j) {
                    //std::cout << " 4 " << j << "," << k << " | ";
                    mat[i][j].addNeighbour(mat[j][k].getLabel());
                }
            }
            nodeList.push_back(mat[i][j]);
            //std::cout << "\n";
        }
    }    
}

GraphWrapper::~GraphWrapper() {}

void GraphWrapper::empty() {
    for(auto &n : nodeList) {
        n.updateNodeState(State::DISABLED);
    }
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
