#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <shellapi.h>
#include <shlobj.h>
#include <dwmapi.h>
#include <gdiplus.h>
#include <algorithm>
#include <cmath>
#include <cwchar>
#include <cstring>
#include <atomic>
#include <memory>
#include <string>
#include "memory.hpp"
#include "system_info.hpp"
#include "brand.hpp"
#include "social_icons.hpp"
#include "win_api.hpp"
#include "window_shape.hpp"
using namespace Gdiplus;
using namespace skylar;

constexpr float W=460, H=704;
constexpr float Inset=16, WindowW=W-2*Inset, WindowH=H-2*Inset;
constexpr UINT MSG_TRAY=WM_APP+1, MSG_CLEAN=WM_APP+2, MSG_INFO=WM_APP+3;
constexpr UINT TIMER_DATA=1, TIMER_ANIM=2;
constexpr wchar_t WindowClass[]=L"AeroRAM.NativeWindow";
struct Preferences { bool cache=true, apps=true, animations=true, closeToTray=true, dark=false; };
struct State {
    HWND window=nullptr;
    Preferences prefs;
    MemorySample memory;
    History history;
    double shownPercent=0, panel=0, hoverMix=0, ownMB=0;
    std::array<double,5> toggle{};
    bool settings=false, busy=false, preview=false, exitAfterClean=false, keyboardFocus=false;
    bool system=false, about=false, infoLoading=false;
    int infoPage=0;
    float infoScroll=0, infoContentHeight=0;
    float scrollGrab=0;
    SystemInfo info;
    std::wstring infoNotice;
    ULONGLONG noticeUntil=0;
    std::shared_ptr<std::atomic_bool> infoCancel;
    bool visible=true, tracking=false, rendering=false;
    int hover=-1, pressed=-1, focus=0;
    float dpiScale=1;
    ULONGLONG lastFrame=0, cleanStart=0, resultTime=0;
    std::wstring result, detail, config;
    NOTIFYICONDATAW tray{};
    HICON icon=nullptr;
    HDC memoryDC=nullptr;
    HBITMAP dib=nullptr, oldDib=nullptr;
    void* pixels=nullptr;
    int pixelW=0,pixelH=0;
    aeroram::RoundedMask outline;
    std::unique_ptr<Bitmap> surface;
    std::unique_ptr<Bitmap> logo;
    std::array<std::unique_ptr<GraphicsPath>,3> socialPaths;
    IStream* logoStream=nullptr;
} g;
const wchar_t* fontName=L"Segoe UI";
Color color(int a,int r,int gg,int b) { return Color(BYTE(a),BYTE(r),BYTE(gg),BYTE(b)); }
struct Palette { Color ink,muted,line; };
Palette palette() {
    if(g.prefs.dark)return {color(255,247,247,249),color(255,184,185,192),color(40,255,255,255)};
    return {color(255,29,30,35),color(255,91,95,106),color(38,51,55,65)};
}
void roundPath(GraphicsPath& p,RectF r,float radius) {
    p.Reset(); radius=std::min(radius,std::min(r.Width,r.Height)/2);
    const float d=radius*2;
    p.AddArc(r.X,r.Y,d,d,180,90);p.AddArc(r.GetRight()-d,r.Y,d,d,270,90);
    p.AddArc(r.GetRight()-d,r.GetBottom()-d,d,d,0,90);p.AddArc(r.X,r.GetBottom()-d,d,d,90,90);p.CloseFigure();
}
void fillRound(Graphics& c,RectF r,float radius,Color fill,Color outline=Color(0,0,0,0)) {
    GraphicsPath p;roundPath(p,r,radius);SolidBrush b(fill);c.FillPath(&b,&p);
    if(outline.GetAlpha()){Pen pen(outline,1);c.DrawPath(&pen,&p);}
}
void glassCard(Graphics& c,RectF r,float radius,bool hover=false) {
    GraphicsPath path;roundPath(path,r,radius);
    fillRound(c,RectF(r.X,r.Y+1.5f,r.Width,r.Height),radius,color(g.prefs.dark?18:9,0,0,0));
    const int lift=hover?12:0;
    LinearGradientBrush fill(r,color((g.prefs.dark?29:104)+lift,255,255,255),color((g.prefs.dark?10:42)+lift,255,255,255),LinearGradientModeVertical);
    c.FillPath(&fill,&path);
    LinearGradientBrush rim(r,color(g.prefs.dark?82:230,255,255,255),color(g.prefs.dark?23:74,255,255,255),LinearGradientModeVertical);
    Pen edge(&rim,1);c.DrawPath(&edge,&path);
    GraphicsPath inset;roundPath(inset,RectF(r.X+1,r.Y+1,r.Width-2,r.Height-2),radius-1);
    Pen depth(color(g.prefs.dark?18:8,0,0,0),.8f);c.DrawPath(&depth,&inset);
}
void text(Graphics& c,const std::wstring& s,RectF box,float size,Color ink,bool bold=false,StringAlignment align=StringAlignmentNear) {
    Font f(fontName,size,bold?FontStyleBold:FontStyleRegular,UnitPixel);
    StringFormat fmt;fmt.SetAlignment(align);fmt.SetLineAlignment(StringAlignmentCenter);
    fmt.SetFormatFlags(StringFormatFlagsNoWrap);fmt.SetTrimming(StringTrimmingEllipsisCharacter);
    SolidBrush b(ink);c.DrawString(s.c_str(),-1,&f,box,&fmt,&b);
}
std::wstring number(double n,int precision=2) { wchar_t b[64]{};swprintf(b,64,precision==0?L"%.0f":precision==1?L"%.1f":L"%.2f",n);return b; }
std::wstring gb(uint64_t bytes) { return number(double(bytes)/GiB)+L" GB"; }
void line(Graphics& c,float x1,float y1,float x2,float y2,Color ink,float width=1) { Pen p(ink,width);p.SetStartCap(LineCapRound);p.SetEndCap(LineCapRound);c.DrawLine(&p,x1,y1,x2,y2); }
void sparkle(Graphics& c,float x,float y,float r,Color ink) {
    PointF pts[]={{x,y-r},{x+r*.22f,y-r*.22f},{x+r,y},{x+r*.22f,y+r*.22f},{x,y+r},{x-r*.22f,y+r*.22f},{x-r,y},{x-r*.22f,y-r*.22f}};
    SolidBrush b(ink);c.FillPolygon(&b,pts,8);
}
void chip(Graphics& c,float x,float y,float size,Color ink) {
    const float k=size/24;GraphicsState st=c.Save();c.TranslateTransform(x,y);c.ScaleTransform(k,k);
    Pen p(ink,1.7f);GraphicsPath path;roundPath(path,RectF(4,4,16,16),4);c.DrawPath(&p,&path);
    for(float i: {8.f,12.f,16.f}){c.DrawLine(&p,i,1.f,i,4.f);c.DrawLine(&p,i,20.f,i,23.f);c.DrawLine(&p,1.f,i,4.f,i);c.DrawLine(&p,20.f,i,23.f,i);}
    c.DrawRectangle(&p,8,8,8,8);c.Restore(st);
}
void gear(Graphics& c,float x,float y,Color ink) {
    Pen p(ink,1.5f);p.SetStartCap(LineCapRound);p.SetEndCap(LineCapRound);c.DrawEllipse(&p,x-5,y-5,10.f,10.f);
    for(int i=0;i<8;++i){const double a=i*3.141592653589793/4;c.DrawLine(&p,x+float(cos(a)*6),y+float(sin(a)*6),x+float(cos(a)*8),y+float(sin(a)*8));}
}
void arrow(Graphics& c,float x,float y,Color ink) { line(c,x+8,y,x-7,y,ink,1.7f);line(c,x-7,y,x-1,y-6,ink,1.7f);line(c,x-7,y,x-1,y+6,ink,1.7f); }
void monitor(Graphics& c,float x,float y,Color ink) {
    GraphicsPath p;roundPath(p,RectF(x-7,y-6,14,10),2);Pen pen(ink,1.4f);c.DrawPath(&pen,&p);line(c,x,y+4,x,y+7,ink,1.4f);line(c,x-4,y+7,x+4,y+7,ink,1.4f);
}
void infoIcon(Graphics& c,float x,float y,Color ink) {
    Pen pen(ink,1.3f);c.DrawEllipse(&pen,x-7,y-7,14.f,14.f);line(c,x,y-1,x,y+4,ink,1.3f);SolidBrush dot(ink);c.FillEllipse(&dot,x-.8f,y-4.5f,1.6f,1.6f);
}
template<size_t N> std::unique_ptr<GraphicsPath> svgPath(const aeroram::SvgStep (&steps)[N]) {
    auto path=std::make_unique<GraphicsPath>(FillModeWinding);PointF last{};
    for(const auto& p:steps){
        if(p.kind==0){path->StartFigure();last=PointF(p.a,p.b);}
        else if(p.kind==1){path->AddLine(last,PointF(p.a,p.b));last=PointF(p.a,p.b);}
        else if(p.kind==2){path->AddBezier(last,PointF(p.a,p.b),PointF(p.c,p.d),PointF(p.e,p.f));last=PointF(p.e,p.f);}
        else path->CloseFigure();
    }
    return path;
}
void loadSocialIcons(){g.socialPaths[0]=svgPath(aeroram::instagramSvg);g.socialPaths[1]=svgPath(aeroram::xSvg);g.socialPaths[2]=svgPath(aeroram::linkedinSvg);}
void socialIcon(Graphics& c,int index,float x,float y,float size) {
    GraphicsState state=c.Save();c.TranslateTransform(x,y);c.ScaleTransform(size/256,size/256);
    RectF box(0,0,256,256);
    if(index==0){
        GraphicsPath shape;roundPath(shape,box,60);LinearGradientBrush gradient(PointF(0,256),PointF(210,0),color(255,255,221,85),color(255,102,0,255));
        Color colors[]={color(255,255,221,85),color(255,255,84,62),color(255,200,55,171),color(255,102,0,255)};REAL positions[]={0,.33f,.72f,1};gradient.SetInterpolationColors(colors,positions,4);c.FillPath(&gradient,&shape);
    }else fillRound(c,box,60,index==2?color(255,10,102,194):color(255,36,41,56));
    if(index<3 && g.socialPaths[index]){
        SolidBrush white(Color::White);GraphicsState glyph=c.Save();if(index==1){c.TranslateTransform(45,45);c.ScaleTransform(6.9167f,6.9167f);}c.FillPath(&white,g.socialPaths[index].get());c.Restore(glyph);
    }else if(index==3){
        Pen pen(color(255,56,189,248),9);c.DrawEllipse(&pen,49,49,158,158);c.DrawEllipse(&pen,94,49,68,158);c.DrawLine(&pen,49,128,207,128);c.DrawLine(&pen,65,83,191,83);c.DrawLine(&pen,65,173,191,173);
    }
    c.Restore(state);
}
void focusOutline(Graphics& c,RectF box,float radius,int id) {
    if(!g.keyboardFocus || g.focus!=id || GetFocus()!=g.window)return;
    GraphicsPath path;roundPath(path,RectF(box.X-2,box.Y-2,box.Width+4,box.Height+4),radius+2);Pen p(color(190,73,151,255),1);p.SetDashStyle(DashStyleDot);c.DrawPath(&p,&path);
}
float wrapped(Graphics& c,const std::wstring& s,float x,float y,float width,float size,Color ink,bool draw=true,bool bold=false) {
    Font f(fontName,size,bold?FontStyleBold:FontStyleRegular,UnitPixel);StringFormat format;format.SetTrimming(StringTrimmingNone);
    SizeF measured;c.MeasureString(s.c_str(),-1,&f,SizeF(width,10000),&format,&measured);
    const float height=std::max(size*1.45f,measured.Height+2);
    if(draw){SolidBrush brush(ink);c.DrawString(s.c_str(),-1,&f,RectF(x,y,width,height),&format,&brush);}
    return height;
}
void background(Graphics& c) {
    c.SetCompositingMode(CompositingModeSourceCopy);c.Clear(Color(0,0,0,0));c.SetCompositingMode(CompositingModeSourceOver);
    RectF body(16,16,428,672);GraphicsPath p;roundPath(p,body,32);
    const int a=255;
    Color top=g.prefs.dark?color(a,30,30,33):color(a,248,248,250);
    Color bottom=g.prefs.dark?color(std::min(a+12,255),18,18,21):color(std::min(a+20,255),233,234,238);
    LinearGradientBrush b(body,top,bottom,LinearGradientModeVertical);c.FillPath(&b,&p);
    LinearGradientBrush reflection(body,color(g.prefs.dark?18:34,255,255,255),color(0,255,255,255),LinearGradientModeForwardDiagonal);
    c.FillPath(&reflection,&p);
    LinearGradientBrush rim(body,color(g.prefs.dark?125:245,255,255,255),color(g.prefs.dark?30:100,255,255,255),LinearGradientModeVertical);
    Pen border(&rim,1.2f);c.DrawPath(&border,&p);
    GraphicsPath inner;roundPath(inner,RectF(17.3f,17.3f,425.4f,669.4f),30.7f);
    Pen depth(color(g.prefs.dark?50:15,0,0,0),.8f);c.DrawPath(&depth,&inner);
}
void header(Graphics& c) {
    auto p=palette();
    if(g.logo)c.DrawImage(g.logo.get(),RectF(35,35,43,27));
    else chip(c,46,40,20,g.prefs.dark?color(255,126,181,255):color(255,28,111,228));
    text(c,L"AeroRAM",RectF(83,34,230,31),16,p.ink,true);
    for(int id: {2,3}){
        const float x=id==2?369:405;
        if(g.hover==id)fillRound(c,RectF(x-13,33,27,28),10,id==3?color(24,243,80,91):color(30,89,146,205));
        if(id==2)line(c,x-4,47,x+4,47,p.muted,1.6f);
        else {line(c,x-4,43,x+4,51,p.muted,1.6f);line(c,x-4,51,x+4,43,p.muted,1.6f);}
        focusOutline(c,RectF(x-13,33,27,28),10,id);
    }
}
void mainView(Graphics& c,double time) {
    auto p=palette();
    text(c,L"Memoria RAM",RectF(40,96,285,35),25,p.ink,true);
    text(c,L"Memoria física de tu PC",RectF(41,132,280,25),13,p.muted);
    const float cx=230,cy=277,r=98;
    RectF sphereBox(cx-r+9,cy-r+9,(r-9)*2,(r-9)*2);
    LinearGradientBrush sphere(sphereBox,color(g.prefs.dark?32:100,255,255,255),color(g.prefs.dark?3:14,255,255,255),LinearGradientModeForwardDiagonal);c.FillEllipse(&sphere,sphereBox);
    Pen halo(g.prefs.dark?color(55,255,255,255):color(180,255,255,255),1);c.DrawEllipse(&halo,cx-r-7,cy-r-7,(r+7)*2,(r+7)*2);
    Pen track(g.prefs.dark?color(29,255,255,255):color(34,50,56,68),10);c.DrawEllipse(&track,cx-r,cy-r,r*2,r*2);
    LinearGradientBrush ringBrush(RectF(cx-r,cy-r,r*2,r*2),color(255,49,132,255),color(255,139,194,255),LinearGradientModeForwardDiagonal);
    Pen ring(&ringBrush,11);ring.SetStartCap(LineCapRound);ring.SetEndCap(LineCapRound);
    if(g.memory.valid && g.shownPercent>.01)c.DrawArc(&ring,cx-r,cy-r,r*2,r*2,-90,float(std::min(359.9,g.shownPercent*3.6)));
    Pen shine(g.prefs.dark?color(95,255,255,255):color(224,255,255,255),1.2f);c.DrawArc(&shine,cx-r+15,cy-r+15,(r-15)*2,(r-15)*2,194,92);
    if(g.busy){
        Pen spin(color(210,20,135,255),3);spin.SetStartCap(LineCapRound);spin.SetEndCap(LineCapRound);
        c.DrawArc(&spin,cx-108,cy-108,216.f,216.f,float(fmod(time*145,360)),64);
    }
    text(c,g.memory.valid?number(g.shownPercent,0)+L"%":L"—",RectF(cx-100,cy-39,200,75),56,p.ink,true,StringAlignmentCenter);
    text(c,L"en uso",RectF(cx-90,cy+30,180,27),14,p.muted,false,StringAlignmentCenter);
    text(c,g.memory.valid?gb(g.memory.used())+L" / "+gb(g.memory.total):L"Esperando datos",RectF(130,329,200,23),11,p.muted,false,StringAlignmentCenter);
    for(int i=0;i<2;++i){
        RectF box(i?240:40,397,180,83);
        glassCard(c,box,19);
        SolidBrush status(i?(g.prefs.dark?color(255,188,193,204):color(255,116,123,137)):color(255,61,143,255));c.FillEllipse(&status,box.X+16,box.Y+18,5.f,5.f);
        text(c,i?L"Disponible":L"En uso",RectF(box.X+29,box.Y+9,137,24),12,p.muted);
        text(c,g.memory.valid?gb(i?g.memory.available:g.memory.used()):L"—",RectF(box.X+15,box.Y+34,151,35),25,p.ink,true);
    }
    glassCard(c,RectF(40,497,380,58),17);
    text(c,L"Último minuto",RectF(54,507,105,19),11,p.muted);
    text(c,L"Uso de RAM",RectF(54,526,105,16),9,p.muted);
    if(g.history.count){
        const float x0=178,y0=508,gw=225,gh=34;
        line(c,x0,y0+gh,x0+gw,y0+gh,p.line,1);
        GraphicsPath graph;
        if(g.history.count==1)graph.AddLine(x0+gw-2,y0+gh-float(g.history.values[0]/100)*gh,x0+gw,y0+gh-float(g.history.values[0]/100)*gh);
        else {
            for(size_t i=1;i<g.history.count;++i){
                const float xa=x0+gw*float(i-1)/59,xb=x0+gw*float(i)/59;
                graph.AddLine(xa,y0+gh-float(g.history.values[i-1]/100)*gh,xb,y0+gh-float(g.history.values[i]/100)*gh);
            }
        }
        Pen stroke(color(255,36,144,247),1.8f);stroke.SetLineJoin(LineJoinRound);c.DrawPath(&stroke,&graph);
    }
    const bool allowed=g.prefs.cache||g.prefs.apps;
    const float push=g.pressed==0?1.4f:0;
    RectF button(40+push,575+push,380-push*2,58-push*2);
    fillRound(c,RectF(button.X,button.Y+4,button.Width,button.Height),29,color(40,24,111,244));
    GraphicsPath shape;roundPath(shape,button,29);
    LinearGradientBrush b(button,allowed?color(224,60+int(g.hoverMix*14),143+int(g.hoverMix*10),255):color(200,135,138,146),allowed?color(226,25,104,235):color(218,93,97,106),LinearGradientModeVertical);
    c.FillPath(&b,&shape);Pen edge(color(130,255,255,255),1);c.DrawPath(&edge,&shape);
    line(c,68,578+push,392,578+push,color(170,230,247,255),.8f);
    if(g.busy){Pen spin(Color::White,2);spin.SetStartCap(LineCapRound);spin.SetEndCap(LineCapRound);c.DrawArc(&spin,128.f,595.f,15.f,15.f,float(fmod(time*185,360)),235);}
    else sparkle(c,127,604,8,color(255,239,250,255));
    text(c,g.busy?L"Limpiando…":allowed?L"Limpiar memoria":L"Elige una opción",RectF(152,582+push,222,42),17,color(255,255,255,255),true);
    focusOutline(c,button,29,0);
    std::wstring status=g.result;
    if(status.empty() && g.preview)status=L"Vista previa · datos de ejemplo";
    if(!status.empty())text(c,status,RectF(43,555,374,18),10,g.result.empty()?p.muted:p.ink,false,StringAlignmentCenter);
    RectF system(40,641,147,31);glassCard(c,system,12,g.hover==5);monitor(c,57,656,p.muted);text(c,L"Sistema",RectF(74,641,102,31),12,p.ink,true);focusOutline(c,system,12,5);
    RectF credits(201,641,115,31);glassCard(c,credits,12,g.hover==6);infoIcon(c,217,656,p.muted);text(c,L"Créditos",RectF(232,641,79,31),12,p.ink);focusOutline(c,credits,12,6);
    gear(c,344,656,p.muted);text(c,L"Ajustes",RectF(359,644,65,24),12,p.muted);
    focusOutline(c,RectF(333,641,89,31),12,1);
}
void toggle(Graphics& c,float x,float y,double value) {
    const int base=g.prefs.dark?74:177;
    fillRound(c,RectF(x,y,43,25),12.5f,color(255,base+int(value*(41-base)),base+int(value*(132-base)),base+int(value*(250-base))),color(g.prefs.dark?65:140,255,255,255));
    SolidBrush shadow(color(37,13,53,96));c.FillEllipse(&shadow,x+3+float(value*18),y+3,20.f,20.f);
    SolidBrush white(color(255,255,255,255));c.FillEllipse(&white,x+3+float(value*18),y+2,20.f,20.f);
}
void settingsView(Graphics& c) {
    auto p=palette();arrow(c,51,111,p.ink);text(c,L"Ajustes",RectF(76,93,220,38),25,p.ink,true);
    text(c,L"A tu manera, sin complicaciones.",RectF(40,137,370,25),13,p.muted);
    const wchar_t* names[]={L"Limpiar caché",L"Reducir memoria de apps",L"Animaciones",L"Minimizar al cerrar",L"Modo oscuro"};
    const wchar_t* hints[]={L"Revisar la memoria en espera de baja prioridad.",L"Las apps pueden recargar datos al usarlas.",L"Transiciones suaves y efectos de cristal.",L"Conservar el icono junto al reloj.",L"Cristal oscuro en gris carbón."};
    for(int i=0;i<5;++i){float y=183+i*84;
        glassCard(c,RectF(40,y-3,380,74),17,g.hover==11+i);
        text(c,names[i],RectF(55,y+1,277,27),14,p.ink,true);
        text(c,hints[i],RectF(55,y+29,280,30),10,p.muted);
        toggle(c,359,y+13,g.toggle[i]);
        focusOutline(c,RectF(40,y-3,380,74),17,11+i);
    }
    text(c,std::wstring(L"AeroRAM · ")+aeroram::Version,RectF(41,635,295,22),12,p.muted);
    text(c,L"Consumo de la app: "+(g.ownMB>0?number(g.ownMB,1):L"—")+L" MB",RectF(41,656,300,20),10,p.muted);
    text(c,L"Listo",RectF(365,640,57,30),13,color(255,24,133,246),true);
    focusOutline(c,RectF(350,635,75,40),12,10);
}
void systemView(Graphics& c,double time) {
    auto p=palette();arrow(c,51,111,p.ink);text(c,L"Tu equipo",RectF(76,93,260,38),25,p.ink,true);
    text(c,g.preview?L"Vista previa · datos de ejemplo":g.infoLoading?L"Leyendo componentes de tu equipo…":L"Hardware, Windows y diagnóstico",RectF(40,137,370,25),13,p.muted);
    focusOutline(c,RectF(38,93,36,36),12,10);
    for(int n=0;n<8;++n){
        const float x=40+(n%4)*97.f,y=178+(n/4)*38.f;RectF tab(x,y,89,30);
        if(g.infoPage==n){fillRound(c,tab,11,g.prefs.dark?color(160,42,104,188):color(205,255,255,255),g.prefs.dark?color(95,125,184,255):color(230,255,255,255));}
        else if(g.hover==20+n)glassCard(c,tab,11,true);
        text(c,SpecCategories[n],tab,n==7?9:11,g.infoPage==n?p.ink:p.muted,g.infoPage==n,StringAlignmentCenter);focusOutline(c,tab,11,20+n);
    }
    RectF card(40,259,380,352);glassCard(c,card,19);
    if(g.info.pages[g.infoPage].empty()){
        if(g.infoLoading && !g.info.ready[g.infoPage]){
            Pen spin(color(220,72,148,255),3);spin.SetStartCap(LineCapRound);spin.SetEndCap(LineCapRound);c.DrawArc(&spin,218.f,367.f,24.f,24.f,float(fmod(time*140,360)),225);
            text(c,L"Consultando Windows",RectF(65,405,330,30),14,p.ink,true,StringAlignmentCenter);text(c,L"Puedes cambiar de apartado mientras carga.",RectF(65,439,330,25),11,p.muted,false,StringAlignmentCenter);
        }else{
            text(c,L"Sin datos reportados",RectF(65,398,330,30),15,p.ink,true,StringAlignmentCenter);
            text(c,L"Prueba Actualizar o abre dxdiag.",RectF(65,434,330,25),11,p.muted,false,StringAlignmentCenter);
        }
        g.infoContentHeight=0;
    }else{
        const auto& blocks=g.info.pages[g.infoPage];float contentHeight=14;
        for(const auto& b:blocks){contentHeight+=wrapped(c,b.title,0,0,328,13,p.ink,false,true)+8;for(const auto& f:b.fields)contentHeight+=18+wrapped(c,f.value,0,0,328,12,p.ink,false)+12;contentHeight+=16;}
        g.infoContentHeight=contentHeight;const float visibleHeight=326;
        g.infoScroll=std::clamp(g.infoScroll,0.f,std::max(0.f,contentHeight-visibleHeight));
        GraphicsState scroll=c.Save();c.SetClip(RectF(50,269,351,332));float y=273-g.infoScroll;
        for(const auto& b:blocks){
            y+=wrapped(c,b.title,55,y,328,13,p.ink,true,true)+8;
            for(const auto& f:b.fields){
                text(c,f.label,RectF(55,y,328,18),10,p.muted);y+=18;
                y+=wrapped(c,f.value,55,y,328,12,p.ink)+12;
            }
            line(c,55,y-3,392,y-3,p.line);y+=16;
        }
        c.Restore(scroll);
        if(contentHeight>visibleHeight){
            const float track=326,thumb=std::max(26.f,track*visibleHeight/contentHeight),position=g.infoScroll/(contentHeight-visibleHeight)*(track-thumb);
            fillRound(c,RectF(407,272,3,track),1.5f,color(g.prefs.dark?15:18,130,134,143));fillRound(c,RectF(407,272+position,3,thumb),1.5f,color(g.prefs.dark?160:105,154,160,171));
        }
    }
    const bool showNotice=GetTickCount64()<g.noticeUntil;
    const std::wstring status=showNotice?g.infoNotice:g.infoLoading?L"La lectura continúa en segundo plano":L"Lectura: "+g.info.collectedAt;
    text(c,status,RectF(42,615,287,18),9,p.muted);
    text(c,L"Actualizar",RectF(339,613,83,22),10,g.infoLoading?p.muted:color(255,57,137,248),true);focusOutline(c,RectF(336,610,87,25),10,31);
    for(int n=0;n<2;++n){RectF button(n?240:40,642,180,31);glassCard(c,button,12,g.hover==30+n*2);text(c,n?L"Abrir dxdiag":L"Copiar informe",button,12,p.ink,true,StringAlignmentCenter);focusOutline(c,button,12,30+n*2);}
}
void creditsView(Graphics& c) {
    auto p=palette();arrow(c,51,111,p.ink);text(c,L"Créditos",RectF(76,93,260,38),25,p.ink,true);text(c,L"Desarrollo y detalles del programa",RectF(40,137,370,25),13,p.muted);
    glassCard(c,RectF(40,180,380,220),22);
    if(g.logo)c.DrawImage(g.logo.get(),RectF(171,193,118,66));
    text(c,L"AeroRAM",RectF(55,262,350,36),27,p.ink,true,StringAlignmentCenter);
    text(c,std::wstring(L"Versión ")+aeroram::Version,RectF(55,302,350,23),12,p.muted,false,StringAlignmentCenter);
    line(c,74,333,386,333,p.line);
    text(c,L"Desarrollador",RectF(55,343,350,19),10,p.muted,false,StringAlignmentCenter);
    text(c,L"Cristian Skylar",RectF(55,363,350,29),19,p.ink,true,StringAlignmentCenter);
    glassCard(c,RectF(40,415,380,95),18);
    text(c,L"Sobre el programa",RectF(55,425,350,24),13,p.ink,true);
    wrapped(c,L"Monitor de memoria RAM, limpieza manual y diagnóstico del sistema para Windows.",55,455,350,11,p.muted);
    for(int n=0;n<4;++n){
        const float x=40+n*98.f;RectF tile(x,528,86,80);glassCard(c,tile,17,g.hover==40+n);socialIcon(c,n,x+25,539,36);
        text(c,aeroram::Socials[n].name,RectF(x+3,580,80,22),11,p.muted,false,StringAlignmentCenter);focusOutline(c,tile,17,40+n);
    }
    text(c,L"© 2026 Cristian Skylar",RectF(41,641,284,25),11,p.muted);text(c,L"Volver",RectF(357,640,65,31),12,color(255,57,137,248),true);focusOutline(c,RectF(350,635,75,40),12,10);
}
void paint(Graphics& c,double time) {
    c.SetSmoothingMode(SmoothingModeAntiAlias);c.SetPixelOffsetMode(PixelOffsetModeHighQuality);
    c.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);c.SetCompositingQuality(CompositingQualityHighQuality);
    background(c);
    GraphicsState frame=c.Save();GraphicsPath silhouette;roundPath(silhouette,RectF(Inset,Inset,WindowW,WindowH),32);c.SetClip(&silhouette,CombineModeIntersect);header(c);
    GraphicsState content=c.Save();c.SetClip(RectF(17,79,426,607),CombineModeIntersect);
    if(g.panel<.999){GraphicsState st=c.Save();c.TranslateTransform(float(-g.panel*W),0);mainView(c,time);c.Restore(st);}
    if(g.panel>.001){GraphicsState st=c.Save();c.TranslateTransform(float((1-g.panel)*W),0);if(g.about)creditsView(c);else if(g.system)systemView(c,time);else settingsView(c);c.Restore(st);}
    c.Restore(content);
    c.Restore(frame);
}
void releaseSurface(){g.surface.reset();g.outline.reset(0,0,0);if(g.memoryDC){SelectObject(g.memoryDC,g.oldDib);DeleteObject(g.dib);DeleteDC(g.memoryDC);}g.memoryDC=nullptr;g.dib=nullptr;g.oldDib=nullptr;g.pixels=nullptr;}
bool makeSurface(){
    releaseSurface();g.pixelW=int(WindowW*g.dpiScale+.5f);g.pixelH=int(WindowH*g.dpiScale+.5f);
    BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=g.pixelW;info.bmiHeader.biHeight=-g.pixelH;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
    HDC screen=GetDC(nullptr);g.memoryDC=CreateCompatibleDC(screen);g.dib=CreateDIBSection(screen,&info,DIB_RGB_COLORS,&g.pixels,nullptr,0);ReleaseDC(nullptr,screen);
    if(!g.memoryDC||!g.dib||!g.pixels){releaseSurface();return false;}
    g.oldDib=static_cast<HBITMAP>(SelectObject(g.memoryDC,g.dib));
    g.surface=std::make_unique<Bitmap>(g.pixelW,g.pixelH,g.pixelW*4,PixelFormat32bppPARGB,static_cast<BYTE*>(g.pixels));
    g.outline.reset(g.pixelW,g.pixelH,32*g.dpiScale);
    return g.surface->GetLastStatus()==Ok;
}
void render(){
    if(!g.surface || !g.visible || g.rendering)return;
    g.rendering=true;
    {Graphics c(g.surface.get());c.TranslateTransform(-Inset*g.dpiScale,-Inset*g.dpiScale);c.ScaleTransform(g.dpiScale,g.dpiScale);c.SetInterpolationMode(InterpolationModeHighQualityBicubic);paint(c,GetTickCount64()/1000.0);}
    g.outline.apply(static_cast<std::uint8_t*>(g.pixels),g.pixelW*4);
    RECT rect{};GetWindowRect(g.window,&rect);POINT pos{rect.left,rect.top},src{0,0};SIZE size{g.pixelW,g.pixelH};BLENDFUNCTION blend{AC_SRC_OVER,0,255,AC_SRC_ALPHA};
    HDC screen=GetDC(nullptr);UpdateLayeredWindow(g.window,screen,&pos,&size,g.memoryDC,&src,0,&blend,ULW_ALPHA);ReleaseDC(nullptr,screen);
    g.rendering=false;
}
void loadPreferences(){
    wchar_t path[MAX_PATH]{};
    if(SUCCEEDED(SHGetFolderPathW(nullptr,CSIDL_LOCAL_APPDATA,nullptr,SHGFP_TYPE_CURRENT,path))){
        const std::wstring legacy=std::wstring(path)+L"\\SkylarMemory\\preferences.ini";
        g.config=std::wstring(path)+L"\\AeroRAM";CreateDirectoryW(g.config.c_str(),nullptr);g.config+=L"\\preferences.ini";
        if(GetFileAttributesW(g.config.c_str())==INVALID_FILE_ATTRIBUTES)CopyFileW(legacy.c_str(),g.config.c_str(),TRUE);
    }
    auto get=[&](const wchar_t* key,bool def){return GetPrivateProfileIntW(L"preferences",key,def?1:0,g.config.c_str())!=0;};
    if(!g.config.empty()){g.prefs.cache=get(L"cache",true);g.prefs.apps=get(L"apps",true);g.prefs.animations=get(L"animations",true);g.prefs.closeToTray=get(L"tray",true);g.prefs.dark=get(L"dark",false);}
    g.toggle={double(g.prefs.cache),double(g.prefs.apps),double(g.prefs.animations),double(g.prefs.closeToTray),double(g.prefs.dark)};
}
void savePreferences(){
    if(g.config.empty())return;
    auto put=[&](const wchar_t* k,bool v){WritePrivateProfileStringW(L"preferences",k,v?L"1":L"0",g.config.c_str());};
    put(L"cache",g.prefs.cache);put(L"apps",g.prefs.apps);put(L"animations",g.prefs.animations);put(L"tray",g.prefs.closeToTray);put(L"dark",g.prefs.dark);
}
void configureFrame(){
    MARGINS margins{};DwmExtendFrameIntoClientArea(g.window,&margins);
    DWMNCRENDERINGPOLICY policy=DWMNCRP_DISABLED;DwmSetWindowAttribute(g.window,DWMWA_NCRENDERING_POLICY,&policy,sizeof(policy));
}
void loadLogo(HINSTANCE instance){
    HRSRC resource=FindResourceW(instance,MAKEINTRESOURCEW(201),RT_RCDATA);if(!resource)return;
    const DWORD size=SizeofResource(instance,resource);HGLOBAL loaded=LoadResource(instance,resource);const void* data=loaded?LockResource(loaded):nullptr;if(!data || !size)return;
    HGLOBAL memory=GlobalAlloc(GMEM_MOVEABLE,size);if(!memory)return;void* bytes=GlobalLock(memory);if(!bytes){GlobalFree(memory);return;}std::memcpy(bytes,data,size);GlobalUnlock(memory);
    if(FAILED(CreateStreamOnHGlobal(memory,TRUE,&g.logoStream))){GlobalFree(memory);return;}
    g.logo=std::make_unique<Bitmap>(g.logoStream);if(g.logo->GetLastStatus()!=Ok){g.logo.reset();g.logoStream->Release();g.logoStream=nullptr;}
}
void releaseLogo(){for(auto& path:g.socialPaths)path.reset();g.logo.reset();if(g.logoStream){g.logoStream->Release();g.logoStream=nullptr;}}
void updateTray(){
    std::wstring tip=L"AeroRAM · "+(g.memory.valid?number(g.memory.usedPercent(),0)+L"% RAM":L"sin datos");
    wcsncpy(g.tray.szTip,tip.c_str(),127);g.tray.szTip[127]=0;g.tray.uFlags=NIF_TIP;Shell_NotifyIconW(NIM_MODIFY,&g.tray);
}
void addTray(){
    g.tray={};g.tray.cbSize=sizeof(g.tray);g.tray.hWnd=g.window;g.tray.uID=1;g.tray.uFlags=NIF_MESSAGE|NIF_ICON|NIF_TIP;
    g.tray.uCallbackMessage=MSG_TRAY;g.tray.hIcon=g.icon;wcscpy(g.tray.szTip,L"AeroRAM");Shell_NotifyIconW(NIM_ADD,&g.tray);
    g.tray.uVersion=NOTIFYICON_VERSION_4;Shell_NotifyIconW(NIM_SETVERSION,&g.tray);updateTray();
}
void frameTimer(){
    if(g.visible && g.prefs.animations)SetTimer(g.window,TIMER_ANIM,33,nullptr);else KillTimer(g.window,TIMER_ANIM);
}
void show(){g.visible=true;ShowWindow(g.window,SW_RESTORE);SetForegroundWindow(g.window);SetFocus(g.window);g.lastFrame=GetTickCount64();frameTimer();render();}
void hide(){g.visible=false;KillTimer(g.window,TIMER_ANIM);ShowWindow(g.window,SW_HIDE);}
void notice(const std::wstring& s){g.infoNotice=s;g.noticeUntil=GetTickCount64()+3500;}
struct InfoJob{HWND window;std::shared_ptr<std::atomic_bool> cancelled;};
DWORD WINAPI infoWorker(void* ptr){
    std::unique_ptr<InfoJob> job(static_cast<InfoJob*>(ptr));
    auto publish=[&](const SystemInfo& info){
        if(*job->cancelled)return;
        auto update=std::make_unique<SystemInfo>(info);
        if(PostMessageW(job->window,MSG_INFO,0,reinterpret_cast<LPARAM>(update.get())))update.release();
    };
    try{auto info=readSystemInfo(*job->cancelled,publish);publish(info);}
    catch(...){SystemInfo error;error.ready.fill(true);error.complete=true;error.note=L"No se pudo completar la lectura. Prueba Actualizar o abre dxdiag.";publish(error);}
    return 0;
}
void refreshSystem(){
    if(g.infoLoading || g.preview)return;
    g.infoLoading=true;g.info=SystemInfo{};g.infoScroll=0;g.infoCancel=std::make_shared<std::atomic_bool>(false);
    auto job=std::make_unique<InfoJob>();job->window=g.window;job->cancelled=g.infoCancel;
    HANDLE thread=CreateThread(nullptr,0,infoWorker,job.get(),0,nullptr);
    if(thread){job.release();CloseHandle(thread);}else{g.infoLoading=false;notice(L"No se pudo iniciar la lectura del equipo.");}
}
void copySystemReport(){
    if(!g.info.complete){notice(L"Espera a que termine la lectura.");return;}
    const auto report=specReport(g.info);const SIZE_T bytes=(report.size()+1)*sizeof(wchar_t);
    HGLOBAL memory=GlobalAlloc(GMEM_MOVEABLE,bytes);if(!memory){notice(L"No se pudo copiar el informe.");return;}
    void* buffer=GlobalLock(memory);if(!buffer){GlobalFree(memory);notice(L"No se pudo copiar el informe.");return;}
    std::memcpy(buffer,report.c_str(),bytes);GlobalUnlock(memory);
    if(!OpenClipboard(g.window)){GlobalFree(memory);notice(L"El portapapeles está ocupado. Inténtalo otra vez.");return;}
    EmptyClipboard();const bool ok=SetClipboardData(CF_UNICODETEXT,memory)!=nullptr;CloseClipboard();if(!ok)GlobalFree(memory);notice(ok?L"Informe completo copiado":L"No se pudo copiar el informe.");
}
void openDxdiag(){
    wchar_t directory[MAX_PATH]{};const UINT length=GetSystemDirectoryW(directory,MAX_PATH);
    if(!length || length>=MAX_PATH){notice(L"No se encontró dxdiag en Windows.");return;}
    const auto path=std::wstring(directory)+L"\\dxdiag.exe";
    if(reinterpret_cast<INT_PTR>(ShellExecuteW(g.window,L"open",path.c_str(),nullptr,nullptr,SW_SHOWNORMAL))<=32)notice(L"Windows no pudo abrir dxdiag.");
}
void openSocial(int index){
    if(index<0 || index>=int(aeroram::Socials.size()) || g.preview)return;
    if(reinterpret_cast<INT_PTR>(ShellExecuteW(g.window,L"open",aeroram::Socials[index].url,nullptr,nullptr,SW_SHOWNORMAL))<=32)
        MessageBoxW(g.window,L"Windows no pudo abrir el enlace en tu navegador.",L"AeroRAM",MB_OK|MB_ICONINFORMATION);
}
void scrollSystem(float delta){g.infoScroll=std::clamp(g.infoScroll+delta,0.f,std::max(0.f,g.infoContentHeight-326));}
void dragScroll(float y){
    const float maximum=std::max(0.f,g.infoContentHeight-326);if(maximum<=0)return;
    const float thumb=std::max(26.f,326.f*326.f/g.infoContentHeight);
    g.infoScroll=std::clamp((y-272-g.scrollGrab)/(326-thumb)*maximum,0.f,maximum);
}
struct Job{HWND window;CleanOptions options;};
DWORD WINAPI cleanWorker(void* ptr){
    std::unique_ptr<Job> job(static_cast<Job*>(ptr));auto result=std::make_unique<CleanResult>(cleanMemory(job->options));
    if(PostMessageW(job->window,MSG_CLEAN,0,reinterpret_cast<LPARAM>(result.get())))result.release();
    return 0;
}
void beginClean(){
    if(g.busy || !(g.prefs.cache||g.prefs.apps))return;
    g.busy=true;g.cleanStart=GetTickCount64();g.result.clear();
    auto job=std::make_unique<Job>();job->window=g.window;job->options={g.prefs.cache,g.prefs.apps};
    HANDLE thread=CreateThread(nullptr,0,cleanWorker,job.get(),0,nullptr);
    if(thread){job.release();CloseHandle(thread);}else{g.busy=false;g.result=L"No se pudo iniciar la limpieza.";g.detail=L"Windows no permitió crear el hilo de limpieza.";}
    render();
}
void quit(){if(g.infoCancel)*g.infoCancel=true;if(g.busy){g.exitAfterClean=true;hide();}else DestroyWindow(g.window);}
void trayMenu(){
    POINT p{};GetCursorPos(&p);HMENU menu=CreatePopupMenu();AppendMenuW(menu,MF_STRING,1,L"Abrir AeroRAM");AppendMenuW(menu,MF_STRING|(g.busy?MF_GRAYED:0),2,L"Limpiar memoria");AppendMenuW(menu,MF_STRING,4,L"Créditos");AppendMenuW(menu,MF_SEPARATOR,0,nullptr);AppendMenuW(menu,MF_STRING,3,L"Salir");
    SetForegroundWindow(g.window);int cmd=TrackPopupMenu(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,p.x,p.y,0,g.window,nullptr);DestroyMenu(menu);PostMessageW(g.window,WM_NULL,0,0);
    if(cmd==1)show();else if(cmd==2)beginClean();else if(cmd==3)quit();else if(cmd==4){g.settings=true;g.about=true;g.system=false;g.focus=10;g.panel=1;show();}
}
bool inside(float x,float y,float bx,float by,float bw,float bh){return x>=bx&&x<=bx+bw&&y>=by&&y<=by+bh;}
int hit(float x,float y){
    if(inside(x,y,356,33,27,28))return 2;
    if(inside(x,y,392,33,27,28))return 3;
    if(g.settings && g.about){
        if(inside(x,y,38,93,36,36)||inside(x,y,350,635,75,40))return 10;
        for(int n=0;n<4;++n)if(inside(x,y,40+n*98.f,528,86,80))return 40+n;
    }else if(g.settings && g.system){
        if(inside(x,y,38,93,36,36))return 10;
        for(int n=0;n<8;++n)if(inside(x,y,40+(n%4)*97.f,178+(n/4)*38.f,89,30))return 20+n;
        if(inside(x,y,40,642,180,31))return 30;
        if(inside(x,y,336,610,87,25))return 31;
        if(inside(x,y,240,642,180,31))return 32;
        if(g.infoContentHeight>326 && inside(x,y,400,270,17,331))return 33;
    }else if(g.settings){if(inside(x,y,38,93,36,36)||inside(x,y,350,629,76,44))return 10;
        for(int i=0;i<5;++i)if(inside(x,y,40,180+i*84,380,74))return 11+i;
    }else {if(inside(x,y,40,575,380,58))return 0;if(inside(x,y,333,636,96,35))return 1;if(inside(x,y,40,641,147,31))return 5;if(inside(x,y,201,641,115,31))return 6;if(!g.result.empty()&&inside(x,y,40,553,380,21))return 4;}
    return -1;
}
void activate(int id){
    if(id==0)beginClean();
    else if(id==1){g.settings=true;g.system=false;g.about=false;g.focus=10;if(!g.prefs.animations)g.panel=1;}
    else if(id==2)ShowWindow(g.window,SW_MINIMIZE);
    else if(id==3){if(g.prefs.closeToTray)hide();else quit();}
    else if(id==4 && !g.detail.empty())MessageBoxW(g.window,g.detail.c_str(),L"Resultado de la limpieza",MB_OK|MB_ICONINFORMATION);
    else if(id==5){g.settings=true;g.system=true;g.about=false;g.focus=10;if(!g.prefs.animations)g.panel=1;if(!g.info.complete && !g.infoLoading)refreshSystem();}
    else if(id==6){g.settings=true;g.system=false;g.about=true;g.focus=10;if(!g.prefs.animations)g.panel=1;}
    else if(id==10){g.settings=false;g.focus=0;if(!g.prefs.animations)g.panel=0;}
    else if(id>=11&&id<=15){
        bool* values[]={&g.prefs.cache,&g.prefs.apps,&g.prefs.animations,&g.prefs.closeToTray,&g.prefs.dark};*values[id-11]=!*values[id-11];savePreferences();
        if(!g.prefs.animations){g.panel=g.settings?1:0;g.toggle={double(g.prefs.cache),double(g.prefs.apps),double(g.prefs.animations),double(g.prefs.closeToTray),double(g.prefs.dark)};}
        frameTimer();
    }
    else if(id>=20 && id<28){g.infoPage=id-20;g.infoScroll=0;g.focus=id;}
    else if(id==30)copySystemReport();
    else if(id==31)refreshSystem();
    else if(id==32)openDxdiag();
    else if(id>=40 && id<44)openSocial(id-40);
    render();
}
std::vector<int> tabOrder(){
    if(g.settings && g.about)return {10,40,41,42,43,2,3};
    if(g.settings && g.system)return {10,20,21,22,23,24,25,26,27,30,31,32,2,3};
    if(g.settings)return {10,11,12,13,14,15,2,3};
    std::vector<int> order={0,5,6,1,2,3};if(!g.result.empty())order.insert(order.begin()+1,4);return order;
}
float mouseX(LPARAM l){return GET_X_LPARAM(l)/g.dpiScale+Inset;}
float mouseY(LPARAM l){return GET_Y_LPARAM(l)/g.dpiScale+Inset;}
LRESULT CALLBACK procedure(HWND hwnd,UINT message,WPARAM w,LPARAM l){
    static UINT taskbar=RegisterWindowMessageW(L"TaskbarCreated");
    if(message==taskbar){addTray();return 0;}
    switch(message){
    case WM_NCCREATE:g.window=hwnd;return TRUE;
    case WM_NCHITTEST:{POINT p{GET_X_LPARAM(l),GET_Y_LPARAM(l)};ScreenToClient(hwnd,&p);if(!g.outline.contains(p.x,p.y))return HTTRANSPARENT;float x=p.x/g.dpiScale+Inset,y=p.y/g.dpiScale+Inset;if(hit(x,y)==-1&&y>=16&&y<80&&x>16&&x<W-16)return HTCAPTION;return HTCLIENT;}
    case WM_ERASEBKGND:return 1;
    case WM_PAINT:{PAINTSTRUCT p{};BeginPaint(hwnd,&p);EndPaint(hwnd,&p);render();return 0;}
    case WM_MOUSEMOVE:{if(g.pressed==33){dragScroll(mouseY(l));render();return 0;}int old=g.hover;g.hover=hit(mouseX(l),mouseY(l));if(!g.tracking){TRACKMOUSEEVENT t{sizeof(t),TME_LEAVE,hwnd,0};TrackMouseEvent(&t);g.tracking=true;}SetCursor(LoadCursorW(nullptr,g.hover>=0?IDC_HAND:IDC_ARROW));if(old!=g.hover || !g.prefs.animations)render();return 0;}
    case WM_MOUSELEAVE:g.hover=-1;g.tracking=false;render();return 0;
    case WM_LBUTTONDOWN:{g.keyboardFocus=false;g.pressed=hit(mouseX(l),mouseY(l));SetCapture(hwnd);SetFocus(hwnd);
        if(g.pressed==33){const float thumb=std::max(26.f,326.f*326.f/g.infoContentHeight),top=272+g.infoScroll/(g.infoContentHeight-326)*(326-thumb);const float y=mouseY(l);g.scrollGrab=y>=top && y<=top+thumb?y-top:thumb/2;dragScroll(y);}
        render();return 0;}
    case WM_LBUTTONUP:{int id=hit(mouseX(l),mouseY(l));int old=g.pressed;g.pressed=-1;ReleaseCapture();if(old==id&&id>=0 && id!=33)activate(id);else render();return 0;}
    case WM_CAPTURECHANGED:g.pressed=-1;return 0;
    case WM_MOUSEWHEEL:if(g.settings && g.system){scrollSystem(-GET_WHEEL_DELTA_WPARAM(w)/120.f*84);render();return 0;}break;
    case WM_KEYDOWN:
        if(w==VK_TAB){g.keyboardFocus=true;const auto order=tabOrder();const auto it=std::find(order.begin(),order.end(),g.focus);int index=it==order.end()?0:int(it-order.begin());index=(index+(GetKeyState(VK_SHIFT)<0?int(order.size())-1:1))%int(order.size());g.focus=order[size_t(index)];render();return 0;}
        if(w==VK_RETURN||w==VK_SPACE){activate(g.focus);return 0;}
        if(g.settings && g.system){
            if(w=='C' && GetKeyState(VK_CONTROL)<0){activate(30);return 0;}if(w==VK_F5){activate(31);return 0;}
            if((w==VK_LEFT || w==VK_RIGHT) && g.focus>=20 && g.focus<28){activate(20+(g.infoPage+(w==VK_LEFT?7:1))%8);return 0;}
            if(w==VK_UP || w==VK_DOWN || w==VK_PRIOR || w==VK_NEXT || w==VK_HOME || w==VK_END){
                if(w==VK_HOME)g.infoScroll=0;else if(w==VK_END)g.infoScroll=std::max(0.f,g.infoContentHeight-326);else scrollSystem((w==VK_UP || w==VK_PRIOR?-1.f:1.f)*(w==VK_PRIOR || w==VK_NEXT?280:48));render();return 0;
            }
        }
        if(w==VK_ESCAPE){if(g.settings)activate(10);else hide();return 0;}break;
    case WM_SETFOCUS:case WM_KILLFOCUS:render();return 0;
    case WM_SIZE:
        if(w==SIZE_MINIMIZED){g.visible=false;KillTimer(hwnd,TIMER_ANIM);}else{g.visible=true;frameTimer();render();}return 0;
    case WM_DPICHANGED:{g.rendering=true;g.dpiScale=HIWORD(w)/96.f;RECT* r=reinterpret_cast<RECT*>(l);SetWindowPos(hwnd,nullptr,r->left,r->top,int(WindowW*g.dpiScale+.5f),int(WindowH*g.dpiScale+.5f),SWP_NOZORDER|SWP_NOACTIVATE);const bool ready=makeSurface();g.rendering=false;if(!ready){MessageBoxW(hwnd,L"No se pudo crear la superficie de la interfaz.",L"AeroRAM",MB_ICONERROR);DestroyWindow(hwnd);}else{configureFrame();render();}return 0;}
    case WM_TIMER:
        if(w==TIMER_DATA){
            g.memory=readMemory();g.ownMB=appMemoryMB();if(g.memory.valid)g.history.push(g.memory.usedPercent());updateTray();
            if(!g.prefs.animations)g.shownPercent=g.memory.usedPercent();
            if(!g.busy&&g.resultTime&&GetTickCount64()-g.resultTime>12000){g.result.clear();g.resultTime=0;}
            if(g.visible)render();
        }else if(w==TIMER_ANIM&&g.visible){
            ULONGLONG now=GetTickCount64();double dt=g.lastFrame?(now-g.lastFrame)/1000.0:.033;g.lastFrame=now;
            double old=g.shownPercent,oldPanel=g.panel,oldHover=g.hoverMix;
            g.shownPercent=smoothToward(g.shownPercent,g.memory.usedPercent(),dt,7);
            g.panel=smoothToward(g.panel,g.settings?1:0,dt,13);g.hoverMix=smoothToward(g.hoverMix,g.hover==0?1:0,dt,14);
            bool toggleChanged=false;bool v[]={g.prefs.cache,g.prefs.apps,g.prefs.animations,g.prefs.closeToTray,g.prefs.dark};
            for(int i=0;i<5;++i){double prev=g.toggle[i];g.toggle[i]=smoothToward(prev,v[i]?1:0,dt,16);toggleChanged|=prev!=g.toggle[i];}
            if(g.busy||(g.settings && g.system && g.infoLoading && g.info.pages[g.infoPage].empty() && !g.info.ready[g.infoPage])||old!=g.shownPercent||oldPanel!=g.panel||oldHover!=g.hoverMix||toggleChanged)render();
        }return 0;
    case MSG_CLEAN:{
        std::unique_ptr<CleanResult> r(reinterpret_cast<CleanResult*>(l));g.busy=false;g.detail=r->details;g.memory=readMemory();g.resultTime=GetTickCount64();
        Outcome o=outcome(r->requested,r->succeeded);
        if(o==Outcome::Failed)g.result=L"No se pudo limpiar. Ver detalles.";
        else if(o==Outcome::Partial)g.result=L"Limpieza parcial. Ver detalles.";
        else if(!r->before.valid||!r->after.valid)g.result=L"Limpieza completada. Sin medición.";
        else{int64_t delta=availableChange(r->before,r->after);g.result=delta>0?L"RAM disponible: +"+number(double(delta)/MiB,0)+L" MB":L"Completado · sin aumento de RAM";}
        if(g.exitAfterClean)DestroyWindow(hwnd);else render();return 0;
    }
    case MSG_INFO:{std::unique_ptr<SystemInfo> info(reinterpret_cast<SystemInfo*>(l));g.info=std::move(*info);g.infoLoading=!g.info.complete;if(g.visible)render();return 0;}
    case MSG_TRAY:{UINT event=LOWORD(l);if(event==NIN_SELECT||event==NIN_KEYSELECT||event==WM_LBUTTONDBLCLK)show();else if(event==WM_CONTEXTMENU||event==WM_RBUTTONUP)trayMenu();return 0;}
    case WM_CLOSE:if(g.prefs.closeToTray)hide();else quit();return 0;
    case WM_DESTROY:if(g.infoCancel)*g.infoCancel=true;KillTimer(hwnd,TIMER_DATA);KillTimer(hwnd,TIMER_ANIM);Shell_NotifyIconW(NIM_DELETE,&g.tray);releaseSurface();releaseLogo();PostQuitMessage(0);return 0;
    }
    return DefWindowProcW(hwnd,message,w,l);
}
int pngEncoder(CLSID* id){UINT num=0,size=0;GetImageEncodersSize(&num,&size);if(!size)return -1;std::unique_ptr<BYTE[]> data(new BYTE[size]);auto enc=reinterpret_cast<ImageCodecInfo*>(data.get());GetImageEncoders(num,size,enc);for(UINT i=0;i<num;++i)if(wcscmp(enc[i].MimeType,L"image/png")==0){*id=enc[i].Clsid;return int(i);}return -1;}
int renderDemo(const wchar_t* prefix){
    g.preview=true;g.prefs={};g.visible=true;
    g.memory.valid=true;g.memory.total=8ULL*1024*1024*1024;g.memory.available=3ULL*1024*1024*1024;
    g.shownPercent=62.5;g.ownMB=0;g.toggle={1,1,1,1,0};
    for(int i=0;i<60;++i)g.history.push(62+sin(i*.13)*2+cos(i*.34));
    CLSID encoder{};if(pngEncoder(&encoder)<0)return 2;
    for(int frame=0;frame<30;++frame){
        Bitmap bitmap(int(WindowW),int(WindowH),PixelFormat32bppARGB);
        {Graphics c(&bitmap);c.TranslateTransform(-Inset,-Inset);if(frame>=10&&frame<22){g.busy=true;g.shownPercent=62.5-(frame-10)*1.4;}else{g.busy=false;if(frame>=22){g.shownPercent=45.7;g.result=L"Vista previa · datos de ejemplo";}}paint(c,frame*.1);}
        wchar_t suffix[64]{};swprintf(suffix,64,L"-frame-%02d.png",frame);std::wstring path=std::wstring(prefix)+suffix;
        if(bitmap.Save(path.c_str(),&encoder,nullptr)!=Ok)return 3;
    }
    for(int mode=0;mode<2;++mode){
        g.busy=false;g.shownPercent=62.5;g.result.clear();g.prefs.dark=mode==1;g.toggle[4]=mode;
        g.info.ready.fill(true);g.info.complete=true;g.info.collectedAt=L"Datos de ejemplo";
        g.info.pages[0]={SpecBlock{L"Equipo · datos de ejemplo",{{L"Fabricante",L"Fabricante de ejemplo"},{L"Modelo",L"Equipo de ejemplo"},{L"Nombre del equipo",L"PC-DEMO"},{L"RAM utilizable",L"8.00 GiB"}}},SpecBlock{L"Windows · datos de ejemplo",{{L"Edición",L"Windows 10 (ejemplo)"},{L"Arquitectura",L"64 bits"}}}};
        for(int panel=0;panel<4;++panel){g.panel=panel?1:0;g.system=panel==2;g.about=panel==3;Bitmap bitmap(int(WindowW),int(WindowH),PixelFormat32bppARGB);{Graphics c(&bitmap);c.TranslateTransform(-Inset,-Inset);paint(c,0);}
            std::wstring path=std::wstring(prefix)+(mode?L"-dark":L"-light")+(panel==3?L"-credits.png":panel==2?L"-system.png":panel?L"-settings.png":L"-main.png");if(bitmap.Save(path.c_str(),&encoder,nullptr)!=Ok)return 4;
        }
    }
    return 0;
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int showCommand){
    using SetDpi=BOOL (WINAPI*)(HANDLE);auto setDpi=loadWinApi<SetDpi>(GetModuleHandleW(L"user32.dll"),"SetProcessDpiAwarenessContext");if(setDpi)setDpi(reinterpret_cast<HANDLE>(-4));
    GdiplusStartupInput startup;ULONG_PTR token=0;if(GdiplusStartup(&token,&startup,nullptr)!=Ok)return 1;
    {FontFamily check(L"Segoe UI");if(check.GetLastStatus()!=Ok)fontName=L"Arial";}
    loadLogo(instance);
    loadSocialIcons();
    int argc=0;LPWSTR* argv=CommandLineToArgvW(GetCommandLineW(),&argc);
    if(argc==3&&wcscmp(argv[1],L"--render-demo")==0){int result=renderDemo(argv[2]);LocalFree(argv);releaseLogo();GdiplusShutdown(token);return result;}
    if(argv)LocalFree(argv);
    HANDLE mutex=CreateMutexW(nullptr,FALSE,L"Local\\SkylarMemory-0.1");
    if(mutex&&GetLastError()==ERROR_ALREADY_EXISTS){HWND existing=FindWindowW(WindowClass,nullptr);if(existing){ShowWindow(existing,SW_RESTORE);SetForegroundWindow(existing);}else MessageBoxW(nullptr,L"Cierra la versión anterior desde su icono junto al reloj > Salir y abre AeroRAM otra vez.",L"AeroRAM",MB_OK|MB_ICONINFORMATION);CloseHandle(mutex);releaseLogo();GdiplusShutdown(token);return 0;}
    loadPreferences();g.memory=readMemory();g.shownPercent=g.memory.usedPercent();if(g.memory.valid)g.history.push(g.shownPercent);g.lastFrame=GetTickCount64();
    WNDCLASSW wc{};wc.hInstance=instance;wc.lpfnWndProc=procedure;wc.lpszClassName=WindowClass;wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hIcon=LoadIconW(instance,MAKEINTRESOURCEW(101));RegisterClassW(&wc);g.icon=wc.hIcon;
    using GetDpi=UINT (WINAPI*)();auto getDpi=loadWinApi<GetDpi>(GetModuleHandleW(L"user32.dll"),"GetDpiForSystem");g.dpiScale=(getDpi?getDpi():96)/96.f;
    RECT work{};SystemParametersInfoW(SPI_GETWORKAREA,0,&work,0);
    const float fit=std::min((work.right-work.left-16)/WindowW,(work.bottom-work.top-16)/WindowH);
    g.dpiScale=std::min(g.dpiScale,std::max(.5f,fit));
    int width=int(WindowW*g.dpiScale+.5f),height=int(WindowH*g.dpiScale+.5f);
    g.window=CreateWindowExW(WS_EX_LAYERED|WS_EX_APPWINDOW,WindowClass,L"AeroRAM",WS_POPUP|WS_SYSMENU|WS_MINIMIZEBOX,work.left+(work.right-work.left-width)/2,work.top+(work.bottom-work.top-height)/2,width,height,nullptr,nullptr,instance,nullptr);
    if(!g.window||!makeSurface()){MessageBoxW(nullptr,L"No se pudo crear la ventana.",L"AeroRAM",MB_OK|MB_ICONERROR);releaseSurface();releaseLogo();if(mutex)CloseHandle(mutex);GdiplusShutdown(token);return 1;}
    SendMessageW(g.window,WM_SETICON,ICON_BIG,reinterpret_cast<LPARAM>(wc.hIcon));
    auto smallIcon=LoadImageW(instance,MAKEINTRESOURCEW(101),IMAGE_ICON,GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_SHARED);SendMessageW(g.window,WM_SETICON,ICON_SMALL,reinterpret_cast<LPARAM>(smallIcon));
    configureFrame();addTray();SetTimer(g.window,TIMER_DATA,1000,nullptr);ShowWindow(g.window,showCommand);SetFocus(g.window);frameTimer();render();
    MSG msg{};while(GetMessageW(&msg,nullptr,0,0)>0){TranslateMessage(&msg);DispatchMessageW(&msg);}if(mutex)CloseHandle(mutex);GdiplusShutdown(token);return 0;
}
