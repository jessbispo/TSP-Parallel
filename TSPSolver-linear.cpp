#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <limits>
#include <fstream>
#include <sstream>
#include <string>

class TSPSolver {
public:
    TSPSolver(unsigned seed)
        : gen_(seed) {
        loadAdjacencyMatrix();
    }

    std::vector<int> solveTSP(int numIterations, int numRestarts) {
        return shotgunHillClimbing(numIterations, numRestarts);
    }

    double calculateTourLength(const std::vector<int>& tour) const {
        double length = 0.0;
        for (size_t i = 0; i < tour.size(); ++i) {
            length += adjacencyMatrix_[tour[i]][tour[(i + 1) % tour.size()]];
        }
        return length;
    }

private:
    std::vector<std::vector<double>> adjacencyMatrix_;
    std::mt19937 gen_;

    void loadAdjacencyMatrix() {
        std::string line;
        while (std::getline(std::cin, line)) {
            adjacencyMatrix_.push_back(parseCSVLine(line));
        }

        if (adjacencyMatrix_.empty() || !isSquareMatrix()) {
            throw std::runtime_error("Invalid adjacency matrix in CSV file");
        }
    }

    std::vector<double> parseCSVLine(const std::string& line) {
        std::vector<double> row;
        std::stringstream ss(line);
        std::string cell;
        while (std::getline(ss, cell, ',')) {
            row.push_back(std::stod(cell));
        }
        return row;
    }

    bool isSquareMatrix() const {
        size_t size = adjacencyMatrix_.size();
        return std::all_of(adjacencyMatrix_.begin(), adjacencyMatrix_.end(),
                           [size](const auto& row) { return row.size() == size; });
    }

    std::vector<int> generateRandomTour() {
        std::vector<int> tour(adjacencyMatrix_.size());
        std::iota(tour.begin(), tour.end(), 0);
        std::shuffle(tour.begin() + 1, tour.end(), gen_);
        return tour;
    }

    std::vector<int> twoOptSwap(const std::vector<int>& tour, int i, int j) {
        std::vector<int> newTour = tour;
        std::reverse(newTour.begin() + i, newTour.begin() + j + 1);
        return newTour;
    }

    std::vector<int> shotgunHillClimbing(int numIterations, int numRestarts) {
        std::vector<int> bestTour;
        double bestLength = std::numeric_limits<double>::max();

        for (int restart = 0; restart < numRestarts; ++restart) {
            auto [currentTour, currentLength] = hillClimb(numIterations);

            if (currentLength < bestLength) {
                bestTour = std::move(currentTour);
                bestLength = currentLength;
            }
        }

        return bestTour;
    }

    std::pair<std::vector<int>, double> hillClimb(int numIterations) {
        std::vector<int> currentTour = generateRandomTour();
        double currentLength = calculateTourLength(currentTour);

        for (int iter = 0; iter < numIterations; ++iter) {
            bool improvement = false;
            for (size_t i = 1; i < currentTour.size() - 1; ++i) {
                for (size_t j = i + 1; j < currentTour.size(); ++j) {
                    std::vector<int> newTour = twoOptSwap(currentTour, i, j);
                    double newLength = calculateTourLength(newTour);
                    if (newLength < currentLength) {
                        currentTour = std::move(newTour);
                        currentLength = newLength;
                        improvement = true;
                        break;
                    }
                }
                if (improvement) break;
            }
            if (!improvement) break;
        }

        return {currentTour, currentLength};
    }
};

int main(int argc, char* argv[]) {
    try {
        std::string line;
        std::getline(std::cin, line);
        std::stringstream myStream(line);
        std::getline(myStream, line, ' ');
        int numIterations = std::stoi(line);
        std::getline(myStream, line, ' ');
        int numRestarts = std::stoi(line);
        std::getline(myStream, line, ' ');
        unsigned seed = std::stoi(line);

        TSPSolver solver(seed);
        std::vector<int> bestTour = solver.solveTSP(numIterations, numRestarts);

        std::cout << "Best tour found: ";
        for (int vertex : bestTour) {
            std::cout << vertex << " ";
        }
        std::cout << "\nTour length: " << solver.calculateTourLength(bestTour) << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
