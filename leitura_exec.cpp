#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <filesystem>
#include <cstdlib>

using namespace std;

int main(){
    string c_cod[] = {"simulated_annealing", "tabu", "grasp", "ils"};
    // cod = "simulated_annealing";
    // cod = "tabu";
    // string c_cod[] = {"grasp"};
    // cod = "ils";

    //Compilar
    //mac
    //string com_compilar = "clang++ -std=c++17 "+cod+".cpp -o "+cod;
    //Outros
    // string com_compilar = "make simulated_annealing";
    // cod = "simulated_annealing";
    // if(system(com_compilar.c_str())!= 0){
    //     cout << "Erro ao compilar:" << cod << '\n';
    // }

    //Pastas em instances
    const int cenarios = 4;
    vector<string> p_tipos = {"correlated_sc", "fully_correlated_sc", "not_correlated_sc"};
    vector<string> p_quant= {"300", "500", "700", "800", "1000"};
    const int total_arquivos = 10;

    //Criar pasta com resultados
    for(int i = 0; i < 30; i++){
        for(string cod: c_cod){
            string dir_cod = "./resultados/"+cod;
            for(string& t: p_tipos){
                string dir_cod_t = dir_cod+"/"+t;
                filesystem::create_directories(dir_cod_t);
            }
        
            //Ler os arquivos
            for(int c = 1; c <= cenarios; c++){
                for(string& t: p_tipos){
                    for(string& q: p_quant){
                        string dir_saida = dir_cod+"/"+t+"/saida_"+q+".txt";
                        for(int i = 1; i <= total_arquivos;i++){
                            string dir_txt = "instances/scenario"+to_string(c)+
                            "/"+t+to_string(c)+"/"+q+"/kpfs_"+to_string(i)+".txt";
                            if (!filesystem::exists(dir_txt)) {
                                cout << "Arquivo não encontrado: " << dir_txt << endl;
                                continue;
                            }
                            if (!filesystem::exists(cod)) {
                                cout << "Erro: o executável '" << cod << "' não foi criado.\n";
                                return 1;
                            }
                            string com_exec = "./"+cod+ " "+dir_txt+" "+dir_saida;
                            system(com_exec.c_str());
                        }
                    }
                }
            }
        }
    }

    return 0;
}