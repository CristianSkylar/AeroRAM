#include "../src/core.hpp"
#include <cassert>
#include <iostream>
using namespace skylar;
int main() {
    MemorySample a{true, 8ULL*1024*1024*1024, 3ULL*1024*1024*1024};
    assert(a.usedPercent() == 62.5);
    MemorySample b=a; b.available=a.available-4096;
    assert(availableChange(a,b)==-4096);
    b.valid=false; assert(availableChange(a,b)==0);
    b=a; b.available=a.total+1; assert(b.usedPercent()==0 && b.used()==0);
    assert(outcome(2,1)==Outcome::Partial);
    assert(outcome(2,0)==Outcome::Failed && outcome(0,0)==Outcome::Failed);
    assert(outcome(2,2)==Outcome::Complete);
    const double x=smoothToward(90,10,.1); assert(x>10 && x<90);
    History h; for (int i=0;i<70;++i)h.push(i);
    assert(h.count==60 && h.values[0]==10 && h.values[59]==69);
    std::cout << "Memory accounting, outcomes, animations and history: passed.\n";
}
