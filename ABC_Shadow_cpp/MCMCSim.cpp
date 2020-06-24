//
//  MCMCSim.cpp
//  ABC_Shadow_cpp
//
//  Created by Quentin on 06/08/2019.
//  Copyright © 2019 Quentin. All rights reserved.
//

#include "MCMCSim.hpp"
Stats applyGibbsProposalParallel(const int i, const int j, GraphWrapper & graph, Stats params, unsigned * seed);
MCMCSim::MCMCSim(GraphWrapper *g): graph(g),
                                   chunkSizeIn(0),
                                   chunkSizeOut(0),
                                   chunksNbr(0),
                                   threadsNbr(0),
                                   parallelSimIters(0),
                                   chunks(0),
                                   poolIn(0),
                                   poolOut(0),
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

            Node &current_node = graph->getNode(nodes[i].first, nodes[i].second);
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
//    std::chrono::time_point<clock_time> time_start{};
    vector<double> times{};
//    double duration;
    unsigned seed = rGen.getSeed();
    for (int gibbs_iter{0}; gibbs_iter < iter_max; ++gibbs_iter) {

        if (gibbs_iter >= burnin && gibbs_iter % by == 0) {
            res.push_back(stats);
        }
        for (size_t i{0}; i < nodeNbr; i++) {
            NodeName current_node = nodes[i];
            Node & node = graph->getNode(current_node.first, current_node.second);
            stats += model.applyGibbsProposalParallel(&node, graph, &seed);
        }
    }
    //double average = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
    //std::cout << "average " << average << std::endl;
    return res;
}


void MCMCSim::parallelSetup(const int & simIter, const int & threadsNbr, RandomGen &rGen) {
    this->threadsNbr = threadsNbr;

    rngs = vector<RandomGen>(threadsNbr);
    for (int i = 0; i < threadsNbr; i++) {
        rngs[i] = RandomGen((i + 1) * rGen.getSeed());
    }

    parallelSimIters = simIter;
    
    int const inSize = graph->getInSize();
    int const outSize = graph->getOutSize();
    
    int const inNodes = inSize * (inSize - 1) / 2;
    int const outNodes = inSize * outSize;

    
    std::cout << "inNodes " << inNodes << std::endl;
    std::cout << "outNodes " << outNodes << std::endl;
    
    chunkSizeIn = inSize % 2 == 0 ? inSize : inSize - 1;
    chunkSizeOut = min(inSize, outSize) * 2;
    
    
    chunksNbrIn = max(inNodes / (chunkSizeIn / 2), 1); // number of chunks "In" for one iteration
    chunksNbrOut = max(outNodes / (chunkSizeOut / 2), 1); // number of chunks "Out" for one iteration
    std::cout << "Generated nodes " <<(chunkSizeIn / 2) * chunksNbrIn +  (chunkSizeOut / 2) * chunksNbrOut << std::endl;
    std::cout << "Generated In nodes " << (chunkSizeIn / 2) * chunksNbrIn << std::endl;
    chunksNbr = chunksNbrIn + chunksNbrOut; // number of chunks for one iteration
    chunks = vector<vector<int>>(chunksNbr * simIter);

    std::cout << "chunks size " << chunks.size() << std::endl;

    for (int iter = 0; iter < simIter; iter ++) {
        for(int cId = 0; cId  < chunksNbr; cId ++) {
            const int idx = iter * chunksNbr + cId;
            if (cId < chunksNbrIn) {
                chunks[idx] = vector<int>(chunkSizeIn);
            } else {
                chunks[idx] = vector<int>(chunkSizeOut);
            }
        }
    }
    
    poolIn = vector<vector<int>>(threadsNbr);
    poolOut = vector<vector<int>>(threadsNbr);

    for (int i = 0; i < poolIn.size(); i++) {
        poolIn[i] = vector<int>(inSize);
        iota(poolIn[i].begin(), poolIn[i].end(), 0);
    }

    for (int i = 0; i < poolOut.size(); i++) {
        poolOut[i] = vector<int>(outSize);
        iota(poolOut[i].begin(), poolOut[i].end(), inSize);
    }
}

void MCMCSim::generateIndependentNodes() {

    omp_set_num_threads(threadsNbr);
    #pragma omp parallel
    {
        const int thid = omp_get_thread_num();

        #pragma omp barrier
    
        #pragma omp for
        for (int iter = 0; iter < parallelSimIters ; iter++) {
        
            for (int cId = 0; cId < chunksNbr; cId++) {
                const int idx = iter * chunksNbr + cId;
                shuffle(poolIn[thid].begin(), poolIn[thid].end(), rngs[thid].getEngine());
                if (cId < chunksNbrIn) {
                    copy(poolIn[thid].begin(), poolIn[thid].begin() + chunkSizeIn, chunks[idx].begin());
                } else {
                    shuffle(poolOut[thid].begin(), poolOut[thid].end(), rngs[thid].getEngine());
                    for (int i = 0; i < chunks[idx].size(); i+=2) {
                        chunks[idx][i] = poolIn[thid][i / 2];
                        chunks[idx][i + 1] = poolOut[thid][i / 2];
                    }
                }
            }
    
        }
    }
}

bool MCMCSim::isParallelSetupReady(const int threads_nbr) {
    return parallelSimIters != 0 && chunks.size() != 0 && rngs.size() != 0;
}

vector<Stats> parallelGibbsSim(PottsModel &model, GraphWrapper & graph, vector<vector<int>> & chunks, const int chunksNbr, const unsigned baseSeed, const int threadsNbr, const int sim_iter, const int by, const int burnin) {

    vector<Stats> res{};
    //Stats stats(model.getStats(*graph));
    Stats stats{};
    size_t chunk, nid;
    
    omp_set_num_threads(threadsNbr);
    
    #pragma omp parallel private(chunk, nid)
    {
        int thid = omp_get_thread_num();
        int thNbr = omp_get_num_threads();
        vector<double> times{};
        //RandomGen & rng = rngs[thid];
        Stats params = model.getParams();
        Stats local_stats{};
        unsigned seed = baseSeed * (1 + thid * 31) ;
        size_t lowerBound = (double) thid * graph.getInSize() / (double) thNbr;
       // #pragma omp critical
        //std::cout << thid << " lower bond " << lowerBound << std::endl;
    
        size_t upperBound = (double) (thid + 1) * graph.getInSize() / (double) thNbr;
        //#pragma omp critical
        //std::cout << thid << " upper bound " << upperBound << std::endl;
        #pragma omp barrier
        for (chunk = 0; chunk < chunks.size(); chunk ++) {
            local_stats.reset();
            //#pragma omp for schedule(static)
            for(nid = 0; nid < chunks[chunk].size(); nid +=2) {
               
                int i = chunks[chunk][nid] <  chunks[chunk][nid + 1] ? chunks[chunk][nid] : chunks[chunk][nid + 1];
                int j = chunks[chunk][nid] <=  chunks[chunk][nid + 1] ? chunks[chunk][nid + 1] : chunks[chunk][nid];
                
                if (lowerBound <= i && i < upperBound) {
                    //Node & node = i < j ? graph->getNode(i, j) : graph->getNode(j, i);
                    //const double start = omp_get_wtime();
                    //local_stats += model.applyGibbsProposal(&node, rng);
                    local_stats += applyGibbsProposalParallel(i,j, graph, params, &seed);
    
                    //const double duration = omp_get_wtime() - start;
                    //times.push_back(duration);
                }
            }
            #pragma omp critical
            stats += local_stats;
            
            #pragma omp barrier
            
            #pragma omp single
            {
                if (chunk % chunksNbr == 0) {
                    res.push_back(stats);
                }
            }
        }
        //double average = std::accumulate(times.begin(), times.end(), 0.0) / times.size();
        //std::cout << "Average time " << average << std::endl;

    }
    
    return res;
}

Stats applyGibbsProposalParallel(const int i, const int j, GraphWrapper & graph, Stats params, unsigned * seed) {
    Stats up{};
    Node & node = graph.getNode(i, j);
    State newState{};
    State oldState = node.getLabel() != NodeType::NIL ? State::ENABLED : State::DISABLED ;
    double upProb{0};
    NodeName nid = node.getName();
    NodeType activation = node.getActivationVal();
    const int activationVal = static_cast<int>(activation);
    //graph->computeChangeStatistics(nid.first, nid.second, up);
    int inCount = graph.getDimsIn(i);
    if (oldState == State::ENABLED && activation == NodeType::INTRA) inCount --;
    
    int outCount = graph.getDimsOut(i);
    if (oldState == State::ENABLED && activation == NodeType::INTER) outCount --;

    if (activation == NodeType::INTRA) {
        inCount += graph.getDimsIn(j);
        if (oldState == State::ENABLED) inCount --;
        
        outCount += graph.getDimsOut(j);
     }
    
     int idx = activationVal + static_cast<int>(NodeType::INTRA) - 2;
    
     up.set(idx, inCount);
     idx = activationVal + static_cast<int>(NodeType::INTER) - 2 ;
     up.set(idx, outCount);
    
    upProb = exp(params.dot(up));
    upProb = upProb / (1+ upProb);

    double r = (double) rand_r(seed) / (double) RAND_MAX;
    if(upProb > r) {
    newState = State::ENABLED;
    } else {
    newState = State::DISABLED;
    }

    node.updateNodeState(newState);

    if(newState == oldState) {
        return Stats();
    }else if (newState == State::ENABLED) {
        graph.enableNode(nid.first, nid.second);
        return up;
    } else {
        graph.disableNode(nid.first, nid.second);
        return -up;
    }
}
