#include "bits/stdc++.h"
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

typedef long long ll;

// --- Variáveis Globais (Dados do Problema) ---
int itens, quant_conj, capacidade;
vector<int> lucro, peso;
vector<vector<int>> conju;
vector<pair<int, int>> inf_conj;

mt19937_64 rng((int)chrono::steady_clock::now().time_since_epoch().count());

// --- Parâmetros da Meta-heurística ---
const double tempoLimite = 2.0;
const int TABU_TENURE = 100;

// Função para calcular o valor total de uma solução.
int calculate_initial_state(const bitset<1000>& solution, int& out_somaPeso, vector<int>& itemsPorConj) {
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
 
int TABU_Optimized(const string& convergence_filepath) {
    // --- Estado da Solução ---
    bitset<1000> currentSolution;
    int somaPeso = 0;

    // --- Inicialização (Gulosa) ---
    vector<pair<double, int>> candidates(itens);
    for (int i = 0; i < itens; i++) {
        candidates[i] = { (peso[i] == 0 ? 1e12 + lucro[i] : (double)lucro[i] / peso[i]), i };
    }
    sort(candidates.rbegin(), candidates.rend());
    
    for (auto const& [ratio, currItem] : candidates) {
        if (somaPeso + peso[currItem] <= capacidade) {
            currentSolution[currItem] = 1;
            somaPeso += peso[currItem];
        }
    }

    // --- Variáveis da Busca Tabu ---
    vector<int> itemsPorConj(quant_conj, 0);
    int currentValue = calculate_initial_state(currentSolution, somaPeso, itemsPorConj);
    
    int bestValue = currentValue;
    
    map<ll,ll> tabuList;
    int iterationsWithoutImproving = 0;
    
    uniform_int_distribution<ll> uid(0, 1e18);
    vector<ll> rand_int(itens);
    for(ll &i: rand_int) i = uid(rng);

    ll chash = 0;
    for(int i = 0; i < itens; i++){
        if(currentSolution[i]) chash ^= rand_int[i];
    }

    tabuList[chash] = -1;

    auto start_time = chrono::high_resolution_clock::now();
    int iter = 0;
 
    vector<pair<double, int>> convergence_data; 
    convergence_data.push_back({0.0, bestValue});

    // --- Loop Principal da Busca Tabu ---
    while (true) {
        auto current_time = chrono::high_resolution_clock::now();
        double elapsed_time = chrono::duration<double>(current_time - start_time).count();
        if (elapsed_time > tempoLimite || iterationsWithoutImproving > 500) {
            break;
        }
        iter++;

        int best_neighbor_value = -2e9;
        int best_move_item = -1;
        int best_move_delta = 0;

        for (int itemFlip = 0; itemFlip < itens; ++itemFlip) {
            int delta = 0;
            if (currentSolution[itemFlip]) {
                delta = -lucro[itemFlip];
                for (int cj : conju[itemFlip]) {
                    if (itemsPorConj[cj] > inf_conj[cj].first) delta += inf_conj[cj].second;
                }
            } else {
                if (somaPeso + peso[itemFlip] > capacidade) continue;
                delta = lucro[itemFlip];
                for (int cj : conju[itemFlip]) {
                    if (itemsPorConj[cj] + 1 > inf_conj[cj].first) delta -= inf_conj[cj].second;
                }
            }

            int neighbor_value = currentValue + delta;
            ll nHash = chash ^ rand_int[itemFlip];
            int is_tabu = (tabuList.find(nHash) == tabuList.end()) ? 1e9 : tabuList[nHash];
            bool aspiration_met = (is_tabu > bestValue);

            if (aspiration_met) {
                if (neighbor_value > best_neighbor_value) {
                    best_neighbor_value = neighbor_value;
                    best_move_item = itemFlip;
                    best_move_delta = delta;
                }
            }
        }

        if (best_move_item == -1) break; 
         
        if (best_neighbor_value > bestValue) {
            bestValue = best_neighbor_value;
            convergence_data.push_back({elapsed_time, bestValue});
            iterationsWithoutImproving = 0;
        } else {
            iterationsWithoutImproving++;
        }

        ll nHash = chash ^ rand_int[best_move_item];
         
        currentSolution.flip(best_move_item);
        currentValue += best_move_delta; 

        if (currentSolution[best_move_item]) {
            somaPeso += peso[best_move_item];
            for (int cj : conju[best_move_item]) itemsPorConj[cj]++;
        } else {
            somaPeso -= peso[best_move_item];
            for (int cj : conju[best_move_item]) itemsPorConj[cj]--;
        }
        
        tabuList[nHash] = iter + TABU_TENURE;
    }
 
    ofstream convergence_file(convergence_filepath);
    if(convergence_file.is_open()){
        if (convergence_data.back().second < bestValue) {
             convergence_data.push_back({tempoLimite, bestValue});
        } else {
             convergence_data.push_back({tempoLimite, convergence_data.back().second});
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
    int sol = TABU_Optimized(dir_saida_convergencia);
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