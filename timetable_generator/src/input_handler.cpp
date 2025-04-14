#include "../include/timetable_generator.h"
#include <iostream>
#include <limits>
#include <algorithm>

using namespace std;

void TimetableGenerator::get_input() {
    get_streams_info();
}

void TimetableGenerator::get_streams_info() {
    int num_streams;
    while (true) {
        cout << "\nHow many streams? ";
        if (cin >> num_streams && num_streams > 0) break;
        handle_invalid_input();
    }
    cin.ignore();

    stream_names.resize(num_streams);
    sections_per_stream.resize(num_streams);
    subjects.resize(num_streams);
    stream_labs.resize(num_streams);
    subject_count.resize(num_streams);
    timetables.resize(num_streams);

    for (int i = 0; i < num_streams; i++) {
        cout << "\n=== STREAM " << i + 1 << " ===" << endl;
        cout << "Enter stream name: ";
        getline(cin, stream_names[i]);
        
        while (true) {
            cout << "How many sections for " << stream_names[i] << "? ";
            if (cin >> sections_per_stream[i] && sections_per_stream[i] > 0) break;
            handle_invalid_input();
        }
        cin.ignore();
        
        get_subjects_info(i);
        get_lab_info(i);
    }
}

void TimetableGenerator::get_lab_info(int stream_idx) {
    cout << "\nLab Configuration for " << stream_names[stream_idx] << endl;
    cout << "Does this stream have laboratory sessions? (y/n): ";
    char has_labs;
    cin >> has_labs;
    cin.ignore();
    
    if (tolower(has_labs) == 'y') {
        cout << "Number of different lab sessions: ";
        int num_labs;
        cin >> num_labs;
        cin.ignore();
        
        stream_labs[stream_idx].resize(num_labs);
        
        for (int i = 0; i < num_labs; i++) {
            Lab& lab = stream_labs[stream_idx][i];
            
            cout << "\nLab " << (i+1) << " Details:" << endl;
            cout << "Name: ";
            getline(cin, lab.name);
            
            cout << "Lab Center/Location: ";
            getline(cin, lab.center);
            
            cout << "Instructor: ";
            getline(cin, lab.instructor);
            
            cout << "Duration (number of slots): ";  // Changed prompt
            cin >> lab.duration;
            
            // Validate lab duration
            if (lab.duration <= 0 || lab.duration > total_teaching_slots) {
                cout << "Invalid duration. Setting to 1 slot.\n";
                lab.duration = 1;
            }
            
            cin.ignore();
        }
    }
}

void TimetableGenerator::get_subjects_info(int stream_idx) {
    string prompt = "\nEnter number of subjects for " + stream_names[stream_idx] + 
                   " (max " + to_string(MAX_SUBJECTS) + "): ";
        
    int subj_count;
    while (true) {
        cout << prompt;
        if (cin >> subj_count && subj_count > 0 && subj_count <= MAX_SUBJECTS) break;
        handle_invalid_input();
    }
    cin.ignore();

    subject_count[stream_idx] = subj_count;
    subjects[stream_idx].resize(subj_count);
    
    for (int i = 0; i < subj_count; i++) {
        cout << "\nSubject " << i + 1 << " details:" << endl;
        cout << "Name: ";
        getline(cin, subjects[stream_idx][i].name);
        if (subjects[stream_idx][i].name.empty()) {
            subjects[stream_idx][i].name = "SUBJ_" + to_string(i+1);
        }
        cout << "Teacher: ";
        getline(cin, subjects[stream_idx][i].teacher);
        if (subjects[stream_idx][i].teacher.empty()) {
            subjects[stream_idx][i].teacher = "TEACHER_" + to_string(i+1);
        }
    }
}

void TimetableGenerator::get_college_time() {
    string prompt = "At what time should classes begin? (8-12): ";
    while (true) {
        cout << prompt;
        if (cin >> start_hour && start_hour >= 8 && start_hour <= 12) break;
        handle_invalid_input();
    }
    cin.ignore();
}

void TimetableGenerator::get_room_info() {
    string prompt = "How many classrooms are available? ";
    while (true) {
        cout << prompt;
        if (cin >> total_rooms && total_rooms > 0) break;
        handle_invalid_input();
    }
    cin.ignore();
}

void TimetableGenerator::get_slot_configuration() {
    cout << "\n=== TEACHING SLOTS CONFIGURATION ===" << endl;
    while (true) {
        cout << "How many teaching slots per day? ";
        if (cin >> total_teaching_slots && total_teaching_slots > 0) break;
        handle_invalid_input();
    }
    cin.ignore();

    slot_durations.resize(total_teaching_slots);
    for (int i = 0; i < total_teaching_slots; i++) {
        while (true) {
            cout << "Duration (minutes) for teaching slot " << i + 1 << ": ";
            if (cin >> slot_durations[i] && slot_durations[i] > 0) break;
            handle_invalid_input();
        }
        cin.ignore();
    }
}

void TimetableGenerator::get_lunch_configuration() {
    cout << "\n=== LUNCH BREAK CONFIGURATION ===" << endl;
    
    string default_time = "13:00";
    
    while (true) {
        cout << "At what time should lunch break start? (HH:MM, default " << default_time << "): ";
        string lunch_time;
        getline(cin, lunch_time);
        
        if (lunch_time.empty()) lunch_time = default_time;
        
        size_t colon_pos = lunch_time.find(':');
        if (colon_pos != string::npos) {
            try {
                int lunch_hour = stoi(lunch_time.substr(0, colon_pos));
                int lunch_min = stoi(lunch_time.substr(colon_pos + 1));
                if (lunch_hour >= 0 && lunch_hour <= 23 && lunch_min >= 0 && lunch_min <= 59) {
                    lunch_break_time = lunch_hour * 60 + lunch_min;
                    break;
                }
            } catch (...) {}
        }
        cout << "Invalid time format. Please use HH:MM format.\n";
    }

    while (true) {
        cout << "Duration of lunch break (minutes, default " << lunch_break_duration << "): ";
        string input;
        getline(cin, input);
        
        if (!input.empty()) {
            try {
                int duration = stoi(input);
                if (duration > 0) {
                    lunch_break_duration = duration;
                    break;
                }
            } catch (...) {}
            cout << "Invalid duration. Please enter a positive number.\n";
        } else {
            break;
        }
    }
}

void TimetableGenerator::handle_invalid_input() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Invalid input. Would you like to continue? (y/n): ";
    char response;
    cin >> response;
    if (tolower(response) != 'y') {
        cout << "Exiting program...\n";
        exit(0);
    }
    cin.ignore();
}

void TimetableGenerator::get_semester_info() {
    int num_semesters;
    cout << "\nHow many semesters to generate timetables for? ";
    cin >> num_semesters;
    cin.ignore();

    semesters.clear(); 
    semesters.resize(num_semesters);
    semester_data_store.clear();
    
    for (int i = 0; i < num_semesters; i++) {
        cout << "\n=== SEMESTER " << (i+1) << " ===\n";
        
        SemesterInfo& current_sem = semesters[i];
        cout << "Year (e.g., 2025): ";
        cin >> current_sem.year;
        cin.ignore();

        do {
            cout << "Term (Fall/Spring): ";
            getline(cin, current_sem.term);
            transform(current_sem.term.begin(), current_sem.term.end(), 
                     current_sem.term.begin(), ::toupper);
        } while (current_sem.term != "FALL" && current_sem.term != "SPRING");

        // Clear and get new data for this semester
        stream_names.clear();
        sections_per_stream.clear();
        subjects.clear();
        stream_labs.clear();
        subject_count.clear();
        timetables.clear();
        
        get_streams_info();

        // Store semester data
        SemesterData sem_data = {
            subjects,
            stream_labs,
            subject_count,
            timetables,
            stream_names,
            sections_per_stream
        };
        
        semester_data_store[i] = sem_data;
        cout << "Stored data for semester " << current_sem.term 
             << " " << current_sem.year << "\n";
    }
}

void TimetableGenerator::run() {
    try {
        // Get all configurations first
        get_semester_info();
        get_common_config();
        
        // Process each semester and generate timetables
        cout << "\nGenerating timetables...\n";
        generate_timetables();
        
        cout << "\nAll timetables generated successfully!\n";
    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
    }
}

void TimetableGenerator::get_common_config() {
    cout << "\n===== COLLEGE TIMETABLE GENERATOR =====\n";
    get_college_time();
    get_room_info();
    get_slot_configuration();
    get_lunch_configuration();
}
