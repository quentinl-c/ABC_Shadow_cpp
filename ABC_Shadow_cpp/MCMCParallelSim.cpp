//
//  MCMCParallelSim.cpp
//  ABC_Shadow_cpp
//
//  Created by Quentin on 25/06/2020.
//  Copyright © 2020 Quentin. All rights reserved.
//

#include "MCMCParallelSim.hpp"

Stats applyGibbsProposalParallel(const int i,
                                 const int j,
                                 GraphWrapper& graph,
                                 Stats params,
                                 unsigned * seed,
                                 vector<double>& times);

MCMCParallelInfo::MCMCParallelInfo(const int simIters,
                                   const int threadsNbr,
                                   GraphWrapper& graph) {
    this->simIters = simIters;
    int const inSize = graph.getInSize();
    int const outSize = graph.getOutSize();
    
    int const inNodes = inSize * (inSize - 1) / 2;
    int const outNodes = inSize * outSize;

    
    std::cout << "inNodes " << inNodes << std::endl;
    std::cout << "outNodes " << outNodes << std::endl;
    
    const int minSize = min(inSize, outSize);
    chunkSize = minSize % 2 == 0 ? minSize : minSize - 1;
    
    chunksNbrIn = max(inNodes / (chunkSize / 2), 1); // number of chunks "In" for one iteration
    chunksNbrOut = max(outNodes / (chunkSize / 2), 1); // number of chunks "Out" for one iteration
    std::cout << "Generated nodes " <<(chunkSize / 2) * chunksNbrIn +  (chunkSize / 2) * chunksNbrOut << std::endl;
    std::cout << "Generated In nodes " << (chunkSize / 2) * chunksNbrIn << std::endl;
    chunksNbr = chunksNbrIn + chunksNbrOut; // number of chunks for one iteration
    chunks = vector<vector<int>>(chunksNbr * simIters);

    std::cout << "chunks size " << chunks.size() << std::endl;

    for (int iter = 0; iter < simIters; iter ++) {
        for(int cId = 0; cId  < chunksNbr; cId ++) {
            const int idx = iter * chunksNbr + cId;
            chunks[idx] = vector<int>(chunkSize);
        }
    }
    
    poolsIn = vector<vector<int>>(omp_get_max_threads());
    poolsOut = vector<vector<int>>(omp_get_max_threads());

    for (int i = 0; i < poolsIn.size(); i++) {
        poolsIn[i] = vector<int>(inSize);
        iota(poolsIn[i].begin(), poolsIn[i].end(), 0);
    }

    for (int i = 0; i < poolsOut.size(); i++) {
        poolsOut[i] = vector<int>(outSize);
        iota(poolsOut[i].begin(), poolsOut[i].end(), inSize);
    }
}
void MCMCParallelInfo::generateIndependentNodes() {
    
    omp_set_num_threads(omp_get_max_threads());
    std::cout << "max threads " << omp_get_max_threads() << std::endl;
    #pragma omp parallel
    {
        const int thid = omp_get_thread_num();
        const unsigned seed = 2019 * (1 + thid);
        mt19937 engine{seed};
        #pragma omp barrier
    
        #pragma omp for
        for (int iter = 0; iter < simIters ; iter++) {
        
            for (int cId = 0; cId < chunksNbr; cId++) {
                const int idx = iter * chunksNbr + cId;
                shuffle(poolsIn[thid].begin(), poolsIn[thid].end(), engine);
                if (cId < chunksNbrIn) {
                    copy(poolsIn[thid].begin(), poolsIn[thid].begin() + chunkSize, chunks[idx].begin());
                } else {
                    shuffle(poolsOut[thid].begin(), poolsOut[thid].end(), engine);
                    for (int i = 0; i < chunks[idx].size(); i+=2) {
                        chunks[idx][i] = poolsIn[thid][i / 2];
                        chunks[idx][i + 1] = poolsOut[thid][i / 2];
                    }
                }
            }
    
        }
    }
}

int MCMCParallelInfo::getThreadsNbr() const {
    return threadsNbr;
}

int MCMCParallelInfo::getChunksNbr() const {
    return chunksNbr;
}

vector<vector<int>>& MCMCParallelInfo::getChunks() {
    return chunks;
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
            //#pragma omp simd
            vector<int>& currentChunk = chunks[chunk];
            for(nid = 0; nid < currentChunk.size(); nid +=2) {
               
                int i = currentChunk[nid] <  currentChunk[nid + 1] ? currentChunk[nid] : currentChunk[nid + 1];
                int j = currentChunk[nid] <=  currentChunk[nid + 1] ? currentChunk[nid + 1] : currentChunk[nid];
                
                if (lowerBound <= i && i < upperBound) {
                    //Node & node = i < j ? graph->getNode(i, j) : graph->getNode(j, i);
                    //const double start = omp_get_wtime();
                    //local_stats += model.applyGibbsProposal(&node, rng);
                    local_stats += applyGibbsProposalParallel(i, j, graph, params, &seed, times);
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

Stats applyGibbsProposalParallel(const int i, const int j, GraphWrapper & graph, Stats params, unsigned * seed, vector<double>& times) {
    Stats up{};
    //const double start = omp_get_wtime();
    Node node = graph.getRefNode(i, j);
    //times.push_back(omp_get_wtime() - start);
    State newState{};
    State oldState = node.getLabel() != NodeType::NIL ? State::ENABLED : State::DISABLED ;
    //State oldState = State::ENABLED;
    double upProb{0};
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

    //node.updateNodeState(newState);

    if(newState == oldState) {
        return Stats();
    }else if (newState == State::ENABLED) {
        graph.enableNode(i, j);
        return up;
    } else {
        graph.disableNode(i, j);
        return -up;
    }
}
