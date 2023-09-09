#include <iostream>
#include <iomanip>
#include <fstream>
#include <condition_variable>
#include "json.hpp"
#include "sha1.hpp"


using namespace std;
using json = nlohmann::json;

// Define the Student struct
struct Student {
	string name;
	int year;
	double grade;
	string hashCode;
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

// ResultMonitor class for managing processed results
class ResultMonitor {
private:
	array<Student, 19> resultBuffer; // Fixed-size array
	int count = 0; // Keeps track of the number of elements in the buffer
	condition_variable resultCondition;

public:
	mutex monitorMutex;
	bool isRunning = true;

	// 5: Iš rezultatų monitoriaus, kuriame saugomi gauti atfiltruoti rezultatai, rezultatus
	// išveda į tekstinį failą lentele.
	// Get the result buffer
	array<Student, 19> getResultBuffer() const {
		return resultBuffer;
	}

	// 8: Patikrina, ar gautas rezultatas tinka pagal pasirinktą kriterijų. Jei tinka, jis įrašomas į rezultatų monitorių taip, kad po įrašymo monitorius liktų surikiuotas.
	// add a student into the result buffer in a sorted manner
	void addSorted(Student newStudent) {
		unique_lock<mutex> lock(monitorMutex);

		if (newStudent.grade < 7) {
			return;
		}

		auto it = lower_bound(resultBuffer.begin(), resultBuffer.begin() + count, newStudent,
			[](const Student& a, const Student& b) {
				return a.name < b.name;
			});

		for (int i = count; i > (it - resultBuffer.begin()); --i) {
			resultBuffer[i] = resultBuffer[i - 1];
		}

		resultBuffer[it - resultBuffer.begin()] = newStudent;
		count++;
		resultCondition.notify_all();
	}

	// 8: Patikrina, ar gautas rezultatas tinka pagal pasirinktą kriterijų. Jei tinka, jis įrašomas į rezultatų monitorių taip, kad po įrašymo monitorius liktų surikiuotas.
	// Get the current count of students in the result buffer
	int getCount() {
		return count;
	}
};

ResultMonitor resultMonitor;

// Function to process student data
void processStudentData(int threadCount) {
	while (dataMonitor.isRunning || dataMonitor.getCount() > 0) {
		Student student = dataMonitor.remove();
		if (student.grade == 404) {
			break;
		}

		// Calculate SHA-1 hash for student data
		SHA1 sha1;
		sha1.update(student.name);
		sha1.update(std::to_string(student.year));
		sha1.update(std::to_string(student.grade));
		std::string hashCode = sha1.final();
		student.hashCode = hashCode;

		ostringstream os;
		os << student.name << "| " << student.year << "| " << student.grade << "| " << hashCode;
		string resultOutput = os.str() + "\n";

		// Output what each thread is doing to both console and file
		cout << "Thread " << threadCount << " - Processing: " << student.name << " | Year: " << student.year << " | Grade: " << student.grade << "\n";
		cout << "Result: " << resultOutput << endl;

		ofstream outputFile("result.txt", ios::app); // Open the file in append mode
		if (outputFile) {
			outputFile << "Thread " << threadCount << " - Processing: " << student.name << " | Year: " << student.year << " | Grade: " << student.grade << "\n";
			outputFile << "Result: " << resultOutput << endl;
			outputFile.close();
		}

		// 8: Patikrina, ar gautas rezultatas tinka pagal pasirinktą kriterijų. Jei tinka, jis įrašomas į rezultatų monitorių taip, kad po įrašymo monitorius liktų surikiuotas.
		// add the result into the result monitor
		resultMonitor.addSorted(student);
	}
}

// 2: Paleidžia pasirinktą kiekį darbininkių gijų 2 ≤ x ≤ n/4 (n — duomenų kiekis faile).
// 4: Palaukia, kol visos darbininkės gijos baigs darbą.
// Function to print header and student data to console and file
void printHeaderAndData(const array<Student, 19>& students, int maxNameWidth, int maxYearWidth, int maxGradeWidth) {
	ofstream outputFile("result.txt");
	if (!outputFile) {
		cerr << "Error opening the output file." << endl;
		return;
	}

	// 10: Abu monitorius rekomenduojama realizuoti kaip klasę ar struktūrą, turinčią bent
	// dvi operacijas: elementui įdėti bei pašalinti. Atliekant su Rust naudoti Rust monitorius.
	// 11: Duomenys viduje saugomi fiksuoto dydžio masyve (sąrašas ar kita struktūra netinka).
	// 12: Duomenų monitoriaus masyvo dydis neviršija pusės faile esančių elementų kiekio.
	// 13: Rezultatų monitoriaus masyvo dydis parenkamas toks, kad tilptų visi rezultatai.
	// 14: Su monitoriumi atliekamos operacijos, kur reikia, apsaugomos kritine sekcija, o gijų
	// blokavimas realizuojamas sąlygine sinchronizacija, panaudojant pasirinktos kalbos
	// priemones.
	// 15: Duomenų monitorius turi galėti tiek prisipildyti, tiek ištuštėti (kitaip nėra prasmės
	// jo turėti :)

	// Print the header to both console and file
	cout << " ---------------------------" << endl;
	cout << " | Student Data            |" << endl;
	cout << " ---------------------------" << endl;
	cout << " |" << setw(maxNameWidth) << "Name     " << " |"
		<< setw(maxYearWidth) << "Year" << "|"
		<< setw(maxGradeWidth) << "    Grade" << "|" << '\n';
	cout << " ---------------------------" << endl;

	outputFile << " ---------------------------" << endl;
	outputFile << " | Student Data            |" << endl;
	outputFile << " ---------------------------" << endl;
	outputFile << " |" << setw(maxNameWidth) << "Name     " << " |"
		<< setw(maxYearWidth) << "Year" << "|"
		<< setw(maxGradeWidth) << "    Grade" << "|" << '\n';
	outputFile << " ---------------------------" << endl;

	// Print the student data with adjusted widths to console and file
	for (int i = 0; i < students.size(); ++i) {
		const auto& student = students[i];
		// Print to console
		cout << " |" << setw(maxNameWidth) << student.name << "  |"
			<< setw(3) << student.year << " |"
			<< setw(maxGradeWidth) << student.grade << " |" << '\n';

		// Print to file
		outputFile << " |" << setw(maxNameWidth) << student.name << "  |"
			<< setw(3) << student.year << " |"
			<< setw(maxGradeWidth) << student.grade << " |" << '\n';
	}

	// Close the file
	outputFile << " ---------------------------" << endl;
	outputFile.close();

	// Close the console output
	cout << " ---------------------------" << endl;
}

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
