//
//  MCMCParallelSim.hpp
//  ABC_Shadow_cpp
//
//  Created by Quentin on 25/06/2020.
//  Copyright © 2020 Quentin. All rights reserved.
//

#ifndef MCMCParallelSim_hpp
#define MCMCParallelSim_hpp

#include <iostream>
#include <algorithm>
#include <numeric>
#include "omp.h"
#include "GraphWrapper.hpp"
#include "PottsModel.hpp"

using std::exp;
using std::min;
using std::max;

vector<Stats> parallelGibbsSim(PottsModel &model, GraphWrapper & graph, vector<vector<int>> & chunks, const int chunksNbr, const unsigned baseSeed, const int threadsNbr, const int sim_iter, const int by=1, const int burnin=0);

class MCMCParallelInfo {
    
private:
    int simIters, threadsNbr, chunkSize, chunksNbr, chunksNbrIn, chunksNbrOut;
    vector<unsigned> seeds;
    vector<vector<int>> poolsIn, poolsOut, chunks;
    

public:
    MCMCParallelInfo(const int simIter, const int threadsNbr, GraphWrapper& graph);
    int getThreadsNbr() const;
    int getChunksNbr() const;
    vector<vector<int>>& getChunks();
    void generateIndependentNodes();

    
};
#endif /* MCMCParallelSim_hpp */
