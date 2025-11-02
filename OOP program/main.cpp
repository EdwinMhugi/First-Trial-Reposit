#include <iostream>
#include <fstream>
#include <string>
#include <regex>
#include <ctime>//used it to get current local date and time when a student makes a tuition Pay.
#include <filesystem>//used in the project to search, delete and iterate over files.
#include <conio.h> // Used for console input/output like getch(),hashing the admin password.
#include <limits>//Used in the project for input validation ie clearing invalid input from the input stream.
#include <vector>//Used in the project for dynamic arrays that can grow or shrink — like storing Student payment lists of records.
#include <sstream>// Provides string streams for string-based input/output and conversions
#include <iomanip>

// This allows easier access to filesystem functions like fs::path, fs::exists, and fs::directory_iterator
namespace fs = std::filesystem;

// ===== Base Class =====
class StudentAccountablility {
public:
    // Declaring pure virtual functions and will be overriden in the child class.
    virtual void viewPaymentHistory() = 0;
    virtual void viewRegistrationCard() = 0;

    //virtual destructor//
    virtual ~StudentAccountablility() = default;
};

// =====Student Structure =====
struct Student {
    std::string name;
    std::string regNo;
    std::string paycode;
    int tuition;
};

// ===== Derived Class: StudentAccount =====
class StudentAccount : public StudentAccountablility {
private:
    std::string name;
    std::string regNo;
    std::string paycode;
    int tuition;
    const int REQUIRED_TUITION = 4400000;//Total Tuition for a Student to Be at full tuition pay.

public:
//StudentAccount Constructor list.
    StudentAccount(const std::string &n, const std::string &r, const std::string &p, int t = 0)
        : name(n), regNo(r), paycode(p), tuition(t) {}

//returns current local date and time of the machine.
    std::string currentDateTime() const {
        time_t now = time(0);
        char buf[80];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&now));
        return std::string(buf);
    }

//validates Paycode input such that its strictly Ten digits.
    static bool validatePaycode(const std::string &paycode) {
        return std::regex_match(paycode, std::regex("^[0-9]{10}$"));
    }

//validates Paycode input such that it matches the form (24/1/370/D/040).
    static bool validateRegNo(const std::string &regNo) {
        return std::regex_match(regNo, std::regex("^[0-9]{2}/[0-9]+/[0-9]+/(D|W|DJ)/[0-9]{3}$"));
    }
//validates the student Name entry is two words, Mugisha Edwin not just Mugisha
    static bool isValidName(const std::string &name) {
    if (name.empty()) return false;

    int spaceCount = 0;
    // Loop through each character in the name to ensure it only contains letters and spaces.
    for (char c : name) {
        if (std::isalpha(c) || c == ' ') {
            if (c == ' ') spaceCount++;
        } else {
            return false; // Invalid character found
        }
    }

    // Ensure at least two words (space between names)
    return spaceCount >= 1;
}

//validates tuition Entry
    static bool isValidTuition(const std::string &input, int &amount) {
    if (input.empty()) return false;

    // Check if all characters are digits
    for (char c : input) {
        if (!isdigit(c)) return false;
    }

    try {
 // Try to convert the string 'input' into an integer value and assign it to 'amount'
        amount = std::stoi(input);
    } catch (...) {
//conversion failed
        return false;
    }

    // Tuition Input Must be positive and non-zero
    return amount > 0;
}

//Determining Card Eligibilty.
    std::string determineCard(int totalPaid) const {
        double percent = (totalPaid * 100.0) / REQUIRED_TUITION;
        if (percent >= 100.0) return "Full Payment, FULL PAY MEAL CARD, ELIGIBLE FOR AN EXAM NUMBER";
        else if (percent >= 60.0) return "Meal Card (One Month)";
        else return "Tuition Average below 60%, Not Eligible for A Meal Card";
    }

// Getting total tuition from the .txt
    int getTotalFromTXT() const {
    std::string recordFile;
// Loop through all files and folders in the current directory (".")
    for (const auto &entry : fs::directory_iterator(".")) {
        std::string fname = entry.path().filename().string(); // Get the filename (without the full path) and convert it to a std::string
        if (fname.rfind(paycode + "_", 0) == 0 && fname.find(".txt") != std::string::npos) {
            recordFile = fname;
            break;
        }
    }

    if (recordFile.empty()) return 0; // file not found

    //if file was found. Open it
    std::ifstream file(recordFile);
    if (!file.is_open()) return 0;

    std::string line;
    int totalPaid = 0;

    while (std::getline(file, line)) {
        // Check if the current line starts with "Tuition Paid:" and extract the value for total tuition
        if (line.rfind("Tuition Paid:",0) != std::string::npos) {
            std::string digits;
            for (char c : line)
                if (isdigit(c)) digits += c;
            if (!digits.empty()) totalPaid += std::stoi(digits); // sum all payments
        }
    }

    file.close();
    return totalPaid;
}

//Getting total Tuition from .csv files, For Admin Use
    int getTotalPaidCSV() const {
        std::ifstream file("StudentPayments.csv");
        if (!file.is_open()) return 0;
        std::string line;
        int totalPaid = 0;
        while (std::getline(file, line)) {
// Iterate through all CSV lines to locate the entry matching the given paycode, skip intermediate fields, and store the TotalPaid amount as an integer.

            std::stringstream ss(line);
            std::string token;
            std::getline(ss, token, ','); // paycode
            if (token == paycode) {
                for (int i = 0; i < 4; ++i) std::getline(ss, token, ','); // skip to TotalPaid
                totalPaid = std::stoi(token);
            }
        }
        file.close();
        return totalPaid;
    }

    void savePayment() {
        // --- Update CSV ---
        int oldTotalCSV = getTotalPaidCSV();// Get the previous total tuition paid from CSV
        int newTotalCSV = oldTotalCSV + tuition;// Add the current payment to get the new total
        std::string cardTypeCSV = determineCard(newTotalCSV);

        std::ifstream checkCSV("StudentPayments.csv");
        bool fileExists = checkCSV.good();
        checkCSV.close();

        // Open StudentPayments.csv file in append mode to write payment details.
        std::ofstream csv("StudentPayments.csv", std::ios::app);
        if (!fileExists)
            csv << "Paycode,Name,RegNo,TuitionPaid,TotalPaid,RemainingBalance,CardType,DateTime\n";
        csv << paycode << "," << name << "," << regNo << "," << tuition << ","
            << newTotalCSV << "," << (REQUIRED_TUITION - newTotalCSV) << ","
            << cardTypeCSV << "," << currentDateTime() << "\n";
        csv.close();
 
       
        
        std::string filename = paycode + "_" + name + ".txt";
        int oldTotalTXT = getTotalFromTXT();
        int newTotalTXT = oldTotalTXT + tuition;
        std::string cardTypeTXT = determineCard(newTotalTXT);
        
        // Open Student text file in append mode to write payment details.
        std::ofstream txt(filename, std::ios::app);
        txt << "Name: " << name << "\n";
        txt << "RegNo: " << regNo << "\n";
        txt << "Paycode: " << paycode << "\n";
        txt << "Tuition Paid: " << tuition << "\n";
        txt << "Total Tuition Paid: " << newTotalTXT << "\n";
        txt << "Remaining Balance: " << (REQUIRED_TUITION - newTotalTXT) << "\n";
        txt << "Meal Card Eligibility: " << cardTypeTXT << "\n";
        txt << "Date/Time: " << currentDateTime() << "\n";
        txt << "----------------------------------\n";
        txt.close();

        std::cout << "Payment saved successfully.\n";

      
    }

    void viewPaymentHistory() override {
        std::string filename;
        // Loop through all files in the current directory and get only the file name.
        for (const auto &entry : fs::directory_iterator(".")) {
            std::string fname = entry.path().filename().string();
            if (fname.rfind(paycode + "_", 0) == 0 && fname.find(".txt") != std::string::npos) {
                filename = fname;
                break;
            }
        }
        //if file is empty.
        if (filename.empty()) { std::cout << "No payment records found.\n"; return; }

        //else open the file and read from it, Student's payment history.
        std::ifstream file(filename);
        std::string line;
        std::cout << "\n--- Payment History ---\n";
        while (std::getline(file, line)) std::cout << line << "\n";
        file.close();
    }

    void viewRegistrationCard() override {
        std::string filename;
        for (const auto &entry : fs::directory_iterator(".")) {
            std::string fname = entry.path().filename().string();
            if (fname.rfind(paycode + "_", 0) == 0 && fname.find(".txt") != std::string::npos) {
                filename = fname;
                break;
            }
        }
        if (filename.empty()) { std::cout << "❌ No record found.\n"; return; }

        // Get totals
        int totalPaid = getTotalFromTXT();
        int balance = REQUIRED_TUITION - totalPaid;
        double percent = (totalPaid * 100.0) / REQUIRED_TUITION;
        std::string cardType = determineCard(totalPaid);

        std::ifstream file(filename);
        std::string nameLine, regLine;// Variables to hold the first two lines from the file
        std::getline(file, nameLine); // Read the first line and its the <Student name>
        std::getline(file, regLine);

        // Extract the actual name and regno by removing the "Name: " prefix, its 6 characters and "RegNo: "
        std::string nameOut = nameLine.substr(6);
        std::string regOut = regLine.substr(7);
        file.close();

        std::cout << "\n--- Registration Card ---\n";
        std::cout << "Name: " << nameOut << "\n";
        std::cout << "RegNo: " << regOut << "\n";
        std::cout << "Paycode: " << paycode << "\n";
        std::cout << "Total Tuition Paid: " << totalPaid << "\n";
        std::cout << "Remaining Balance: " << balance << "\n";
        std::cout << "Percentage Paid: " << std::fixed << std::setprecision(2) << percent << "%\n";
        std::cout << "Meal Card Eligibility: " << cardType << "\n";
    }
 // ===== Find File by Paycode =====
static std::string findFileByPaycode(const std::string &paycode)
{
// Loop through all files in the current directory and get only the file name.
    for (const auto &entry : fs::directory_iterator(".")) {
        std::string fname = entry.path().filename().string();
        if (fname.rfind(paycode + "_", 0) == 0 && fname.find(".txt") != std::string::npos) {
            return fname;
        }
    }
    return "";
}

};

// ===== Standalone Admin Class =====
class Admin {
private:
    const std::string ADMIN_PASSWORD = "admin123";

public:
//Admin Login
    bool login() {
        std::string password;
        char ch;
        std::cout << "Enter Admin Password: ";

        // Loop until the Enter key ('\r') is pressed
        while ((ch = _getch()) != '\r') {
            // If Backspace is pressed and password is not empty
            if (ch == '\b' && !password.empty()) {
                password.pop_back(); // Remove the last character from the password
                std::cout << "\b \b";
            } else if (ch != '\b') {  // If any other character is pressed
                password.push_back(ch); // Add the character to the password string
                std::cout << '*';// Print '*' to mask the character
            }
        }
        std::cout << std::endl;
        return password == ADMIN_PASSWORD;
    }

//view all the student records stored in StudentPayments.csv.
    void viewAllRecords() {
        std::ifstream file("StudentPayments.csv");
        if (!file.is_open()) { std::cout << "No records found.\n"; return; }

        std::string line;
        std::cout << "\n--- All Student Records ---\n";

// Read each line from the CSV file, parse its comma-separated fields, and display the payment details in a formatted manner.

        while (std::getline(file, line)) {
            std::stringstream ss(line);
            std::string paycode, name, regNo, tuitionStr, totalStr, remainingStr, card, dateTime;
            std::getline(ss, paycode, ','); // Extract the first field: paycode
            std::getline(ss, name, ',');
            std::getline(ss, regNo, ',');
            std::getline(ss, tuitionStr, ',');
            std::getline(ss, totalStr, ',');
            std::getline(ss, remainingStr, ',');
            std::getline(ss, card, ',');
            std::getline(ss, dateTime, ',');
            std::cout << "Name: " << name 
                      << " | RegNo: " << regNo 
                      << " | Paycode: " << paycode 
                      << " | Tuition Paid: " << tuitionStr
                      << " | Total Paid: " << totalStr
                      << " | Remaining: " << remainingStr
                      << " | Card Type: " << card
                      << " | Date: " << dateTime
                      << "\n";
        }
        file.close();
    }

//Search Student Records
    void searchRecord() {
        std::string paycode;
        std::cout << "Enter Paycode: ";
        std::getline(std::cin, paycode);
        std::ifstream file("StudentPayments.csv");
        if (!file.is_open()) { std::cout << "No records found.\n"; return; }

        std::string line;
        bool found = false;

 // Read each line from the CSV file until the end
        while (std::getline(file, line)) {
            if (line.rfind(paycode + ",", 0) == 0) {// Check if the line starts with the given paycode followed by a comma
                std::stringstream ss(line);
                std::string pc, name, regNo, tuitionStr, totalStr, remainingStr, card, dateTime;
                std::getline(ss, pc, ',');  // Extract paycode field
                std::getline(ss, name, ',');
                std::getline(ss, regNo, ',');
                std::getline(ss, tuitionStr, ',');
                std::getline(ss, totalStr, ',');
                std::getline(ss, remainingStr, ',');
                std::getline(ss, card, ',');
                std::getline(ss, dateTime, ',');
                std::cout << "Name: " << name 
                          << " | RegNo: " << regNo 
                          << " | Paycode: " << pc
                          << " | Tuition Paid: " << tuitionStr
                          << " | Total Paid: " << totalStr
                          << " | Remaining: " << remainingStr
                          << " | Card Type: " << card
                          << " | Date: " << dateTime
                          << "\n";
                found = true;
            }
        }
        if (!found) std::cout << "No record found.\n";
        file.close();
    }

    void updateRecord() {
    std::string paycode;
    std::cout << "Enter Paycode to update: ";
    std::getline(std::cin, paycode);

    std::ifstream file("StudentPayments.csv");
    if (!file.is_open()) { 
        std::cout << "No records found.\n"; 
        return; 
    }

// Vector to store all lines from the CSV file
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line))
        lines.push_back(line); // Store each line in the vector
    file.close();

    bool found = false;

// Loop through all lines stored in the vector
    for (auto &l : lines) {
        if (l.rfind(paycode + ",", 0) == 0) {// Check if the line starts with the given paycode
            found = true;

     // Create stringstream to parse the line
            std::stringstream ss(l);
            std::string tokens[8]; // Array to store the 8 fields of the CSV line
            for (int i = 0; i < 8; ++i)
                std::getline(ss, tokens[i], ','); // Extract each comma-separated field

            // Display current details
            std::cout << "\n--- Current Record Details ---\n";
            std::cout << "Name: " << tokens[1] << "\n";
            std::cout << "RegNo: " << tokens[2] << "\n";
            std::cout << "Tuition Paid: " << tokens[3] << "\n";
            std::cout << "Total Paid: " << tokens[4] << "\n";
            std::cout << "Remaining: " << tokens[5] << "\n";
            std::cout << "Card Type: " << tokens[6] << "\n";
            std::cout << "Date: " << tokens[7] << "\n";

            // Ask for updates
            std::cout << "\nEnter updated Name (press Enter to keep current): ";
            std::string newName;
            std::getline(std::cin, newName);

            std::cout << "Enter updated RegNo (press Enter to keep current): ";
            std::string newRegNo;
            std::getline(std::cin, newRegNo);

            if (!newName.empty()) tokens[1] = newName;  // Update Name if user entered a new one
            if (!newRegNo.empty()) tokens[2] = newRegNo;// Update RegNo if user entered a new one

            // Rebuild updated line
            l = tokens[0];  // Start with paycode
            for (int i = 1; i < 8; ++i)
                l += "," + tokens[i]; // Append remaining fields with commas

            break;
        }
    }

    if (!found) { 
        std::cout << "Record not found.\n"; 
        return; 
    }

    std::ofstream out("StudentPayments.csv");

// Write all lines back to the CSV file (overwriting it)
    for (const auto &l : lines)
        out << l << "\n";
    out.close();

    std::cout << "\nRecord updated successfully.\n";
}

//delete student Records
    void deleteRecord() {
         std::string paycode;
    std::cout << "Enter Paycode to delete: ";
    std::getline(std::cin, paycode);

    std::ifstream file("StudentPayments.csv");
    if (!file.is_open()) { std::cout << " No records found.\n"; return; }

    // Vector to store all lines from the CSV file
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line))//Read each line from the csv
        lines.push_back(line);// Store each line in the vector
    file.close();

    // Open CSV file for writing (overwrites existing file)
    std::ofstream csvOut("StudentPayments.csv");
    bool deleted = false;

    // Loop through all lines stored in the vector
    for (const auto &l : lines) {
        // If the line does NOT start with the given paycode, write the line back to the csv file.
        if (l.rfind(paycode + ",", 0) != 0)
            csvOut << l << "\n";
        else deleted = true;
    }
    csvOut.close();

    if (deleted) std::cout << "Record deleted successfully.\n";
    else std::cout << "Record not found.\n";
    }
};

// ===== Student Menu =====
void studentMenu() {
    // Clear the input buffer up to the next newline to avoid leftover input interfering with std::getline()
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 
    int choice;
    int tuitionAmount;
    std::string tuitionInput;
    do {
        std::cout << "\n===== Student/Parent Menu =====\n";
        std::cout << "1. Make Payment\n";
        std::cout << "2. View Payment History\n";
        std::cout << "3. View Registration Card\n";
        std::cout << "4. Back\n";
        std::cout << "Enter choice: ";
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');//clearing the input buffer

        switch (choice) {
            case 1: {
                Student student; // Declare a variable 'student' of type Student (a struct) to store a single student's information
                do {
                    std::cout << "Enter Paycode (10 digits): ";
                    std::getline(std::cin, student.paycode);
                    if (!StudentAccount::validatePaycode(student.paycode))
                        std::cout << "Invalid Paycode!\n";
                } while (!StudentAccount::validatePaycode(student.paycode));

               std::string fname = StudentAccount::findFileByPaycode(student.paycode);
                if (!fname.empty()) {
                    student.name = fname.substr(fname.find("_") + 1);
                    student.name = student.name.substr(0, student.name.find(".txt"));
                    std::cout << "Record found for student: " << student.name << "\n";
                    student.regNo = "N/A";
                }

                else {
                    while (true) {
                    std::cout << "Enter Full Name (e.g., Lukwago Enoch): ";
                    std::getline(std::cin,student.name);
                    if (StudentAccount::isValidName(student.name))
                        break;
                    else
                        std::cout << "Invalid name! Please enter at least two words with letters only.\n";
                                }
                    do {
                        std::cout << "Enter Registration Number (e.g., 24/1/370/D/040): ";
                        std::getline(std::cin, student.regNo);
                        if (!StudentAccount::validateRegNo(student.regNo))
                            std::cout << "Invalid format!\n";
                       } while (!StudentAccount::validateRegNo(student.regNo));
                 }
        while (true) {
        std::cout << "Enter Tuition Amount to pay(UGX): ";
        std::getline(std::cin, tuitionInput);

        if (StudentAccount::isValidTuition(tuitionInput, tuitionAmount))
        break;
        else
        std::cout << "Invalid input! Enter digits only and ensure it's a positive value.\n";
}

// Assign the validated numeric value to student.tuition
        student.tuition = tuitionAmount;
                
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        // Creating a StudentAccount object 'sa' using the data from the 'student' struct:
                StudentAccount sa(student.name, student.regNo, student.paycode, student.tuition);
                sa.savePayment();
                break;
            }

            {//2. View Payment History
            case 2: 
                std::string paycode;
                std::cout << "Enter Paycode: ";
                std::getline(std::cin, paycode);
                StudentAccount sa("", "", paycode);
                sa.viewPaymentHistory();
                break;
            }

            // 3. View Registration card
            case 3: {
                std::string paycode;
                std::cout << "Enter Paycode: ";
                std::getline(std::cin, paycode);
                StudentAccount sa("", "", paycode);
                sa.viewRegistrationCard();
                break;
            }

            case 4: std::cout << "Returning to Access Point Menu...\n"; break;
            default: std::cout << "Invalid choice.\n";
        }
    } while (choice != 4);
}

// ===== Admin Menu =====
void adminMenu() {
    //Creating an  object for the Admin class
    Admin admin;
    if (!admin.login()) { std::cout << "Wrong password.\n"; return; }

    int choice;
    do {
        std::cout << "\n===== Admin Menu =====\n";
        std::cout << "1. View All Records\n2. Search Record\n3. Update Record\n4. Delete Record\n5. Back\n";
        std::cout << "Enter choice: ";
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        switch (choice) {
            case 1: admin.viewAllRecords(); break;
            case 2: admin.searchRecord(); break;
            case 3: admin.updateRecord(); break;
            case 4: admin.deleteRecord(); break;
            case 5: std::cout << "Returning to Access Point Menu...\n"; break;
            default: std::cout << "Invalid choice.\n";
        }
    } while (choice != 5);
}

// ===== Main =====
int main() {
    int mainChoice;
    do {
        std::cout << "\n==== NDU-STUDENT TUITION TRACKING AND REGISTRATION SYSTEM (NDU-STTARS)====\n";
        std::cout << "1. Student/Parent\n2. Admin\n3. Exit\n";
        std::cout << "Enter choice: ";
        std::cin >> mainChoice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');//clearing the input buffer

        switch (mainChoice) {
            case 1: studentMenu(); break;
            case 2: adminMenu(); break;
            case 3: std::cout << "Logging out...\n"; break;
            default: std::cout << "Invalid choice.\n";
        }
    } while (mainChoice != 3);

    return 0;
}
