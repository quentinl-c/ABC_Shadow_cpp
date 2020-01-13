//
//  ABCShadow.cpp
//  ABC_Shadow_cpp
//
//  Created by Quentin on 13/01/2020.
//  Copyright © 2020 Quentin. All rights reserved.
//

#include "ABCShadow.hpp"

ABCShadow::ABCShadow(MCMCSim* mcmcSim,
                     PottsModel* model,
                     RandomGen* rGen,
                     Stats yObs,
                     Stats theta0,
                     Stats delta,
                     int iter,
                     int n,
                     int samplerIt,
                     int samplerBy,
                     int samplerBurnin,
                     int minBound,
                     int maxBound,
                     bool gibbsSampler):
                     mcmcSim(mcmcSim),
                     model(model),
                     rGen(rGen),
                     yObs(yObs),
                     theta0(theta0),
                     delta(delta),
                     iter(iter),
                     n(n),
                     samplerIt(samplerIt),
                     samplerBy(samplerBy),
                     samplerBurnin(samplerBurnin),
                     minBound(minBound),
                     maxBound(maxBound),
                     gibbsSampler(gibbsSampler),
                     chain(iter){}

/*
 =====================================================
 Private methods
 =====================================================
 */

Stats ABCShadow::computeShadowChain(const Stats &theta) {
    Stats thetaRes = theta;
    Stats candidate, ySim;
    double alpha;

    model->setParams(thetaRes);
    
    if(gibbsSampler) {
        ySim = sampleFromGibbs();
    } else {
        ySim = sampleFromMH();
    }

    for(int i=0; i < n; i++) {
        candidate = getCandidate(thetaRes);
        alpha = computeDensityRatio(thetaRes, candidate, ySim);
        if(alpha > rGen->getUnifornRealD()) {
            thetaRes = candidate;
        }
    }
    return thetaRes;
}

Stats ABCShadow::sampleFromGibbs() {
    vector<Stats> samples = mcmcSim->gibbsSim(*rGen, *model, samplerIt, samplerBurnin, samplerBy);
    Stats avg{};
    int t = 1;
    for (auto el : samples) {
        avg = avg + (el - avg)/t;
        t +=1 ;
    }
    mcmcSim->reset();
    return avg;
}

Stats ABCShadow::sampleFromMH() {
    vector<Stats> samples = mcmcSim->mhSim(*rGen, *model, samplerIt, samplerBurnin, samplerBy);
    Stats avg{};
    int t = 1;
    for (auto el : samples) {
        avg = avg + (el - avg)/t;
        t +=1 ;
    }
    mcmcSim->reset();
    return avg;
}

Stats ABCShadow::getCandidate(const Stats &theta){
    int index = rGen->getUniformIntD(0, 2);
    Stats candidate = theta;
    double oldVal = candidate[index];
    double newVal = rGen->getUnifornRealD(oldVal - delta[index], oldVal + delta[index]);

    newVal = (newVal > maxBound) ? maxBound : newVal;
    newVal = (newVal < minBound) ? minBound : newVal;
    candidate.set(index, newVal);

    return candidate;
}


double ABCShadow::computeDensityRatio(const Stats &oldTheta,
                                      const Stats &candidateTheta,
                                      const Stats &ySim) {
    double p1, p2, q1, q2, ratio;

    model->setParams(candidateTheta);
    p1 = model->evaluateFromStats(yObs);
    q2 = model->evaluateFromStats(ySim);
    
    model->setParams(oldTheta);
    p2 = model->evaluateFromStats(yObs);
    q1 = model->evaluateFromStats(ySim);
    
    ratio = (exp(p1 - p2)) * (exp(q1 - q2));
    ratio = (ratio >= 1) ? 1 : ratio;
    return ratio;
}


/*
 =====================================================
 Public methods
 =====================================================
 */
void ABCShadow::runABCShadow() {
    cout << "Run the ABC" <<endl;
    
    Stats theta = theta0;
    chain.push_back(theta0);

    for(int i=0; i< iter - 1; i++) {
        theta = computeShadowChain(theta);
        if(i % 100 == 0) {
            cout <<  i << " iters - theta :" << theta << "\n";
        }
        chain.push_back(theta);
    }
}

void ABCShadow::saveChain(ofstream &outputfile) {
    if( !outputfile ) {
      cerr << "Error: file could not be opened" << endl;
      exit(1);
    }
    for(auto const &v : chain) {
        outputfile << v << endl;
    }
}
