//
//  GraphWrapper.hpp
//  ABC_Shadow_cpp
//
//  Created by Quentin on 02/08/2019.
//  Copyright © 2019 Quentin. All rights reserved.
//

#ifndef GraphWrapper_hpp
#define GraphWrapper_hpp

#include <iostream>
#include <vector>
#include <set>
#include <utility>
#include "Stats.hpp"
#include "Node.hpp"

using std::vector;


class GraphWrapper {
private:
    int inSize;
    int outSize;
    vector<vector<Node>> adj;
    vector<vector<State>> adjState;
    vector<vector<NodeType>> adjType;
    
    vector<NodeName> nodeList;
    // Experimental:
    vector<int> dimsIn;
    vector<int> dimsOut;
    

public:

    GraphWrapper(const int inSize, const int outSize);
    ~GraphWrapper();
    void empty();
    int getInSize();
    int getOutSize();
    const vector<NodeName> & getNodes() const;
    vector<vector<Node>> &getAdj();
    Node getNode(int i, int j);
    Node & getRefNode(int i, int j);
    Stats getInteractionStats();

    // Experimental :
    void enableNode(int i, int j);
    void disableNode(int i, int j);
    int getDimsIn(const int & idx);
    int getDimsOut(const int & idx);
    void computeChangeStatistics(const int &i, const int & j,  Stats & s);
    Stats fastGetInteractionStats();
    State getNodeState(int i, int j) const;
    NodeType getNodeType(int i, int j) const;
};
#endif /* GraphWrapper_hpp */
