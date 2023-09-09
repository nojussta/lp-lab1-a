﻿#include <iostream>
#include <iomanip>
#include <fstream>
#include <condition_variable>
#include "json.hpp"


using namespace std;
using json = nlohmann::json;

// Define the Student struct
struct Student {
	string name;
	int year;
	double grade;
	string hashed;
};

// DataMonitor class for managing student data
class DataMonitor {
private:
	array<Student, 19> dataBuffer; // Fixed-size array
	int count = 0; // Keeps track of the number of elements in the buffer
	condition_variable dataCondition;

public:
	mutex monitorMutex;
	bool isRunning = true;
	bool shouldReturnEmpty = false;

	// 1: Nuskaito duomenų failą į lokalų masyvą, sąrašą ar kitą duomenų struktūrą;
	// Notify waiting threads that data is available
	void notify() {
		dataCondition.notify_all();
	}

	// 3: Į duomenų monitorių, įrašo visus nuskaitytus duomenis po vieną. Jei monitorius
	// yra pilnas, gija blokuojama, kol atsiras vietos.
	// Add a student into the data buffer
	void add(Student newStudent) {
		unique_lock<mutex> lock(monitorMutex);
		while (count >= 19) {
			dataCondition.wait(lock);
		}
		dataBuffer[count] = newStudent;
		count++;
		dataCondition.notify_all();
	}

	// 6: Darbininkės gijos atlieka tokius veiksmus:
	// Iš duomenų monitoriaus paima elementą. Jei duomenų monitorius yra tuščias, gija
	// laukia, kol jame atsiras duomenų.
	// Remove a student from the data buffer
	Student remove() {
		Student student;
		unique_lock<mutex> lock(monitorMutex);
		while (count == 0) {
			if (shouldReturnEmpty) {
				student.grade = 404;
				return student;
			}
			dataCondition.wait(lock);
		}
		count--;
		student = dataBuffer[count];
		dataCondition.notify_all();
		return student;
	}

	// Get the current count of students in the data buffer
	int getCount() {
		return count;
	}
};

DataMonitor dataMonitor;

int main() {
	ifstream inputFile("duomenys.json");
	string jsonData((istreambuf_iterator<char>(inputFile)), istreambuf_iterator<char>());
	json jsonStudents = json::parse(jsonData);
	vector<Student> mainStudents;

	// Parse student data from JSON
	for (const auto& studentData : jsonStudents["students"]) {
		Student student;
		student.name = studentData["name"];
		student.year = studentData["year"];
		student.grade = studentData["grade"];
		mainStudents.push_back(student);
	}

	int maxNameWidth = 0;
	int maxYearWidth = 0;
	int maxGradeWidth = 0;

	for (const auto& student : mainStudents) {
		maxNameWidth = max(maxNameWidth, int(student.name.length()));
		maxYearWidth = max(maxYearWidth, int(to_string(student.year).length()));
		maxGradeWidth = max(maxGradeWidth, int(to_string(student.grade).length()));
	}

	// Print the header
	cout << " ---------------------------" << endl;
	cout << " | Student Data            |" << endl;
	cout << " ---------------------------" << endl;
	cout << " |" << setw(maxNameWidth) << "Name     " << " |"
		<< setw(maxYearWidth) << "Year" << "|"
		<< setw(maxGradeWidth) << "    Grade" << "|" << '\n';
	cout << " ---------------------------" << endl;

	// Print the student data with adjusted widths
	for (const auto& student : mainStudents) {
		cout << " |" << setw(maxNameWidth) << student.name << "  |"
			<< setw(3) << student.year << " |"
			<< setw(maxGradeWidth) << student.grade << " |" << '\n';
	}

	cout << " ---------------------------" << endl;

	return 0;
}
