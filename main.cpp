#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <filesystem>
#include "Map.hpp"
#include "Solver.hpp"

using namespace std;

int main() {
    string filename;
    cout << ">> Masukan file input :";
    cin >> ws;
    getline(cin, filename);

    Map map;
    if (!map.load(filename)) {
        cout << "Gagal membaca file.";
        return 1;
    }

    string algo;
    cout << ">> Algoritma apa yang anda pilih? (UCS/GBFS/A*)";
    cin >> algo;

    string heuristic = "H1";
    if (algo == "GBFS" || algo == "A*") {
        cout << ">> Heuristic apa yang anda pilih? (H1/H2/H3)";
        cin >> heuristic;
    }

    int iterations = 0;
    
    auto start_time = chrono::high_resolution_clock::now();
    Node result = solve(map, algo, heuristic, iterations);
    auto end_time = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end_time - start_time).count();

    if (result.path == "" && (result.state.r == -1 || result.g == -1)) {
        cout << "\nSolusi tidak ditemukan!\n";
        return 0;
    }

    cout << "\nSolusi Yang Ditemukan\n";
    cout << result.path << "\n";
    cout << "Cost dari Solusi: " << result.g << "\n\n";

    vector<State> path_states;
    path_states.push_back({map.start.r, map.start.c, 0});

    cout << "[Print Initial Map]\n";
    map.printMap(path_states.back());
    cout << "\n";

    State curr = path_states.back();
    for (int i = 0; i < result.path.size(); ++i) {
        char move = result.path[i];
        cout << "Step " << (i + 1) << ": " << move << "\n";
        curr = performSlide(map, curr, move).final_state;
        path_states.push_back(curr);
        cout << "[Print Map at Step " << (i + 1) << "]\n";
        map.printMap(curr);
        cout << "\n";
    }

    cout << ">> Waktu eksekusi: " << duration << " ms\n";
    cout << ">> Banyak iterasi yang dilakukan: " << iterations << " iterasi\n";
    
    string ans;
    cout << ">> Apakah Anda ingin melakukan playback? (Ya/Tidak) :";
    cin >> ans;

    if (ans == "Ya") {
        int step;
        cout << ">> Pada step berapa anda ingin melakukan playback :";
        cin >> step;

        if (step >= 0 && step < path_states.size()) {
            if (step == 0) {
                cout << "[Print Initial Map]\n";
                map.printMap(path_states[0]);
            } else {
                cout << "Step " << step << ": " << result.path[step - 1] << "\n";
                cout << "[Print Map at Step " << step << "]\n";
                map.printMap(path_states[step]);
            }
            cout << "\n";
        } else {
            cout << "Step tidak valid.\n\n";
        }
    }

    cout << ">> Apakah Anda ingin menyimpan solusi? (Ya/Tidak) :";
    cin >> ans;

    if (ans == "Ya") {
        string out_filename = "solusi.txt";
        ofstream out(out_filename);
        if (out) {
            out << "Solusi Yang Ditemukan\n";
            out << result.path << "\n";
            out << "Cost dari Solusi: " << result.g << "\n";
            out.close();
            
            filesystem::path cwd = filesystem::current_path();
            filesystem::path full_path = cwd / out_filename;
            
            cout << ">> Solusi disimpan pada " << full_path.string() << "\n";
        } else {
            cout << ">> Gagal menyimpan solusi.\n";
        }
    }

    return 0;
}