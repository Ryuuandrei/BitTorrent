#include <pthread.h>
#include <iostream>
#include "tema3.h"
#include <thread>
#include <bits/stdc++.h>

void download_thread_func(peer_data& data, int rank)
{
    int num_peers, peer, num_hashes;
    MPI_Status status;
    int request_type = 0;
    int max = 0, seed;
    char hash[HASH_SIZE + 1];
    char ok[3] = {0};
    int downloaded_segments = 0;
    std::vector<std::string> current_file_hashes;

    for (const auto& wanted_file : data.wanted_files)
    {
        std::unordered_map<int, std::vector<std::string>> peers;
        request_type = 0;
        MPI_Send(&request_type, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD);

        MPI_Send(wanted_file.c_str(), wanted_file.size() + 1, MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD);
        
        MPI_Recv(&num_hashes, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        current_file_hashes.resize(num_hashes);

        for (int i = 0; i < num_hashes; ++i)
        {
            MPI_Recv(hash, HASH_SIZE + 1, MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            current_file_hashes[i] = hash;
        }

        data.recv_peers(peers);
        for (const auto& hash : current_file_hashes)
        {
            if (downloaded_segments > 0 && downloaded_segments % 10 == 0)
            {
                request_type = 1;
                MPI_Send(&request_type, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD);

                data.send_tracker_owned_files();
                data.recv_peers(peers);
            }

            std::vector<int> good_peers;
            for (const auto& e : peers)
                if (std::find(e.second.begin(), e.second.end(), hash) != e.second.end())
                    good_peers.push_back(e.first);

            peer = good_peers[rand() % good_peers.size()];
            MPI_Send(hash.c_str(), HASH_SIZE + 1, MPI_CHAR, peer, 1, MPI_COMM_WORLD);
            MPI_Recv(ok, 2, MPI_CHAR, peer, 2, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            if (strcmp(ok, "OK") == 0) {
                downloaded_segments++;
                data.owned_files[wanted_file].push_back(hash);
            }
        }

        request_type = 2;
        MPI_Send(&request_type, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD);
        std::ofstream out("client" + std::to_string(rank) + "_" + wanted_file);
        for (const auto& hash : data.owned_files[wanted_file])
            out << hash << "\n";
    }
    request_type = 3;
    MPI_Send(&request_type, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD);
    return;
}

void upload_thread_func(peer_data& data, int rank)
{
    while (true)
    {
        MPI_Status status;
        char hash[HASH_SIZE + 1];   
        MPI_Recv(hash, HASH_SIZE + 1, MPI_CHAR, MPI_ANY_SOURCE, 1, MPI_COMM_WORLD, &status);
        if (strcmp(hash, "SHUTDOWN") == 0)
            return;
        
        for (const auto& file : data.owned_files)
        {
            if (std::find(file.second.begin(), file.second.end(), hash) != file.second.end())
            {
                MPI_Send("OK", 2, MPI_CHAR, status.MPI_SOURCE, 2, MPI_COMM_WORLD);
                continue;   
            }
        }
    }
}

void tracker(int numtasks, int rank)
{
    tracker_data data;
    for (int i = 1; i < numtasks; ++i)
    {
        int owned_files;
        MPI_Recv(&owned_files, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (int k = 0; k < owned_files; ++k)
        {
            char file_name[MAX_FILENAME], hash[HASH_SIZE + 1];
            int chunks;
            MPI_Recv(file_name, MAX_FILENAME, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&chunks, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            data.files[file_name].resize(chunks);
            data.swarm[file_name][i].resize(chunks);
            for (int j = 0; j < chunks; ++j)
            {
                MPI_Recv(hash, HASH_SIZE + 1, MPI_CHAR, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                data.files[file_name][j] = hash;
                data.swarm[file_name][i][j] = hash;
			}
        }
    }

    for (int i = 1; i < numtasks; ++i)
        MPI_Send("ACK", 4, MPI_CHAR, i, 0, MPI_COMM_WORLD);

    MPI_Status status;
    int tasks = numtasks - 1;
    char message[MAX_FILENAME];
    int request_type, count;

    std::vector<std::string> downloading_file;
    downloading_file.resize(numtasks);

    while (tasks)
    {
        MPI_Recv(&request_type, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
        switch (request_type)
        {
        // cerere de fisier
        case 0:
            MPI_Recv(message, MAX_FILENAME, MPI_CHAR, status.MPI_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            downloading_file[status.MPI_SOURCE] = message;
            count = data.files[message].size();
            MPI_Send(&count, 1, MPI_INT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);
            for (const auto& hash : data.files[message])
                MPI_Send(hash.c_str(), HASH_SIZE + 1, MPI_CHAR, status.MPI_SOURCE, 0, MPI_COMM_WORLD);

            data.send_peers(message, status.MPI_SOURCE);
            break;
            
        case 1:
            data.recv_files_from_client(status.MPI_SOURCE);
            data.send_peers(downloading_file[status.MPI_SOURCE], status.MPI_SOURCE);
            break;
        case 2:
            data.swarm[downloading_file[status.MPI_SOURCE]][status.MPI_SOURCE] = data.files[downloading_file[status.MPI_SOURCE]];
            break;
        case 3:
            tasks--;
            break;
            
        default:
            break;
        }
    }
    for (int i = 1; i < numtasks; ++i)
        MPI_Send("SHUTDOWN", 9, MPI_CHAR, i, 1, MPI_COMM_WORLD);
}

void peer(int numtasks, int rank) {

    peer_data data("in" + std::to_string(rank) + ".txt");
    data.send_tracker_owned_files();

    std::thread download_thread(download_thread_func, std::ref(data), rank);
    std::thread upload_thread(upload_thread_func, std::ref(data), rank);

    download_thread.join();
    upload_thread.join();
}
 
int main (int argc, char *argv[]) {
    int numtasks, rank;
 
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
    if (provided < MPI_THREAD_MULTIPLE) {
        std::cerr << "MPI nu are suport pentru multi-threading\n";
        exit(-1);
    }
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == TRACKER_RANK) {
        tracker(numtasks, rank);
    } else {
        peer(numtasks, rank);
    }

    MPI_Finalize();
}
