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

mt19937 rng((int)chrono::steady_clock::now().time_since_epoch().count());

// --- Parâmetros da Meta-heurística ---
const double tempoLimite = 2.0;
const double alpha = 0.999;
const double temperatura_inicial = 1000.0;
 
int Simulated_Annealing_Optimized(const string& convergence_filepath) {
    // --- Estado da Solução ---
    bitset<1000> currentItems; 
    int somaPeso = 0;
    vector<int> itemsPorConj(quant_conj, 0);

    // --- Inicialização ---
    uniform_int_distribution<int> item_dist(0, itens - 1);
    for (int i = 0; i < itens; ++i) { 
        int item_idx = item_dist(rng);
        if (!currentItems[item_idx] && (somaPeso + peso[item_idx] <= capacidade)) {
            currentItems[item_idx] = 1;
            somaPeso += peso[item_idx];
        }
    }
 
    int currentValue = 0;
    int initial_somaValor = 0;
    int initial_somaPenalidade = 0;
    fill(itemsPorConj.begin(), itemsPorConj.end(), 0);
    for(int i = 0; i < itens; ++i) {
        if(currentItems[i]) {
            initial_somaValor += lucro[i];
            for(int cj : conju[i]) {
                itemsPorConj[cj]++;
            }
        }
    }
    for(int j = 0; j < quant_conj; ++j) {
        if(itemsPorConj[j] > inf_conj[j].first) {
            initial_somaPenalidade += (itemsPorConj[j] - inf_conj[j].first) * inf_conj[j].second;
        }
    }
    currentValue = initial_somaValor - initial_somaPenalidade;
    
    // --- Variáveis do Algoritmo SA ---
    int bestValue = currentValue;
    
    double temperature = temperatura_inicial;
    int iterationsWithoutImproving = 0;

    uniform_real_distribution<double> prob_dist(0.0, 1.0);
    auto start_time = chrono::high_resolution_clock::now();
 
    vector<pair<double, int>> convergence_data; 
    convergence_data.push_back({0.0, bestValue});

    // --- Loop Principal ---
    while (true) {
        auto current_time = chrono::high_resolution_clock::now();
        double elapsed_time = chrono::duration<double>(current_time - start_time).count();

        if (elapsed_time > tempoLimite || iterationsWithoutImproving > 100000) {
            break;
        }
        
        int itemFlip = item_dist(rng);
        int delta = 0;
        
        if (currentItems[itemFlip]) {
            delta = -lucro[itemFlip];
            for (int currConj : conju[itemFlip]) {
                if (itemsPorConj[currConj] > inf_conj[currConj].first) {
                    delta += inf_conj[currConj].second;
                }
            }
        } else {
            if (somaPeso + peso[itemFlip] > capacidade) continue;
            delta = lucro[itemFlip];
            for (int currConj : conju[itemFlip]) {
                if (itemsPorConj[currConj] + 1 > inf_conj[currConj].first) {
                    delta -= inf_conj[currConj].second;
                }
            }
        }
        
        if (delta > 0 || prob_dist(rng) < exp(delta / temperature)) {
            currentItems.flip(itemFlip);
            currentValue += delta;

            if (currentItems[itemFlip]) {
                somaPeso += peso[itemFlip];
                for (int cj : conju[itemFlip]) itemsPorConj[cj]++;
            } else {
                somaPeso -= peso[itemFlip];
                for (int cj : conju[itemFlip]) itemsPorConj[cj]--;
            }
            
            if (currentValue > bestValue) {
                bestValue = currentValue;
                iterationsWithoutImproving = 0; 
                convergence_data.push_back({elapsed_time, bestValue});
            } else {
                iterationsWithoutImproving++;
            }
        } else {
            iterationsWithoutImproving++;
        }
        
        temperature *= alpha;
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
    // MUDANÇA: Espera 4 argumentos
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
    int sol = Simulated_Annealing_Optimized(dir_saida_convergencia);
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