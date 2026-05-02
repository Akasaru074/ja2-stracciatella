#ifndef TACTICAL_NETWORK_H
#define TACTICAL_NETWORK_H

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>

inline void PackSoldier(uint8_t* buf,
    uint32_t id, float x, float y, float z,
    int32_t grid, uint32_t dir, int32_t level,
    uint32_t anim, uint32_t frame,
    int32_t life, int32_t ap, int32_t breath,
    uint32_t flags, int32_t stance,
    uint32_t hand, uint32_t secondHand, bool visible)
{
    size_t off = 0;
    memcpy(buf + off, &id, 4); off += 4;
    memcpy(buf + off, &x, 4); off += 4;
    memcpy(buf + off, &y, 4); off += 4;
    memcpy(buf + off, &z, 4); off += 4;
    memcpy(buf + off, &grid, 4); off += 4;
    memcpy(buf + off, &dir, 4); off += 4;
    memcpy(buf + off, &level, 4); off += 4;
    memcpy(buf + off, &anim, 4); off += 4;
    memcpy(buf + off, &frame, 4); off += 4;
    memcpy(buf + off, &life, 4); off += 4;
    memcpy(buf + off, &ap, 4); off += 4;
    memcpy(buf + off, &breath, 4); off += 4;
    memcpy(buf + off, &flags, 4); off += 4;
    memcpy(buf + off, &stance, 4); off += 4;
    memcpy(buf + off, &hand, 4); off += 4;
    memcpy(buf + off, &secondHand, 4); off += 4;
    buf[off] = visible ? 1 : 0;
}

inline void PackEvent(uint8_t* buf, uint8_t type, uint32_t sid, int32_t grid, uint32_t tid, int32_t value) {
    size_t off = 0;
    buf[off] = type; off += 1;
    memcpy(buf + off, &sid, 4); off += 4;
    memcpy(buf + off, &grid, 4); off += 4;
    memcpy(buf + off, &tid, 4); off += 4;
    memcpy(buf + off, &value, 4); off += 4;
}

struct SoldierData {
    uint32_t id;
    float x, y;
    int32_t grid, life, ap;
    uint32_t hand, stance;
};

class TacticalNetwork {
private:
    FILE* txt_file;
    FILE* bin_file;
    uint32_t tick;
    std::vector<SoldierData> frame_soldiers;
    std::vector<uint8_t> frame_events;

    void WriteBin(const uint8_t* data, size_t size) {
        if (bin_file) {
            uint32_t s = (uint32_t)size;
            fwrite(&s, sizeof(s), 1, bin_file);
            fwrite(data, 1, s, bin_file);
        }
    }

public:
    TacticalNetwork() : txt_file(nullptr), bin_file(nullptr), tick(0) {}

    void Init() {
        txt_file = fopen("network_test.txt", "w");
        bin_file = fopen("network_packets.bin", "wb");
    }

    void Close() {
        if (txt_file) {
            fprintf(txt_file, "frames: %u\n", tick);
            fclose(txt_file);
        }
        if (bin_file) fclose(bin_file);
    }

    void LogSoldier(uint32_t id, float x, float y, int32_t grid,
        int32_t life, int32_t ap, uint32_t hand, uint32_t stance)
    {
        if (txt_file) {
            fprintf(txt_file, "S %u|%.1f,%.1f|G%d|L%d|AP%d|H%u|ST%d\n",
                id, x, y, grid, life, ap, hand, stance);
        }

        SoldierData sd;
        sd.id = id; sd.x = x; sd.y = y;
        sd.grid = grid; sd.life = life; sd.ap = ap;
        sd.hand = hand; sd.stance = stance;
        frame_soldiers.push_back(sd);
    }

    void LogEvent(const char* type, uint32_t sid, uint32_t tid = 0) {
        if (txt_file) {
            fprintf(txt_file, "E %s|%u|%u\n", type, sid, tid);
        }

        uint8_t evtType = 0;
        if (strcmp(type, "SHOOT") == 0) evtType = 0;
        else if (strcmp(type, "DEATH") == 0) evtType = 1;
        else if (strcmp(type, "END_TURN") == 0) evtType = 2;

        uint8_t evt[17];
        PackEvent(evt, evtType, sid, 0, tid, 0);
        frame_events.insert(frame_events.end(), evt, evt + 17);
    }

    void EndFrame() {
        if (txt_file) {
            fprintf(txt_file, "--- FRAME %u ---\n", tick);
            fflush(txt_file);
        }

        if (bin_file && !frame_soldiers.empty()) {
            size_t totalSize = 9 + frame_soldiers.size() * 71 + frame_events.size();
            std::vector<uint8_t> packet(totalSize);

            packet[0] = 0x01;
            memcpy(&packet[1], &tick, 4);
            uint8_t count = (uint8_t)frame_soldiers.size();
            packet[5] = count;
            packet[6] = 0; packet[7] = 0; packet[8] = 0;

            size_t off = 9;
            for (const auto& sd : frame_soldiers) {
                PackSoldier(&packet[off], sd.id, sd.x, sd.y, 0.0f,
                    sd.grid, 0, 0, 0, 0,
                    sd.life, sd.ap, 0, 0, sd.stance,
                    sd.hand, 0, true);
                off += 71;
            }

            if (!frame_events.empty()) {
                memcpy(&packet[off], frame_events.data(), frame_events.size());
            }

            WriteBin(packet.data(), packet.size());
        }

        frame_soldiers.clear();
        frame_events.clear();
        tick++;
    }
};

extern TacticalNetwork gNetwork;

#endif
