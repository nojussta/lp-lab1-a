#include <iostream>
#include <iomanip>
#include "json.hpp"
#include <fstream>

using namespace std;
using json = nlohmann::json;

// Define the Student struct
struct Student {
	string name;
	int year;
	double grade;
	string hashed;
};

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
		maxNameWidth = max(maxNameWidth, static_cast<int>(student.name.length()));
		maxYearWidth = max(maxYearWidth, static_cast<int>(to_string(student.year).length()));
		maxGradeWidth = max(maxGradeWidth, static_cast<int>(to_string(student.grade).length()));
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
