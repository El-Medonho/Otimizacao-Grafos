#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <utility>
#include <chrono>
#include <cmath>
#include <random>
#include <algorithm>
#include <bitset>
#include <cassert>

using namespace std;

// --- Variáveis Globais (Dados do Problema) ---
int itens, quant_conj, capacidade;
vector<int> lucro, peso;
vector<vector<int>> conju;
vector<pair<int, int>> inf_conj;

mt19937_64 rng((int)chrono::steady_clock::now().time_since_epoch().count());

// --- Parâmetros da Meta-heurística ---
const double tempoLimite = 2.0;

// Função para calcular o valor total de uma solução
int calculate_solution_value(const bitset<1000>& solution, int& out_somaPeso, vector<int>& itemsPorConj) {
    out_somaPeso = 0;
    int current_valor = 0;
    int current_penalidade = 0;
    fill(itemsPorConj.begin(), itemsPorConj.end(), 0);

    for (int i = 0; i < itens; ++i) {
        if (solution[i]) {
            out_somaPeso += peso[i];
            current_valor += lucro[i];
            for (int cj : conju[i]) {
                itemsPorConj[cj]++;
            }
        }
    }
    if (out_somaPeso > capacidade) return -2e9;

    for (int j = 0; j < quant_conj; ++j) {
        if (itemsPorConj[j] > inf_conj[j].first) {
            current_penalidade += (itemsPorConj[j] - inf_conj[j].first) * inf_conj[j].second;
        }
    }
    return current_valor - current_penalidade;
}
 
void FastLocalSearch(bitset<1000>& solution, int& solutionValue, int& solutionPeso, vector<int>& itemsPorConj) {
    bool improvement_found = true;
    while (improvement_found) {
        improvement_found = false;
        for (int itemFlip = 0; itemFlip < itens; ++itemFlip) {
            int delta = 0;
            if (solution[itemFlip]) {
                delta = -lucro[itemFlip];
                for (int cj : conju[itemFlip]) {
                    if (itemsPorConj[cj] > inf_conj[cj].first) {
                        delta += inf_conj[cj].second;
                    }
                }
            } else {
                if (solutionPeso + peso[itemFlip] > capacidade) continue;
                delta = lucro[itemFlip];
                for (int cj : conju[itemFlip]) {
                    if (itemsPorConj[cj] + 1 > inf_conj[cj].first) {
                        delta -= inf_conj[cj].second;
                    }
                }
            }
            if (delta > 0) {
                solution.flip(itemFlip);
                solutionValue += delta;
                if (solution[itemFlip]) {
                    solutionPeso += peso[itemFlip];
                    for (int cj : conju[itemFlip]) itemsPorConj[cj]++;
                } else {
                    solutionPeso -= peso[itemFlip];
                    for (int cj : conju[itemFlip]) itemsPorConj[cj]--;
                }
                improvement_found = true;
                break;
            }
        }
    }
}
 
int GRASP(const string& convergence_filepath) {
    uniform_real_distribution<double> prob_dist(0.0, 1.0);

    vector<pair<double, int>> candidates(itens);
    for (int i = 0; i < itens; i++) {
        candidates[i] = { (peso[i] == 0 ? 1e12 + lucro[i] : (double)lucro[i] / peso[i]), i };
    }
    sort(candidates.rbegin(), candidates.rend());

    int bestValue = -2e9;
    int iterationsWithoutImproving = 0;
    
    vector<int> itemsPorConj_buffer(quant_conj, 0);
 
    vector<pair<double, int>> convergence_data;

    auto start_time = chrono::high_resolution_clock::now();

    while (true) {
        auto current_time = chrono::high_resolution_clock::now();
        double elapsed_time = chrono::duration<double>(current_time - start_time).count();
        if (elapsed_time > tempoLimite || iterationsWithoutImproving > 300) {
            break;
        }

        bitset<1000> currentSolution;
        int somaPeso = 0;
        double prob_alpha = 0.85; 
        for (auto const& [ratio, currItem] : candidates) {
            if (prob_dist(rng) < prob_alpha) {
                if (somaPeso + peso[currItem] <= capacidade) {
                    currentSolution[currItem] = 1;
                    somaPeso += peso[currItem];
                }
            }
            prob_alpha *= 0.97;
        }
        
        int currentPeso;
        int currentValue = calculate_solution_value(currentSolution, currentPeso, itemsPorConj_buffer);
        
        FastLocalSearch(currentSolution, currentValue, currentPeso, itemsPorConj_buffer);
        
        if (currentValue > bestValue) {
            bestValue = currentValue;
            iterationsWithoutImproving = 0; 
            convergence_data.push_back({elapsed_time, bestValue});
        } else {
            iterationsWithoutImproving++;
        }
    }
 
    ofstream convergence_file(convergence_filepath);
    if(convergence_file.is_open()){ 
        if (convergence_data.empty() || convergence_data.back().second < bestValue) {
             convergence_data.push_back({tempoLimite, bestValue});
        }
        for(const auto& point : convergence_data){
            convergence_file << point.first << " " << point.second << "\n";
        }
        convergence_file.close();
    }

    return bestValue;
}
 
int main(int argc, char* argv[]) {
    if (argc != 4) {
        cerr << "Uso: " << argv[0] << " <arquivo_entrada> <arquivo_saida_final> <arquivo_saida_convergencia>" << endl;
        return 1;
    }
    string dir_entrada = argv[1];
    string dir_saida_final = argv[2];
    string dir_saida_convergencia = argv[3];

    ifstream arquivo(dir_entrada);
    if (!arquivo.is_open()) {
        cout << "Erro ao abrir o arquivo: " << dir_entrada << endl;
        return 1;
    }

    arquivo >> itens >> quant_conj >> capacidade;
    lucro.assign(itens, 0);
    peso.assign(itens, 0);

    for (int j = 0; j < itens; j++) arquivo >> lucro[j];
    for (int j = 0; j < itens; j++) arquivo >> peso[j];

    conju.assign(itens, vector<int>());
    inf_conj.assign(quant_conj, make_pair(0, 0));

    for (int j = 0; j < quant_conj; j++) {
        int lim_conj, penalidade_conj, itens_conj;
        arquivo >> lim_conj >> penalidade_conj >> itens_conj;
        inf_conj[j] = {lim_conj, penalidade_conj};
        for (int k = 0; k < itens_conj; k++) {
            int item;
            arquivo >> item;
            conju[item].push_back(j);
        }
    }
    arquivo.close();

    auto start = chrono::high_resolution_clock::now(); 
    int sol = GRASP(dir_saida_convergencia);
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> time = end - start;
    double execution_time = time.count();
 
    ofstream saida_arquivo(dir_saida_final, ios::app);
    if (!saida_arquivo.is_open()) {
        cout << "Erro ao abrir " << dir_saida_final << " para escrita.\n";
        return 1;
    }
    saida_arquivo << sol << " " << execution_time << '\n';
    saida_arquivo.close();

    return 0;
}