#include "../include/timetable_generator.h"
#include "../include/csp_solver.h"          // Add this line
#include "../include/genetic_algorithm.h"    // Add this line
#include <iostream>
#include <fstream>
#include <numeric>
#include <algorithm>
#include <stdexcept>
#include "../include/utils.h"

using namespace std;

/*
void TimetableGenerator::run() {
    try {
        initialize();
        generate_timetables();
        generate_teacher_timetables();
        save_to_txt_column_wise();
        save_to_csv_column_wise();
        save_individual_section_timetables();
        
        cout << "\nTimetables saved successfully\n";
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << "\n";
        cout << "Would you like to try again? (y/n): ";
        char response;
        cin >> response;
        if (tolower(response) == 'y') {
            run();
        } else {
            cout << "Exiting program...\n";
            exit(1);
        }
    }
}
*/

void TimetableGenerator::initialize() {
    srand(static_cast<unsigned int>(time(nullptr)));
    initialize_data_structures();
}

void TimetableGenerator::validate_inputs() {
    if (stream_names.empty() || total_teaching_slots <= 0) {
        throw runtime_error("Invalid configuration: no streams/classes or slots");
    }
    if (lunch_break_time < 0) {
        throw runtime_error("Invalid lunch break time");
    }
    
    for (size_t stream = 0; stream < stream_names.size(); stream++) {
        if (subject_count[stream] == 0) {
            cerr << "Warning: No subjects defined for " << stream_names[stream] << endl;
        }
    }
}

void TimetableGenerator::initialize_data_structures() {
    timetables.clear();
    
    for (size_t stream = 0; stream < stream_names.size(); stream++) {
        timetables.push_back(vector<Timetable>(sections_per_stream[stream]));
        
        for (int section = 0; section < sections_per_stream[stream]; section++) {
            auto& tt = timetables[stream][section];
            tt.subject_id.resize(DAY_NAMES.size(), vector<int>(total_teaching_slots, -1));
            tt.teacher.resize(DAY_NAMES.size(), vector<string>(total_teaching_slots));
            tt.room.resize(DAY_NAMES.size(), vector<string>(total_teaching_slots));
            tt.is_lab.resize(DAY_NAMES.size(), vector<bool>(total_teaching_slots, false));
            tt.lab_details.resize(DAY_NAMES.size(), vector<string>(total_teaching_slots));
            tt.is_lab_slot.resize(DAY_NAMES.size(), vector<bool>(total_teaching_slots, false));
        }
    }
}

void TimetableGenerator::generate_timetables() {
    // Create base directory for all semesters
    string base_dir = "timetables";
    system(("mkdir -p " + base_dir).c_str());
    
    // Process each semester
    for (size_t sem = 0; sem < semesters.size(); sem++) {
        current_semester = sem;
        cout << "\nProcessing semester " << (sem + 1) << ": " 
             << semesters[sem].term << " " << semesters[sem].year << "\n";
        
        // Load semester-specific data
        auto& sem_data = semester_data_store[sem];
        stream_names = sem_data.stream_names;
        sections_per_stream = sem_data.sections_per_stream;
        subjects = sem_data.subjects;
        stream_labs = sem_data.stream_labs;
        subject_count = sem_data.subject_count;
        
        // Clear state for new semester
        clear_semester_state();
        initialize_data_structures();
        
        // Generate timetables for each stream and section
        for (size_t stream = 0; stream < stream_names.size(); stream++) {
            for (int section = 0; section < sections_per_stream[stream]; section++) {
                cout << "Generating timetable for " << stream_names[stream] 
                     << " Section " << static_cast<char>('A' + section) << "\n";
                     
                generate_timetable(timetables[stream][section], stream);
            }
        }
        
        // Store generated timetables in semester data
        sem_data.timetables = timetables;
        semester_data_store[sem] = sem_data;
        
        // Save to timetables directory with proper semester naming
        string sem_dir = base_dir + "/semester_" + to_string(sem + 1) + "_" + 
                        semesters[sem].term + "_" + to_string(semesters[sem].year);
        save_semester_timetables(sem_dir);
    }
    
    // Update master index file
    update_master_index(base_dir);
}

// Add new helper function
void TimetableGenerator::update_master_index(const string& base_dir) {
    string index_file = base_dir + "/semester_index.txt";
    ofstream index(index_file);
    if (index) {
        index << "TIMETABLE GENERATION SUMMARY\n";
        index << "==========================\n\n";
        index << "Total Semesters: " << semesters.size() << "\n\n";
        
        for (size_t sem = 0; sem < semesters.size(); sem++) {
            string sem_dir = "semester_" + to_string(sem + 1) + "_" + 
                           semesters[sem].term + "_" + to_string(semesters[sem].year);
            
            index << "Semester " << (sem + 1) << ":\n";
            index << "- Term: " << semesters[sem].term << "\n";
            index << "- Year: " << semesters[sem].year << "\n";
            index << "- Directory: " << sem_dir << "\n\n";
        }
    }
}

void TimetableGenerator::generate_timetable(Timetable &tt, int stream_idx) {
    if (stream_idx < 0 || stream_idx >= static_cast<int>(subjects.size())) {
        throw runtime_error("Invalid stream index in generate_timetable");
    }
    
    calculate_lunch_slot();
    clear_semester_state();
    
    try {
        schedule_labs(tt, stream_idx);
        assign_regular_subjects(tt, stream_idx);
        validate_timetable(tt, stream_idx);
    } catch (const exception& e) {
        cerr << "Error generating timetable: " << e.what() << endl;
        throw;
    }
}

void TimetableGenerator::calculate_lunch_slot() {
    int current_time = start_hour * 60;
    lunch_break_slot = -1;
    
    for (int slot = 0; slot < total_teaching_slots; slot++) {
        int slot_end_time = current_time + slot_durations[slot];
        
        if (current_time <= lunch_break_time && lunch_break_time < slot_end_time) {
            lunch_break_slot = slot;
            break;
        }
        current_time = slot_end_time + break_duration;
    }
}

bool TimetableGenerator::check_room_teacher_conflicts(int day, int slot, 
    const string& teacher, const string& room) {
    
    pair<int,int> time_slot = {day, slot};

    // Check current semester conflicts
    if (occupied_rooms[time_slot][room] || occupied_teachers[time_slot][teacher]) {
        return true;
    }

    // Check previous assignments in all sections
    for (size_t stream = 0; stream < stream_names.size(); stream++) {
        for (int section = 0; section < sections_per_stream[stream]; section++) {
            const auto& other_tt = timetables[stream][section];
            if (!other_tt.teacher[day][slot].empty()) {
                if (other_tt.teacher[day][slot] == teacher || 
                    other_tt.room[day][slot] == room) {
                    return true;
                }
            }
        }
    }

    return false;
}

bool TimetableGenerator::can_place_subject(int day, int slot, 
    const string& teacher, const string& room) {
    
    pair<int,int> time_slot = {day, slot};
    
    // Check if room or teacher is already occupied
    if (occupied_rooms[time_slot][room] || occupied_teachers[time_slot][teacher]) {
        return false;
    }

    // Check all existing timetables for this semester
    for (size_t stream = 0; stream < stream_names.size(); stream++) {
        for (int section = 0; section < sections_per_stream[stream]; section++) {
            const auto& other_tt = timetables[stream][section];
            if (!other_tt.room[day][slot].empty()) {
                // Check for room or teacher conflict
                if (other_tt.room[day][slot] == room || 
                    other_tt.teacher[day][slot] == teacher) {
                    return false;
                }
            }
        }
    }
    
    // Check for lab conflicts
    for (const auto& stream_labs : stream_labs) {
        for (const auto& lab : stream_labs) {
            if (lab.day == day && lab.start_slot <= slot && 
                slot < lab.start_slot + ceil(static_cast<float>(lab.duration) / slot_durations[0])) {
                if (lab.center == room || lab.instructor == teacher) {
                    return false;
                }
            }
        }
    }

    return true;
}

void TimetableGenerator::record_assignment(int day, int slot, 
    const string& teacher, const string& room) {
    
    pair<int,int> time_slot = {day, slot};
    
    // Clear any existing assignments
    if (occupied_rooms[time_slot].find(room) != occupied_rooms[time_slot].end()) {
        occupied_rooms[time_slot][room] = false;
    }
    if (occupied_teachers[time_slot].find(teacher) != occupied_teachers[time_slot].end()) {
        occupied_teachers[time_slot][teacher] = false;
    }
    
    // Record new assignment
    occupied_rooms[time_slot][room] = true;
    occupied_teachers[time_slot][teacher] = true;
    teacher_schedule[teacher][day].insert(slot);
    room_schedule[room][day].insert(slot);
}

void TimetableGenerator::assign_regular_subjects(Timetable &tt, int stream_idx) {
    if (stream_idx >= static_cast<int>(subject_count.size())) {
        cerr << "Warning: Invalid stream index in assign_regular_subjects\n";
        return;
    }
    
    vector<int> subject_assigned(subject_count[stream_idx], 0);
    int total_subjects = subject_count[stream_idx];
    
    // Create pools of available rooms and time slots
    vector<string> available_rooms;
    for (int i = 1; i <= total_rooms; i++) {
        available_rooms.push_back("Room " + to_string(i));
    }
    
    vector<pair<int, int>> time_slots;
    for (size_t day = 0; day < DAY_NAMES.size(); day++) {
        for (int slot = 0; slot < total_teaching_slots; slot++) {
            if (slot != lunch_break_slot) {
                time_slots.emplace_back(day, slot);
            }
        }
    }
    
    // Try to assign each subject required number of times
    for (int subject_idx = 0; subject_idx < total_subjects; subject_idx++) {
        int attempts = 0;
        const int MAX_ATTEMPTS = 200;  // Increased from 100
        
        while (subject_assigned[subject_idx] < MAX_ASSIGNMENTS && attempts < MAX_ATTEMPTS) {
            attempts++;
            
            // Randomly select time slot and room
            random_shuffle(time_slots.begin(), time_slots.end());
            random_shuffle(available_rooms.begin(), available_rooms.end());
            
            for (const auto& [day, slot] : time_slots) {
                if (tt.subject_id[day][slot] != -1) continue;
                if (subject_already_on_day(tt, day, subject_idx)) continue;
                
                const string& teacher = subjects[stream_idx][subject_idx].teacher;
                
                // Try each available room
                for (const string& room : available_rooms) {
                    if (can_place_subject(day, slot, teacher, room)) {
                        // Place subject
                        tt.subject_id[day][slot] = subject_idx;
                        tt.teacher[day][slot] = teacher;
                        tt.room[day][slot] = room;
                        record_assignment(day, slot, teacher, room);
                        subject_assigned[subject_idx]++;
                        goto next_assignment;  // Break both loops
                    }
                }
            }
            
            next_assignment:
            if (subject_assigned[subject_idx] >= MAX_ASSIGNMENTS) break;
        }
        
        if (subject_assigned[subject_idx] < MAX_ASSIGNMENTS) {
            cerr << "Warning: Could only assign subject " 
                 << subjects[stream_idx][subject_idx].name
                 << " " << subject_assigned[subject_idx] 
                 << " times (target: " << MAX_ASSIGNMENTS << ")\n";
        }
    }
}

void TimetableGenerator::schedule_labs(Timetable &tt, int stream_idx) {
    if (stream_idx >= static_cast<int>(stream_labs.size())) {
        return;
    }

    // Sort labs by duration (longer labs first)
    vector<Lab> sorted_labs = stream_labs[stream_idx];
    sort(sorted_labs.begin(), sorted_labs.end(),
         [](const Lab& a, const Lab& b) { return a.duration > b.duration; });

    for (Lab &lab : sorted_labs) {
        if (!schedule_lab_session(tt, lab, stream_idx)) {
            cerr << "Warning: Could not schedule lab " << lab.name << endl;
        }
    }
}

bool TimetableGenerator::schedule_lab_session(Timetable& tt, Lab& lab, int stream_idx) {
    int slots_needed = lab.duration;
    lab.total_slots = slots_needed;
    
    // Create a vector of all possible days
    vector<int> available_days(DAY_NAMES.size());
    iota(available_days.begin(), available_days.end(), 0);
    random_shuffle(available_days.begin(), available_days.end());

    for (int day : available_days) {
        // Check if this day already has a lab
        int labs_on_day = 0;
        for (int s = 0; s < total_teaching_slots; s++) {
            if (tt.is_lab[day][s]) labs_on_day++;
        }
        if (labs_on_day >= 1) continue;

        // Create vector of possible start slots and shuffle them
        vector<int> possible_start_slots;
        for (int slot = 0; slot < total_teaching_slots - slots_needed + 1; slot++) {
            // Skip lunch break slot and slots that would overlap with lunch
            if (slot <= lunch_break_slot && slot + slots_needed > lunch_break_slot) {
                continue;
            }
            possible_start_slots.push_back(slot);
        }
        random_shuffle(possible_start_slots.begin(), possible_start_slots.end());

        // Try each possible start slot
        for (int start_slot : possible_start_slots) {
            bool can_place = true;
            
            // Check consecutive slots availability
            for (int s = start_slot; s < start_slot + slots_needed; s++) {
                if (s >= total_teaching_slots || s == lunch_break_slot || 
                    tt.subject_id[day][s] != -1 || tt.is_lab[day][s]) {
                    can_place = false;
                    break;
                }
            }
            
            if (can_place && validate_lab_constraints(lab, day, start_slot)) {
                string lab_details = lab.name + "_" + lab.instructor + "_" + lab.center;
                cout << "Successfully scheduled lab " << lab.name << " on " << DAY_NAMES[day] 
                     << " starting at slot " << start_slot << endl;
                
                // Place the lab
                for (int s = start_slot; s < start_slot + slots_needed; s++) {
                    tt.subject_id[day][s] = -3;
                    tt.teacher[day][s] = lab.instructor;
                    tt.room[day][s] = lab.center;
                    tt.is_lab[day][s] = true;
                    tt.lab_details[day][s] = lab_details;
                    tt.is_lab_slot[day][s] = true;
                    record_assignment(day, s, lab.instructor, lab.center);
                }
                
                lab.day = day;
                lab.start_slot = start_slot;
                lab.is_scheduled = true;
                return true;
            }
        }
    }
    
    cout << "Warning: Lab " << lab.name << " could not be scheduled" << endl;
    return false;
}

bool TimetableGenerator::place_lab(Lab &lab, Timetable &tt) {
    vector<int> days(DAY_NAMES.size());
    iota(days.begin(), days.end(), 0);
    random_shuffle(days.begin(), days.end());

    int slots_needed = max(1, lab.duration / slot_durations[0]);
    
    for (int day : days) {
        // Try different start times throughout the day
        vector<int> possible_starts;
        for (int i = 0; i <= total_teaching_slots - slots_needed; i++) {
            possible_starts.push_back(i);
        }
        random_shuffle(possible_starts.begin(), possible_starts.end());

        for (int start : possible_starts) {
            if (can_place_lab(day, start, slots_needed, tt)) {
                bool conflict = false;
                // Check for teacher and room conflicts
                for (int s = start; s < start + slots_needed; s++) {
                    if (!can_place_subject(day, s, lab.instructor, lab.center)) {
                        conflict = true;
                        break;
                    }
                }
                if (conflict) continue;

                // Place the lab
                lab.day = day;
                lab.start_slot = start;

                for (int s = start; s < start + slots_needed; s++) {
                    tt.subject_id[day][s] = -3;  // Special marker for labs
                    tt.teacher[day][s] = lab.instructor;
                    tt.room[day][s] = lab.center;
                    tt.is_lab[day][s] = true;
                    record_assignment(day, s, lab.instructor, lab.center);
                }
                return true;
            }
        }
    }
    return false;
}

bool TimetableGenerator::can_place_lab(int day, int start_slot, int slots_needed, const Timetable &tt) {
    if (day < 0 || day >= static_cast<int>(DAY_NAMES.size())) return false;
    if (start_slot < 0 || start_slot + slots_needed > total_teaching_slots) return false;
    
    // Check for lunch break and existing assignments
    for (int s = start_slot; s < start_slot + slots_needed; s++) {
        if (s == lunch_break_slot || tt.subject_id[day][s] != -1) {
            return false;
        }
    }

    // Check for other labs scheduled at this time
    for (const auto& schedule : room_schedule) {
        if (schedule.second.count(day)) {
            for (int s = start_slot; s < start_slot + slots_needed; s++) {
                if (schedule.second.at(day).count(s)) {
                    return false;
                }
            }
        }
    }

    return true;
}

bool TimetableGenerator::subject_already_on_day(const Timetable &tt, int day, int subject_idx) {
    for (int s = 0; s < total_teaching_slots; s++) {
        if (tt.subject_id[day][s] == subject_idx) {
            return true;
        }
    }
    return false;
}

void TimetableGenerator::validate_timetable(const Timetable &tt, int stream_idx) {
    map<string, map<int, map<int, bool>>> teacher_slots;  // teacher -> day -> slot
    map<string, map<int, map<int, bool>>> room_slots;     // room -> day -> slot
    
    for (size_t day = 0; day < DAY_NAMES.size(); day++) {
        bool has_class = false;
        
        for (int slot = 0; slot < total_teaching_slots; slot++) {
            if (slot == lunch_break_slot) continue;
            
            if (tt.subject_id[day][slot] != -1) {
                has_class = true;
                const string& teacher = tt.teacher[day][slot];
                const string& room = tt.room[day][slot];
                
                // Check for conflicts
                if (!teacher.empty()) {
                    if (teacher_slots[teacher][day][slot]) {
                        cerr << "Warning: Teacher " << teacher 
                             << " has multiple classes at the same time on " 
                             << DAY_NAMES[day] << endl;
                    }
                    teacher_slots[teacher][day][slot] = true;
                }
                
                if (!room.empty()) {
                    if (room_slots[room][day][slot]) {
                        cerr << "Warning: Room " << room 
                             << " has multiple classes at the same time on "
                             << DAY_NAMES[day] << endl;
                    }
                    room_slots[room][day][slot] = true;
                }
            }
        }
        
        if (!has_class) {
            cerr << "Warning: No classes scheduled for " << stream_names[stream_idx] 
                 << " on " << DAY_NAMES[day] << endl;
        }
    }
    
    // Add lab-specific validation
    for (const Lab& lab : stream_labs[stream_idx]) {
        if (!lab.is_scheduled) {
            cerr << "Warning: Lab " << lab.name << " could not be scheduled\n";
        }
    }
    
    // Validate consecutive lab slots
    for (size_t day = 0; day < DAY_NAMES.size(); day++) {
        for (int slot = 0; slot < total_teaching_slots - 1; slot++) {
            if (tt.is_lab[day][slot] && tt.is_lab[day][slot + 1]) {
                // Verify same lab session
                if (tt.lab_details[day][slot] != tt.lab_details[day][slot + 1]) {
                    cerr << "Warning: Different labs scheduled consecutively on " 
                         << DAY_NAMES[day] << endl;
                }
            }
        }
    }
}

string TimetableGenerator::find_lab_name(int stream, int day) {
    if (stream < 0 || stream >= static_cast<int>(stream_labs.size())) {
        return "UNKNOWN LAB";
    }

    for (const Lab &lab : stream_labs[stream]) {
        // Check if lab slot matches the day and also check if lab has a name
        if (lab.day == day) {
            if (!lab.name.empty()) {
                return lab.name;
            }
        }
    }

    // If no matching lab found or lab name is empty, create a default name with stream and section
    string default_name = stream_names[stream] + "_LAB";
    
    // If there are labs configured for this stream, use the first lab's details
    if (!stream_labs[stream].empty()) {
        const Lab& first_lab = stream_labs[stream][0];
        if (!first_lab.name.empty()) {
            return first_lab.name;
        }
    }
    
    return default_name;
}

void TimetableGenerator::clear_semester_state() {
    occupied_rooms.clear();
    occupied_teachers.clear();
    teacher_schedule.clear();
    room_schedule.clear();
}

bool TimetableGenerator::validate_lab_constraints(const Lab& lab, int day, int slot) {
    // Check instructor availability
    if (teacher_schedule[lab.instructor][day].count(slot)) {
        return false;
    }
    
    // Check lab room availability
    if (room_schedule[lab.center][day].count(slot)) {
        return false;
    }
    
    // Check if slot is near lunch break
    if (abs(slot - lunch_break_slot) <= 1) {
        return false;
    }
    
    return true;
}
