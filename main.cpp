#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include "Map.hpp"
#include "Solver.hpp"

#ifdef _WIN32
#include <direct.h>
#define GetCurrentDir _getcwd
#else
#include <unistd.h>
#define GetCurrentDir getcwd
#endif

std::string getCurrentWorkingDir() {
    char buff[FILENAME_MAX];
    GetCurrentDir(buff, FILENAME_MAX);
    std::string current_working_dir(buff);
    return current_working_dir;
}

int main() {
    std::string filename;
    std::cout << ">> Masukan file input :\n";
    std::cin >> filename;

    Map map;
    if (!map.load(filename)) {
        std::cout << "Gagal membaca file.\n";
        return 1;
    }

    std::string algo;
    std::cout << ">> Algoritma apa yang anda pilih? (UCS/GBFS/A*)\n";
    std::cin >> algo;

    std::string heuristic = "H1";
    if (algo == "GBFS" || algo == "A*") {
        std::cout << ">> Heuristic apa yang anda pilih? (H1/H2/H3)\n";
        std::cin >> heuristic;
    }

    int iterations = 0;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    Node result = solve(map, algo, heuristic, iterations);
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    if (result.path == "" && (result.state.r == -1 || result.g == -1)) {
        std::cout << "\nSolusi tidak ditemukan!\n";
        return 0;
    }

    std::cout << "\nSolusi Yang Ditemukan\n";
    std::cout << result.path << "\n";
    std::cout << "Cost dari Solusi: " << result.g << "\n\n";

    std::vector<State> path_states;
    path_states.push_back({map.start.r, map.start.c, 0});

    std::cout << "[Print Initial Map]\n";
    map.printMap(path_states.back());
    std::cout << "\n";

    State curr = path_states.back();
    for (int i = 0; i < result.path.size(); ++i) {
        char move = result.path[i];
        std::cout << "Step " << (i + 1) << ": " << move << "\n";
        curr = performSlide(map, curr, move).final_state;
        path_states.push_back(curr);
        std::cout << "[Print Map at Step " << (i + 1) << "]\n";
        map.printMap(curr);
        std::cout << "\n";
    }

    std::cout << ">> Waktu eksekusi: " << duration << " ms\n";
    std::cout << ">> Banyak iterasi yang dilakukan: " << iterations << " iterasi\n";
    
    std::string ans;
    std::cout << ">> Apakah Anda ingin melakukan playback? (Ya/Tidak) :\n";
    std::cin >> ans;

    if (ans == "Ya") {
        int step;
        std::cout << ">> Pada step berapa anda ingin melakukan playback :\n";
        std::cin >> step;

        if (step >= 0 && step < path_states.size()) {
            if (step == 0) {
                std::cout << "[Print Initial Map]\n";
                map.printMap(path_states[0]);
            } else {
                std::cout << "Step " << step << ": " << result.path[step - 1] << "\n";
                std::cout << "[Print Map at Step " << step << "]\n";
                map.printMap(path_states[step]);
            }
            std::cout << "\n";
        } else {
            std::cout << "Step tidak valid.\n\n";
        }
    }

    std::cout << ">> Apakah Anda ingin menyimpan solusi? (Ya/Tidak) :\n";
    std::cin >> ans;

    if (ans == "Ya") {
        std::string out_filename = "solusi.txt";
        std::ofstream out(out_filename);
        if (out) {
            out << "Solusi Yang Ditemukan\n";
            out << result.path << "\n";
            out << "Cost dari Solusi: " << result.g << "\n";
            out.close();
            
            std::string cwd = getCurrentWorkingDir();
#ifdef _WIN32
            std::string full_path = cwd + "\\" + out_filename;
#else
            std::string full_path = cwd + "/" + out_filename;
#endif
            std::cout << ">> Solusi disimpan pada " << full_path << "\n";
        }
    }

    return 0;
}
