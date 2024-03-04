#include "mpi.h"
#include <cstdio>
#include <cstdlib>
#include <queue>

// TODO what if i have even/odd amount of numbers
enum tags {LEFT, RIGHT};


void switch_tag(enum tags *t) {
    if (*t == LEFT) {
        *t = RIGHT;
    } else {
        *t = LEFT;
    }
}

void flush_the_queue(std::queue<int> q, int sending_rank, int sending_tag) {
    while (!q.empty()) {
        MPI_Send(&q.front(), 1, MPI_INT, sending_rank, sending_tag, MPI_COMM_WORLD);
        q.pop();
    }
    int number = -1;
    MPI_Send(&number, 1, MPI_INT, sending_rank, sending_tag, MPI_COMM_WORLD);
}

void merge_sort_step(int rank, int &number, const tags &receive_tag, const tags &sending_tag, std::queue<int> &left) {
    if (receive_tag == LEFT) {
        left.push(number);
    } else {
        while (!left.empty() && left.front() < number) {
            int send = left.front();
            left.pop();
            MPI_Send(&send, 1, MPI_INT, rank + 1, sending_tag, MPI_COMM_WORLD);
        }
        MPI_Send(&number, 1, MPI_INT, rank + 1, sending_tag, MPI_COMM_WORLD);
    }
}

int process_received_num(int rank, tags &sending_tag, std::queue<int> &left, int &number, tags &receive_tag) {
    if (number == -1) {
        if (receive_tag == RIGHT) {
            flush_the_queue(left, rank + 1, sending_tag);
            switch_tag(&sending_tag);
        }
        switch_tag(&receive_tag);
    } else if (number == -2) {
        MPI_Send(&number, 1, MPI_INT, rank + 1, RIGHT, MPI_COMM_WORLD);
        MPI_Send(&number, 1, MPI_INT, rank + 1, LEFT, MPI_COMM_WORLD);
        return 1;
    } else {
        merge_sort_step(rank, number, receive_tag, sending_tag, left);
    }
    return 0;
}

int run() {
    printf("Hello, world! \n");
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
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
    } else { // TODO debugging purposes
        int number;
        enum tags receive_tag = LEFT;
        enum tags sending_tag = LEFT;
        std::queue<int> left;

        while (true) {
            MPI_Status status; // TODO check for status?
            int sender_rank = rank - 1;
            MPI_Recv(&number, 1, MPI_INT, sender_rank, receive_tag, MPI_COMM_WORLD, &status);
            printf("(%d <- %d) Received number %d...\n", rank, sender_rank, number);

            if (rank == size - 1) {
                return 0;
            } else {
                if (process_received_num(rank, sending_tag, left, number, receive_tag) == 1) {
                    return 0;
                }
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
