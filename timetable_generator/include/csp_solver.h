#ifndef CSP_SOLVER_H
#define CSP_SOLVER_H

#include <vector>
#include <string>
#include "structures.h"

using namespace std;

class CSPTimetableSolver {
public:
    struct Variable {
        int slot;
        int day; 
        string subject;
        vector<string> domain;
    };
    
    struct Constraint {
        bool (*check)(const Variable&, const Variable&);
    };
    
    bool solveTimetable(Timetable& tt);

private:    
    vector<Variable> variables;
    vector<Constraint> constraints;
    
    void initializeVariables(Timetable& tt);
    void addHardConstraints();
    void addSoftConstraints();
    bool backtrack(vector<Variable>& assignment);
    bool isConsistent(const Variable& var, const vector<Variable>& assignment);
    Variable selectUnassignedVariable(const vector<Variable>& assignment);
    void updateTimetable(Timetable& tt, const vector<Variable>& assignment);
};

#endif