#ifndef STRUCTURES_H
#define STRUCTURES_H

#include <string>
#include <vector>
#include <map>

using namespace std;

// Add TimeSlot structure
struct TimeSlot {
    int subject_id;
    string teacher;
    string room;
    bool is_lab;
    int lab_duration;
    
    TimeSlot() : subject_id(-1), is_lab(false), lab_duration(0) {}
};

// Forward declarations
struct Subject;
struct Lab;
struct Timetable;
struct SemesterData;

// Define Subject first since it's used by others
struct Subject {
    string name;
    string teacher;
    int credits;
    bool is_elective;
};

// Define Lab next
struct Lab {
    string name;
    string center;
    string instructor;
    int duration;           // Number of slots needed (not minutes)
    int total_slots;        // Same as duration
    int max_slots_per_day;  // Maximum slots per day
    int day = -1;
    int start_slot = -1;
    bool is_scheduled = false;
    vector<string> required_equipment;
};

// Define Timetable
struct Timetable {
    vector<vector<int>> subject_id;
    vector<vector<string>> teacher;
    vector<vector<string>> room;
    vector<vector<bool>> is_lab;          // Indicates if slot is a lab
    vector<vector<string>> lab_details;   // Stores lab names
    vector<vector<bool>> is_lab_slot;     // Track consecutive lab slots
};

// Define SemesterData
struct SemesterData {
    vector<vector<Subject>> subjects;
    vector<vector<Lab>> stream_labs;
    vector<int> subject_count;
    vector<vector<Timetable>> timetables;
    vector<string> stream_names;
    vector<int> sections_per_stream;
};

#endif
