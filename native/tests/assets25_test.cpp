#include "assets25.h"
#include <iostream>
#include <stdexcept>
using namespace stoneage;
void require(bool value) { if (!value) throw std::runtime_error("Asset check failed"); }
Bytes record(unsigned w, unsigned h, unsigned flag, Bytes payload) {
    Bytes b{'R','D',static_cast<std::uint8_t>(flag),0};
    for (auto n : {w,h,static_cast<unsigned>(16+payload.size())})
        for (int i=0;i<4;++i) b.push_back((n>>(i*8))&255);
    b.insert(b.end(),payload.begin(),payload.end()); return b;
}
void rejects(const Bytes& b) {
    try { decodeRD(b); } catch (const std::runtime_error&) { return; }
    throw std::runtime_error("Malformed image accepted");
}
int main(int argc, char** argv) try {
    require(decodeRD(record(2,2,0,{1,2,3,4})).pixels == Bytes({1,2,3,4}));
    require(decodeRD(record(3,2,1,{0xc2,0x82,7,0x02,8,9})).pixels == Bytes({0,0,7,7,8,9}));
    require(decodeRD(record(16,1,1,{0xd0,16})).pixels == Bytes(16,0));
    require(decodeRD(record(256,256,1,{0xe1,0,0})).pixels == Bytes(65536,0));
    auto legacyRaw=record(2,2,0,{1,2,3,4});
    legacyRaw[12]=0xff; legacyRaw[13]=0xff; legacyRaw[14]=0xff;
    require(decodeRD(legacyRaw).pixels == Bytes({1,2,3,4}));
    rejects(record(2,2,0,{1,2,3}));
    rejects({}); rejects(record(0,1,0,{})); rejects(record(1,1,16,{1}));
    rejects(record(2,1,1,{0x82})); rejects(record(1,1,1,{0xc2}));
    rejects(record(2,1,1,{0x02,1})); rejects(record(2,1,1,{0xc1}));
    rejects(record(1,1,1,{0})); rejects(record(1,1,1,{0xe0,1}));
    auto valid=record(2,2,1,{0x04,1,2,3,4});
    for (std::size_t n=0;n<valid.size();++n) rejects(Bytes(valid.begin(),valid.begin()+n));
    std::cout << "RD literal/repeat/zero/long-run and malformed-input checks passed\n";
    if (argc==2) {
        Assets25 assets(argv[1]);
        std::size_t pixels=0,frames=0,failures=0;
        for (const auto& entry : assets.graphics()) {
            try { pixels += assets.image(entry.first).pixels.size(); }
            catch (const std::exception& e) { if (failures++ < 20) std::cerr << e.what() << '\n'; }
        }
        for (const auto& sprite : assets.sprites()) for (const auto& animation : sprite.second) frames += animation.frames.size();
        std::cout << "Scanned " << assets.graphics().size() << " unique graphics, " << assets.sprites().size()
                  << " sprites, " << frames << " frames, " << pixels << " decoded pixels, " << failures << " rejected records\n";
        if (failures) return 1;
    }
    return 0;
} catch(const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
