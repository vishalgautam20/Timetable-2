#include "../include/csp_solver.h"
#include <algorithm>
#include <stdexcept>

using namespace std;

void CSPTimetableSolver::initializeVariables(Timetable& tt) {
    // Initialize variables from timetable
    for (size_t day = 0; day < tt.subject_id.size(); day++) {
        for (size_t slot = 0; slot < tt.subject_id[day].size(); slot++) {
            Variable var;
            var.day = day;
            var.slot = slot;
            variables.push_back(var);
        }
    }
}

void CSPTimetableSolver::addHardConstraints() {
    // Add hard constraints
    Constraint c;
    c.check = [](const Variable& v1, const Variable& v2) {
        // Check if assignments are valid
        if (v1.day == v2.day && v1.slot == v2.slot) {
            return false; // Can't schedule same time
        }
        return true;
    };
    constraints.push_back(c);
}

void CSPTimetableSolver::addSoftConstraints() {
    // Add soft constraints
    Constraint c;
    c.check = [](const Variable& v1, const Variable& v2) {
        // Implement soft constraints
        return true;
    };
    constraints.push_back(c);
}

CSPTimetableSolver::Variable CSPTimetableSolver::selectUnassignedVariable(
    const vector<Variable>& assignment) {
    for (const Variable& var : variables) {
        if (find_if(assignment.begin(), assignment.end(),
            [&var](const Variable& a) { 
                return a.day == var.day && a.slot == var.slot; 
            }) == assignment.end()) {
            return var;
        }
    }
    throw runtime_error("No unassigned variables found");
}

bool CSPTimetableSolver::isConsistent(const Variable& var, const vector<Variable>& assignment) {
    for (const auto& constraint : constraints) {
        for (const auto& assigned : assignment) {
            if (!constraint.check(var, assigned)) {
                return false;
            }
        }
    }
    return true;
}

void CSPTimetableSolver::updateTimetable(Timetable& tt, const vector<Variable>& assignment) {
    for (const auto& var : assignment) {
        tt.subject_id[var.day][var.slot] = stoi(var.subject);
    }
}

bool CSPTimetableSolver::solveTimetable(Timetable& tt) {
    initializeVariables(tt);
    addHardConstraints();
    addSoftConstraints();
    
    vector<Variable> assignment;
    if (backtrack(assignment)) {
        updateTimetable(tt, assignment);
        return true;
    }
    return false;
}

bool CSPTimetableSolver::backtrack(vector<Variable>& assignment) {
    if (assignment.size() == variables.size()) {
        return true;
    }
    
    Variable var = selectUnassignedVariable(assignment);
    
    for (const string& value : var.domain) {
        var.subject = value; // Use the value
        if (isConsistent(var, assignment)) {
            assignment.push_back(var);
            if (backtrack(assignment)) {
                return true;
            }
            assignment.pop_back();
        }
    }
    return false;
}