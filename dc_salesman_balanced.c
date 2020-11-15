
// Brute force and heuristic to the Salesman problem
// Calculate the cylle from one city (Karlsruhe) to all others and back

/*--------------------------- INCLUDES ---------------------------*/

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

/*---------------------------- DEFINES ---------------------------*/

#define NUM_CITIES 12
#define KILL 1
#define DELTA 44

/*-------------------------- STRUCTURES --------------------------*/

typedef struct {
                int x;
                int y;
               } COORD;

typedef struct {
                char name[15];
                COORD coord;
                } CITY;
                
typedef int SOLUTION[NUM_CITIES];
                
/*--------------------------- VARIABLES --------------------------*/

CITY cities[NUM_CITIES] = {
                           {"Karlsruhe", {   0,   0}},
                           {"Koeln"    , { -92, 183}},
                           {"Frankfurt", {  17, 106}},
		                   {"Muenchen" , { 200, -86}},
                           {"Freiburg" , { -36,-108}},
                           {"Dresden"  , { 331, 197}},
                           {"Berlin"   , { 311, 344}},
                           {"Hannover" , {  81, 331}},
                           {"Hamburg"  , {  95, 450}},
                           {"Stuttgart", {  47, -22}},
                           {"Ulm",       { 100, -58}},
                           {"Nurenberg", { 164,  42}}  // 12 - 25.00 seconds to calculate solutions
                           /*{"Bremmen",   {  25, 403}},  // 13 - 303.00 seconds to calculate solutions (5 minutes)
                           {"Dortmund",  { -58, 244}}   // 14 - 4258.00 seconds to calculate solutions (70 minutes)*/
                          }; // horizontal and vertical distances in Km
   
                          
SOLUTION final_solution;    /* Final Order of the Cities  */ 
double best_result = 100000L; /* Final cost (Global Minima) */                    
                         
/*--------------------------- PROTOTYPES -------------------------*/

double calc_dist( COORD point1, COORD point2 );
double calc_total_dist( SOLUTION solution ); 
void print_indexes ( SOLUTION solution );
void print_names ( SOLUTION solution );
unsigned int fat ( int number ); 
void perm_solution ( SOLUTION solution, int first, int last );
                            

double calc_dist( COORD point1, COORD point2 ) {
    double distx, disty, result;
  
    distx = point1.x - point2.x;
    disty = point1.y - point2.y;
    result = sqrt (pow(distx,2) + pow(disty,2));
    return( result );
} /* calc_dist */


double calc_total_dist( SOLUTION solution ) {
    int i;
    double total = 0L;
 
    for ( i = 0 ; i < NUM_CITIES-1; i++ )   // sum distance from each city from this solution to the next one
        total += calc_dist(cities[solution[i]].coord, cities[solution[i+1]].coord);
    total += calc_dist(cities[solution[i]].coord, cities[solution[0]].coord); // sum distance from last city to the first
    return(total);
} /* calc_total_dist */


void print_indexes ( SOLUTION solution ) {
    int i;
 
    for ( i = 0 ; i < NUM_CITIES ; i++ )
        printf("[%d] ", solution[i]);
    printf("\n");

} /* print_indexes */

void print_names ( SOLUTION solution ) {
    int i;
 
    for ( i = 0 ; i < NUM_CITIES ; i++ )
        printf("%s -> ",cities[solution[i]].name);
    printf("%s\n",cities[solution[0]].name);

} /* print_names */

void perm_solution ( SOLUTION solution, int first, int last ) {
    int i, j,temp; 
    double custo;

    if (first == last) {
        //print_indexes(solution);
        custo = calc_total_dist(solution);
        //printf("\nCusto %3.2f\n", custo );
        if ( custo < best_result ) {
            best_result = custo;
            for ( j=0 ; j < NUM_CITIES ; j++ )
                final_solution[j] = solution[j];
            }
        }
    else { 
        for (i = first; i <= last ; i++) { 
            // swap((a + l), (a + i)) 
            temp = solution[first];  
            solution[first] = solution[i];
            solution[i] = temp;
            perm_solution (solution, first+1, last); // permute(a, l + 1, r); 
            // swap((a + l), (a + i))
            temp = solution[first]; 
            solution[first] = solution[i];
            solution[i] = temp;
        } 
    } 
} 
  

/*----------------------------------------------------------------*
|                              fat                                |
*-----------------------------------------------------------------*/

unsigned int fat ( int number ) {
    unsigned int soma;
    int i;

    soma = number;
    for ( i = number-1 ; i > 1 ; i-- )
        soma *= i;
    return ( soma );
}  /* fat */

int **alloc_2d_int(int rows, int cols) {
    int *data = (int *)malloc(rows*cols*sizeof(int));
    int **array= (int **)malloc(rows*sizeof(int*));
    int i;
    for (i=0; i<rows; i++)
        array[i] = &(data[cols*i]);

    return array;
}

/*----------------------------------------------------------------*
|                               main                              |
*-----------------------------------------------------------------*/
                                
int main(int argc, char **argv) {

    int i, j, k, l, m, w, kills;
    int my_rank, proc_n, work_size;
    double t1, t2;
    int **received = alloc_2d_int(NUM_CITIES*(NUM_CITIES-1), NUM_CITIES);

    MPI_Status status;
    SOLUTION best; // melhor solução até o momento
    int rank_pai; // rank do processo pai
    double best_cost = 100000L;

    MPI_Init (&argc , & argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &proc_n);

    if(my_rank == 0) // sou a raiz
    {
        t1 = MPI_Wtime(); // capturo o tempo de início
        SOLUTION initial_solution;
        for(i = 0; i < NUM_CITIES; i++)
        {
            initial_solution[i] = i; // inicializo a solução inicial {0, 1, 2, 3, ...}
        }
        double best_cost = 100000L; // inicialização do melhor custo
        double returned_cost; // custo da solução retornada pelo escravo
        w = 0; // inicializo o índice do saco de trabalho
        // montando o workbag
        for(i = 0; i < NUM_CITIES; i++)
        {
            for(j = 0; j < NUM_CITIES; j++)
            {
                if(i == j) continue;
                received[w][0] = initial_solution[i];
                received[w][1] = initial_solution[j];
                l = 2;
                m = 0;
                while(l < NUM_CITIES)
                {
                    if(m == i || m == j)
                    {
                        m++;
                        continue;
                    }
                    received[w][l] = m; // o resto vai com o que sobrou
                    l++;
                    m++;
                }
                w++;
            }
        }
        work_size = NUM_CITIES*(NUM_CITIES-1); // salvo o tamanho do meu vetor de trabalho
        
    }
    else // não sou a raiz
    {
        if(my_rank % 2 == 1) // meu rank é ímpar -> sou o filho da esquerda
        {
            rank_pai = (my_rank-1)/2;
        }
        else // meu rank é par -> sou o filho da direita
        {
            rank_pai = (my_rank-2)/2;
        }
        MPI_Recv(&received[0][0], NUM_CITIES*NUM_CITIES*(NUM_CITIES-1), MPI_INT, rank_pai, MPI_ANY_TAG, MPI_COMM_WORLD, &status); // recebe tarefa do mestre
        MPI_Get_count(&status, MPI_INT, &work_size);
        work_size = work_size/NUM_CITIES;
    }

    // decido se divido ou conquisto
    if(work_size <= DELTA) // é hora de conquistar
    {
        double dist;
        for(i = 0; i < work_size; i++)
        {
            perm_solution(received[i], 2, NUM_CITIES-1);
            dist = calc_total_dist(final_solution);
            if(dist < best_cost)
            {
                best_cost = dist;
                for(j = 0; j < NUM_CITIES; j++)
                {
                    best[j] = final_solution[j];
                }
            }
        }
        
    }
    else // não é hora de conquistar -> devo dividir
    {
        int work_size_to_send = work_size/3;
        int **work_to_left = alloc_2d_int(work_size_to_send, NUM_CITIES);
        int **work_to_right = alloc_2d_int(work_size_to_send, NUM_CITIES);
        SOLUTION left_received;
        SOLUTION right_received;
        int right_rank, left_rank;
        left_rank = (my_rank*2)+1;
        right_rank = (my_rank*2)+2;
        for(i = 0; i < work_size_to_send; i++)
        {
            for(j = 0; j < NUM_CITIES; j++)
            {
                work_to_left[i][j] = received[i][j];
            }
        }
        for(i = 0; i < work_size_to_send; i++)
        {
            for(j = 0; j < NUM_CITIES; j++)
            {
                work_to_right[i][j] = received[i+work_size_to_send][j];
            }
        }
        MPI_Send(&work_to_left[0][0], work_size_to_send*NUM_CITIES, MPI_INT, left_rank, 0, MPI_COMM_WORLD); // mando pro filho da esquerda
        MPI_Send(&work_to_right[0][0], (work_size_to_send)*NUM_CITIES, MPI_INT, right_rank, 0, MPI_COMM_WORLD); // mando pro filho da direita

        MPI_Recv(&left_received, NUM_CITIES, MPI_INT, left_rank, MPI_ANY_TAG, MPI_COMM_WORLD, &status); // recebe tarefa do filho esquerdo
        MPI_Recv(&right_received, NUM_CITIES, MPI_INT, right_rank, MPI_ANY_TAG, MPI_COMM_WORLD, &status); // recebe tarefa do filho direito

        if(calc_total_dist(right_received) < calc_total_dist(left_received))
        {
            for(i = 0; i < NUM_CITIES; i++)
            {
                best[i] = right_received[i];
            }
            best_cost = calc_total_dist(right_received);
        }
        else
        {
            for(i = 0; i < NUM_CITIES; i++)
            {
                best[i] = left_received[i];
            }
            best_cost = calc_total_dist(left_received);
        }

        double dist;
        for(i = 0; i < work_size-work_size_to_send*2; i++)
        {
            perm_solution(received[i+work_size_to_send*2], 2, NUM_CITIES-1);
            dist = calc_total_dist(final_solution);
            if(dist < best_cost)
            {
                best_cost = dist;
                for(j = 0; j < NUM_CITIES; j++)
                {
                    best[j] = final_solution[j];
                }
            }
        }
        
    }  

    if(my_rank == 0) // sou a raiz
    {
        t2 = MPI_Wtime(); // capturo o tempo de fim
        printf("\nMinimal for brute force solution: %3.2f Km roundtrip.\n\n", best_cost );
        //print_indexes (final_solution);   
        print_names (best);
        printf("\nTime: %lf seconds\n\n", t2-t1);
    }
    else // não sou a raiz
    {
        MPI_Send(&best, NUM_CITIES, MPI_INT, rank_pai, 0, MPI_COMM_WORLD); // mando pro pai
    }  
        
    MPI_Finalize();
}                                
                                
                        