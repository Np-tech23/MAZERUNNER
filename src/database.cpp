#include "database.h"
#include <iostream>
#include <iomanip>
#include <sstream>

Database::Database() : db_(nullptr) {}

Database::~Database() {
    close();
}

bool Database::open(const std::string& db_path) {
    db_path_ = db_path;
    int rc = sqlite3_open(db_path_.c_str(), &db_);
    if (rc != SQLITE_OK) {
        std::cerr << "[Database] Failed to open database: " << sqlite3_errmsg(db_) << "\n";
        db_ = nullptr;
        return false;
    }

    // Enable WAL mode & synchronous = NORMAL for optimal throughput and durability
    char* err_msg = nullptr;
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, &err_msg);
    sqlite3_exec(db_, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, &err_msg);
    sqlite3_exec(db_, "PRAGMA foreign_keys=ON;", nullptr, nullptr, &err_msg);

    return initSchema();
}

void Database::close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool Database::initSchema() {
    if (!db_) return false;

    const char* schema = R"(
        CREATE TABLE IF NOT EXISTS players (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT UNIQUE NOT NULL,
            char_class TEXT DEFAULT 'RUNNER',
            created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            last_played_at DATETIME DEFAULT CURRENT_TIMESTAMP,
            playtime_s INTEGER DEFAULT 0
        );

        CREATE TABLE IF NOT EXISTS save_slots (
            slot_id INTEGER PRIMARY KEY,
            player_name TEXT NOT NULL,
            maze_w INTEGER NOT NULL,
            maze_h INTEGER NOT NULL,
            seed INTEGER NOT NULL,
            player_x REAL NOT NULL,
            player_y REAL NOT NULL,
            player_z REAL NOT NULL,
            yaw REAL NOT NULL,
            pitch REAL NOT NULL,
            health INTEGER NOT NULL,
            stamina REAL NOT NULL,
            keys_mask INTEGER NOT NULL,
            time_elapsed REAL NOT NULL,
            score INTEGER NOT NULL,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS sessions (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            player_name TEXT NOT NULL,
            mode TEXT NOT NULL,
            difficulty INTEGER DEFAULT 1,
            outcome TEXT NOT NULL,
            duration_ms INTEGER NOT NULL,
            score INTEGER NOT NULL,
            keys_collected INTEGER NOT NULL,
            monsters_evaded INTEGER NOT NULL,
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
        );

        CREATE TABLE IF NOT EXISTS telemetry_logs (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            session_id INTEGER,
            time_s REAL,
            px REAL, py REAL, pz REAL,
            fps REAL
        );

        CREATE VIEW IF NOT EXISTS v_leaderboard AS
        SELECT 
            ROW_NUMBER() OVER (ORDER BY score DESC, duration_ms ASC) AS rank,
            player_name,
            mode,
            score,
            duration_ms,
            keys_collected,
            outcome,
            timestamp
        FROM sessions
        WHERE outcome = 'ESCAPED' OR outcome = 'VICTORY';
    )";

    char* err_msg = nullptr;
    int rc = sqlite3_exec(db_, schema, nullptr, nullptr, &err_msg);
    if (rc != SQLITE_OK) {
        std::cerr << "[Database] Schema creation failed: " << (err_msg ? err_msg : "unknown") << "\n";
        if (err_msg) sqlite3_free(err_msg);
        return false;
    }
    return true;
}

bool Database::saveSlot(int slot_id, const SaveSlotData& data) {
    if (!db_) return false;

    const char* sql = R"(
        INSERT OR REPLACE INTO save_slots (
            slot_id, player_name, maze_w, maze_h, seed,
            player_x, player_y, player_z, yaw, pitch,
            health, stamina, keys_mask, time_elapsed, score, timestamp
        ) VALUES (
            ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP
        );
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[Database] Failed to prepare save statement: " << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    sqlite3_bind_int(stmt, 1, slot_id);
    sqlite3_bind_text(stmt, 2, data.player_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, data.maze_w);
    sqlite3_bind_int(stmt, 4, data.maze_h);
    sqlite3_bind_int(stmt, 5, data.seed);
    sqlite3_bind_double(stmt, 6, data.player_x);
    sqlite3_bind_double(stmt, 7, data.player_y);
    sqlite3_bind_double(stmt, 8, data.player_z);
    sqlite3_bind_double(stmt, 9, data.yaw);
    sqlite3_bind_double(stmt, 10, data.pitch);
    sqlite3_bind_int(stmt, 11, data.health);
    sqlite3_bind_double(stmt, 12, data.stamina);
    sqlite3_bind_int(stmt, 13, data.keys_mask);
    sqlite3_bind_double(stmt, 14, data.time_elapsed);
    sqlite3_bind_int(stmt, 15, data.score);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "[Database] Failed to save slot: " << sqlite3_errmsg(db_) << "\n";
        return false;
    }

    std::cout << "[Database] Saved game successfully to Slot #" << slot_id << "\n";
    return true;
}

bool Database::loadSlot(int slot_id, SaveSlotData& out_data) {
    if (!db_) return false;

    const char* sql = "SELECT player_name, maze_w, maze_h, seed, player_x, player_y, player_z, yaw, pitch, health, stamina, keys_mask, time_elapsed, score, timestamp FROM save_slots WHERE slot_id = ?;";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_int(stmt, 1, slot_id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        out_data.slot_id = slot_id;
        out_data.player_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        out_data.maze_w = sqlite3_column_int(stmt, 1);
        out_data.maze_h = sqlite3_column_int(stmt, 2);
        out_data.seed = sqlite3_column_int(stmt, 3);
        out_data.player_x = static_cast<float>(sqlite3_column_double(stmt, 4));
        out_data.player_y = static_cast<float>(sqlite3_column_double(stmt, 5));
        out_data.player_z = static_cast<float>(sqlite3_column_double(stmt, 6));
        out_data.yaw = static_cast<float>(sqlite3_column_double(stmt, 7));
        out_data.pitch = static_cast<float>(sqlite3_column_double(stmt, 8));
        out_data.health = sqlite3_column_int(stmt, 9);
        out_data.stamina = static_cast<float>(sqlite3_column_double(stmt, 10));
        out_data.keys_mask = sqlite3_column_int(stmt, 11);
        out_data.time_elapsed = static_cast<float>(sqlite3_column_double(stmt, 12));
        out_data.score = sqlite3_column_int(stmt, 13);
        out_data.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 14));
        out_data.is_empty = false;
        sqlite3_finalize(stmt);
        std::cout << "[Database] Loaded Slot #" << slot_id << " (" << out_data.player_name << " - " << out_data.timestamp << ")\n";
        return true;
    }

    sqlite3_finalize(stmt);
    out_data.is_empty = true;
    return false;
}

std::vector<SaveSlotData> Database::getAllSlots() {
    std::vector<SaveSlotData> slots(3);
    for (int i = 1; i <= 3; ++i) {
        slots[i - 1].slot_id = i;
        loadSlot(i, slots[i - 1]);
    }
    return slots;
}

bool Database::recordSession(const SessionRecord& s) {
    if (!db_) return false;

    const char* sql = R"(
        INSERT INTO sessions (player_name, mode, difficulty, outcome, duration_ms, score, keys_collected, monsters_evaded, timestamp)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?, CURRENT_TIMESTAMP);
    )";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;

    sqlite3_bind_text(stmt, 1, s.player_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, s.mode.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, s.difficulty);
    sqlite3_bind_text(stmt, 4, s.outcome.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, s.duration_ms);
    sqlite3_bind_int(stmt, 6, s.score);
    sqlite3_bind_int(stmt, 7, s.keys_collected);
    sqlite3_bind_int(stmt, 8, s.monsters_evaded);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

std::vector<LeaderboardEntry> Database::getLeaderboard(int limit) {
    std::vector<LeaderboardEntry> entries;
    if (!db_) return entries;

    std::string sql = "SELECT rank, player_name, mode, score, duration_ms, keys_collected, outcome, timestamp FROM v_leaderboard LIMIT " + std::to_string(limit) + ";";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return entries;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        LeaderboardEntry e;
        e.rank = sqlite3_column_int(stmt, 0);
        e.player_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        e.mode = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        e.score = sqlite3_column_int(stmt, 3);
        e.duration_ms = sqlite3_column_int(stmt, 4);
        e.keys_collected = sqlite3_column_int(stmt, 5);
        e.outcome = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        e.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        entries.push_back(e);
    }
    sqlite3_finalize(stmt);
    return entries;
}

void Database::printLeaderboard(int limit) {
    auto entries = getLeaderboard(limit);
    std::cout << "\n==================== MAZERUNNER LEADERBOARD ====================\n";
    std::cout << std::left << std::setw(6) << "Rank"
              << std::setw(16) << "Player"
              << std::setw(14) << "Mode"
              << std::setw(10) << "Score"
              << std::setw(12) << "Time (s)"
              << std::setw(8) << "Keys"
              << std::setw(20) << "Date" << "\n";
    std::cout << std::string(84, '-') << "\n";

    if (entries.empty()) {
        std::cout << "  No recorded victories yet! Escape the maze to place your score.\n";
    } else {
        for (const auto& e : entries) {
            std::cout << std::left << std::setw(6) << e.rank
                      << std::setw(16) << e.player_name
                      << std::setw(14) << e.mode
                      << std::setw(10) << e.score
                      << std::setw(12) << std::fixed << std::setprecision(1) << (e.duration_ms / 1000.0)
                      << std::setw(8) << e.keys_collected
                      << std::setw(20) << e.timestamp << "\n";
        }
    }
    std::cout << "================================================================\n\n";
}

bool Database::logTelemetry(int session_id, float time_s, float px, float py, float pz, float fps) {
    if (!db_) return false;
    const char* sql = "INSERT INTO telemetry_logs (session_id, time_s, px, py, pz, fps) VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int(stmt, 1, session_id);
    sqlite3_bind_double(stmt, 2, time_s);
    sqlite3_bind_double(stmt, 3, px);
    sqlite3_bind_double(stmt, 4, py);
    sqlite3_bind_double(stmt, 5, pz);
    sqlite3_bind_double(stmt, 6, fps);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}
