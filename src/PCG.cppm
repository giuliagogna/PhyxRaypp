module;

export module PCG;
import std;

export struct PCG {
    std::uint64_t state;
    std::uint64_t inc;

    PCG(std::uint64_t init_state = 42, std::uint64_t init_seq = 54) {
        state = 0;
        inc = (init_seq << 1) | 1u;
        random();
        state += init_state;
        random();
    }

    // It returns a random 32 bit integer
    std::uint32_t random() {
        std::uint64_t oldstate = state;
        state = oldstate * 6364136223846793005ULL + inc;

        // 2. Applicazione della funzione di output (XSH-RR)
        // Questa parte trasforma lo stato interno a 64-bit in un output a 32-bit
        std::uint32_t xorshifted = static_cast<std::uint32_t>(((oldstate >> 18u) ^ oldstate) >> 27u);
        std::uint32_t rot = static_cast<std::uint32_t>(oldstate >> 59u);
        return std::rotr(xorshifted, rot);
    }

    // It returns a uniform RGN in [0, 1)
    float random_float() {
        return static_cast<float>(random()) / 4294967296.0f;
    }
};