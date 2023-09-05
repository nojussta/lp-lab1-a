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

	for (const auto& student : mainStudents) {
		cout << "Name: " << student.name << " Year: " << student.year << " Grade: " << student.grade << '\n';
	}

	return 0;
}
