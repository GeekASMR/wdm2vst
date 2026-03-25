#pragma once
// Minimal correct QR Code generator - Byte mode, ECC Level L, Versions 1-6
// Based on Nayuki's algorithm (public domain concepts)
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <array>
#include <algorithm>

namespace QR {

struct QRResult {
    int size = 0;
    std::vector<bool> modules;
    bool get(int x, int y) const {
        if (x < 0 || x >= size || y < 0 || y >= size) return false;
        return modules[(size_t)(y * size + x)];
    }
};

namespace detail {
    // GF(256) arithmetic with prime poly 0x11D
    static uint8_t gfMul(uint8_t x, uint8_t y) {
        int z = 0;
        for (int i = 7; i >= 0; i--) {
            z = (z << 1) ^ ((z >> 7) * 0x11D);
            z ^= ((y >> i) & 1) * x;
        }
        return (uint8_t)z;
    }

    // Reed-Solomon: compute ECC bytes
    static std::vector<uint8_t> rsEncode(const std::vector<uint8_t>& data, int numEcc) {
        // Build generator polynomial by multiplying (x - α^i) for i=0..numEcc-1
        std::vector<uint8_t> gen(numEcc, 0);
        gen[numEcc - 1] = 1; // Start with 1 (coefficients stored high-to-low)
        uint8_t root = 1;
        for (int i = 0; i < numEcc; i++) {
            for (int j = 0; j < numEcc; j++) {
                gen[j] = gfMul(gen[j], root);
                if (j + 1 < numEcc) gen[j] ^= gen[j + 1];
            }
            root = gfMul(root, 2);
        }
        // Polynomial division
        std::vector<uint8_t> rem(numEcc, 0);
        for (uint8_t b : data) {
            uint8_t factor = b ^ rem[0];
            // Shift left
            for (int i = 0; i < numEcc - 1; i++)
                rem[i] = rem[i + 1];
            rem[numEcc - 1] = 0;
            for (int i = 0; i < numEcc; i++)
                rem[i] ^= gfMul(gen[i], factor);
        }
        return rem;
    }

    // QR Version data (ECC Level L for simplicity - more data capacity)
    struct VersionInfo {
        int ver, size, dataCapacity, eccPerBlock, numBlocks;
    };
    
    static const VersionInfo VERSIONS[] = {
        {1, 21, 19, 7, 1},
        {2, 25, 34, 10, 1},
        {3, 29, 55, 15, 1},
        {4, 33, 80, 20, 1},
        {5, 37, 108, 26, 1},
        {6, 41, 136, 18, 2},
    };

    struct QRMatrix {
        int n;
        std::vector<bool> mod;
        std::vector<bool> func; // true = function pattern, don't overwrite
        
        QRMatrix(int size) : n(size), mod(size*size, false), func(size*size, false) {}
        
        void set(int x, int y, bool v) {
            if (x>=0 && x<n && y>=0 && y<n) mod[y*n+x] = v;
        }
        bool get(int x, int y) const {
            if (x>=0 && x<n && y>=0 && y<n) return mod[y*n+x];
            return false;
        }
        void setFunc(int x, int y, bool v) {
            if (x>=0 && x<n && y>=0 && y<n) { mod[y*n+x] = v; func[y*n+x] = true; }
        }
        bool isFunc(int x, int y) const {
            if (x>=0 && x<n && y>=0 && y<n) return func[y*n+x];
            return true;
        }
    };

    static void drawFinder(QRMatrix& m, int cx, int cy) {
        for (int dy = -4; dy <= 4; dy++) {
            for (int dx = -4; dx <= 4; dx++) {
                int x = cx+dx, y = cy+dy;
                if (x<0||x>=m.n||y<0||y>=m.n) continue;
                int dist = std::max(std::abs(dx), std::abs(dy));
                m.setFunc(x, y, dist != 2 && dist != 4);
            }
        }
    }

    static void drawAlignment(QRMatrix& m, int cx, int cy) {
        for (int dy=-2; dy<=2; dy++)
            for (int dx=-2; dx<=2; dx++)
                m.setFunc(cx+dx, cy+dy, std::max(std::abs(dx),std::abs(dy)) != 1);
    }

    static void drawTimingPatterns(QRMatrix& m) {
        for (int i = 8; i < m.n - 8; i++) {
            m.setFunc(6, i, i%2==0);
            m.setFunc(i, 6, i%2==0);
        }
    }

    // Alignment pattern positions for versions 2-6
    static std::vector<int> getAlignmentPositions(int ver) {
        if (ver == 1) return {};
        int last = ver * 4 + 10; // = size - 7
        return {6, last};
    }

    static void drawFormatBits(QRMatrix& m, int mask) {
        // ECC Level L = 01
        int data = (1 << 3) | mask;
        int rem = data;
        for (int i = 0; i < 10; i++)
            rem = (rem << 1) ^ ((rem >> 9) * 0x537);
        int bits = ((data << 10) | rem) ^ 0x5412;
        
        // First copy near top-left finder
        for (int i = 0; i < 6; i++) m.setFunc(8, i, ((bits >> i)&1)!=0);
        m.setFunc(8, 7, ((bits>>6)&1)!=0);
        m.setFunc(8, 8, ((bits>>7)&1)!=0);
        m.setFunc(7, 8, ((bits>>8)&1)!=0);
        for (int i = 9; i < 15; i++) m.setFunc(14-i, 8, ((bits>>i)&1)!=0);
        
        // Second copy
        for (int i = 0; i < 8; i++) m.setFunc(m.n-1-i, 8, ((bits>>i)&1)!=0);
        for (int i = 8; i < 15; i++) m.setFunc(8, m.n-15+i, ((bits>>i)&1)!=0);
        m.setFunc(8, m.n-8, true); // Dark module
    }

    static bool getMaskBit(int mask, int x, int y) {
        switch (mask) {
            case 0: return (x+y)%2==0;
            case 1: return y%2==0;
            case 2: return x%3==0;
            case 3: return (x+y)%3==0;
            case 4: return (x/3+y/2)%2==0;
            case 5: return x*y%2+x*y%3==0;
            case 6: return (x*y%2+x*y%3)%2==0;
            case 7: return ((x+y)%2+x*y%3)%2==0;
            default: return false;
        }
    }

    static void placeData(QRMatrix& m, const std::vector<uint8_t>& data, int mask) {
        int bitIdx = 0;
        int totalBits = (int)data.size() * 8;
        for (int right = m.n - 1; right >= 1; right -= 2) {
            if (right == 6) right = 5;
            for (int vert = 0; vert < m.n; vert++) {
                for (int j = 0; j < 2; j++) {
                    int x = right - j;
                    bool upward = ((right + 1) & 2) == 0;
                    int y = upward ? (m.n - 1 - vert) : vert;
                    if (m.isFunc(x, y)) continue;
                    bool bit = false;
                    if (bitIdx < totalBits)
                        bit = ((data[bitIdx/8] >> (7 - bitIdx%8)) & 1) != 0;
                    bitIdx++;
                    if (getMaskBit(mask, x, y)) bit = !bit;
                    m.set(x, y, bit);
                }
            }
        }
    }

    static int calcPenalty(const QRMatrix& m) {
        int pen = 0, n = m.n;
        // Rule 1: runs of same color
        for (int y = 0; y < n; y++) {
            int run = 1;
            for (int x = 1; x < n; x++) {
                if (m.get(x,y) == m.get(x-1,y)) { run++; if(run==5) pen+=3; else if(run>5) pen++; }
                else run=1;
            }
        }
        for (int x = 0; x < n; x++) {
            int run = 1;
            for (int y = 1; y < n; y++) {
                if (m.get(x,y) == m.get(x,y-1)) { run++; if(run==5) pen+=3; else if(run>5) pen++; }
                else run=1;
            }
        }
        // Rule 2: 2x2 blocks
        for (int y = 0; y < n-1; y++)
            for (int x = 0; x < n-1; x++) {
                bool c = m.get(x,y);
                if (c==m.get(x+1,y) && c==m.get(x,y+1) && c==m.get(x+1,y+1)) pen+=3;
            }
        // Rule 4: dark/light balance
        int dark = 0;
        for (int y = 0; y < n; y++) for (int x = 0; x < n; x++) if (m.get(x,y)) dark++;
        int total = n*n;
        int k = (std::abs(dark*20 - total*10) + total - 1) / total - 1;
        pen += k * 10;
        return pen;
    }
}

static QRResult generate(const std::string& text) {
    using namespace detail;
    
    // Find minimum version
    const VersionInfo* vi = nullptr;
    for (auto& v : VERSIONS) {
        int overhead = 4 + 8 + (int)text.size() * 8; // mode + length + data
        int capacity = v.dataCapacity * 8;
        if (overhead <= capacity) { vi = &v; break; }
    }
    if (!vi) vi = &VERSIONS[5]; // max version

    int dataCap = vi->dataCapacity;
    int eccPerBlock = vi->eccPerBlock;
    int numBlocks = vi->numBlocks;
    int n = vi->size;

    // Encode: mode=0100 (byte), 8-bit length, data bytes
    std::vector<bool> bits;
    auto addBits = [&](unsigned val, int cnt) {
        for (int i = cnt-1; i >= 0; i--) bits.push_back((val>>i)&1);
    };
    addBits(4, 4); // byte mode
    addBits((unsigned)text.size(), 8); // length (8 bits for v1-9)
    for (char c : text) addBits((uint8_t)c, 8);
    // Terminator
    int termBits = std::min(4, dataCap*8 - (int)bits.size());
    addBits(0, termBits);
    // Byte-align
    while (bits.size() % 8 != 0) bits.push_back(false);
    // Pad to capacity
    uint8_t padBytes[] = {0xEC, 0x11};
    for (int i = 0; (int)bits.size() < dataCap * 8; i++)
        addBits(padBytes[i%2], 8);

    // Convert to bytes
    std::vector<uint8_t> dataBytes(dataCap);
    for (int i = 0; i < dataCap*8 && i < (int)bits.size(); i++)
        if (bits[i]) dataBytes[i/8] |= (1 << (7 - i%8));

    // Split into blocks, compute ECC
    int shortLen = dataCap / numBlocks;
    int longBlocks = dataCap % numBlocks;
    std::vector<std::vector<uint8_t>> dataBlocks, eccBlocks;
    int offset = 0;
    for (int i = 0; i < numBlocks; i++) {
        int blen = shortLen + (i >= numBlocks - longBlocks ? 1 : 0);
        std::vector<uint8_t> block(dataBytes.begin()+offset, dataBytes.begin()+offset+blen);
        dataBlocks.push_back(block);
        eccBlocks.push_back(rsEncode(block, eccPerBlock));
        offset += blen;
    }

    // Interleave data blocks, then ECC blocks
    std::vector<uint8_t> codewords;
    for (int i = 0; i <= shortLen; i++)
        for (int j = 0; j < numBlocks; j++)
            if (i < (int)dataBlocks[j].size()) codewords.push_back(dataBlocks[j][i]);
    for (int i = 0; i < eccPerBlock; i++)
        for (int j = 0; j < numBlocks; j++)
            codewords.push_back(eccBlocks[j][i]);

    // Build matrix - try all 8 masks and pick best
    int bestMask = 0, bestPen = INT_MAX;
    QRMatrix bestM(n);

    for (int mask = 0; mask < 8; mask++) {
        QRMatrix m(n);
        // Function patterns
        drawFinder(m, 3, 3);
        drawFinder(m, n-4, 3);
        drawFinder(m, 3, n-4);
        drawTimingPatterns(m);
        
        // Alignment patterns
        auto aligns = getAlignmentPositions(vi->ver);
        for (size_t a = 0; a < aligns.size(); a++)
            for (size_t b = 0; b < aligns.size(); b++) {
                if ((a==0&&b==0)||(a==0&&b==aligns.size()-1)||(a==aligns.size()-1&&b==0)) continue;
                drawAlignment(m, aligns[a], aligns[b]);
            }
        
        // Format info
        drawFormatBits(m, mask);
        // Data
        placeData(m, codewords, mask);
        
        int pen = calcPenalty(m);
        if (pen < bestPen) { bestPen = pen; bestMask = mask; bestM = m; }
    }

    QRResult r;
    r.size = n;
    r.modules = bestM.mod;
    return r;
}

} // namespace QR
