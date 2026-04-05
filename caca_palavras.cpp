#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <thread>
#include <cctype>

using namespace std;

/*
                               Relatório de Projeto: Caça-Palavras com Threads

1. Estrutura do Código
O programa foi estruturado em funções específicas para garantir a separação de responsabilidades e facilitar a manutenção:

    lerArquivoEntrada: Responsável pela leitura e validação dos dados iniciais.

    buscarNaDirecao: Verifica a existência de uma palavra em uma direção específica.

    procurarPalavra: Gerencia a busca de uma palavra completa dentro da matriz.

    marcarPalavrasMaiusculas: Realiza o destaque visual das palavras encontradas.

    escreverArquivoSaida: Gera o arquivo final com os resultados.

2. Estratégia de Paralelização
  A abordagem adotada foi o modelo de "uma thread por palavra".

  Nessa lógica, cada thread é responsável por localizar uma única palavra em toda a matriz.

  Vantagem: Simplifica a implementação e garante a integridade dos resultados, evitando que o processamento de uma palavra interfira em outra.

3. Sincronização e Concorrência
  Não utilizei mutex porque não há compartilhamento de escrita entre threads, e a matriz de saída é gerada após o join.

  Acesso de Leitura: A matriz original é acessada pelas threads apenas para leitura, o que é inerentemente seguro.

  Isolamento de Escrita: Cada thread escreve em uma posição exclusiva do vetor de resultados.

  Processamento Sequencial: A alteração da matriz de saída ocorre apenas após o encerramento de todas as threads (via join()), eliminando qualquer risco de condição de corrida.

4. Otimização de Busca
 Para melhorar o desempenho e reduzir comparações desnecessárias, foi implementada uma verificação prévia:
 O programa só inicia a checagem das direções se a letra atual da matriz corresponder à primeira letra da palavra buscada.
 Caso contrário, o algoritmo ignora a posição e segue para a próxima, economizando ciclos de processamento.
*/

// Guarda uma posicao da matriz
struct Posicao {
    int linha;
    int coluna;
};

// Guarda o resultado da busca de uma palavra
struct Resultado {
    string palavra;             // palavra buscada
    bool encontrada = false;    // indica se foi encontrada
    int linhaInicial = -1;      // linha inicial da palavra
    int colunaInicial = -1;     // coluna inicial da palavra
    string direcao = "";        // direcao em que foi encontrada
    vector<Posicao> posicoes;   // posições das letras para marcar em maiusculo
};

// Guarda uma direção de busca
struct Direcao {
    int dl;         // deslocamento da linha
    int dc;         // deslocamento da coluna
    string nome;    // nome da direção
};

// As 8 direções possiveis
const vector<Direcao> direcoes = {
    {0, 1, "direita"},
    {0, -1, "esquerda"},
    {1, 0, "abaixo"},
    {-1, 0, "cima"},
    {1, 1, "baixo/direita"},
    {1, -1, "baixo/esquerda"},
    {-1, 1, "cima/direita"},
    {-1, -1, "cima/esquerda"}
};

// Le o arquivo de entrada e separa dimesões, matriz e palavras
bool lerArquivoEntrada(const string& nomeArquivo, int& linhas, int& colunas,
                       vector<string>& matriz, vector<string>& palavras) {
    ifstream entrada(nomeArquivo);

    // Verifica se o arquivo abriu corretamente
    if (!entrada.is_open()) {
        return false;
    }

    // Le linhas e colunas e confere se a leitura foi valida
    if (!(entrada >> linhas >> colunas)) {
        return false;
    }

    // Verifica se as dimensões são validas
    if (linhas <= 0 || colunas <= 0) {
        return false;
    }

    //O ignore é usado para remover o caractere de quebra de linha que sobra após a leitura com operador
    entrada.ignore();

    matriz.clear();
    palavras.clear();

    string linha;

    // Le as linhas da matriz
    for (int i = 0; i < linhas; i++) {
        getline(entrada, linha);

        // Remove '\r' caso o arquivo tenha sido salvo no Windows
        if (!linha.empty() && linha.back() == '\r') {
            linha.pop_back();
        }

        // Verifica se a linha da matriz tem exatamente o número de colunas esperado
        if ((int)linha.size() != colunas) {
            return false;
        }

        matriz.push_back(linha);
    }

    // Lê as palavras restantes
    while (getline(entrada, linha)) {
        // Remove '\r' se existir
        if (!linha.empty() && linha.back() == '\r') {
            linha.pop_back();
        }

        // Ignora linhas vazias
        if (!linha.empty()) {
            palavras.push_back(linha);
        }
    }

    // Verifica se existe pelo menos uma palavra para procurar
    if (palavras.empty()) {
        return false;
    }

    entrada.close();
    return true;
}

// Confere se a palavra existe a partir de uma posicão e seguindo uma direcão
bool buscarNaDirecao(const vector<string>& matriz, int linhas, int colunas,
                     const string& palavra, int lin, int col,
                     const Direcao& dir, vector<Posicao>& posicoesEncontradas) {
    posicoesEncontradas.clear();

    // Testa letra por letra da palavra
    for (int i = 0; i < (int)palavra.size(); i++) {
        int novaLin = lin + i * dir.dl;
        int novaCol = col + i * dir.dc;

        // Se sair da matriz, a palavra não cabe nessa direcao
        if (novaLin < 0 || novaLin >= linhas || novaCol < 0 || novaCol >= colunas) {
            return false;
        }

        // Se a letra não bater, a palavra não está nessa direcão
        if (matriz[novaLin][novaCol] != palavra[i]) {
            return false;
        }

        // Guarda a posicão da letra encontrada
        posicoesEncontradas.push_back({novaLin, novaCol});
    }

    // Se todas as letras bateram, encontrou a palavra
    return true;
}

// Funcão executada por cada thread
// Cada thread procura uma palavra em toda a matriz
void procurarPalavra(const vector<string>& matriz, int linhas, int colunas,
                     const string& palavra, Resultado& resultado) {
    resultado.palavra = palavra;

    // Protege contra palavra vazia
    if (palavra.empty()) {
        return;
    }

    // Percorre toda a matriz
    for (int i = 0; i < linhas; i++) {
        for (int j = 0; j < colunas; j++) {

            // Otimizacão:
            // só tenta procurar a palavra se a primeira letra combinar
            if (matriz[i][j] != palavra[0]) {
                continue;
            }

            // Testa todas as 8 direcões
            for (const auto& dir : direcoes) {
                vector<Posicao> posicoesTemp;

                if (buscarNaDirecao(matriz, linhas, colunas, palavra, i, j, dir, posicoesTemp)) {
                    resultado.encontrada = true;
                    resultado.linhaInicial = i;
                    resultado.colunaInicial = j;
                    resultado.direcao = dir.nome;
                    resultado.posicoes = posicoesTemp;
                    return;
                }
            }
        }
    }
}

// Marca as letras das palavras encontradas em maiusculo
void marcarPalavrasMaiusculas(vector<string>& matrizSaida, const vector<Resultado>& resultados) {
    for (const auto& resultado : resultados) {
        if (resultado.encontrada) {
            for (const auto& pos : resultado.posicoes) {
                matrizSaida[pos.linha][pos.coluna] =
                    toupper((unsigned char)matrizSaida[pos.linha][pos.coluna]);
            }
        }
    }
}

// Escreve o arquivo de saida
bool escreverArquivoSaida(const string& nomeArquivo, const vector<string>& matrizSaida,
                          const vector<Resultado>& resultados) {
    ofstream saida(nomeArquivo);

    // Verifica se conseguiu abrir o arquivo de saida
    if (!saida.is_open()) {
        return false;
    }

    // Escreve a matriz final com as palavras destacadas
    for (const auto& linha : matrizSaida) {
        saida << linha << '\n';
    }

    // Escreve o resultado de cada palavra
    for (const auto& resultado : resultados) {
        if (resultado.encontrada) {
            saida << resultado.palavra << " - ("
                  << resultado.linhaInicial + 1 << ","
                  << resultado.colunaInicial + 1 << "):"
                  << resultado.direcao << '\n';
        } else {
            
            saida << resultado.palavra << ": nao encontrada" << '\n';
        }
    }

    saida.close();
    return true;
}

int main() {
    string arquivoEntrada, arquivoSaida;
    int linhas, colunas;
    vector<string> matriz;
    vector<string> palavras;

    // Pede ao usuário os nomes dos arquivos
    cout << "Digite o nome do arquivo de entrada: ";
    cin >> arquivoEntrada;

    cout << "Digite o nome do arquivo de saida: ";
    cin >> arquivoSaida;

    // Lê e valida o arquivo de entrada
    if (!lerArquivoEntrada(arquivoEntrada, linhas, colunas, matriz, palavras)) {
        cout << "Erro ao ler o arquivo de entrada. Verifique se o arquivo existe e se o formato esta correto.\n";
        return 1;
    }

    // Cria um resultado para cada palavra
    vector<Resultado> resultados(palavras.size());

    // Vetor de threads
    vector<thread> threads;
    threads.reserve(palavras.size());

    // Cria uma thread para cada palavra
    for (int i = 0; i < (int)palavras.size(); i++) {
        threads.push_back(thread(procurarPalavra,
                                 cref(matriz),
                                 linhas,
                                 colunas,
                                 cref(palavras[i]),
                                 ref(resultados[i])));
    }

    // Espera todas as threads terminarem
    for (auto& t : threads) {
        t.join();
    }

    // Cria uma copia da matriz original para gerar a saida
    vector<string> matrizSaida = matriz;

    // Marca as palavras encontradas em maiusculo
    marcarPalavrasMaiusculas(matrizSaida, resultados);

    // Escreve o arquivo final
    if (!escreverArquivoSaida(arquivoSaida, matrizSaida, resultados)) {
        cout << "Erro ao escrever o arquivo de saida.\n";
        return 1;
    }

    cout << "Processamento concluido com sucesso.\n";
    return 0;
}