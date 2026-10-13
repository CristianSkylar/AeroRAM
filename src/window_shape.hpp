#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace aeroram {
class RoundedMask {
    int width_=0,height_=0,extent_=0;
    std::vector<std::uint8_t> corner_;
public:
    void reset(int width,int height,double radius) {
        width_=std::max(0,width);height_=std::max(0,height);extent_=0;corner_.clear();
        if(!width_ || !height_)return;
        radius=std::isfinite(radius)?std::clamp(radius,0.0,std::min(width_,height_)/2.0):0;
        extent_=int(std::ceil(radius));
        corner_.resize(std::size_t(extent_)*extent_);
        constexpr int samples=8,total=samples*samples;
        for(int y=0;y<extent_;++y)for(int x=0;x<extent_;++x) {
            int inside=0;
            for(int sy=0;sy<samples;++sy)for(int sx=0;sx<samples;++sx) {
                const double px=x+(sx+.5)/samples,py=y+(sy+.5)/samples;
                const double dx=std::max(0.0,std::abs(px-width_/2.0)-(width_/2.0-radius));
                const double dy=std::max(0.0,std::abs(py-height_/2.0)-(height_/2.0-radius));
                inside+=dx*dx+dy*dy<=radius*radius;
            }
            corner_[std::size_t(y)*extent_+x]=std::uint8_t((inside*255+total/2)/total);
        }
    }
    std::uint8_t coverage(int x,int y) const {
        if(x<0 || y<0 || x>=width_ || y>=height_)return 0;
        x=std::min(x,width_-1-x);y=std::min(y,height_-1-y);
        return x<extent_ && y<extent_?corner_[std::size_t(y)*extent_+x]:255;
    }
    bool contains(int x,int y) const {return coverage(x,y)!=0;}
    bool apply(std::uint8_t* pixels,std::ptrdiff_t stride) const {
        const auto rowBytes=std::ptrdiff_t(width_)*4;
        if(!pixels || !width_ || !height_ || (stride>=0?stride<rowBytes:stride>-rowBytes))return false;
        for(int y=0;y<extent_;++y)for(int x=0;x<extent_;++x) {
            const auto cap=corner_[std::size_t(y)*extent_+x];
            if(cap==255)continue;
            const int xs[]={x,width_-1-x},ys[]={y,height_-1-y};
            for(int py:ys)for(int px:xs) {
                auto* p=pixels+std::ptrdiff_t(py)*stride+std::ptrdiff_t(px)*4;
                if(!cap){p[0]=p[1]=p[2]=p[3]=0;continue;}
                const unsigned alpha=p[3];
                if(alpha<=cap)continue;
                for(int channel=0;channel<3;++channel)p[channel]=std::uint8_t((unsigned(p[channel])*cap+alpha/2)/alpha);
                p[3]=cap;
            }
        }
        return true;
    }
};
}
