/**
 * ==================================================================================
 * BENCHMARK: DialSort-Counting vs Classic Counting Sort vs Std Sort
 * ==================================================================================
 * Course  : Data Structures and Algorithms
 * Objetive: Experimentally compare both algorithms by varying
 *           n (input size) and U (universe), using three data distributions.
 *
 *
 * HOW TO COMPILE IN CLION:
 *  - Add this file to your CMakeLists.txt
 *  - O desde terminal: g++ -O2 -std=c++17 -o benchmark benchmark_dialsort_vs_counting.cpp
 *
 *HOW TO RUN IT:
 *  - Set the CMake profile to "Release", it is usually in Debug mode, however at the
 *    right top, next to the 'Run' bottom, you can select the Release mode. If it is not
 *    installed you should select "Edit CMake Profile" and at the plus symbol [+] you can
 *    add the Release mode.
 *
 * HOW TO READ THE RESULTS:
 *   - "ms_media"   : average time in milliseconds (lower = better)
 *   - "ms_desv"    : standard deviation (lower = more stable)
 *   - "Mkeys/s"    : millions of elements sorted per second (higher = better)
 *   - "speedup"    : how many times faster DialSort is compared to Counting Sort
 *                    (>1.0 = DialSort wins, <1.0 = Counting Sort wins)
 * ==================================================================================
 */

#include <iostream>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <numeric>    // para std::accumulate
#include <cmath>      // para std::sqrt
#include <iomanip>    // para std::setw, std::fixed
#include <string>
#include <fstream>    // para exportar CSV
using namespace std;

// ============================================================
// SECTION 1: BENCHMARK PARAMETERS
// ============================================================

const int WARMUP_ROUNDS  = 3;   // ejecuciones de calentamiento (se descartan)
const int MEASURE_ROUNDS = 5;   // ejecuciones que SÍ se miden
const long long SEED     = 10000000;  // semilla fija = mismos datos siempre


// ============================================================
// SECTION 2: HIGH-RESOLUTION TIMER
// ============================================================

long long current_time_ns() {
    using namespace chrono;
    return chrono::duration_cast<chrono::nanoseconds>(
        chrono::high_resolution_clock::now().time_since_epoch()
    ).count();
}

// ============================================================
// SECTION 3: BOTH ALGORITHMS
// ============================================================

void dialsort_counting(vector<int>& array) {
    int n = array.size();
    if (n <= 1) return; // si tiene 1 o 0 elementos no hay nada que ordenar

    // Encontrar los extremos del arreglo (para definir el universo)
    int minimum = array[0];
    int maximum = array[0];
    for (int i = 1; i < n; i++) {
        if (array[i] < minimum) {minimum = array[i];}
        if (array[i] > maximum) {maximum = array[i];}
    }

    int U = (maximum-minimum) + 1; //tamaño del universo

    // Construir frecuencias de cada número
    vector<int> H(U, 0); //crear un vecto H con U cantidad de posiciones inicializadas en 0
    for (int i = 0; i < n; i++) {
        H[array[i] - minimum]++; //así solo incluye los números del arreglo y no todos los que no estan
    }

    // Barrido del histograma
    int pos = 0; //indice de escritura en el arreglo original
    for (int j = 0; j < U; j++) {
        int valor = j + minimum; //valor real que voy a escribir H[j] cantidad de veces
        for (int c = 0; c < H[j]; c++) {
            array[pos] = valor;
            pos++;
        }
    }
}

void counting_sort_clasico(vector<int>& array) {
    int n = array.size();
    if (n <= 1) return;

    // Encontrar mínimo y máximo (igual que en Dial Sort)
    int minimum = array[0];
    int maximum = array[0];
    for (int i = 1; i < n; i++) {
        if (array[i] < minimum) minimum = array[i];
        if (array[i] > maximum) maximum = array[i];
    }

    int U = maximum - minimum + 1;

    // Histograma (igual que en Dial Sort)
    vector<int> H(U, 0);
    for (int i = 0; i < n; i++) {
        H[array[i] - minimum]++;
    }

    // PREFIX-SUM (el paso que DialSort elimina)
    for (int i = 1; i < U; i++) {
        H[i] = H[i] + H[i - 1];
    }

    // Arreglo auxiliar (memoria extra O(n))
    vector<int> output(n);
    for (int i = n - 1; i >= 0; i--) {
        int index = array[i] - minimum;
        H[index]--;
        output[H[index]] = array[i];
    }

    // Copiar resultado al arreglo original
    for (int i = 0; i < n; i++) {
        array[i] = output[i];
    }
}


// ============================================================
// SECTION 4: DATA GENERATORS
// ============================================================

// Distribución UNIFORME: todos los valores tienen la misma probabilidad
vector<int> gen_uniform(int n, int U, long long seed) {
    mt19937_64 rng(seed); //
    uniform_int_distribution<int> dist(0, U - 1);
    vector<int> array(n);
    for (int i = 0; i < n; i++) {
        array[i] = dist(rng);
    }
    return array;
}

// Distribución SESGADA: 80% de los datos en el 5% inferior del universo
vector<int> gen_sesgada(int n, int U, long long seed) {
    mt19937_64 rng(seed);
    int limit = max(1, U / 20);  // 5% del universo
    uniform_int_distribution<int> hot_zone(0, limit - 1);
    uniform_int_distribution<int> cold_zone(0, U - 1);
    bernoulli_distribution pick_hot(0.80);  // 80% van al 5% inferior
    vector<int> a(n);
    for (int i = 0; i < n; i++) {
        if (pick_hot(rng)) {
            a[i] = hot_zone(rng);
        } else {
            a[i] = cold_zone(rng);
        }
    }
    return a;
}

// Distribución CASI-ORDENADA: ordenada con 5% de elementos perturbados
vector<int> gen_almost_ordered(int n, int U, long long seed) {
    vector<int> array = gen_uniform(n, U, seed);
    sort(array.begin(), array.end());

    mt19937 rng(seed + 1);
    uniform_int_distribution<int> idx_rnd(0, n - 1);
    uniform_int_distribution<int> val_rnd(0, U - 1);

    int perturbaciones = n / 20;  // 5% del arreglo
    for (int i = 0; i < perturbaciones; i++) {
        array[idx_rnd(rng)] = val_rnd(rng);
    }
    return array;
}


// ============================================================
// SECTION 5: CORRECTNESS VERIFIER
// ============================================================
bool is_ordered(const vector<int>& array) {
    for (int i = 1; i < array.size(); i++)
        if (array[i-1] > array[i]) return false;
    return true;
}


// ============================================================
// SECTION 6: RESULTS STRUCTURE
// ============================================================

struct Results {
    string algorithm;
    string distribution;
    int n;
    int U;
    double ms_media;        // tiempo promedio en ms
    double ms_desv;         // desviación estándar en ms
    double ms_min;          // tiempo mínimo (mejor caso real)
    double mkeys_s;         // throughput: millones de keys por segundo
    long long bytes_mem;
    string bigO_best;
    string bigO_avg;
    string bigO_worst;
    bool correct;

};


// ============================================================
// SECTION 7: MEASURE FUNCTION
// ============================================================

Results measure(
    const string name_algo,
    const string name_dist,
    const vector<int>& base_data,  // datos originales (no se modifican)
    int U,
    int which_algo
) {
    // --- Calentamiento: ejecutar sin medir ---
    // Esto "calienta" la caché del procesador, haciendo que las
    // mediciones posteriores sean más representativas del uso real.
    long long bytes = 0;
    string bigO_best;
    string bigO_avg;
    string bigO_worst;

    if (which_algo==1) {
        bytes= (long long)U*sizeof(int);
        bigO_best  ="O(n+U)";
        bigO_avg   = "O(n+U)";
        bigO_worst = "O(n+U)";
    }
    else if (which_algo==2) {
        bytes= (long long)(U + base_data.size()*sizeof(int));
        bigO_best  ="O(n+U)";
        bigO_avg   = "O(n+U)";
        bigO_worst = "O(n+U)";
    }

    for (int r = 0; r < WARMUP_ROUNDS; r++) {
        vector<int> copy = base_data;  // copia fresca cada vez
        if (which_algo == 1) {
            dialsort_counting(copy);
        }else {counting_sort_clasico(copy);}
    }

    // --- Mediciones reales ---
    vector<double> time_ms;
    bool correct = true;

    for (int r = 0; r < MEASURE_ROUNDS; r++) {
        vector<int> copy= base_data;  // copia fresca cada vez

        long long start = current_time_ns();
        if (which_algo == 1) dialsort_counting(copy);
        else                counting_sort_clasico(copy);
        long long finish = current_time_ns();

        double ms = (finish - start) / 1000000.0;  // ns a ms
        time_ms.push_back(ms);

        if (!is_ordered(copy)) correct = false;
    }

    // --- Calcular estadísticas ---
    //Promedio
    double sum = accumulate(time_ms.begin(), time_ms.end(), 0.0);
    double media = sum / MEASURE_ROUNDS;

    //desviacion estandar
    double sum_cuad = 0.0;
    for (int i = 0; i < MEASURE_ROUNDS; i++) {
        double d = time_ms[i] - media;
        sum_cuad += d * d;
    }
    double desviacion = sqrt(sum_cuad / MEASURE_ROUNDS);

    //Minimo
    double min = *min_element(time_ms.begin(), time_ms.end());

    // throughput: cuántos millones de elementos por segundo ordena
    double mkeys_s = (media > 0) ? (base_data.size() / (media / 1000.0)) / 1e6 : 0.0;

    Results res;
    res.algorithm    = name_algo;
    res.distribution = name_dist;
    res.n            = base_data.size();
    res.U            = U;
    res.ms_media     = media;
    res.ms_desv      = desviacion;
    res.ms_min       = min;
    res.mkeys_s      = mkeys_s;
    res.bytes_mem    = bytes;;
    res.bigO_best    = bigO_best;
    res.bigO_avg     = bigO_avg;
    res.bigO_worst   = bigO_worst;
    res.correct      = correct;
    return res;
}


// ============================================================
// SECTION 8: PRINTING RESULTS
// ============================================================

void print_header() {
    cout << "\n";
    cout << string(128, '=') << "\n";
    cout << left
              << setw(16) << "Algorithm"
              << setw(20) << "Distribution"
              << setw(8) << "N"
              << setw(6)  << "U"
              << setw(12) << "Media(ms)"
              << setw(12) << "Desv(ms)"
              << setw(12) << "Mín(ms)"
              << setw(14) << "Mkeys/s"
              << "OK?\n";
    cout << string(128, '-') << "\n";
}

void print_rows(const Results& r) {
    cout << left << fixed << setprecision(3)
              << setw(16) << r.algorithm
              << setw(16) << r.distribution
              << setw(12) << r.n
              << setw(8)  << r.U
              << setw(12) << r.ms_media
              << setw(11) << r.ms_desv
              << setw(11) << r.ms_min
              << setw(12) << r.mkeys_s
              << (r.correct ? "PASSED" : "*** FAILED ***") << "\n";
}

void print_comparisons(const Results& dial, const Results& cs) {
    double speedup = (cs.ms_media > 0) ? cs.ms_media / dial.ms_media : 0.0;
    cout << "  --> Speedup DialSort vs Counting Sort: "
              << fixed << setprecision(2) << speedup << "x";
    if (speedup > 1.0)
        cout << "  (DialSort is FASTER)\n";
    else if (speedup < 1.0)
        cout << "  (Counting Sort is FASTER)\n";
    else
        cout << "  (They are pretty identical)\n";
    cout << "\n";
}


// ============================================================
// SECTION 9: EXPORT TO CSV
// ============================================================
// Útil para graficar los resultados en Excel o Python

void export_csv(const vector<Results>& resultsBench, const string& file_name) {
    ofstream file(file_name);
    if (!file.is_open()) {
        cout << "[AVISO] No se pudo crear " << file_name << "\n";
        return;
    }

    // Encabezado
    file << "Algorithm,distribution,n,U,ms_media,ms_desv,ms_min,Mkeys_s,correct,speedup\n";

    // Recorre de a pares: [0]=DialSort, [1]=CountingSort, [2]=DialSort, [3]=CountingSort...
    for (int i = 0; i + 1 < (int)resultsBench.size(); i += 2) {
        const Results& dial = resultsBench[i];      // fila DialSort
        const Results& cs   = resultsBench[i + 1];  // fila CountingSort

        // Calcular speedup: cuántas veces más rápido es DialSort
        // Si dial.ms_media es 0 (no debería pasar) ponemos 0 para evitar división por cero
        double speedup = 0.0;
        if (dial.ms_media > 0.0) {
            speedup = cs.ms_media / dial.ms_media;
        }

        // Escribir fila de DialSort CON su speedup
        file << fixed << setprecision(4)
             << dial.algorithm    << ","
             << dial.distribution << ","
             << dial.n            << ","
             << dial.U            << ","
             << dial.ms_media     << ","
             << dial.ms_desv      << ","
             << dial.ms_min       << ","
             << dial.mkeys_s      << ","
             << (dial.correct ? "PASSED" : "FAILED") << ","
             << speedup           << "\n";  // speedup de DialSort (el que gana >1.0)

        // Escribir fila de CountingSort con speedup inverso
        // Si speedup de DialSort es 2.0, el de CountingSort es 0.5
        double speedup_cs = 0.0;
        if (speedup > 0.0) {
            speedup_cs = 1.0 / speedup;
        }

        file << fixed << setprecision(4)
             << cs.algorithm    << ","
             << cs.distribution << ","
             << cs.n            << ","
             << cs.U            << ","
             << cs.ms_media     << ","
             << cs.ms_desv      << ","
             << cs.ms_min       << ","
             << cs.mkeys_s      << ","
             << (cs.correct ? "PASSED" : "FAILED") << ","
             << speedup_cs      << "\n";  // speedup de CountingSort (siempre <= 1.0)
    }

    file.close();
    cout << "\n[OK] Results exported to: " << file_name << "\n";
}


// ============================================================
// SECCIÓN 10: EXPERIMENTO PRINCIPAL
// ============================================================

int main() {

    cout << "================================================================\n";
    cout << " BENCHMARK: DialSort-Counting  vs  Classic Counting Sort\n";
    cout << " Warmup rounds           : " << WARMUP_ROUNDS << "\n";
    cout << " Measurement rounds      : " << MEASURE_ROUNDS << "\n";
    cout << " Random seed             : " << SEED << "\n";
    cout << "================================================================\n";

    // --- Dimensiones del experimento ---
    // n: tamaños de entrada (de 100k a 10M como pide la práctica)
    vector<int> sizes = {100000, 500000, 1000000, 5000000, 10000000};

    // U: tamaños del universo (rango de valores posibles)
    // Pequeño: muchas repeticiones (ventaja para ambos)
    // Mediano: caso típico
    // Grande: pocos duplicados, el barrido del histograma domina
    const vector<int> Us = {256, 1024, 65536};

    // Distribuciones a probar
    string distributions[]      = {"Uniform", "Biased", "Nearly Uniform"};

    // Recolector de todos los resultados para el CSV
    vector<Results> all_results;

    // --- Bucle principal ---
    for (int in = 0; in < 5; in++) {
        int n = sizes[in];

        for (int iu = 0; iu < 3; iu++) {
            int U = Us[iu];

            print_header();
            cout << " n = " << n << "  |  U = " << U << "\n";
            cout << string(128, '-') << "\n";

            for (int id = 0; id < 3; id++) {
                string name_dist = distributions[id];

                // Semilla unica para cada combinacion de parametros
                long long seed = SEED + n + (long long)U * 1000 + id * 7;

                // Generar datos base
                vector<int> data;
                if      (name_dist == "Uniform")      data = gen_uniform(n, U, seed);
                else if (name_dist == "Biased")       data = gen_sesgada(n, U, seed);
                else                                     data = gen_almost_ordered(n, U, seed);

                // Medir DialSort
                Results r_dial = measure("DialSort", name_dist, data, U, 1);

                // Medir Counting Sort clasico
                Results r_cs   = measure("CountingSort", name_dist, data, U, 2);

                print_rows(r_dial);
                print_rows(r_cs);
                print_comparisons(r_dial, r_cs);

                all_results.push_back(r_dial);
                all_results.push_back(r_cs);
            }
        }
    }

    // --- Resumen global ---
    cout << "\n" << string(108, '=') << "\n";
    cout << " GLOBAL SUMMARY\n";
    cout << string(108, '-') << "\n";

    double speedup_sum = 0.0;
    int count_dial_wins = 0;
    int count_cs_wins = 0;
    double better_speedup = 0.0;
    string better_config;
    int total = 0;

    for (int i = 0; i + 1 < (int)all_results.size(); i += 2) {
        const Results& dial = all_results[i];
        const Results& cs   = all_results[i + 1];

        if (!dial.correct || !cs.correct) continue;
        if (dial.ms_media <= 0.0) continue;

        double speedup = cs.ms_media / dial.ms_media;
        speedup_sum += speedup;
        total++;

        if (speedup > 1.0) count_dial_wins++;
        else               count_cs_wins++;

        if (speedup > better_speedup) {
            better_speedup = speedup;
            better_config  = "n=" + to_string(dial.n)
                          + " U=" + to_string(dial.U)
                          + " dist=" + dial.distribution;
        }
    }

    double speedup_prom = (total > 0) ? speedup_sum / total : 0.0;

    cout << fixed << setprecision(2);
    cout << " Total of configurations   : " << total << "\n";
    cout << " DialSort wins in          : " << count_dial_wins << " out of " << total << " cases\n";
    cout << " Counting Sort wins in     : " << count_cs_wins   << " out of " << total << " cases\n";
    cout << " Average  Speedup          : " << speedup_prom    << "x\n";
    cout << " Better speedup of DialSort : " << better_speedup   << "x  (" << better_config << ")\n";
    cout << string(108, '=') << "\n";

    export_csv(all_results, "results_benchmark.csv");
    return 0;
}