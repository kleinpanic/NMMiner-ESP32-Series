#include "sdPendingTransfer.h"
#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

int main() {
    std::vector<uint8_t> input(1201), output;
    for (size_t i=0;i<input.size();++i) input[i]=uint8_t(i);
    size_t readAt=0, sent=0, reads=0, writes=0;
    SdPendingTransfer transfer;
    while (sent<input.size()) {
        auto r=transfer.advance(input.size()-sent,
          [&](uint8_t* p,size_t n){++reads; assert(n<=512);std::memcpy(p,input.data()+readAt,n);readAt+=n;return int(n);},
          [&](const uint8_t* p,size_t n){++writes;if(writes%3==1)return 0;size_t k=std::min(n,size_t(97));output.insert(output.end(),p,p+k);return int(k);});
        assert(r.state!=SdWriteState::Failed);
        if(r.state==SdWriteState::Wait)assert(r.sent==0);
        sent+=r.sent;
    }
    assert(output==input && reads==3 && writes>reads);
    transfer.reset();
    auto read=[](uint8_t*,size_t n){return int(n);};
    for(int invalid:{-1,0,3,513}) {
        transfer.reset();bool wrote=false;
        auto r=transfer.advance(512,[&](uint8_t*,size_t){return invalid;},[&](const uint8_t*,size_t){wrote=true;return 512;});
        assert(r.state==SdWriteState::Failed && !wrote);
    }
    for(int invalid:{-1,513}) {
        transfer.reset();auto r=transfer.advance(512,read,[&](const uint8_t*,size_t){return invalid;});
        assert(r.state==SdWriteState::Failed && !r.sent);
    }
    transfer.reset();bool called=false;
    auto empty=transfer.advance(0,[&](uint8_t*,size_t){called=true;return 0;},[&](const uint8_t*,size_t){called=true;return 0;});
    assert(!called && !empty.sent && empty.state==SdWriteState::Progress);
    puts("PASS pending SD transfer: exact multi-block bytes, repeated backpressure, partial sends, bounded reads, read/write failures, reset and empty input");
}
