//
//  MCMCSim.cpp
//  ABC_Shadow_cpp
//
//  Created by Quentin on 06/08/2019.
//  Copyright © 2019 Quentin. All rights reserved.
//

#include "MCMCSim.hpp"

MCMCSim::MCMCSim(GraphWrapper *g): graph(g),
                                   rngs(0) {}

MCMCSim:: ~MCMCSim() {}

void MCMCSim::setGraph(GraphWrapper *new_g) {
    graph = new_g;
}

void MCMCSim::reset() {
    graph->empty();
}


vector<Stats> MCMCSim::mhSim(RandomGen &rGen, PottsModel& model, const int iter_max, const int by, const int burnin) {
    vector<Stats> res{};

    vector<NodeName> nodes{graph->getNodes()};
    size_t nodeNbr = nodes.size();
    
    NodeType old_label;
    State new_state;
    
    double delta{0}, epsilon{0};

    Stats stats(model.getStats(*graph));
    Stats* delta_stats = new Stats();

    const Stats params = model.getParams();
    
    for (int mh_iter{0}; mh_iter < iter_max; ++mh_iter) {


        if (mh_iter >= burnin && mh_iter % by == 0) {
            res.push_back(stats);
        }
        
        for (size_t i{0}; i < nodeNbr; i++) {

            Node & current_node = graph->getRefNode(nodes[i].first, nodes[i].second);
            old_label = current_node.getLabel();
            new_state = model.getNewProposal(rGen);

            model.applyChangeStats(*delta_stats, new_state, &current_node);
            delta = delta_stats->dot(params);

            epsilon = rGen.getUnifornRealD();

            if (epsilon >= exp(delta)) {
                current_node.setNodeType(old_label);
            } else {
                stats += *delta_stats;
            }
            delta_stats->reset();
        }
    }

    return res;
}

vector<Stats> MCMCSim::gibbsSim(RandomGen &rGen, PottsModel &model, const int iter_max, const int by, const int burnin) {
    vector<Stats> res{};
    const vector<NodeName> & nodes {graph->getNodes()};
    size_t nodeNbr = nodes.size();
    //Stats stats(model.getStats(*graph));
    Stats stats{};
    std::chrono::time_point<clock_time> time_start{};
    vector<double> times{};
    double duration;
    unsigned seed = rGen.getSeed();
    for (int gibbs_iter{0}; gibbs_iter < iter_max; ++gibbs_iter) {

        if (gibbs_iter >= burnin && gibbs_iter % by == 0) {
            res.push_back(stats);
        }
        for (size_t i{0}; i < nodeNbr; i++) {
            NodeName current_node = nodes[i];
            //time_start = clock_time::now();
            Node & node = graph->getRefNode(current_node.first, current_node.second);
            stats += model.applyGibbsProposalParallel(&node, graph, &seed, times);
            //duration = std::chrono::duration_cast<second_t>(clock_time::now() - time_start).count();
            //times.push_back(duration);
        }
    }
    //double average = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
    //std::cout << "average " << average << std::endl;
    return res;
}


