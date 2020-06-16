//
//  MCMCSim.cpp
//  ABC_Shadow_cpp
//
//  Created by Quentin on 06/08/2019.
//  Copyright © 2019 Quentin. All rights reserved.
//

#include "MCMCSim.hpp"

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

    vector<Node> &nodes{graph->getNodes()};
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

            Node &current_node = nodes[i];
            old_label = *current_node.getLabel();
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
    vector<Node> &nodes{graph->getNodes()};
    size_t nodeNbr = nodes.size();
    Stats stats(model.getStats(*graph));

    for (int gibbs_iter{0}; gibbs_iter < iter_max; ++gibbs_iter) {

        if (gibbs_iter >= burnin && gibbs_iter % by == 0) {
            res.push_back(stats);
        }
        for (size_t i{0}; i < nodeNbr; i++) {
            Node &current_node = nodes[i];
            stats += model.applyGibbsProposal(&current_node, rGen);
        }
    }

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
    chunksNbr = chunksNbrIn + chunksNbrOut; // number of chunks for one iteration
    chunks = vector<vector<int>>(chunksNbr * simIter);
    
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

vector<Stats> MCMCSim::parallelGibbsSim(PottsModel &model, const int threads_nbr, const int sim_iter, const int by, const int burnin) {

    vector<Stats> res{};
    Stats stats(model.getStats(*graph));
    size_t chunk, nid;
    
    omp_set_num_threads(threadsNbr);
    
    #pragma omp parallel private(chunk, nid)
    {
        int thid = omp_get_thread_num();
        RandomGen & rng = rngs[thid];
        Stats local_stats{};
        int counter{0};

        #pragma omp barrier
        for (chunk = 0; chunk < chunks.size(); chunk ++) {
            local_stats.reset();
            #pragma omp for
            for(nid = 0; nid < chunks[chunk].size(); nid +=2) {
               
                int i = chunks[chunk][nid];
                int j = chunks[chunk][nid + 1];
                //#pragma omp critical
                //std::cout << thid << " => " << chunk << " | " << nid << " : " << i << ", " << j << std::endl;
                counter ++;
                Node & node = i < j ? graph->getNode(i, j) : graph->getNode(j, i);
                local_stats += model.applyGibbsProposal(&node, rng);
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
        #pragma omp critical
        std::cout << "Counter " << counter << std::endl;
//#pragma omp critical
//        std::cout << "Time taken by thread " << thid << " " << wtime << "s." << std::endl;
    }
    
    return res;
}
