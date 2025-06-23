#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <cstdlib> // Para system()
#include <iomanip> // Para std::setw e std::left

int main() {
    // --- Configuração dos Experimentos ---
    const std::vector<std::string> algorithmNames = {"simulated_annealing", "tabu", "grasp", "ils"};
    
    const std::vector<std::string> instanceTypes = {"correlated_sc", "fully_correlated_sc", "not_correlated_sc"};
    const std::vector<std::string> instanceSizes = {"300", "500", "700", "800", "1000"};
    const int totalScenarios = 4;
    const int totalFilesPerConfig = 10;
    const int totalRuns = 30;

    // --- Verificação Pré-Execução ---
    std::cout << "Verificando a existência dos executáveis...\n";
    for (const std::string& algoName : algorithmNames) {
        if (!std::filesystem::exists(algoName)) {
            std::cerr << "ERRO FATAL: O executável '" << algoName << "' não foi encontrado.\n"
                      << "Certifique-se de que todos os algoritmos foram compilados com otimização.\n";
            return 1;
        }
    }
    std::cout << "Todos os executáveis foram encontrados. Iniciando os testes.\n";

    // --- Loop Principal dos Experimentos ---
    for (int run = 1; run <= totalRuns; ++run) {
        std::cout << "\n======================================================\n"
                  << "===                INICIANDO RUN " << std::setw(2) << run << " / " << totalRuns << "                ===\n"
                  << "======================================================\n";

        for (const std::string& algoName : algorithmNames) {
            std::cout << "\n[Algoritmo: " << algoName << "]\n";

            for (int scenario = 1; scenario <= totalScenarios; ++scenario) {
                for (const std::string& type : instanceTypes) {
                    for (const std::string& size : instanceSizes) {
                        
                        // --- LÓGICA DE SAÍDA NÃO-AGREGADA ---
                        // Cria um diretório de saída ÚNICO para cada execução (run).
                        std::string outputDir = "./resultados/run_" + std::to_string(run) + "/" + algoName + "/" + type;
                        std::filesystem::create_directories(outputDir);

                        // O arquivo de saída agrega os 10 testes de uma mesma configuração, para uma dada execução.
                        std::string outputFile = outputDir + "/saida_" + size + ".txt";

                        // Feedback visual para o usuário
                        std::cout << "  > Cenário " << scenario << " | "
                                  << std::setw(20) << std::left << type 
                                  << "| Tamanho " << std::setw(4) << size << ": " << std::flush;

                        for (int fileNum = 1; fileNum <= totalFilesPerConfig; ++fileNum) {
                            // Constrói o caminho do arquivo de entrada
                            std::string inputFile = "instances/scenario" + std::to_string(scenario) +
                                                   "/" + type + std::to_string(scenario) +
                                                   "/" + size + "/kpfs_" + std::to_string(fileNum) + ".txt";

                            if (!std::filesystem::exists(inputFile)) {
                                // Se o arquivo não existe, pulamos, mas não quebramos a barra de progresso
                                std::cout << "X" << std::flush;
                                continue;
                            }

                            // Monta e executa o comando
                            std::string command = "./" + algoName + " " + inputFile + " " + outputFile;
                            
                            system(command.c_str());
                            std::cout << "." << std::flush; // Imprime um ponto para cada arquivo processado
                        }
                        std::cout << " Concluído.\n"; // Fim da linha para esta configuração
                    }
                }
            }
        }
    }

    std::cout << "\n\n******************************************************\n"
              << "*   Todos os experimentos foram concluídos!          *\n"
              << "******************************************************\n";
    return 0;
}