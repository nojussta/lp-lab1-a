#include <mutex>
#include "json.hpp"
#include "sha1.hpp"

using namespace std;
using json = nlohmann::json;

// Define the Car struct with additional fields
struct Car {
	string make;
	double consumption;
	int power;
	string hashCode;
	double performanceScore;
};

// Function to calculate the performance score
double calculatePerformanceScore(const Car& car) {
	return car.power / car.consumption;
}

// Function to check if a car meets the filter criteria
bool meetsFilterCriteria(const Car& car, double filterThreshold) {
	return car.performanceScore > filterThreshold;
}

// Define the DataMonitor class for managing car data
class DataMonitor {
private:
	array<Car, 16> dataBuffer; // Fixed-size array
	int count = 0; // Keeps track of the number of elements in the buffer
	condition_variable dataCondition;

public:
	mutex monitorMutex;
	bool isRunning = false;
	bool shouldReturnEmpty = false;

	void isFinished()
	{
		isRunning = true;
	}

	// Add a car into the data buffer
	void add(Car newCar) {
		unique_lock<mutex> lock(monitorMutex);
		dataCondition.wait(lock, [this] { return count < 16; });
		dataBuffer[count++] = newCar;
		dataCondition.notify_all();

		// Output when an object is added to DataMonitor
		//cout << endl;
		cout << "Added car to DataMonitor. Count: " << count << endl;
		//cout << endl;

		// Output when the DataMonitor is full
		if (count == 16) {
			cout << "DataMonitor is full. Waiting for space." << endl;
		}
	}

	// Remove a car from the data buffer
	Car remove() {
		Car car;
		unique_lock<mutex> lock(monitorMutex);
		dataCondition.wait(lock, [this] { return count > 0 || shouldReturnEmpty; });

		if (shouldReturnEmpty) {
			car.power = -1;
		}
		else {
			car = dataBuffer[--count];
			cout << "Removed car from DataMonitor. Count: " << count << endl;
			//cout << endl;
		}

		dataCondition.notify_all();

		// Output when the DataMonitor is empty
		if (count == 0) {
			cout << "DataMonitor is empty. Waiting for data." << endl;
		}

		return car;
	}

	// Get the current count of cars in the data buffer
	int getCount() {
		return count;
	}
};

DataMonitor dataMonitor;

// ResultMonitor class for managing processed results
class ResultMonitor {
private:
	array<Car, 16> resultBuffer; // Fixed-size array
	int count = 0; // Keeps track of the number of elements in the buffer
	condition_variable resultCondition;
	mutex monitorMutex;

public:
	bool isRunning = true;

	void addSorted(Car newCar) {
		unique_lock<mutex> lock(monitorMutex);

		resultBuffer[count] = newCar;
		count++;

		// Bubble Sort
		for (int i = 0; i < count - 1; ++i) {
			for (int j = 0; j < count - i - 1; ++j) {
				if (resultBuffer[j].make < resultBuffer[j + 1].make) {
					// Swap if the cars are in the wrong order
					swap(resultBuffer[j], resultBuffer[j + 1]);
				}
			}
		}

		resultCondition.notify_all();
	}

	// Get the result buffer
	array<Car, 16> getFilteredCars() const {
		return resultBuffer;
	}

	// Get the current count of cars in the result buffer
	int getCount() {
		return count;
	}

	void printResult(const Car& car, int maxMakeWidth, int maxConsumptionWidth, int maxPowerWidth) {
		unique_lock<mutex> lock(monitorMutex);

		string dashHeader = " ----------------------------------------------------------------------------";
		string carHeader = " | Car Data                                                                 |";

		cout << dashHeader << endl;
		cout << carHeader << endl;
		cout << dashHeader << endl;
		cout << " |" << setw(maxMakeWidth) << "Make      " << " |"
			<< setw(maxConsumptionWidth) << " Consumption" << "|"
			<< setw(maxPowerWidth) << "  Power" << "|"
			<< setw(40) << "  Hash Code" << " |" << '\n';
		cout << dashHeader << endl;
		cout << " |" << setw(maxMakeWidth) << car.make << "  |"
			<< setw(11) << car.consumption << " |"
			<< setw(maxPowerWidth) << car.power << "    |"
			<< setw(40) << car.hashCode << " |" << '\n';
		cout << dashHeader << endl;

		// Output what each thread is doing to both console and file
		//cout << "Processing: " << car.make << " | Consumption: " << car.consumption << " | Power: " << car.power << " | Hash Code: " << car.hashCode << " | Performance Score: " << car.performanceScore << "\n";
		cout << "Performance Score: " << car.performanceScore << "\n";

		// Output to the result file
		ofstream outputFile("result.txt", ios::app); // Open the file in append mode
		if (outputFile) {
			outputFile << "Processing: " << car.make << " | Consumption: " << car.consumption << " | Power: " << car.power << " | Hash Code: " << car.hashCode << " | Performance Score: " << car.performanceScore << "\n";
			outputFile << dashHeader << endl;
			outputFile << carHeader << endl;
			outputFile << dashHeader << endl;
			outputFile << " |" << setw(maxMakeWidth) << "Make      " << " |"
				<< setw(maxConsumptionWidth) << "Consumption " << "|"
				<< setw(maxPowerWidth) << "  Power" << "|"
				<< setw(40) << "  Hash Code" << " |" << '\n';
			outputFile << dashHeader << endl;
			outputFile << " |" << setw(maxMakeWidth) << car.make << "  |"
				<< setw(11) << car.consumption << " |"
				<< setw(maxPowerWidth) << car.power << "    |"
				<< setw(40) << car.hashCode << " |" << '\n';
			outputFile << dashHeader << endl;
			//outputFile << "Processing: " << car.make << " | Consumption: " << car.consumption << " | Power: " << car.power << " | Performance Score: " << car.performanceScore << "\n";
			outputFile << "Performance Score: " << car.performanceScore << "\n";
		}
		outputFile.close();
	}
};

ResultMonitor resultMonitor;

void processCarData(int threadCount, int maxMakeWidth, int maxConsumptionWidth, int maxPowerWidth, const array<Car, 16>& cars, string threadType, double filterThreshold) {
	string dashHeader = " ----------------------------------------------------------------------------";
	string carHeader = " | Car Data                                                                 |";

	while (true) {
		if (dataMonitor.isRunning || dataMonitor.getCount() > 0)
		{
			Car car = dataMonitor.remove();
			if (car.power == -1) {
				break;
			}

			// Calculate SHA-1 hash for car data
			SHA1 sha1;
			sha1.update(car.make);
			sha1.update(std::to_string(car.consumption));
			sha1.update(std::to_string(car.power));
			std::string hashCode = sha1.final();
			car.hashCode = hashCode;

			// Calculate the performance score
			car.performanceScore = calculatePerformanceScore(car);

			// Check if the car meets the filter criteria
			if (car.power > 100) {
				// Add the result into the result monitor
				resultMonitor.addSorted(car);
			}
		}
		else
			break;
	}
}

// Function to print header and car data to console and file
void printHeaderAndData(const array<Car, 16>& cars, int maxMakeWidth, int maxConsumptionWidth, int maxPowerWidth) {
	// Print the header to both console and file
	cout << " ----------------------------------" << endl;
	cout << " | Car Data                       |" << endl;
	cout << " ----------------------------------" << endl;
	cout << " |" << setw(maxMakeWidth) << "Make      " << " |"
		<< setw(maxConsumptionWidth) << " Consumption" << "|"
		<< setw(maxPowerWidth) << "  Power" << "|" << '\n';
	cout << " ----------------------------------" << endl;

	ofstream outputFile("result.txt");
	if (!outputFile) {
		cerr << "Error opening the output file." << endl;
		return;
	}

	outputFile << " ----------------------------------" << endl;
	outputFile << " | Car Data                       |" << endl;
	outputFile << " ----------------------------------" << endl;
	outputFile << " |" << setw(maxMakeWidth) << "Make      " << " |"
		<< setw(maxConsumptionWidth) << "Consumption " << "|"
		<< setw(maxPowerWidth) << "  Power" << "|" << '\n';
	outputFile << " ----------------------------------" << endl;

	// Print the car data with adjusted widths to console and file
	for (int i = 0; i < cars.size(); ++i) {
		const auto& car = cars[i];
		// Print to console
		cout << " |" << setw(maxMakeWidth) << car.make << "  |"
			<< setw(11) << car.consumption << " |"
			<< setw(maxPowerWidth) << car.power << "    |" << '\n';

		// Print to file
		outputFile << " |" << setw(maxMakeWidth) << car.make << "  |"
			<< setw(11) << car.consumption << " |"
			<< setw(maxPowerWidth) << car.power << "    |" << '\n';
	}

	// Close the file
	outputFile << " ----------------------------------" << endl;
	outputFile.close();

	// Close the console output
	cout << " ----------------------------------" << endl;
}

int main() {
	//ResultMonitor resultMonitor;
	const int threadCount = 4;
	double filterThreshold = 50.0;

	ifstream inputFile("duomenys.json");
	string jsonData((istreambuf_iterator<char>(inputFile)), istreambuf_iterator<char>());
	json jsonCars = json::parse(jsonData);

	array<Car, 16> mainCars;

	// Parse car data from JSON
	int i = 0;
	for (const auto& carData : jsonCars["cars"]) {
		Car car;
		car.make = carData["make"];
		car.consumption = carData["consumption"];
		car.power = carData["power"];
		mainCars[i++] = car;
	}

	int maxMakeWidth = 0;
	int maxConsumptionWidth = 0;
	int maxPowerWidth = 0;

	for (const auto& car : mainCars) {
		maxMakeWidth = max(maxMakeWidth, int(car.make.length()));
		maxConsumptionWidth = max(maxConsumptionWidth, int(to_string(car.consumption).length()));
		maxPowerWidth = max(maxPowerWidth, int(to_string(car.power).length()));
	}

	// Call the printHeaderAndData function to print header and car data
	printHeaderAndData(mainCars, maxMakeWidth, maxConsumptionWidth, maxPowerWidth);

	vector<thread> threads;
	for (int i = 0; i < threadCount; i++)
	{
		threads.emplace_back([&] {processCarData(i + 1, maxMakeWidth, maxConsumptionWidth, maxPowerWidth, mainCars, "WorkerThread", filterThreshold); });
	}

	for (int i = 0; i < mainCars.size(); i++)
	{
		dataMonitor.add(mainCars[i]);
	}

	// Signal threads to stop and wait for them to finish
	dataMonitor.isFinished();
	dataMonitor.shouldReturnEmpty = true;
	for_each(threads.begin(), threads.end(), mem_fn(&thread::join));

	//this_thread::sleep_for(std::chrono::seconds(10));

	// Print a message indicating that the DataMonitor is completely empty
	cout << "DataMonitor is completely empty." << endl;

	// Print the results directly from the result monitor
	array<Car, 16> sortedCars;
	sortedCars = resultMonitor.getFilteredCars();
	for (size_t i = 0; i < resultMonitor.getCount(); i++)
	{
		resultMonitor.printResult(sortedCars[i], maxMakeWidth, maxConsumptionWidth, maxPowerWidth);
	}
	return 0;
}
