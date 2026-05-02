#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>

inline void UnpackSoldier(const uint8_t* buf,
	uint32_t& id, float& x, float& y, float& z,
	int32_t& grid, uint32_t& dir, int32_t& level,
	uint32_t& anim, uint32_t& frame,
	int32_t& life, int32_t& ap, int32_t& breath,
	uint32_t& flags, int32_t& stance,
	uint32_t& hand, uint32_t& secondHand, bool& visible)
{
	size_t off = 0;
	memcpy(&id, buf + off, 4); off += 4;
	memcpy(&x, buf + off, 4); off += 4;
	memcpy(&y, buf + off, 4); off += 4;
	memcpy(&z, buf + off, 4); off += 4;
	memcpy(&grid, buf + off, 4); off += 4;
	memcpy(&dir, buf + off, 4); off += 4;
	memcpy(&level, buf + off, 4); off += 4;
	memcpy(&anim, buf + off, 4); off += 4;
	memcpy(&frame, buf + off, 4); off += 4;
	memcpy(&life, buf + off, 4); off += 4;
	memcpy(&ap, buf + off, 4); off += 4;
	memcpy(&breath, buf + off, 4); off += 4;
	memcpy(&flags, buf + off, 4); off += 4;
	memcpy(&stance, buf + off, 4); off += 4;
	memcpy(&hand, buf + off, 4); off += 4;
	memcpy(&secondHand, buf + off, 4); off += 4;
	visible = buf[off] != 0;
}

inline void UnpackEvent(const uint8_t* buf, uint8_t& type, uint32_t& sid, int32_t& grid, uint32_t& tid, int32_t& value) {
	size_t off = 0;
	type = buf[off]; off += 1;
	memcpy(&sid, buf + off, 4); off += 4;
	memcpy(&grid, buf + off, 4); off += 4;
	memcpy(&tid, buf + off, 4); off += 4;
	memcpy(&value, buf + off, 4); off += 4;
}

int main() {
	FILE* f = fopen("network_packets.bin", "rb");

	printf("проверка\n");

	int frames = 0, totalEvents = 0;
	const char* evtNames[] = { "SHOOT","DEATH","END_TURN","DROP","PICKUP","MOVE","STANCE" };

	while (!feof(f)) {
		uint32_t size;
		if (fread(&size, 4, 1, f) != 1) break;

		std::vector<uint8_t> data(size);
		if (fread(data.data(), 1, size, f) != size) break;

		if (data[0] == 0x01) {
			frames++;
			uint32_t tick;
			memcpy(&tick, &data[1], 4);
			uint8_t soldierCount = data[5];

			size_t off = 9;
			size_t eventStart = 9 + soldierCount * 71;
			while (eventStart + 17 <= size) {
				uint8_t type; uint32_t sid, tid; int32_t grid, value;
				UnpackEvent(&data[eventStart], type, sid, grid, tid, value);
				printf("  [КАДР %u] >>> %s от S%u", tick, evtNames[type], sid);
				if (tid > 0) printf(" на S%u", tid);
				printf("\n");
				totalEvents++;
				eventStart += 17;
			}

			if (frames % 200 == 0) {
				printf("\n=== КАДР %u (tick %u) [%u солдат] ===\n", frames, tick, soldierCount);
				for (uint8_t i = 0; i < soldierCount && off + 71 <= size; i++) {
					uint32_t id; float x, y, z; int32_t grid, level, life, ap, breath, stance;
					uint32_t dir, anim, frame, flags, hand, secondHand; bool visible;
					UnpackSoldier(&data[off], id, x, y, z, grid, dir, level, anim, frame, life, ap, breath, flags, stance, hand, secondHand, visible);
					printf("  S%u: pos=(%.0f,%.0f) grid=%d life=%d AP=%d\n", id, x, y, grid, life, ap);
					off += 71;
				}
				printf("\n");
			}
		}
	}

	fclose(f);
	printf(" кадров=%d событий=%d \n", frames, totalEvents);
	printf(frames > 0 && totalEvents > 0 ? "[OK] ПРОВЕРКА ПРОЙДЕНА\n" : "[ОШИБКА]\n");
	return 0;
}
