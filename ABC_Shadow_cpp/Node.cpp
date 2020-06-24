//
//  Node.cpp
//  ABC_Shadow_cpp
//
//  Created by Quentin on 16/10/2019.
//  Copyright © 2019 Quentin. All rights reserved.
//

#include "Node.hpp"
Node::Node() {
    nodeName = NodeName(0, 0);
    label = NodeType::NIL;
    //neighbourhood = vector<shared_ptr<NodeType>>();
}

Node::Node(int i, int j, NodeType activationVal)  {
    this->nodeName = NodeName(i, j);
    this->label = NodeType::NIL;
    //this->neighbourhood = vector<shared_ptr<NodeType>>();
    this->activationVal = activationVal;
}

NodeName Node::getName() const {
    return nodeName;
}

NodeType Node::getActivationVal() const {
    return activationVal;
}

NodeType Node::getLabel() const {
    return label;
}


void Node::setNodeType(const NodeType newLabel) {
    if(newLabel == NodeType::NIL) {
        label = newLabel;
    } else {
        label = activationVal;
    }

}

void Node::updateNodeState(const State newState) {
    int newIntState = static_cast<int>(newState);
    if(newIntState == 0) {
        label = NodeType::NIL;
    } else {
        label = activationVal;
    }
}

State Node::getNodeState() const {
    if(label == NodeType::NIL) {
        return State::DISABLED;
    } else {
        return State::ENABLED;
    }
}
