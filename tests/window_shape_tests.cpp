#include "../src/window_shape.hpp"
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>

void require(bool valid,const char* message) {
    if(!valid){std::cerr<<message<<'\n';std::exit(1);}
}
void checkScale(double scale) {
    const int width=int(428*scale+.5),height=int(672*scale+.5);
    const double radius=32*scale;
    aeroram::RoundedMask mask;mask.reset(width,height,radius);
    require(!mask.contains(0,0) && !mask.contains(width-1,0) && !mask.contains(0,height-1) && !mask.contains(width-1,height-1),"Corner cutouts must be transparent");
    require(mask.coverage(width/2,0)==255 && mask.coverage(0,height/2)==255 && mask.coverage(width/2,height-1)==255,"Straight edges must remain intact");
    require(!mask.contains(-1,0) && !mask.contains(width,0) && !mask.contains(0,height),"Hit testing must reject points outside the bitmap");
    bool antialias=false;
    const int extent=int(std::ceil(radius));
    for(int y=0;y<extent;++y)for(int x=0;x<extent;++x) {
        const auto a=mask.coverage(x,y);
        require(a==mask.coverage(width-1-x,y) && a==mask.coverage(x,height-1-y) && a==mask.coverage(width-1-x,height-1-y),"All four corners must use the same shape");
        antialias|=a>0 && a<255;
        const double nearX=std::max(0.0,radius-x-1),nearY=std::max(0.0,radius-y-1);
        if(nearX*nearX+nearY*nearY>radius*radius)require(a==0,"No pixel wholly outside the rounded body may remain visible");
        const double farX=std::max(0.0,radius-x),farY=std::max(0.0,radius-y);
        if(farX*farX+farY*farY<radius*radius)require(a==255,"The mask must preserve pixels wholly inside the body");
    }
    require(antialias,"Fractional edge coverage must be retained");
    const std::ptrdiff_t stride=width*4+12;
    for(unsigned alpha:std::array<unsigned,5>{0,125,198,216,255}) {
        std::vector<std::uint8_t> pixels(std::size_t(stride)*height,0xA5);
        for(int y=0;y<height;++y)for(int x=0;x<width;++x) {
            auto* p=pixels.data()+y*stride+x*4;
            p[0]=std::uint8_t(alpha);p[1]=std::uint8_t(alpha/2);p[2]=std::uint8_t(alpha/4);p[3]=std::uint8_t(alpha);
        }
        require(mask.apply(pixels.data(),stride),"A valid surface must accept the mask");
        const auto once=pixels;
        require(mask.apply(pixels.data(),stride) && pixels==once,"Repeated rendering must not erode the rounded edges");
        for(int y=0;y<height;++y) {
            for(int x=0;x<width;++x) {
                const auto* p=pixels.data()+y*stride+x*4;
                const auto cap=mask.coverage(x,y);
                require(p[3]==std::min<unsigned>(alpha,cap),"Pixels must not exceed the shape's alpha coverage");
                require(p[0]<=p[3] && p[1]<=p[3] && p[2]<=p[3],"Clipping must retain premultiplied alpha");
                if(!cap)require(!(p[0]|p[1]|p[2]|p[3]),"The corner cutouts must contain no residual color");
                if(cap==255)require(p[0]==alpha && p[1]==alpha/2 && p[2]==alpha/4 && p[3]==alpha,"Glass tint and interior pixels must remain unchanged");
            }
            for(auto i=width*4;i<stride;++i)require(pixels[std::size_t(y*stride+i)]==0xA5,"The mask must not write into row padding");
        }
        auto reversed=std::vector<std::uint8_t>(std::size_t(stride)*height,0xA5);
        for(int y=0;y<height;++y)for(int x=0;x<width;++x) {
            auto* p=reversed.data()+(height-1-y)*stride+x*4;
            p[0]=std::uint8_t(alpha);p[1]=std::uint8_t(alpha/2);p[2]=std::uint8_t(alpha/4);p[3]=std::uint8_t(alpha);
        }
        require(mask.apply(reversed.data()+(height-1)*stride,-stride),"Bottom-up surfaces must accept the mask");
        require(reversed==pixels,"Top-down and bottom-up clipping must agree");
    }
    std::array<std::uint8_t,4> guard{1,2,3,4};
    require(!mask.apply(guard.data(),3) && guard==std::array<std::uint8_t,4>{1,2,3,4},"Invalid strides must not touch the surface");
    require(!mask.apply(nullptr,stride),"Null surfaces must be rejected");
}
int main() {
    for(double scale:{.5,.73,1.,1.25,1.5,1.75,2.,2.5,3.})checkScale(scale);
    aeroram::RoundedMask mask;
    require(!mask.contains(0,0),"An empty shape must not receive clicks");
    mask.reset(1,1,32);require(mask.coverage(0,0)>0,"Tiny windows must clamp their radius safely");
    mask.reset(7,9,100);require(mask.contains(3,4),"Clamping a large radius must preserve the center");
    mask.reset(4,4,-1);require(mask.coverage(0,0)==255,"A zero radius must produce a rectangle");
    mask.reset(4,4,std::numeric_limits<double>::quiet_NaN());require(mask.coverage(0,0)==255,"Non-finite radii must not corrupt the shape");
    mask.reset(0,9,32);require(!mask.contains(0,0),"Resizing to an empty surface must clear the old mask");
    std::cout<<"Rounded-window tests passed: four corners, 9 scales, glass alpha, hit testing and buffer bounds.\n";
}
