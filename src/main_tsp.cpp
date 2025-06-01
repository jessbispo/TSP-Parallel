#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <limits>
#include <fstream>
#include <sstream>
#include <string>

/**
 * @class TSPSolver
 * @brief Solves the Traveling Salesman Problem using Shotgun Hill Climbing with 2-opt optimization
 * 
 * This class implements a heuristic approach to solve TSP by:
 * 1. Reading an adjacency matrix from CSV input
 * 2. Using multiple random restarts (shotgun approach)
 * 3. Applying hill climbing with 2-opt local search
 * 4. Finding the best tour among all restarts
 */
class TSPSolver {
public:
    /**
     * @brief Constructor that initializes the random number generator and loads the adjacency matrix
     * @param seed Random seed for reproducible results
     */
    TSPSolver(unsigned seed)
        : gen_(seed) {
        loadAdjacencyMatrix();
    }

    /**
     * @brief Main solver function that finds the best TSP tour using shotgun hill climbing
     * @param numIterations Maximum number of iterations per hill climbing run
     * @param numRestarts Number of random restarts to perform
     * @return Vector of city indices representing the best tour found
     */
    std::vector<int> solveTSP(int numIterations, int numRestarts) {
        return shotgunHillClimbing(numIterations, numRestarts);
    }

    /**
     * @brief Calculates the total length/cost of a given tour
     * @param tour Vector of city indices representing the tour path
     * @return Total distance of the tour (sum of edge weights)
     */
    double calculateTourLength(const std::vector<int>& tour) const {
        double length = 0.0;
        // Sum distances between consecutive cities, including return to start
        for (size_t i = 0; i < tour.size(); ++i) {
            length += adjacencyMatrix_[tour[i]][tour[(i + 1) % tour.size()]];
        }
        return length;
    }

private:
    std::vector<std::vector<double>> adjacencyMatrix_; // Distance matrix between cities
    std::mt19937 gen_; // Random number generator for reproducible randomness

    /**
     * @brief Loads the adjacency matrix from standard input (CSV format)
     * @throws std::runtime_error if the matrix is invalid or not square
     * 
     * Expected input format:
     * - First line: "numIterations numRestarts seed"
     * - Following lines: CSV adjacency matrix where entry [i][j] is distance from city i to city j
     */
    void loadAdjacencyMatrix() {
        std::string line;
        while (std::getline(std::cin, line)) {
            adjacencyMatrix_.push_back(parseCSVLine(line));
        }

        // Validate that we have a valid square matrix
        if (adjacencyMatrix_.empty() || !isSquareMatrix()) {
            throw std::runtime_error("Invalid adjacency matrix in CSV file");
        }
    }

    /**
     * @brief Parses a single CSV line into a vector of doubles
     * @param line Comma-separated string of numbers
     * @return Vector of parsed double values
     */
    std::vector<double> parseCSVLine(const std::string& line) {
        std::vector<double> row;
        std::stringstream ss(line);
        std::string cell;
        // Split by comma and convert each cell to double
        while (std::getline(ss, cell, ',')) {
            row.push_back(std::stod(cell));
        }
        return row;
    }

    /**
     * @brief Validates that the adjacency matrix is square (n×n)
     * @return True if matrix is square, false otherwise
     */
    bool isSquareMatrix() const {
        size_t size = adjacencyMatrix_.size();
        return std::all_of(adjacencyMatrix_.begin(), adjacencyMatrix_.end(),
                           [size](const auto& row) { return row.size() == size; });
    }

    /**
     * @brief Generates a random tour starting from city 0
     * @return Random permutation of cities with city 0 fixed at the start
     * 
     * Note: City 0 is kept fixed at the beginning since TSP tours are cyclic
     * and we can always rotate a tour to start from any city
     */
    std::vector<int> generateRandomTour() {
        std::vector<int> tour(adjacencyMatrix_.size());
        std::iota(tour.begin(), tour.end(), 0); // Fill with 0, 1, 2, ..., n-1
        std::shuffle(tour.begin() + 1, tour.end(), gen_); // Shuffle all except first city
        return tour;
    }

    /**
     * @brief Performs a 2-opt swap on a tour segment
     * @param tour Original tour
     * @param i Start index of the segment to reverse
     * @param j End index of the segment to reverse
     * @return New tour with the segment [i, j] reversed
     * 
     * 2-opt is a local search technique that removes two edges and reconnects
     * the tour in a different way, potentially reducing the total distance
     */
    std::vector<int> twoOptSwap(const std::vector<int>& tour, int i, int j) {
        std::vector<int> newTour = tour;
        std::reverse(newTour.begin() + i, newTour.begin() + j + 1); // Reverse segment [i, j]
        return newTour;
    }

    /**
     * @brief Implements shotgun hill climbing - multiple hill climbing runs with random restarts
     * @param numIterations Maximum iterations per hill climb
     * @param numRestarts Number of independent hill climbing runs
     * @return Best tour found across all restarts
     * 
     * Shotgun approach helps escape local optima by trying multiple starting points
     */
    std::vector<int> shotgunHillClimbing(int numIterations, int numRestarts) {
        std::vector<int> bestTour;
        double bestLength = std::numeric_limits<double>::max(); // Initialize to infinity

        // Perform multiple independent hill climbing runs
        for (int restart = 0; restart < numRestarts; ++restart) {
            auto [currentTour, currentLength] = hillClimb(numIterations);

            // Keep track of the globally best solution
            if (currentLength < bestLength) {
                bestTour = std::move(currentTour);
                bestLength = currentLength;
            }
        }

        return bestTour;
    }

    /**
     * @brief Performs hill climbing optimization using 2-opt moves
     * @param numIterations Maximum number of iterations to perform
     * @return Pair containing the best tour found and its length
     * 
     * Hill climbing algorithm:
     * 1. Start with a random tour
     * 2. Try all possible 2-opt swaps
     * 3. Accept the first improvement found
     * 4. Repeat until no improvement is found or max iterations reached
     */
    std::pair<std::vector<int>, double> hillClimb(int numIterations) {
        std::vector<int> currentTour = generateRandomTour();
        double currentLength = calculateTourLength(currentTour);

        for (int iter = 0; iter < numIterations; ++iter) {
            bool improvement = false;
            
            // Try all possible 2-opt swaps
            for (size_t i = 1; i < currentTour.size() - 1; ++i) {
                for (size_t j = i + 1; j < currentTour.size(); ++j) {
                    std::vector<int> newTour = twoOptSwap(currentTour, i, j);
                    double newLength = calculateTourLength(newTour);
                    
                    // Accept first improvement found (greedy hill climbing)
                    if (newLength < currentLength) {
                        currentTour = std::move(newTour);
                        currentLength = newLength;
                        improvement = true;
                        break; // Exit inner loop on first improvement
                    }
                }
                if (improvement) break; // Exit outer loop on improvement
            }
            
            // If no improvement found, we've reached a local optimum
            if (!improvement) break;
        }

        return {currentTour, currentLength};
    }
};

/**
 * @brief Main function that parses command line arguments and runs the TSP solver
 * 
 * Expected input format:
 * - First line: "numIterations numRestarts seed"
 * - Following lines: CSV adjacency matrix
 * 
 * Output:
 * - Best tour found (sequence of city indices)
 * - Total tour length
 */
int main(int argc, char* argv[]) {
    try {
        // Parse the first line containing algorithm parameters
        std::string line;
        std::getline(std::cin, line);
        std::stringstream myStream(line);
        
        // Extract numIterations, numRestarts, and seed
        std::getline(myStream, line, ' ');
        int numIterations = std::stoi(line);
        std::getline(myStream, line, ' ');
        int numRestarts = std::stoi(line);
        std::getline(myStream, line, ' ');
        unsigned seed = std::stoi(line);

        // Create solver and find best tour
        TSPSolver solver(seed);
        std::vector<int> bestTour = solver.solveTSP(numIterations, numRestarts);

        // Output results
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