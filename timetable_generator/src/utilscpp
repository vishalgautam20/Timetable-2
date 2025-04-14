#include "../include/utils.h"
#include <iomanip>
#include <sstream>

using namespace std;

string format_time(int minutes) {
    int hrs = minutes / 60;
    int mins = minutes % 60;
    string period = (hrs >= 12) ? "PM" : "AM";
    if (hrs > 12) hrs -= 12;
    if (hrs == 0) hrs = 12;
    
    stringstream ss;
    ss << setw(2) << setfill('0') << hrs << ":" 
       << setw(2) << setfill('0') << mins << " " << period;
    return ss.str();
}
