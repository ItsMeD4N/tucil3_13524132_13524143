#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <filesystem>
#include <limits>
#include <conio.h>
#include <iomanip>
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
        cout << "Gagal membaca file. " << map.last_error << "\n";
        return 1;
    }

    string algo;
    while (true) {
        cout << ">> Algoritma apa yang anda pilih? (UCS/BFS/GBFS/A*/IDA*)";
        cin >> algo;
        if (algo == "UCS" || algo == "BFS" || algo == "GBFS" || algo == "A*" || algo == "IDA*") {
            break;
        }
        cout << "Pilihan algoritma tidak valid.\n";
    }

    string heuristic = "H1";
    if (algo == "GBFS" || algo == "A*" || algo == "IDA*") {
        while (true) {
            cout << ">> Heuristic apa yang anda pilih? (H1/H2/H3)";
            cin >> heuristic;
            if (heuristic == "H1" || heuristic == "H2" || heuristic == "H3") {
                break;
            }
            cout << "Pilihan heuristic tidak valid.\n";
        }
    }

    int iterations = 0;
    vector<IterationLog> iteration_logs;
    
    auto start_time = chrono::high_resolution_clock::now();
    Node result = solve(map, algo, heuristic, iterations, &iteration_logs);
    auto end_time = chrono::high_resolution_clock::now();
    const double duration = chrono::duration<double, std::milli>(end_time - start_time).count();

    bool found_solution = !(result.path == "" && (result.state.r == -1 || result.g == -1));
    if (!found_solution) {
        cout << "\nSolusi tidak ditemukan!\n\n";
    } else {
        cout << "\nSolusi Yang Ditemukan\n";
        cout << result.path << "\n";
        cout << "Cost dari Solusi: " << result.g << "\n\n";
    }

    vector<State> path_states;
    path_states.push_back({map.start.r, map.start.c, 0});

    if (found_solution) {
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
    }

    cout << ">> Waktu eksekusi: " << fixed << setprecision(5) << duration << " ms\n";
    cout << ">> Banyak iterasi yang dilakukan: " << iterations << " iterasi\n";

    string iteration_filename = "iterasi_pencarian.txt";
    ofstream iter_out(iteration_filename);
    if (iter_out) {
        iter_out << "Algoritma: " << algo << "\n";
        if (algo == "GBFS" || algo == "A*" || algo == "IDA*") {
            iter_out << "Heuristic: " << heuristic << "\n";
        }
        iter_out << "Total iterasi: " << iterations << "\n\n";

        for (const IterationLog& log : iteration_logs) {
            iter_out << "Iterasi " << log.iteration << "\n";
            iter_out << "Posisi: (" << log.state.r << ", " << log.state.c << ")\n";
            iter_out << "Mask waypoint: " << log.state.mask << "\n";
            iter_out << "Path: " << log.path << "\n";
            iter_out << "g: " << log.g << ", h: " << log.h << "\n";
            map.printMap(iter_out, log.state);
            iter_out << "\n";
        }
        iter_out.close();

        filesystem::path iter_path = filesystem::current_path() / iteration_filename;
        cout << ">> Konfigurasi state per iterasi disimpan pada " << iter_path.string() << "\n";
    } else {
        cout << ">> Gagal menyimpan konfigurasi iterasi.\n";
    }
    
    string ans;
    if (found_solution) {
        cout << ">> Apakah Anda ingin melakukan playback? (Ya/Tidak) :";
        cin >> ans;
    } else {
        ans = "Tidak";
    }

    if (ans == "Ya" && found_solution) {
        int step = 0;
        while (true) {
            cout << "\n================ PLAYBACK ================\n";
            if (step == 0) {
                cout << "[Print Initial Map]\n";
                map.printMap(path_states[0]);
            } else {
                cout << "Step " << step << ": " << result.path[step - 1] << "\n";
                cout << "[Print Map at Step " << step << "]\n";
                map.printMap(path_states[step]);
            }
            cout << "\nKontrol:\n";
            cout << "- Arrow kanan: step berikutnya\n";
            cout << "- Arrow kiri : step sebelumnya\n";
            cout << "- ESC        : lompat ke step tertentu / keluar playback\n";

            int key = _getch();
            if (key == 224) {
                int arrow = _getch();
                if (arrow == 77 && step < static_cast<int>(path_states.size()) - 1) {
                    step++;
                } else if (arrow == 75 && step > 0) {
                    step--;
                }
            } else if (key == 27) {
                cout << "\nMasukkan step tujuan (0.." << (path_states.size() - 1)
                     << ", isi -1 untuk keluar): ";
                int jump_step;
                if (!(cin >> jump_step)) {
                    cin.clear();
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    cout << "Input step tidak valid.\n";
                    continue;
                }

                if (jump_step == -1) {
                    break;
                }

                if (jump_step >= 0 && jump_step < static_cast<int>(path_states.size())) {
                    step = jump_step;
                } else {
                    cout << "Step tidak valid.\n";
                }
            }
        }
    }

    if (found_solution) {
        cout << ">> Apakah Anda ingin menyimpan solusi? (Ya/Tidak) :";
        cin >> ans;
    } else {
        ans = "Tidak";
    }

    if (ans == "Ya" && found_solution) {
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