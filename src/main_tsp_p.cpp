#include <iostream>
#include <vector>
#include <random>
#include <algorithm>
#include <limits>
#include <fstream>
#include <sstream>
#include <string>
#include <omp.h>  // OpenMP for parallelization

/**
 * @class TSPSolver
 * @brief Parallel implementation of TSP solver using Shotgun Hill Climbing algorithm
 * 
 * This class implements a parallel version of the Traveling Salesman Problem solver
 * using the Shotgun Hill Climbing heuristic with 2-opt local search optimization.
 * The parallelization is achieved through OpenMP, distributing multiple restarts
 * across available CPU threads to explore the solution space more efficiently.
 */
class TSPSolver {
public:
    /**
     * @brief Constructor that initializes the solver with a base seed
     * @param seed Base seed for random number generation across threads
     */
    TSPSolver(unsigned seed)
        : baseSeed_(seed) {
        loadAdjacencyMatrix(); // Load distance matrix from stdin
    }

    /**
     * @brief Main solving method that orchestrates the parallel TSP solving process
     * @param numIterations Maximum iterations per hill climbing run
     * @param numRestarts Total number of random restarts to perform
     * @return Best tour found as a vector of city indices
     */
    std::vector<int> solveTSP(int numIterations, int numRestarts) {
        return shotgunHillClimbingParallel(numIterations, numRestarts);
    }

    /**
     * @brief Calculates the total length/cost of a given tour
     * @param tour Vector representing the sequence of cities to visit
     * @return Total distance of the tour
     */
    double calculateTourLength(const std::vector<int>& tour) const {
        double length = 0.0;
        // Sum distances between consecutive cities + return to start
        for (size_t i = 0; i < tour.size(); ++i) {
            length += adjacencyMatrix_[tour[i]][tour[(i + 1) % tour.size()]];
        }
        return length;
    }

private:
    std::vector<std::vector<double>> adjacencyMatrix_; ///< Distance matrix between cities
    unsigned baseSeed_; ///< Base seed for generating unique seeds per thread

    /**
     * @brief Loads the adjacency matrix from standard input (CSV format)
     * @throws std::runtime_error if matrix is invalid or not square
     */
    void loadAdjacencyMatrix() {
        std::string line;
        // Read CSV lines from stdin
        while (std::getline(std::cin, line)) {
            adjacencyMatrix_.push_back(parseCSVLine(line));
        }

        // Validate matrix integrity
        if (adjacencyMatrix_.empty() || !isSquareMatrix()) {
            throw std::runtime_error("Invalid adjacency matrix in CSV file");
        }
    }

    /**
     * @brief Parses a CSV line into a vector of doubles
     * @param line Comma-separated string of numbers
     * @return Vector of parsed double values
     */
    std::vector<double> parseCSVLine(const std::string& line) {
        std::vector<double> row;
        std::stringstream ss(line);
        std::string cell;
        // Split by comma and convert to double
        while (std::getline(ss, cell, ',')) {
            row.push_back(std::stod(cell));
        }
        return row;
    }

    /**
     * @brief Validates that the loaded matrix is square (n×n)
     * @return True if matrix is square, false otherwise
     */
    bool isSquareMatrix() const {
        size_t size = adjacencyMatrix_.size();
        return std::all_of(adjacencyMatrix_.begin(), adjacencyMatrix_.end(),
                           [size](const auto& row) { return row.size() == size; });
    }

    /**
     * @brief Generates a random permutation tour starting from city 0
     * @param gen Random number generator for this thread
     * @return Random tour as vector of city indices
     */
    std::vector<int> generateRandomTour(std::mt19937& gen) {
        std::vector<int> tour(adjacencyMatrix_.size());
        std::iota(tour.begin(), tour.end(), 0); // Fill with 0,1,2,...,n-1
        std::shuffle(tour.begin() + 1, tour.end(), gen); // Keep city 0 fixed, shuffle rest
        return tour;
    }

    /**
     * @brief Performs 2-opt swap operation on a tour
     * @param tour Original tour to modify
     * @param i Start index of the segment to reverse
     * @param j End index of the segment to reverse
     * @return New tour with reversed segment between i and j
     * 
     * 2-opt removes two edges and reconnects the tour in a different way,
     * effectively reversing a segment of the tour to eliminate edge crossings
     */
    std::vector<int> twoOptSwap(const std::vector<int>& tour, int i, int j) {
        std::vector<int> newTour = tour;
        std::reverse(newTour.begin() + i, newTour.begin() + j + 1); // Reverse segment [i,j]
        return newTour;
    }

    /**
     * @brief Parallel implementation of shotgun hill climbing using OpenMP
     * @param numIterations Maximum iterations per hill climb
     * @param numRestarts Total number of random restarts
     * @return Best tour found across all threads
     * 
     * This method distributes the restarts across multiple threads, where each
     * thread runs independent hill climbing instances with different random seeds.
     * The best solution found by any thread is returned.
     */
    std::vector<int> shotgunHillClimbingParallel(int numIterations, int numRestarts) {
        std::vector<int> bestTour;
        double bestLength = std::numeric_limits<double>::max();

        // Parallel region - each thread executes this block
        #pragma omp parallel
        {
            // Each thread gets unique random generator with different seed
            int threadId = omp_get_thread_num();
            std::mt19937 localGen(baseSeed_ + threadId);
            
            // Thread-local variables to avoid race conditions
            std::vector<int> localBestTour;
            double localBestLength = std::numeric_limits<double>::max();

            // Distribute restarts among threads using OpenMP work-sharing
            #pragma omp for
            for (int restart = 0; restart < numRestarts; ++restart) {
                // Run hill climbing from random starting point
                auto [currentTour, currentLength] = hillClimb(numIterations, localGen);

                // Update thread-local best if improvement found
                if (currentLength < localBestLength) {
                    localBestTour = currentTour;
                    localBestLength = currentLength;
                }
            }

            // Critical section to safely compare results from all threads
            #pragma omp critical
            {
                if (localBestLength < bestLength) {
                    bestTour = std::move(localBestTour); // Move semantics for efficiency
                    bestLength = localBestLength;
                }
            }
        } // End of parallel region

        return bestTour;
    }

    /**
     * @brief Single hill climbing run with 2-opt local search
     * @param numIterations Maximum iterations before giving up
     * @param gen Random number generator for this instance
     * @return Pair of (best tour found, tour length)
     * 
     * Hill climbing explores the neighborhood of the current solution using
     * 2-opt moves, always accepting improvements (greedy local search).
     * Stops when no improvement is found or max iterations reached.
     */
    std::pair<std::vector<int>, double> hillClimb(int numIterations, std::mt19937& gen) {
        std::vector<int> currentTour = generateRandomTour(gen); // Start with random tour
        double currentLength = calculateTourLength(currentTour);

        // Hill climbing main loop
        for (int iter = 0; iter < numIterations; ++iter) {
            bool improvement = false;
            
            // Try all possible 2-opt swaps
            for (size_t i = 1; i < currentTour.size() - 1; ++i) {
                for (size_t j = i + 1; j < currentTour.size(); ++j) {
                    std::vector<int> newTour = twoOptSwap(currentTour, i, j);
                    double newLength = calculateTourLength(newTour);
                    
                    // Accept first improvement found (first-improvement strategy)
                    if (newLength < currentLength) {
                        currentTour = std::move(newTour);
                        currentLength = newLength;
                        improvement = true;
                        break; // Exit inner loop
                    }
                }
                if (improvement) break; // Exit outer loop
            }
            
            // If no improvement found, we've reached local optimum
            if (!improvement) break;
        }

        return {currentTour, currentLength};
    }
};

/**
 * @brief Main function that handles input parsing and orchestrates the TSP solving
 * 
 * Expected input format:
 * Line 1: numIterations numRestarts seed
 * Following lines: CSV adjacency matrix
 */
int main(int argc, char* argv[]) {
    try {
        // Parse command line parameters from first line of input
        std::string line;
        std::getline(std::cin, line);
        std::stringstream myStream(line);
        
        std::getline(myStream, line, ' ');
        int numIterations = std::stoi(line);  // Max iterations per hill climb
        
        std::getline(myStream, line, ' ');
        int numRestarts = std::stoi(line);    // Total number of restarts
        
        std::getline(myStream, line, ' ');
        unsigned seed = std::stoi(line);      // Base random seed

        // Display parallelization info
        std::cout << "Using " << omp_get_max_threads() << " threads" << std::endl;

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