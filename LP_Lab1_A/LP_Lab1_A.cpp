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
	bool isRunning = true;
	bool shouldReturnEmpty = false;

	// Notify waiting threads that data is available
	void notify() {
		dataCondition.notify_all();
	}

	// Add a car into the data buffer
	void add(Car newCar) {
		unique_lock<mutex> lock(monitorMutex);
		dataCondition.wait(lock, [this] { return count < 16; });
		dataBuffer[count++] = newCar;
		dataCondition.notify_all();

		// Output when an object is added to DataMonitor
		cout << "Added car to DataMonitor. Count: " << count << endl;

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

public:
	mutex monitorMutex;
	bool isRunning = true;

	// Get the result buffer
	array<Car, 16> getResultBuffer() const {
		return resultBuffer;
	}

	// Add a car into the result buffer in a sorted manner
	void addSorted(Car newCar) {
		unique_lock<mutex> lock(monitorMutex);

		if (newCar.power < 100) {
			return;
		}

		auto it = lower_bound(resultBuffer.begin(), resultBuffer.begin() + count, newCar,
			[](const Car& a, const Car& b) {
				return a.make < b.make;
			});

		for (int i = count; i > (it - resultBuffer.begin()); --i) {
			resultBuffer[i] = resultBuffer[i - 1];
		}

		resultBuffer[it - resultBuffer.begin()] = newCar;
		count++;
		resultCondition.notify_all();
	}

	// Get the current count of cars in the result buffer
	int getCount() {
		return count;
	}
};

ResultMonitor resultMonitor;

// Define a mutex for console and file output
mutex outputMutex;

// Function to process car data
void processCarData(int threadCount, int maxMakeWidth, int maxConsumptionWidth, int maxPowerWidth, const array<Car, 16>& cars, string threadType, double filterThreshold) {
	string dashHeader = " ----------------------------------------------------------------------------";
	string carHeader = " | Car Data                                                                 |";

	while (dataMonitor.isRunning || dataMonitor.getCount() > 0) {
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
			ostringstream os;
			os << car.make << car.consumption << car.power;
			string resultOutput = os.str() + "\n";

			// Acquire the output mutex before printing to console and file
			unique_lock<mutex> lock(outputMutex);

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
			cout << threadType << " " << threadCount << " - Processing: " << car.make << " | Consumption: " << car.consumption << " | Power: " << car.power << " | Hash Code: " << car.hashCode << " | Performance Score: " << car.performanceScore << "\n";
			cout << "Result: " << resultOutput << endl;

			// Output to the result file
			ofstream outputFile("result.txt", ios::app); // Open the file in append mode
			if (outputFile) {
				outputFile << threadType << " " << threadCount << " - Processing: " << car.make << " | Consumption: " << car.consumption << " | Power: " << car.power << " | Hash Code: " << car.hashCode << " | Performance Score: " << car.performanceScore << "\n";
				outputFile << "Result: " << resultOutput << endl;
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
				outputFile << threadType << " " << threadCount << " - Processing: " << car.make << " | Consumption: " << car.consumption << " | Power: " << car.power << " | Performance Score: " << car.performanceScore << "\n";
				outputFile << "Result: " << resultOutput << endl;
			}
			outputFile.close();

			// Release the output mutex
			lock.unlock();

			// Add the result into the result monitor
			resultMonitor.addSorted(car);
		}
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

	thread mainThread = thread(processCarData, 0, maxMakeWidth, maxConsumptionWidth, maxPowerWidth, mainCars, "MainThread", filterThreshold);

	thread threads[threadCount];
	for (int i = 0; i < threadCount; ++i) {
		threads[i] = thread(processCarData, i + 1, maxMakeWidth, maxConsumptionWidth, maxPowerWidth, mainCars, "WorkerThread", filterThreshold);
	}

	// Add cars into the data monitor
	for (int i = mainCars.size() - 1; i >= 0; --i) {
		dataMonitor.add(mainCars[i]);
	}

	// Signal threads to stop and wait for them to finish
	dataMonitor.isRunning = false;
	dataMonitor.shouldReturnEmpty = true;
	dataMonitor.notify();
	for (auto& thread : threads) {
		thread.join();
	}

	// Wait for the main thread to finish
	mainThread.join();

	// Print a message indicating that the DataMonitor is completely empty
	cout << "DataMonitor is completely empty." << endl;

	return 0;
}