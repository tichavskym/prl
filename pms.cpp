#include "mpi.h"
#include <cstdio>
#include <cstdlib>

enum tags {LEFT, RIGHT};

int run(){
    printf("Hello, world! \n");
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0) {
        FILE *f = fopen("numbers", "rb");
        if (f == nullptr) {
            perror("Error while opening a file.");
            return 1;
        }

        unsigned char number_as_char;
        int i = 0;
        int receiver_rank = 1;
        while (fread(&number_as_char, sizeof(unsigned char), 1, f) == 1) {
            int number = number_as_char;
            enum tags tag;
            if (i % 2 == 0) {
                tag = LEFT;
            } else {
                tag = RIGHT;
            }
            printf("(%d -> %d) Sending number %d...\n", 0, 1, number);
            MPI_Send(&number, 1, MPI_INT, receiver_rank, tag, MPI_COMM_WORLD);
            // TODO do I wanna keep it this way or make it alternate?
            number = -1;
            printf("(%d -> %d) Sending number %d...\n", 0, 1, number);
            MPI_Send(&number, 1, MPI_INT, receiver_rank, tag, MPI_COMM_WORLD);
            i++;
        }
        int number = -2;
        MPI_Send(&number, 1, MPI_INT, receiver_rank, RIGHT, MPI_COMM_WORLD);
        MPI_Send(&number, 1, MPI_INT, receiver_rank, LEFT, MPI_COMM_WORLD);
        fclose(f);
    } else if (rank == 1) {
        int i = 0;
        int number;
        enum tags tag;
        while (true) {
            if (i < 2) {
                tag = LEFT;
            } else if (i == 3) {
                tag = RIGHT;
                i = 0;
            } else {
                tag = RIGHT;
            }
            int sender_rank = 0;
            MPI_Status status; // TODO check for status?
            MPI_Recv(&number, 1, MPI_INT, sender_rank, tag, MPI_COMM_WORLD, &status);
            printf("(%d -> %d) Received number %d...\n", 1, 0, number);
            if (number == -2) {
                return 0;
            }
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    MPI_Init (&argc, &argv);
    int ret = run();
    MPI_Finalize();
    return ret;
}
