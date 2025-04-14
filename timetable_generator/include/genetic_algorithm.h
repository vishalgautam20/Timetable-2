#ifndef GENETIC_ALGORITHM_H
#define GENETIC_ALGORITHM_H

#include <vector>
#include <random>
#include "structures.h"

using namespace std;

class GeneticTimetableOptimizer {
private:
    struct Chromosome {
        vector<vector<int>> genes; // Represents a complete timetable
        double fitness;
        
        Chromosome() : fitness(0.0) {}
    };
    
    const size_t POPULATION_SIZE = 100;  // Change to size_t
    const int MAX_GENERATIONS = 1000;
    const double MUTATION_RATE = 0.1;
    
    vector<Chromosome> population;
    
    void initializePopulation();
    double calculateFitness(const Chromosome& chromosome);
    Chromosome selectParent();
    Chromosome crossover(const Chromosome& parent1, const Chromosome& parent2);
    void mutate(Chromosome& chromosome);
    void convertChromosomeToTimetable(const Chromosome& chromosome, Timetable& tt);
    
public:
    void optimizeTimetable(Timetable& tt);
};

#endif