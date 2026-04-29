/**
 * ==================================================================================
 * BENCHMARK: DialSort-Counting vs Parallel Bucket Sort
 * ==================================================================================
 * Course  : Data Structures and Algorithms
 * Objetive: Experimentally compare both algorithms by varying
 *           n (input size) and U (universe), using three data distributions.
 *
 *
 * HOW TO COMPILE IN CLION:
 *  - Add this file to your CMakeLists.txt
 *  - O desde terminal: g++ -O2 -std=c++17 -o benchmark benchmark_dialsort_vs_bucketsort.cpp
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
#include <thread>
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
    ).count(); // desde que se creo la funcion en nanosegundos
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

void parallel_bucket_sort(vector<int>& array) {
    int n = array.size();
    if (n <= 1) return; // caso base, si hay 0 o 1 elemento ya esta ordenado

    // Encontrar mínimo y máximo
    int minimum = array[0]; // se asume que el máximo y el mínimo es el primer elemento
    int maximum = array[0];
    for (int i = 1; i < n; i++) { //recorro todo el arreglo para encontrar el mínimo y el máximo
        if (array[i] < minimum) minimum = array[i]; //comparo cada elemento con el siguiente y actualizo si hallo uno menor o mayor que el que ya se definió como min o max
        if (array[i] > maximum) maximum = array[i];
    }

    //Definir la cantidad de hilos
    int K = thread::hardware_concurrency(); // pregunta al procesador cuántos hilos simultáneos soporta (subconjuntos que creará)
    if (K == 0) K = 4; // Valor de seguridad

    vector<vector<int>> buckets(K); // define la estructura de datos para tus subconjuntos: matriz bidimensional. Es un arreglo principal que contiene K arreglos vacíos por dentro

    // Scatter
    double range = (double)(maximum - minimum + 1) / K; //define el rango de cada subcnonjunto
    for (int i = 0; i < n; i++) { // recorre todo el arreglo
        int b_idx = (array[i] - minimum) / range;  //define el índice de en cual subconjunto va ese elemento
        if (b_idx >= K) b_idx = K - 1; // Protección por redondeo, fuerza a colocar los números que se salen de rango en el último subconjunto
        buckets[b_idx].push_back(array[i]); // coloca el número en la última posición de la cubeta seleccionada
    }

    // Ordenamiento Paralelo
    vector<std::thread> hilos; //crea un vector de handlers, que permite llevar control de los procesos en cada hilo
    for (int i = 0; i < K; i++) { //asigna cada hilo a un subconjunto porque es un ciclo que se repite k veces
        hilos.push_back(thread([&buckets, i]() { //aca estoy creando un hilo y pasandole la función a ejectuar sobre el subconjunto (como parametros le paso el arreglo por referencia y la i [# de subconjunto])
            // Cada hilo ordena su propio subconjunto de forma independiente
            sort(buckets[i].begin(), buckets[i].end()); // sortea los subconjuntos hilo por hilo, dando el primer y el último elemento
        }));
    }

    // Control de los hilos
    for (auto& t: hilos) {
        t.join(); // revisa uno por uno los hilos y une el resultado al flujo del programa, para esto debe esperar a que todos terminen y los elimina una vez terminen su tarea
    }

    // Gather
    int pos = 0;
    for (int i = 0; i < K; i++) { // acá los datos en los subconjuntos estan ordenados entonces solo se recorren en orden y se sobreescribe el arreglo original
        for (int val : buckets[i]) { //por cada valor tipo entero en el subconjunto i
            array[pos] = val; // asigna al array en la posicion 'pos' ese valor
            pos++;
        }
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
bool is_ordered(const vector<int>& array) { // evitar bugs y revisar que el arreglo sí este ordenado
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
    long long bytes_mem;    // memoria en bytes
    string bigO_best;       // mejor caso de big0
    string bigO_avg;        // caso promedio de bigO
    string bigO_worst;      // peor caso de bigO
    bool correct;           // los valores pasan o no la prueba (son coherentes)

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
    // Eejecutar sin medir
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
    else if (which_algo == 2) {
        bytes = (long long)(base_data.size() * sizeof(int)); // Memoria buckets
        bigO_best  = "O(n+k)";
        bigO_avg   = "O(n+n^2/k+k)";
        bigO_worst = "O(n^2)";
    }

    for (int r = 0; r < WARMUP_ROUNDS; r++) {
        vector<int> copy = base_data;
        if (which_algo == 1) dialsort_counting(copy);
        else parallel_bucket_sort(copy);
    }

    // Mediciones reales
    vector<double> time_ms;
    bool correct = true;

    for (int r = 0; r < MEASURE_ROUNDS; r++) {
        vector<int> copy= base_data;  // copia fresca cada vez

        long long start = current_time_ns();
        if (which_algo == 1) dialsort_counting(copy);
        else parallel_bucket_sort(copy);
        long long finish = current_time_ns();

        double ms = (finish - start) / 1000000.0;  // ns a ms
        time_ms.push_back(ms);

        if (!is_ordered(copy)) correct = false;
    }

    // Calcular estadísticas
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
              << setw(20) << "Algorithm"
              << setw(22) << "Distribution"
              << setw(10) << "N"
              << setw(8)  << "U"
              << setw(14) << "Media(ms)"
              << setw(12) << "Desv(ms)"
              << setw(10) << "Mín(ms)"
              << setw(12) << "Mkeys/s"
              << "OK?\n";
    cout << string(128, '-') << "\n";
}

void print_rows(const Results& r) {
    cout << left << fixed << setprecision(3)
              << setw(20) << r.algorithm
              << setw(18) << r.distribution
              << setw(12) << r.n
              << setw(10)  << r.U
              << setw(12) << r.ms_media
              << setw(11) << r.ms_desv
              << setw(11) << r.ms_min
              << setw(11) << r.mkeys_s
              << (r.correct ? "PASSED" : "*** FAILED ***") << "\n";
}

void print_comparisons(const Results& dial, const Results& pb) {
    double speedup = (pb.ms_media > 0) ? pb.ms_media / dial.ms_media : 0.0;
    cout << "  --> Speedup DialSort vs Parallel Bucket Sort: "
              << fixed << setprecision(2) << speedup << "x";
    if (speedup > 1.0)
        cout << "  (DialSort is FASTER)\n";
    else if (speedup < 1.0)
        cout << "  (Parallel Bucket Sort is FASTER)\n";
    else
        cout << "  (They are pretty identical)\n";
    cout << "\n";
}


// ============================================================
// SECTION 9: EXPORT TO CSV
// ============================================================

void export_csv(const vector<Results>& resultsBench, const string& file_name) {
    ofstream file(file_name);
    if (!file.is_open()) {
        cout << "[AVISO] No se pudo crear " << file_name << "\n";
        return;
    }

    // Encabezado
    file << "Algorithm,distribution,n,U,ms_media,ms_desv,ms_min,Mkeys_s,correct,speedup\n";

    // Recorre de a pares: [0]=DialSort, [1]=Parallel Bucket Sort, [2]=DialSort, [3]=Parallel Bucket Sort...
    for (int i = 0; i + 1 < (int)resultsBench.size(); i += 2) {
        const Results& dial = resultsBench[i];      // fila DialSort
        const Results& pb   = resultsBench[i + 1];  // fila Parallel Bucket Sort

        // Calculo de speedup: cuántas veces más rápido es DialSort
        double speedup = 0.0;
        if (dial.ms_media > 0.0) {
            speedup = pb.ms_media / dial.ms_media;
        }

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

        // Si speedup de DialSort es 2.0, el de Parallel Bucket sort es 0.5
        double speedup_pb = 0.0;
        if (speedup > 0.0) {
            speedup_pb = 1.0 / speedup;
        }

        file << fixed << setprecision(4)
             << pb.algorithm    << ","
             << pb.distribution << ","
             << pb.n            << ","
             << pb.U            << ","
             << pb.ms_media     << ","
             << pb.ms_desv      << ","
             << pb.ms_min       << ","
             << pb.mkeys_s      << ","
             << (pb.correct ? "PASSED" : "FAILED") << ","
             << speedup_pb      << "\n";  // speedup de CountingSort (siempre <= 1.0)
    }

    file.close();
    cout << "\n[OK] Results exported to: " << file_name << "\n";
}


// ============================================================
// SECCIÓN 10: EXPERIMENTO PRINCIPAL
// ============================================================

int main() {

    cout << "================================================================\n";
    cout << " BENCHMARK: DialSort-Counting  vs Parallel Bucket Sort\n";
    cout << " Warmup rounds           : " << WARMUP_ROUNDS << "\n";
    cout << " Measurement rounds      : " << MEASURE_ROUNDS << "\n";
    cout << " Random seed             : " << SEED << "\n";
    cout << "================================================================\n";

    // Dimensiones
    // n: tamaños de entrada
    vector<int> sizes = {100000, 500000, 1000000, 5000000, 10000000};

    // U: tamaños del universo
    const vector<int> Us = {256, 1024, 65536};

    // Distribuciones a probar
    string distributions[]      = {"Uniform", "Biased", "Nearly Uniform"};

    // Recolector de todos los resultados para el CSV
    vector<Results> all_results;

    // Bucle principal
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
                Results r_dial = measure("DialSort              ", name_dist, data, U, 1);

                // Medir Parallel Bucket sort
                Results r_cs   = measure("Parallel Bucket Sort  ", name_dist, data, U, 2);

                print_rows(r_dial);
                print_rows(r_cs);
                print_comparisons(r_dial, r_cs);

                all_results.push_back(r_dial);
                all_results.push_back(r_cs);
            }
        }
    }

    // Resumen global
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
    cout << " Parallel Bucket Sort wins in     : " << count_cs_wins   << " out of " << total << " cases\n";
    cout << " Average  Speedup          : " << speedup_prom    << "x\n";
    cout << " Better speedup of DialSort : " << better_speedup   << "x  (" << better_config << ")\n";
    cout << string(108, '=') << "\n";

    export_csv(all_results, "results_benchmark.csv");
    return 0;
}