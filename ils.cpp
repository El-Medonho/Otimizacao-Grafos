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
#include <cassert> // Para testes de sanidade (debug)

using namespace std;

// --- Variáveis Globais (Dados do Problema) ---
int itens, quant_conj, capacidade;
vector<int> lucro, peso;
vector<vector<int>> conju;
vector<pair<int, int>> inf_conj;

mt19937_64 rng((int)chrono::steady_clock::now().time_since_epoch().count());

// --- Parâmetros da Meta-heurística ---
const double tempoLimite = 2.0;
const int PERTURBATION_STRENGTH = 4;

// Função para calcular o valor total de uma solução.
// Usada para validação e após a perturbação.
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

// OTIMIZAÇÃO: Busca Local Rápida com Avaliação Delta
void FastLocalSearch(bitset<1000>& solution, int& solutionValue, int& solutionPeso, vector<int>& itemsPorConj) {
    bool improvement_found = true;
    while (improvement_found) {
        improvement_found = false;

        for (int itemFlip = 0; itemFlip < itens; ++itemFlip) {
            int delta = 0;

            if (solution[itemFlip]) { // Tentar REMOVER
                delta = -lucro[itemFlip];
                for (int cj : conju[itemFlip]) {
                    if (itemsPorConj[cj] > inf_conj[cj].first) {
                        delta += inf_conj[cj].second;
                    }
                }
            } else { // Tentar ADICIONAR
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

                if (solution[itemFlip]) { // Item foi adicionado
                    solutionPeso += peso[itemFlip];
                    for (int cj : conju[itemFlip]) itemsPorConj[cj]++;
                } else { // Item foi removido
                    solutionPeso -= peso[itemFlip];
                    for (int cj : conju[itemFlip]) itemsPorConj[cj]--;
                }
                
                improvement_found = true;
                break; // Estratégia "First Improvement"
            }
        }
    }
}

// Função de Perturbação
void Perturb(bitset<1000>& solution, int strength) {
    uniform_int_distribution<int> item_dist(0, itens - 1);
    for (int i = 0; i < strength; ++i) {
        solution.flip(item_dist(rng));
    }
}

int ILS() {
    // 1. GERAÇÃO DA SOLUÇÃO INICIAL (Gulosa)
    bitset<1000> current_solution;
    int current_peso = 0;
    
    vector<pair<double, int>> candidates(itens);
    for (int i = 0; i < itens; i++) {
        candidates[i] = { (peso[i] == 0 ? 1e12 + lucro[i] : (double)lucro[i] / peso[i]), i };
    }
    sort(candidates.rbegin(), candidates.rend());
    
    for (auto const& [ratio, currItem] : candidates) {
        if (current_peso + peso[currItem] <= capacidade) {
            current_solution[currItem] = 1;
            current_peso += peso[currItem];
        }
    }
    
    // Buffer para contagem de itens, para ser reutilizado
    vector<int> itemsPorConj_buffer(quant_conj, 0);

    // 2. APLICAR BUSCA LOCAL PARA ENCONTRAR O PRIMEIRO ÓTIMO LOCAL
    int current_value = calculate_solution_value(current_solution, current_peso, itemsPorConj_buffer);
    FastLocalSearch(current_solution, current_value, current_peso, itemsPorConj_buffer);
    
    bitset<1000> best_solution_so_far = current_solution;
    int best_value_so_far = current_value;

    auto start_time = chrono::high_resolution_clock::now();
    int iterationsWithoutImproving = 0;

    // 3. LOOP PRINCIPAL DO ILS
    while (true) {
        auto current_time = chrono::high_resolution_clock::now();
        double elapsed_time = chrono::duration<double>(current_time - start_time).count();
        if (elapsed_time > tempoLimite || iterationsWithoutImproving > 300) {
            break;
        }

        // a. Perturbação
        bitset<1000> perturbed_solution = current_solution;
        Perturb(perturbed_solution, PERTURBATION_STRENGTH);

        // b. Busca Local na solução perturbada
        // Recalculamos o estado aqui, pois a perturbação o invalidou
        int perturbed_peso;
        int perturbed_value = calculate_solution_value(perturbed_solution, perturbed_peso, itemsPorConj_buffer);

        if (perturbed_value > -2e9) { // Checa se a perturbação não tornou a solução inviável
            FastLocalSearch(perturbed_solution, perturbed_value, perturbed_peso, itemsPorConj_buffer);
        }
        
        // c. Critério de Aceitação
        if (perturbed_value > current_value) {
            current_solution = perturbed_solution;
            current_value = perturbed_value;
        }

        // d. Atualiza a melhor solução global
        if (current_value > best_value_so_far) {
            best_value_so_far = current_value;
            best_solution_so_far = current_solution;
            iterationsWithoutImproving = 0;
        } else {
            iterationsWithoutImproving++;
        }
    }

    return best_value_so_far;
}

int main(int argc, char* argv[]) {
    // ... (o seu main continua igual aqui) ...
    if (argc != 3) {
        cerr << "Uso: " << argv[0] << " <arquivo_entrada> <arquivo_saida>" << endl;
        return 1;
    }
    string dir_entrada = argv[1];
    string dir_saida = argv[2];

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
    int sol = ILS();
    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> time = end - start;
    double execution_time = time.count();

    ofstream saida_arquivo(dir_saida, ios::app);
    if (!saida_arquivo.is_open()) {
        cout << "Erro ao abrir " << dir_saida << " para escrita.\n";
        return 1;
    }
    saida_arquivo << sol << " " << execution_time << '\n';
    saida_arquivo.close();
    return 0;
}