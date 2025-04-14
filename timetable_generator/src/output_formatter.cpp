#include "../include/timetable_generator.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>  // For sort()
#include <cmath>     // Add this for ceil()

// Add these constants at the top of the file
const int TIME_COL_WIDTH = 20;
const int DAY_COL_WIDTH = 35;
const int ROOM_COL_WIDTH = 15;
const int CLASS_COL_WIDTH = 20;
const int SUBJECT_COL_WIDTH = 30;

using namespace std;

void TimetableGenerator::save_to_csv_column_wise(const string& filename) {
    ofstream file(filename);
    if (!file) {
        throw runtime_error("Could not open file for writing: " + filename);
    }

    // Add semester information and header
    file << "COLLEGE TIMETABLE\n\n";
    file << "Semester: " << semesters[current_semester].term << " " 
         << semesters[current_semester].year << "\n\n";
    
    // Add basic information
    file << "Basic Information,\n";
    file << "Start Time," << format_time(start_hour * 60) << "\n";
    file << "Lunch Time," << format_time(lunch_break_time) << "\n";
    file << "Lunch Duration," << lunch_break_duration << " minutes\n\n";

    // For each stream and section
    for (size_t stream = 0; stream < stream_names.size(); stream++) {
        for (int section = 0; section < sections_per_stream[stream]; section++) {
            const auto& tt = timetables[stream][section];
            
            // Header for stream and section
            file << stream_names[stream] << " - Section " 
                 << static_cast<char>('A' + section) << "\n\n";
            
            // Column headers
            file << "Time,";
            for (const auto& day : DAY_NAMES) {
                file << day << ",";
            }
            file << "\n";

            // Timetable content
            int current_time = start_hour * 60;
            for (int slot = 0; slot < total_teaching_slots; slot++) {
                string time_range = format_time_range(current_time, slot);
                file << time_range << ",";

                for (size_t day = 0; day < DAY_NAMES.size(); day++) {
                    if (slot == lunch_break_slot) {
                        file << "LUNCH BREAK,";
                    } else if (tt.is_lab[day][slot]) {
                        // Modified to show lab details
                        file << tt.lab_details[day][slot] << ",";
                    } else if (tt.subject_id[day][slot] >= 0) {
                        // Format: Subject [Teacher] Room
                        file << subjects[stream][tt.subject_id[day][slot]].name 
                             << " [" << tt.teacher[day][slot] << "] " 
                             << tt.room[day][slot] << ",";
                    } else {
                        file << "FREE,";
                    }
                }
                file << "\n";
                
                if (slot == lunch_break_slot) {
                    current_time = lunch_break_time + lunch_break_duration;
                } else {
                    current_time += slot_durations[slot] + break_duration;
                }
            }
            file << "\n\n";
        }
    }

    // Update notes section to include labs
    file << "\nNotes:\n";
    file << "Format: Subject [Teacher] Room\n";
    file << "Laboratory Sessions: LabName_Instructor_LabCenter\n";
    file << "FREE: No class scheduled\n";
    file << "LUNCH BREAK: Lunch period\n";
}

void TimetableGenerator::save_to_txt_column_wise(const string& filename) {
    ofstream file(filename);
    if (!file) {
        throw runtime_error("Could not open file for writing: " + filename);
    }

    // Header
    file << "COLLEGE TIMETABLE\n";
    file << "================\n\n";
    file << "Semester: " << semesters[current_semester].term << " " 
         << semesters[current_semester].year << "\n\n";
    file << "Basic Information:\n";
    file << "Start Time: " << format_time(start_hour * 60) << "\n";
    file << "Lunch Time: " << format_time(lunch_break_time) 
         << " (" << lunch_break_duration << " mins)\n\n";

    // Constants for formatting
    const int time_width = 14;
    const int col_width = 30;

    // For each stream and section
    for (size_t stream = 0; stream < stream_names.size(); stream++) {
        for (int section = 0; section < sections_per_stream[stream]; section++) {
            const auto& tt = timetables[stream][section];
            
            // Stream and section header
            file << stream_names[stream] << " - Section " 
                 << static_cast<char>('A' + section) << "\n";
            file << string(60, '=') << "\n\n";

            // Column headers
            file << left << setw(time_width) << "TIME";
            for (const auto& day : DAY_NAMES) {
                file << "| " << left << setw(col_width - 2) << day;
            }
            file << "\n" << string(time_width + DAY_NAMES.size() * col_width, '-') << "\n";

            // Timetable content
            int current_time = start_hour * 60;
            for (int slot = 0; slot < total_teaching_slots; slot++) {
                file << left << setw(time_width) 
                     << format_time_range(current_time, slot);

                for (size_t day = 0; day < DAY_NAMES.size(); day++) {
                    file << "| ";
                    if (slot == lunch_break_slot) {
                        file << left << setw(col_width - 2) << "LUNCH BREAK";
                    } else if (tt.is_lab[day][slot]) {
                        // Modified to show lab details
                        file << left << setw(col_width - 2) << tt.lab_details[day][slot];
                    } else if (tt.subject_id[day][slot] >= 0) {
                        string entry = subjects[stream][tt.subject_id[day][slot]].name + 
                                     " (" + tt.teacher[day][slot] + ") - " + 
                                     tt.room[day][slot];
                        file << left << setw(col_width - 2) << entry;
                    } else {
                        file << left << setw(col_width - 2) << "FREE PERIOD";
                    }
                }
                file << "\n";

                // Update time
                if (slot == lunch_break_slot) {
                    current_time = lunch_break_time + lunch_break_duration;
                } else {
                    current_time += slot_durations[slot] + break_duration;
                }
            }
            file << "\n\n";
        }
    }

    // Update legend to include labs
    file << "\nLEGEND:\n";
    file << "-------\n";
    file << "Regular Classes: Subject (Teacher) - Room\n";
    file << "Laboratory Sessions: LabName_Instructor_LabCenter\n";
    file << "Free Periods\n";
    file << "Lunch Break\n";
}

void TimetableGenerator::generate_teacher_timetables(const string& filename) {
    ofstream file(filename);
    if (!file) {
        throw runtime_error("Could not open teacher timetable file for writing: " + filename);
    }

    // Header with semester info
    file << "TEACHER TIMETABLES - " << semesters[current_semester].term 
         << " " << semesters[current_semester].year << "\n";
    file << string(100, '=') << "\n\n";

    // Collect all teacher schedules with better organization
    map<string, vector<tuple<string, string, string, string, string>>> teacher_schedules;
    // Format: teacher -> vector of (day, time, room, stream, subject)

    // Process each stream and section
    for (size_t stream = 0; stream < stream_names.size(); stream++) {
        for (int section = 0; section < sections_per_stream[stream]; section++) {
            const auto& tt = timetables[stream][section];
            string stream_sec = stream_names[stream] + " Sec " + 
                              static_cast<char>('A' + section);

            // Process each day and slot
            for (size_t day = 0; day < DAY_NAMES.size(); day++) {
                for (int slot = 0; slot < total_teaching_slots; slot++) {
                    if (slot == lunch_break_slot) continue;
                    
                    if (tt.subject_id[day][slot] >= 0) {
                        string teacher = tt.teacher[day][slot];
                        string subject = subjects[stream][tt.subject_id[day][slot]].name;
                        
                        // Calculate time range
                        int current_time = start_hour * 60;
                        for (int s = 0; s < slot; s++) {
                            if (s == lunch_break_slot) {
                                current_time = lunch_break_time + lunch_break_duration;
                            } else {
                                current_time += slot_durations[s] + break_duration;
                            }
                        }
                        int end_time = current_time + slot_durations[slot];
                        
                        if (!teacher.empty()) {
                            teacher_schedules[teacher].push_back(
                                make_tuple(
                                    DAY_NAMES[day],
                                    format_time(current_time) + "-" + format_time(end_time),
                                    tt.room[day][slot],
                                    stream_sec,
                                    subject
                                )
                            );
                        }
                    }
                }
            }
        }
    }

    // Output each teacher's schedule with improved formatting
    for (const auto& [teacher, schedule] : teacher_schedules) {
        file << "TEACHER: " << teacher << "\n";
        file << string(100, '-') << "\n";
        
        // Enhanced header with more columns
        file << left << setw(15) << "DAY" 
             << setw(20) << "TIME" 
             << setw(15) << "ROOM"
             << setw(20) << "CLASS"
             << "SUBJECT\n";
        file << string(100, '-') << "\n";

        // Sort schedule by day and time
        vector<tuple<string, string, string, string, string>> sorted_schedule = schedule;
        sort(sorted_schedule.begin(), sorted_schedule.end());

        for (const auto& [day, time, room, cls, subj] : sorted_schedule) {
            file << left << setw(15) << day 
                 << setw(20) << time 
                 << setw(15) << room
                 << setw(20) << cls
                 << subj << "\n";
        }
        file << "\n\n";
    }

    // Add summary at the end
    file << "\nSUMMARY\n";
    file << string(50, '=') << "\n";
    file << "Total Teachers: " << teacher_schedules.size() << "\n";
    for (const auto& [teacher, schedule] : teacher_schedules) {
        file << teacher << ": " << schedule.size() << " classes\n";
    }
}

// Add new function to save individual section timetables
void TimetableGenerator::save_individual_section_timetables() {
    for (size_t stream = 0; stream < stream_names.size(); stream++) {
        for (int section = 0; section < sections_per_stream[stream]; section++) {
            // Create filename like "cse_sectionA.txt"
            string filename = stream_names[stream] + "_section" + 
                            static_cast<char>('A' + section) + ".txt";
            
            ofstream file(filename);
            if (!file) {
                throw runtime_error("Could not open file: " + filename);
            }

            file << "COLLEGE TIMETABLE\n";
            file << "================\n\n";
            file << "Stream: " << stream_names[stream] << "\n";
            file << "Section: " << static_cast<char>('A' + section) << "\n\n";
            file << "Start Time: " << format_time(start_hour * 60) << "\n";
            file << "Lunch Time: " << format_time(lunch_break_time) 
                 << " (" << lunch_break_duration << " mins)\n\n";

            print_section_timetable(file, stream, section);
            
            file << "\nLEGEND:\n";
            file << "-------\n";
            file << "- Regular Classes: Subject (Teacher) - Room\n";
            file << "- Free Periods\n";
            file << "- Lunch Break\n";
            
            cout << "Created timetable for " << stream_names[stream] 
                 << " Section " << static_cast<char>('A' + section) 
                 << " in " << filename << "\n";
        }
    }
}

// Update the format_time_range function to accept current_slot parameter
string TimetableGenerator::format_time_range(int start_time, int current_slot) {
    int end_time = start_time + slot_durations[current_slot];
    return format_time(start_time) + "-" + format_time(end_time);
}

// First implementation (with stream parameter)
string TimetableGenerator::format_class_entry(const Timetable &tt, int day, int slot, int stream) {
    if (tt.subject_id[day][slot] == -3 || tt.is_lab[day][slot]) {  // Lab identifier
        if (!tt.lab_details[day][slot].empty()) {
            return tt.lab_details[day][slot];  // This already contains LabName_Instructor_LabCenter
        }
        // Find the corresponding lab
        for (const Lab& lab : stream_labs[stream]) {
            if (lab.day == day && slot >= lab.start_slot && 
                slot < lab.start_slot + lab.duration) {
                return lab.name + "_" + lab.instructor + "_" + lab.center;
            }
        }
    }

    if (tt.subject_id[day][slot] >= 0) {
        int subj_idx = tt.subject_id[day][slot];
        return subjects[stream][subj_idx].name + " (" + 
               tt.teacher[day][slot] + ") - " + 
               tt.room[day][slot];
    }
    
    return "FREE PERIOD";
}

// Second implementation (without stream parameter)
string TimetableGenerator::format_class_entry(const Timetable& tt, int day, int slot) {
    if (tt.is_lab[day][slot]) {
        return tt.lab_details[day][slot] + " (LAB) - " + tt.room[day][slot];
    }
    
    if (tt.subject_id[day][slot] >= 0) {
        return tt.teacher[day][slot] + " - " + tt.room[day][slot];
    }
    
    return "FREE PERIOD";
}

// In output_formatter.cpp
string TimetableGenerator::format_lab_entry(const Lab& lab) {
    return lab.name + "_" + lab.instructor + "_" + lab.center;
}

// string TimetableGenerator::format_class_entry(const Timetable& tt, int day, int slot, int stream) {
//     if (tt.is_lab[day][slot]) {
//         return tt.lab_details[day][slot] + " (LAB)";
//     }
//     if (tt.subject_id[day][slot] == -3) {  // Lab identifier
//         string lab_info = "";
//         // Find the corresponding lab
//         for (const Lab& lab : stream_labs[stream]) {
//             if (lab.day == day && slot >= lab.start_slot && 
//                 slot < lab.start_slot + ceil(static_cast<float>(lab.duration) / 55.0)) {
//                 lab_info = lab.name + " (LAB) - " + lab.center;
//                 break;
//             }
//         }
//         return lab_info.empty() ? "LAB SESSION" : lab_info;
//     }

//     if (tt.subject_id[day][slot] >= 0) {
//         int subj_idx = tt.subject_id[day][slot];
//         return subjects[stream][subj_idx].name + " (" + 
//                tt.teacher[day][slot] + ") - " + 
//                tt.room[day][slot];
//     }
    
//     return "FREE PERIOD";
// }

string TimetableGenerator::center_text(const string& text, int width) {
    int padding = width - text.length();
    int left_pad = padding / 2;
    return string(left_pad, ' ') + text + 
           string(padding - left_pad, ' ');
}

void TimetableGenerator::print_legend(ofstream& file) {
    file << "\nLEGEND:\n";
    file << "-------\n";
    file << "Regular Classes: Subject (Teacher) - Room\n";
    file << "Laboratory Sessions: Lab Name (LAB) - Room (Instructor)\n";
    file << "Free Periods\n";
    file << "Lunch Break\n\n";
    file << "Format: Subject (Teacher) - Room\n";
    file << "Lab Format: Lab Name (LAB) - Room (Instructor)\n\n";
}

void TimetableGenerator::print_summary(ofstream& file, const Timetable& tt) {
    map<string, int> teacher_hours;
    map<string, int> room_usage;
    
    for (size_t day = 0; day < DAY_NAMES.size(); day++) {
        for (int slot = 0; slot < total_teaching_slots; slot++) {
            if (tt.subject_id[day][slot] >= 0) {
                teacher_hours[tt.teacher[day][slot]]++;
                room_usage[tt.room[day][slot]]++;
            }
        }
    }

    file << "\nSUMMARY STATISTICS:\n";
    file << "-----------------\n";
    file << "Teaching Hours per Teacher:\n";
    for (const auto& [teacher, hours] : teacher_hours) {
        file << teacher << ": " << hours << " hours\n";
    }
    
    file << "\nRoom Utilization:\n";
    for (const auto& [room, usage] : room_usage) {
        file << room << ": " << usage << " slots\n";
    }
}

void TimetableGenerator::print_section_timetable(ofstream& file, size_t stream, int section) {
    const Timetable &tt = timetables[stream][section];
    const int time_width = 20;
    const int col_width = 35;
    
    // Print header
    file << left << setw(time_width) << "TIME";
    for (const string& day : DAY_NAMES) {
        file << "| " << left << setw(col_width - 2) << day;
    }
    file << "\n" << string(time_width + DAY_NAMES.size() * col_width, '-') << "\n";

    // Print timetable content
    int current_time = start_hour * 60;
    for (int slot = 0; slot < total_teaching_slots; slot++) {
        string time_slot = format_time_range(current_time, slot);
        file << left << setw(time_width) << time_slot;

        for (size_t day = 0; day < DAY_NAMES.size(); day++) {
            file << "| ";
            if (slot == lunch_break_slot) {
                file << left << setw(col_width - 2) << "LUNCH BREAK";
            } else {
                string entry = format_class_entry(tt, day, slot, stream);
                file << left << setw(col_width - 2) << entry;
            }
        }
        file << "\n";

        // Update time
        if (slot == lunch_break_slot) {
            current_time = lunch_break_time + lunch_break_duration;
        } else {
            current_time += slot_durations[slot] + break_duration;
        }
    }
}

void TimetableGenerator::save_semester_timetables(const string& sem_dir) {
    system(("mkdir -p " + sem_dir).c_str());
    system(("mkdir -p " + sem_dir + "/sections").c_str());
    
    cout << "Saving timetables to directory: " << sem_dir << "\n";
    cout << "Number of streams: " << stream_names.size() << "\n";
    
    // Save combined timetable files
    string all_timetables_txt = sem_dir + "/all_timetables_semester_" + 
                               to_string(current_semester + 1) + ".txt";
    string all_timetables_csv = sem_dir + "/all_timetables_semester_" + 
                               to_string(current_semester + 1) + ".csv";
    save_to_txt_column_wise(all_timetables_txt);
    save_to_csv_column_wise(all_timetables_csv);
    
    // Save teacher timetables
    string teacher_timetables = sem_dir + "/teacher_timetables_semester_" + 
                               to_string(current_semester + 1) + ".txt";
    generate_teacher_timetables(teacher_timetables);
    
    // Save individual section timetables
    for (size_t stream = 0; stream < stream_names.size(); stream++) {
        cout << "Stream " << stream_names[stream] << " has " 
             << sections_per_stream[stream] << " sections\n";
        
        for (int section = 0; section < sections_per_stream[stream]; section++) {
            string filename = sem_dir + "/sections/" + stream_names[stream] + 
                            "_section" + static_cast<char>('A' + section) + 
                            "_semester_" + to_string(current_semester + 1) + ".txt";
            save_section_timetable(stream, section, filename);
        }
    }
    
    cout << "Finished saving semester " << (current_semester + 1) << " timetables\n";
}

void TimetableGenerator::save_section_timetable(size_t stream, int section, const string& filename) {
    ofstream file(filename);
    if (!file) {
        throw runtime_error("Could not open file for writing: " + filename);
    }

    // Add semester information at the top
    file << "COLLEGE TIMETABLE\n";
    file << "================\n\n";
    file << "Semester: " << semesters[current_semester].term << " " 
         << semesters[current_semester].year << "\n\n";  // Add this line
    file << "Stream: " << stream_names[stream] << "\n";
    file << "Section: " << static_cast<char>('A' + section) << "\n\n";
    file << "Start Time: " << format_time(start_hour * 60) << "\n";
    file << "Lunch Time: " << format_time(lunch_break_time) 
         << " (" << lunch_break_duration << " mins)\n\n";

    print_section_timetable(file, stream, section);
    print_legend(file);
}
