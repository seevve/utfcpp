#include "../source/utf8.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>


using namespace std;

int main(int argc, char** argv)
{
    if (argc != 2) {
        cout << "\nUsage: docsample filename\n";
        return 0;
    }
    const char* test_file_path = argv[1];
    // Open the test file (must be UTF-8 encoded)
    ifstream fs8(test_file_path);
    if (!fs8.is_open()) {
    cout << "Could not open " << test_file_path << endl;
    return 0;
    }

    // Read the first line of the file
    unsigned line_count = 1;
    string line;
    if (!getline(fs8, line)) 
        return 0;

    // Look for utf-8 byte-order mark at the beginning
    if (line.size() > 2) {
        if (utf8::is_bom(line.c_str()))
          cout << "There is a byte order mark at the beginning of the file\n";
    }

    // Play with all the lines in the file
    do {
        // check for invalid utf-8 (for a simple yes/no check, there is also utf8::is_valid function)
        string::iterator end_it = utf8::find_invalid(line.begin(), line.end());
        if (end_it != line.end()) {
            cout << "Invalid UTF-8 encoding detected at line " << line_count << "\n";
            cout << "This part is fine: " << string(line.begin(), end_it) << "\n";
        }
        // Get the line length (at least for the valid part)
        int length = utf8::distance(line.begin(), end_it);
        cout << "Length of line " << line_count << " is " << length <<  "\n";

        // Convert it to utf-16
        vector<unsigned short> utf16line;
        utf8::utf8to16(line.begin(), end_it, back_inserter(utf16line));
        // And back to utf-8;
        string utf8line; 
        utf8::utf16to8(utf16line.begin(), utf16line.end(), back_inserter(utf8line));
        // Confirm that the conversion went OK:
        if (utf8line != string(line.begin(), end_it))
            cout << "Error in UTF-16 conversion at line: " << line_count << "\n";        

        getline(fs8, line);
        line_count++;
    } while (!fs8.eof());

    return 0;
}
