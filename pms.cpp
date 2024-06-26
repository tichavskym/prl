// 1st PRL project - Pipeline Merge Sort
// Author: Milan Tichavský <xticha09>

#include "mpi.h"
#include <cstdio>
#include <cstdlib>
#include <queue>

#ifdef DEBUG
#define debug(...) \
    do {           \
        if (DEBUG) { \
            fprintf(stderr, "%s:%d: ", __FILE__, __LINE__);  \
            fprintf(stderr, __VA_ARGS__);                    \
        } \
    } while (0);
#else
#define debug(...) do {} while(0);
#endif

// Each process (except the first one and the last one) has two input and two output channels, which are modeled
// by LEFT and RIGHT tags in MPI_Send/MPI_Recv functions.
//
// After sending a sorted queue, every process sends -1 integer to signal the end of queue. This is not really
// necessary, because the information about the end of the queue can be inferred from the rank of the process and
// the number of numbers already received, but it marks the end of queue more clearly. Combined with sending -2 when
// all the input data have been sent, it makes it easier to tell processes to stop expecting any more numbers, and
// finish the program execution.
//
// Each process creates one std::queue<int>. This is also not really necessary, because MPI_Send/MPI_Recv have
// internal buffers that act as queues which would be enough. However, to demonstrate that the sorting starts
// only after the whole queue have been received, the queue from the LEFT channel is copied into this internal queue
// and once the RIGHT channel receives the first number, the sorting starts.


enum tags {LEFT, RIGHT};

void switch_tag(enum tags *t) {
    if (*t == LEFT) {
        *t = RIGHT;
    } else {
        *t = LEFT;
    }
}

// dry_run is needed by the last processor, which should only print the data, not send it to some other process
void send(int number, int sending_rank, const tags &sending_tag, bool dry_run) {
    if (dry_run) {
        printf("%d\n", number);
    } else {
        MPI_Send(&number, 1, MPI_INT, sending_rank, sending_tag, MPI_COMM_WORLD);
    }
}

// Send all remaining data in the queue to the next process
void flush_the_queue(std::queue<int> &q, int sending_rank, const tags sending_tag, bool dry_run) {
    while (!q.empty()) {
        send(q.front(), sending_rank, sending_tag, dry_run);
        q.pop();
    }
    if (!dry_run) {
        int number = -1;
        send(number, sending_rank, sending_tag, dry_run);
    }
}

void merge_sort_step(int rank, int &number, const tags &receive_tag, const tags &sending_tag, std::queue<int> &left, int dry_run) {
    if (receive_tag == LEFT) {
        left.push(number);
    } else {
        while (!left.empty() && left.front() < number) {
            int s = left.front();
            left.pop();
            send(s, rank + 1, sending_tag, dry_run);
        }
        send(number, rank + 1, sending_tag, dry_run);
    }
}

int process_received_num(int rank, tags &sending_tag, std::queue<int> &left, int &number, tags &receive_tag) {
    if (number == -1) {
        if (receive_tag == RIGHT) {
            flush_the_queue(left, rank + 1, sending_tag, false);
            switch_tag(&sending_tag);
        }
        switch_tag(&receive_tag);
    } else if (number == -2) {
        flush_the_queue(left, rank + 1, sending_tag, false);
        MPI_Send(&number, 1, MPI_INT, rank + 1, RIGHT, MPI_COMM_WORLD);
        MPI_Send(&number, 1, MPI_INT, rank + 1, LEFT, MPI_COMM_WORLD);
        return 1;
    } else {
        merge_sort_step(rank, number, receive_tag, sending_tag, left, false);
    }
    return 0;
}

int last_rank_process_received_num(int rank, tags &sending_tag, std::queue<int> &left, int &number, tags &receive_tag) {
    if (number == -1) {
        if (receive_tag == RIGHT) {
            flush_the_queue(left, rank + 1, sending_tag, true);
            switch_tag(&sending_tag);
        }
        switch_tag(&receive_tag);
    } else if (number == -2) {
        return 1;
    } else {
        merge_sort_step(rank, number, receive_tag, sending_tag, left, true);
    }
    return 0;
}

int run() {
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
        int receiver_rank = 1;
        enum tags tag = RIGHT;
        while (fread(&number_as_char, sizeof(unsigned char), 1, f) == 1) {
            int number = number_as_char;
            printf("%d ", number);
            switch_tag(&tag);
            debug("(%d -> %d) Sending number %d...\n", 0, 1, number)
            MPI_Send(&number, 1, MPI_INT, receiver_rank, tag, MPI_COMM_WORLD);
            // Mark the end of the queue
            number = -1;
            debug("(%d -> %d) Sending number %d...\n", 0, 1, number)
            MPI_Send(&number, 1, MPI_INT, receiver_rank, tag, MPI_COMM_WORLD);
        }
        printf("\n");
        // Mark that the all data were already sent
        int number = -2;
        debug("(%d -> %d) Sending 2x number %d...\n", 0, 1, number)
        MPI_Send(&number, 1, MPI_INT, receiver_rank, RIGHT, MPI_COMM_WORLD);
        MPI_Send(&number, 1, MPI_INT, receiver_rank, LEFT, MPI_COMM_WORLD);
        fclose(f);
    } else {
        int number;
        enum tags receive_tag = LEFT;
        enum tags sending_tag = LEFT;
        std::queue<int> left;
        int sender_rank = rank - 1;

        while (true) {
            MPI_Status status;
            MPI_Recv(&number, 1, MPI_INT, sender_rank, receive_tag, MPI_COMM_WORLD, &status);
            debug("(%d <- %d) Received number %d...\n", rank, sender_rank, number)

            if (rank == (size - 1)) {
                if (last_rank_process_received_num(rank, sending_tag, left, number, receive_tag) == 1) {
                    return 0;
                }
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
