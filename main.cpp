#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <random>
#include <set>
#include <map>
#include <queue>
#include <sstream>
#include <thread>
#include <future>

using std::vector;
using std::pair;
using std::set;
using std::map;
using std::priority_queue;
using std::string;

std::random_device r;
std::default_random_engine e(r());

using Matching = vector<int>;

vector<Matching> allPermutations(int numCouples) {

    vector<Matching> matchings;

    Matching matching(numCouples);
    std::iota(matching.begin(), matching.end(), 0);

    do {
        matchings.push_back(matching);
    } while (std::next_permutation(matching.begin(), matching.end()));

    return matchings;
}

Matching randomChoice(vector<Matching>& options) {
    std::uniform_int_distribution<size_t> dist(0, options.size() - 1);
    return options.at(dist(e));
}

vector<vector<int>> getProbs(int n, vector<Matching>& candidates) {

    vector<vector<int>> probs(n, vector<int>(n, 0));
    for (const auto& matching : candidates) {
        for (int woman = 0; woman < n; woman++) {
            int man = matching[woman];
            probs[woman][man] += 1;
        }
    }
    return probs;
}

Matching pickGuess(int numCouples, vector<Matching>& candidates) {
    auto probs = getProbs(numCouples, candidates);
    Matching guess(numCouples, -1);

    set<int> menMatched;
    priority_queue<pair<double,pair<int,int>>> pq;

    std::uniform_real_distribution<double> dist(0, 0.1);

    for (int woman = 0; woman < numCouples; woman++) {
        for (int man = 0; man < numCouples; man++) {
            double prob = probs[woman][man] + dist(e);
            pq.push({ prob, { woman, man }});
        }
    }

    while (!pq.empty()) {
        auto [ woman, man ] = pq.top().second;
        pq.pop();

        if (guess[woman] < 0 && !menMatched.count(man)) {
            guess[woman] = man;
            menMatched.insert(man);
        }
    }

    for (auto man : guess) {
        assert(man >= 0);
    }
    return guess;

}

int equalSlots(const Matching& a, const Matching& b) {
    assert(a.size() == b.size());
    int count = 0; 
    for (int i = 0; i < a.size(); i++) {
        count += (a[i] == b[i]);
    }
    return count;
}

vector<Matching> doRound(const int numCouples,
                         const Matching& correct,
                         set<pair<int,int>>& checked,
                         vector<Matching>& candidates) {
    
    auto probs = getProbs(numCouples, candidates);
    vector<Matching> afterTruthBooth;
    vector<Matching> afterGuess;

    // find best truth booth candidate
    int bestR = -1, bestC = -1;
    for (int r = 0; r < numCouples; r++) {
        for (int c = 0; c < numCouples; c++) {
            if (checked.count({ r, c })) continue;

            if (bestR == -1 || probs[r][c] > probs[bestR][bestC]) {
                bestR = r;
                bestC = c;
            }
        }
    }
    assert(bestR >= 0 && bestC >= 0);

    // try a correct first truth booth
    if (checked.empty()) {
        bestR = 0;
        bestC = correct[bestR];
    }

    checked.insert({ bestR, bestC });


    // do truth booth
    if (correct[bestR] == bestC) {
        for (auto& matching : candidates) {
            if (matching[bestR] == bestC) afterTruthBooth.push_back(matching);
        }
    } else {
        for (auto& matching : candidates) {
            if (matching[bestR] != bestC) afterTruthBooth.push_back(matching);
        }
    }
 
    auto guess = pickGuess(numCouples, afterTruthBooth);

    int hits = 0;
    for (int i = 0; i < numCouples; i++) {
        if (guess[i] == correct[i]) hits++;
    }

    // filter our matchings
    for (const auto& matching : afterTruthBooth) {
        if (equalSlots(matching, guess) == hits) {
            afterGuess.push_back(matching);
        }
    }

    return afterGuess;
}

bool simulate(int numCouples) {

    auto candidates = allPermutations(numCouples);
    auto correct = randomChoice(candidates);

    set<pair<int,int>> checked;

    for (int round = 0; round < numCouples; round++) {
        candidates = doRound(numCouples, correct, checked, candidates);
        assert(!candidates.empty());     
    }
    return candidates.size() == 1;
}

int thrFunc(int numSimulations, int numCouples, int id) {
    printf("thr %d: got %d work to do\n", id, numSimulations);
    int good = 0;
    for (int i = 0; i < numSimulations; i++) {
        good += simulate(numCouples);
        printf("thr: %d, %f\n", id, (double) good / (double) (i + 1));
    }
    return good;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        printf("usage: ./simulate <num couples> <num simulations>\n");
        return -2;
    }

    int numCouples = std::stoi(argv[1]);
    int numSimulations = std::stoi(argv[2]);

    int numThreads = std::thread::hardware_concurrency();
    int perThread = (numSimulations + numThreads - 1) / numThreads;

    vector<std::future<int>> futures;
    int workRemaining = numSimulations;
    for (int i = 0; i < numThreads; i++) {
        int thrWork = std::min(workRemaining, perThread);
        workRemaining -= thrWork;
        futures.push_back(std::async(thrFunc, thrWork, numCouples, i));
    }

    int totalGood = 0;
    for (auto& future : futures) {
        totalGood += future.get();
    }

    printf("%f\n", (double) totalGood / (double) numSimulations);
act
    return 0;
}