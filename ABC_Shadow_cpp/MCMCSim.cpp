//
//  MCMCSim.cpp
//  ABC_Shadow_cpp
//
//  Created by Quentin on 06/08/2019.
//  Copyright © 2019 Quentin. All rights reserved.
//

#include "MCMCSim.hpp"

MCMCSim::MCMCSim(GraphWrapper *g): graph(g),
                                   chunkSize(0),
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
    int const totalNodes = inNodes + outNodes;
    
    std::cout << "inNodes " << inNodes << std::endl;
    std::cout << "outNodes " << outNodes << std::endl;
    
    double const outNodesProportion = (double) outNodes / totalNodes;
    std::cout << outNodesProportion << std::endl;
    int const ratio = (2.0 - outNodesProportion) / outNodesProportion;
    
    chunkSizeOut = min((int) inSize / ratio, outSize);
    chunkSizeIn = chunkSizeOut * ratio;
    
    if (ratio < 2) {
        chunkSizeOut -= 2; // At least one inNode must be computed
    }
    
    chunkSize = chunkSizeIn + chunkSizeOut;
    
    if (chunkSize % 2 != 0) { // if the chunksize is uneven, decrement the most import side of the chunk (In/Out)
        if (chunkSizeIn - chunkSizeOut > 2) {
            chunkSizeIn --;
        } else {
            chunkSizeOut --;
        }
        chunkSize --;
    }
    std::cout << "chunkSizeIn " << chunkSizeIn << std::endl;
    std::cout << "chunkSizeOut " << chunkSizeOut << std::endl;
    
    chunksNbr = totalNodes / (chunkSize / 2);
    
    chunks = vector<int>(chunksNbr * chunkSize * simIter);
    poolIn = vector<int>(chunksNbr * inSize * simIter);
    poolOut = vector<int>(chunksNbr * outSize * simIter);
    
    for (int i = 0; i < poolIn.size(); i += inSize) {
        iota(poolIn.begin() + i, poolIn.begin() + i + inSize, 0);
        assert(i + inSize <= poolIn.size());
    }
    assert(poolIn.back() == inSize - 1);
    for (int i = 0; i < poolOut.size(); i+= outSize) {
        iota(poolOut.begin() + i, poolOut.begin() + i + outSize, inSize);
        assert(i + outSize <= poolOut.size());
    }
    assert(poolOut.back() == inSize + outSize - 1);
}

void MCMCSim::generateIndependentNodes() {
    std::cout << "test " << threadsNbr << std::endl;
    omp_set_num_threads(threadsNbr);
            std::cout << "chunk size " << chunks.size() << std::endl;
    #pragma omp parallel
    {
    const int inSize = graph->getInSize();
    const int outSize = graph->getOutSize();
    const int threadId = omp_get_thread_num();

    #pragma omp barrier
    
    #pragma omp for
    for (int i = 0; i < chunksNbr * parallelSimIters ; i++) {
        //#pragma omp critical
        //std::cout << "th, i " << threadId << " " << i << std::endl;
        shuffle(poolIn.begin() + i * inSize, poolIn.begin() + (i + 1) * inSize, rngs[threadId].getEngine());
        //assert((i + 1) * inSize <= poolIn.size());

        shuffle(poolOut.begin() + i * outSize, poolOut.begin() + (i + 1) * outSize, rngs[threadId].getEngine());
        //assert((i + 1) * outSize <= poolOut.size());
        const int exclusiveIn = chunkSizeIn - chunkSizeOut;
        //std::cout << "exclusiveIn " << exclusiveIn << std::endl;
        //assertm(i * inSize < poolIn.size(), "i * inSize < poolIn.size()");
        //assertm(i * chunkSize + exclusiveIn < chunks.size(), "i * chunkSize < chunks.size()");
        copy(poolIn.begin() + i * inSize, poolIn.begin() + i * inSize + exclusiveIn, chunks.begin() + i * chunkSize);
        
        int index = exclusiveIn;
        for (int j = exclusiveIn; j < chunkSize; j += 2) {
            chunks[i * chunkSize + j] = poolIn[i * inSize + index];
            //assert(i * chunkSize + j < chunks.size());
            //assert(i * inSize + index < poolIn.size());
            chunks[i * chunkSize + j + 1] = poolOut[i * outSize + index - exclusiveIn];
            //assert(i * chunkSize + j + 1 < chunks.size());
            //assert(i * outSize + index - exclusiveIn < chunks.size());
            index++;
        }
        //auto it = std::unique(chunks.begin() + i * chunkSize, chunks.begin() + (i + 1) * chunkSize);
        //assert(it == chunks.begin() + (i + 1) * chunkSize);
    }
    }
    /*for(int j = 0; j < chunks.size(); j ++) {
        if (j % chunkSize == 0) {
            std::cout << " || ";
        }
        std::cout << chunks[j] << " ";
    }
    std::cout << std::endl;*/
}

bool MCMCSim::isParallelSetupReady(const int threads_nbr) {
    return parallelSimIters != 0 && chunks.size() != 0 && rngs.size() != 0;
}

vector<Stats> MCMCSim::parallelGibbsSim(PottsModel &model, const int threads_nbr, const int sim_iter, const int by, const int burnin) {
    vector<Stats> res{};
    Stats stats(model.getStats(*graph));
    int iter_max = sim_iter * chunksNbr;
    int gibbs_iter, chunk;
    
    omp_set_num_threads(threadsNbr);
    
    #pragma omp parallel private(gibbs_iter, chunk)
    {
        int thid = omp_get_thread_num();
        RandomGen & rng = rngs[thid];
        Stats local_stats{};

        #pragma omp barrier
        for (gibbs_iter = 0; gibbs_iter < iter_max; gibbs_iter ++) {
            local_stats.reset();
            #pragma omp for schedule(static)
            for(chunk = 0; chunk < chunkSize; chunk +=2) {
               
                int i = chunks[gibbs_iter + chunk];
                int j = chunks[gibbs_iter + chunk + 1];
                #pragma omp critical
                std::cout << thid << " => " << gibbs_iter << " | " << chunk << " : " << i << ", " << j << std::endl;
                Node & node = i < j ? graph->getNode(i, j) : graph->getNode(j, i);
                local_stats += model.applyGibbsProposal(&node, rng);
                
            }
            #pragma omp critical
            stats += local_stats;
            
            #pragma omp barrier
            
            #pragma omp single
            {
                if (gibbs_iter % chunksNbr == 0) {
                    res.push_back(stats);
                }
            }
        }
    }
    
    return res;
}
