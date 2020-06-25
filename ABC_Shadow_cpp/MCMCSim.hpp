//
//  MCMCSim.hpp
//  ABC_Shadow_cpp
//
//  Created by Quentin on 06/08/2019.
//  Copyright © 2019 Quentin. All rights reserved.
//

#ifndef MCMCSim_hpp
#define MCMCSim_hpp

#include <iostream>
#include <algorithm>
#include <numeric>
#include "PottsModel.hpp"
#include "GraphWrapper.hpp"
#include "RandomGen.hpp"
#include "omp.h"
#include <chrono> // for std::chrono functions  
#include <cassert>
#define assertm(exp, msg) assert(((void)msg, exp))

using std::exp;
using std::min;
using std::max;
using std::iota;
using std::shuffle;
using std::copy;

using clock_time = std::chrono::high_resolution_clock;
using second_t = std::chrono::duration<double, std::ratio<1> >;


class MCMCSim {
private:
    GraphWrapper *graph;
    
    vector<RandomGen> rngs;
    
public:
    
    MCMCSim(GraphWrapper *g);
    ~MCMCSim();
    
    void setGraph(GraphWrapper *new_g);
    
    void reset();
  
    vector<Stats> mhSim(RandomGen &rGen, PottsModel &model, const int iter_max, const int by=1, const int burnin=0);
    vector<Stats> gibbsSim(RandomGen &rGen, PottsModel &model, const int iter_max, const int by=1, const int burnin=0);
};

#endif /* MCMCSim_hpp */
