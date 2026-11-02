#pragma once
#include <string>
#include <vector>
#include <sqlite3.h>

struct SaveSlotData {
    int slot_id = 1;
    std::string player_name = "Player1";
    int maze_w = 15;
    int maze_h = 10;
    int seed = 42;
    float player_x = 1.5f;
    float player_y = 0.5f;
    float player_z = 1.5f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    int health = 100;
    float stamina = 100.0f;
    int keys_mask = 0; // bit 0: bronze, 1: silver, 2: gold
    float time_elapsed = 0.0f;
    int score = 0;
    std::string timestamp = "";
    bool is_empty = true;
};

struct SessionRecord {
    std::string player_name = "Player1";
    std::string mode = "STANDARD"; // STANDARD, TIME_TRIAL, SURVIVAL
    int difficulty = 1;
    std::string outcome = "ESCAPED"; // ESCAPED, DIED, TIMEOUT, QUIT
    int duration_ms = 0;
    int score = 0;
    int keys_collected = 0;
    int monsters_evaded = 0;
};

struct LeaderboardEntry {
    int rank = 1;
    std::string player_name;
    std::string mode;
    int score;
    int duration_ms;
    int keys_collected;
    std::string outcome;
    std::string timestamp;
};

class Database {
public:
    Database();
    ~Database();

    bool open(const std::string& db_path = "data/mazerunner.db");
    void close();

    bool initSchema();
    bool saveSlot(int slot_id, const SaveSlotData& data);
    bool loadSlot(int slot_id, SaveSlotData& out_data);
    std::vector<SaveSlotData> getAllSlots();
    
    bool recordSession(const SessionRecord& session);
    std::vector<LeaderboardEntry> getLeaderboard(int limit = 10);
    void printLeaderboard(int limit = 10);

    bool logTelemetry(int session_id, float time_s, float px, float py, float pz, float fps);

private:
    sqlite3* db_ = nullptr;
    std::string db_path_;
};
