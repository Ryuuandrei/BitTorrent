#include <vector>
#include <unordered_map>
#include <string>
#include <bits/stdc++.h>
#include <inttypes.h>
#include <mpi.h>

#define TRACKER_RANK 0
#define MAX_FILES 10
#define MAX_FILENAME 15
#define HASH_SIZE 32
#define MAX_CHUNKS 100

void print_map(std::unordered_map<std::string, std::vector<std::string>> map)
{
    for (const auto& e : map)
    {
        std::cout << e.first << "\n";
        for (const auto& hash : e.second)
            std::cout << hash << "\n";
    }
}

struct peer_data {
    // file_name -> hashes
    std::unordered_map<std::string, std::vector<std::string>> owned_files;

    std::vector<std::string> wanted_files;

    peer_data(const std::string& file_name) {
        std::ifstream in(file_name);
        std::string file;
        int n, count;
        in >> n;
        for (int i = 0; i < n; i++) 
        {
            in >> file >> count;
            owned_files[file].resize(count);
            for (int j = 0; j < count; j++)
                in >> owned_files[file][j];
        }
        in >> n;
        wanted_files.resize(n);
        for (int i = 0; i < n; i++)
            in >> wanted_files[i];
    }

    void send_tracker_owned_files()
    {
        int num_files = owned_files.size();
        MPI_Send(&num_files, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD);
        for (const auto& e : owned_files)
        {
            MPI_Send(e.first.c_str(), e.first.size(), MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD);
            int chunks = e.second.size();
            MPI_Send(&chunks, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD);
            for (const auto& hash : e.second)
                MPI_Send(hash.c_str(), hash.size() + 1, MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD);
        }
        char ack[4];
        MPI_Recv(ack, 4, MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    void recv_peers(std::unordered_map<int, std::vector<std::string>>& peers)
    {
        int num_peers, num_hashes, peer;
        char hash[HASH_SIZE + 1];
        MPI_Recv(&num_peers, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (int i = 0; i < num_peers; ++i)
        {
            MPI_Recv(&peer, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&num_hashes, 1, MPI_INT, TRACKER_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            peers[peer].resize(num_hashes);
            for (int j = 0; j < num_hashes; ++j)
            {
                MPI_Recv(hash, HASH_SIZE + 1, MPI_CHAR, TRACKER_RANK, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                peers[peer][j] = hash;
            }
        }
    }
};

struct tracker_data
{
    // file_name -> hashes
    std::unordered_map<std::string, std::vector<std::string>> files;

    //file_name -> (client -> hashes) 
    std::unordered_map<std::string, std::unordered_map<int, std::vector<std::string>>> swarm;

    void recv_files_from_client(int rank)
    {
        int owned_files;
        MPI_Recv(&owned_files, 1, MPI_INT, rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (int k = 0; k < owned_files; ++k)
        {
            char file_name[MAX_FILENAME], hash[HASH_SIZE + 1];
            int chunks;
            MPI_Recv(file_name, MAX_FILENAME, MPI_CHAR, rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&chunks, 1, MPI_INT, rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            swarm[file_name][rank].resize(chunks);
            for (int j = 0; j < chunks; ++j)
            {
                MPI_Recv(hash, HASH_SIZE + 1, MPI_CHAR, rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                swarm[file_name][rank][j] = hash;
            }
        }
        MPI_Send("ACK", 4, MPI_CHAR, rank, 0, MPI_COMM_WORLD);
    }

    void send_peers(const std::string& file_name, int dest)
    {
        int count = swarm[file_name].size();
        MPI_Send(&count, 1, MPI_INT,  dest, 0, MPI_COMM_WORLD);
        for (const auto& e : swarm[file_name])
        {
            MPI_Send(&e.first, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
            int num_hashes = e.second.size();
            MPI_Send(&num_hashes, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
            for (const auto& hash : e.second)
                MPI_Send(hash.c_str(), hash.size() + 1, MPI_CHAR, dest, 0, MPI_COMM_WORLD);
        }
    }
};