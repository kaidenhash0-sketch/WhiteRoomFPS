#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <random>

static const int W=960,H=600;
struct Enemy { double x,y,angle; int hp; double stun; bool alive; };
struct Particle { double x,y,vx,vy,life; COLORREF c; };
static HWND g_hwnd; static HDC memdc; static HBITMAP bmp; static HBITMAP oldbmp;
static bool keys[256]={}; static double px=0,py=0,pa=0; static int ammo=12, reserveAmmo=96, score=0;
static double fireCooldown=0, reloadTime=0, hitFlash=0, hurtFlash=0, muzzle=0, timeAcc=0;
static std::vector<Enemy> enemies; static std::vector<Particle> particles;
static std::mt19937 rng((unsigned)GetTickCount());
static double rnd(double a,double b){return std::uniform_real_distribution<double>(a,b)(rng);}
static double norm(double a){while(a>3.14159265)a-=6.2831853; while(a<-3.14159265)a+=6.2831853; return a;}
static void txt(HDC dc,int x,int y,const wchar_t*s,int size,COLORREF c){SetBkMode(dc,TRANSPARENT);SetTextColor(dc,c);HFONT f=CreateFontW(size,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Arial");auto old=(HFONT)SelectObject(dc,f);TextOutW(dc,x,y,s,(int)wcslen(s));SelectObject(dc,old);DeleteObject(f);}
static void rect(HDC dc,int x,int y,int w,int h,COLORREF c){HBRUSH b=CreateSolidBrush(c);RECT r{x,y,x+w,y+h};FillRect(dc,&r,b);DeleteObject(b);}
static void line(HDC dc,int x1,int y1,int x2,int y2,COLORREF c,int width=1){HPEN p=CreatePen(PS_SOLID,width,c);auto o=SelectObject(dc,p);MoveToEx(dc,x1,y1,nullptr);LineTo(dc,x2,y2);SelectObject(dc,o);DeleteObject(p);}
static void circle(HDC dc,int x,int y,int r,COLORREF c){HBRUSH b=CreateSolidBrush(c);HPEN p=CreatePen(PS_SOLID,1,c);auto ob=SelectObject(dc,b);auto op=SelectObject(dc,p);Ellipse(dc,x-r,y-r,x+r,y+r);SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(b);DeleteObject(p);}
static void spawnEnemy(){double a=rnd(-3.14,3.14), d=rnd(7,13); enemies.push_back({px+cos(a)*d,py+sin(a)*d,rnd(-3.14,3.14),100,0,true});}
static void particlesAt(double x,double y,COLORREF c,int n){for(int i=0;i<n;i++){double a=rnd(0,6.283),v=rnd(.8,4.0);particles.push_back({x,y,cos(a)*v,sin(a)*v,rnd(.25,.7),c});}}
static void reload(){if(reloadTime<=0&&ammo<12&&reserveAmmo>0)reloadTime=1.25;}
static void shoot(){if(fireCooldown>0||reloadTime>0)return;if(ammo<=0){reload();return;}ammo--;fireCooldown=.22;muzzle=.07;
 int best=-1;double bestDiff=.10,bestDist=1e9;
 for(int i=0;i<(int)enemies.size();i++){auto&e=enemies[i];if(!e.alive)continue;double dx=e.x-px,dy=e.y-py,d=sqrt(dx*dx+dy*dy);double diff=fabs(norm(atan2(dy,dx)-pa));double tolerance=.055+0.25/d;if(diff<tolerance&&d<bestDist){best=i;bestDist=d;bestDiff=diff;}}
 if(best>=0){auto&e=enemies[best];e.hp-= (bestDiff<.025?100:55);e.stun=.35;hitFlash=.12;particlesAt(e.x,e.y,RGB(160,10,15),18);if(e.hp<=0){e.alive=false;score+=100;particlesAt(e.x,e.y,RGB(110,0,0),12);}}
}
static void resetGame(){px=py=0;pa=0;ammo=12;reserveAmmo=96;score=0;enemies.clear();particles.clear();for(int i=0;i<7;i++)spawnEnemy();}
static void render(){
 HDC dc=memdc;rect(dc,0,0,W,H,RGB(238,240,242));
 // white enclosed arena perspective floor/ceiling
 POINT ceiling[]={{0,0},{W,0},{W,210},{480,280},{0,210}}; HBRUSH cb=CreateSolidBrush(RGB(248,248,248));HPEN cp=CreatePen(PS_SOLID,1,RGB(210,212,215));auto ob=SelectObject(dc,cb);auto op=SelectObject(dc,cp);Polygon(dc,ceiling,5);SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(cb);DeleteObject(cp);
 POINT floor[]={{0,600},{W,600},{W,210},{480,280},{0,210}};HBRUSH fb=CreateSolidBrush(RGB(205,209,213));HPEN fp=CreatePen(PS_SOLID,1,RGB(180,184,190));ob=SelectObject(dc,fb);op=SelectObject(dc,fp);Polygon(dc,floor,5);SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(fb);DeleteObject(fp);
 // wall seams
 for(int x=0;x<=W;x+=120)line(dc,x,0,480+(x-480)/3,245,RGB(224,226,229));
 line(dc,0,210,W,210,RGB(190,194,199),2);
 // enemy silhouettes sorted far to near
 std::vector<int> order;for(int i=0;i<(int)enemies.size();i++)if(enemies[i].alive)order.push_back(i);
 std::sort(order.begin(),order.end(),[](int a,int b){double da=hypot(enemies[a].x-px,enemies[a].y-py),db=hypot(enemies[b].x-px,enemies[b].y-py);return da>db;});
 for(int idx:order){auto&e=enemies[idx];double dx=e.x-px,dy=e.y-py,d=hypot(dx,dy),ang=norm(atan2(dy,dx)-pa);if(fabs(ang)>1.15||d<.5)continue;int sx=(int)(W/2+ang/(1.15)*(W/2));int sz=(int)(260/d);int foot=300+(int)(sz*.40)+ (int)(dy*.5);int bw=std::max(8,sz/3),bh=std::max(22,sz);
   COLORREF suit=e.stun>0?RGB(110,40,40):RGB(48,56,65);rect(dc,sx-bw/2,foot-bh,bw,bh, suit);circle(dc,sx,foot-bh-std::max(5,sz/10),std::max(5,sz/10),RGB(194,157,132));
   // arms and weapon
   line(dc,sx-bw/2,foot-bh/2,sx-bw,foot-bh/3,RGB(48,56,65),std::max(3,bw/6));line(dc,sx+bw/2,foot-bh/2,sx+bw,foot-bh/3,RGB(48,56,65),std::max(3,bw/6));line(dc,sx+bw/2,foot-bh/3,sx+bw+sz/4,foot-bh/3,RGB(30,32,35),std::max(2,bw/8));
   rect(dc,sx-bw/2,foot-bh-12,bw,4,RGB(70,70,70));rect(dc,sx-bw/2,foot-bh-12,(int)(bw*std::max(0,e.hp)/100.0),4,RGB(50,200,70));
 }
 // simple first-person gun
 rect(dc,665,430,145,160,RGB(52,57,64));rect(dc,690,390,85,95,RGB(75,81,88));rect(dc,706,360,56,115,RGB(34,38,43));rect(dc,717,342,34,75,RGB(20,23,27));rect(dc,724,336,20,20,RGB(8,9,10));
 if(muzzle>0){POINT p[]={{724,335},{744,300},{760,335},{744,326}};HBRUSH b=CreateSolidBrush(RGB(255,208,74));ob=SelectObject(dc,b);Polygon(dc,p,4);SelectObject(dc,ob);DeleteObject(b);}
 // crosshair
 line(dc,470,300,490,300,RGB(25,25,25),2);line(dc,480,290,480,310,RGB(25,25,25),2);circle(dc,480,300,2,RGB(255,255,255));
 if(hitFlash>0){circle(dc,480,300,9,RGB(200,20,25));line(dc,472,292,488,308,RGB(255,255,255),2);line(dc,488,292,472,308,RGB(255,255,255),2);}
 // particles are projected crudely relative to direction
 for(auto&p:particles){double dx=p.x-px,dy=p.y-py,d=hypot(dx,dy);double a=norm(atan2(dy,dx)-pa);if(fabs(a)<1.1){int sx=(int)(480+a/1.1*480),sy=320+(int)(d? (py-p.y)*10/d:0);circle(dc,sx,sy,2,p.c);}}
 // HUD
 rect(dc,15,15,240,88,RGB(25,29,34));txt(dc,28,25,L"WHITE ROOM // FPS",20,RGB(245,245,245));wchar_t buf[128];swprintf(buf,128,L"AMMO  %02d / %03d",ammo,reserveAmmo);txt(dc,28,56,buf,20,RGB(255,255,255));swprintf(buf,128,L"TARGETS %d    SCORE %d",(int)std::count_if(enemies.begin(),enemies.end(),[](auto&e){return e.alive;}),score);txt(dc,28,80,buf,14,RGB(205,215,225));
 if(hurtFlash>0)rect(dc,0,0,W,5,RGB(190,25,30));
 txt(dc,350,15,L"WASD move   Mouse aim   LMB fire   R reload   ESC quit",15,RGB(50,55,60));
 if(enemies.empty()||std::none_of(enemies.begin(),enemies.end(),[](auto&e){return e.alive;})){rect(dc,300,210,360,100,RGB(25,29,34));txt(dc,365,230,L"ROOM CLEARED",26,RGB(255,255,255));txt(dc,337,272,L"Press ENTER for another wave",17,RGB(220,225,230));}
}
static void update(double dt){
 fireCooldown=std::max(0.0,fireCooldown-dt);reloadTime=std::max(0.0,reloadTime-dt);hitFlash=std::max(0.0,hitFlash-dt);hurtFlash=std::max(0.0,hurtFlash-dt);muzzle=std::max(0.0,muzzle-dt);
 if(reloadTime>0&&reloadTime<dt+.02){int n=std::min(12-ammo,reserveAmmo);ammo+=n;reserveAmmo-=n;}
 POINT cur;GetCursorPos(&cur);ScreenToClient(g_hwnd,&cur);int cx=W/2,cy=H/2;pa+=((cur.x-cx)*.0024);SetCursorPos((GetSystemMetrics(SM_CXSCREEN)/2),(GetSystemMetrics(SM_CYSCREEN)/2)); // center cursor below
 double speed=4.2*dt,fx=cos(pa),fy=sin(pa),sx=-fy,sy=fx;
 double mx=0,my=0;if(keys['W']){mx+=fx;my+=fy;}if(keys['S']){mx-=fx;my-=fy;}if(keys['A']){mx+=sx;my+=sy;}if(keys['D']){mx-=sx;my-=sy;}double ml=hypot(mx,my);if(ml>0){px+=mx/ml*speed;py+=my/ml*speed;}
 // keep player in arena
 double d=hypot(px,py);if(d>5.2){px=px/d*5.2;py=py/d*5.2;}
 for(auto&e:enemies){if(!e.alive)continue;e.stun=std::max(0.0,e.stun-dt);double dx=px-e.x,dy=py-e.y,dist=hypot(dx,dy);e.angle=atan2(dy,dx);if(e.stun<=0&&dist>3.0){e.x+=dx/dist*dt*.45;e.y+=dy/dist*dt*.45;}if(dist<10&&rnd(0,1)<dt*.10){hurtFlash=.10;}}
 for(auto&p:particles){p.x+=p.vx*dt;p.y+=p.vy*dt;p.life-=dt;}particles.erase(std::remove_if(particles.begin(),particles.end(),[](auto&p){return p.life<=0;}),particles.end());
}
static LRESULT CALLBACK WndProc(HWND h,UINT m,WPARAM wp,LPARAM lp){switch(m){
case WM_KEYDOWN: if(wp<256)keys[wp]=true;if(wp==VK_ESCAPE)PostQuitMessage(0);if(wp=='R')reload();if(wp==VK_RETURN&&std::none_of(enemies.begin(),enemies.end(),[](auto&e){return e.alive;})){for(int i=0;i<7+score/100;i++)spawnEnemy();}return 0;
case WM_KEYUP:if(wp<256)keys[wp]=false;return 0;
case WM_LBUTTONDOWN:shoot();return 0;
case WM_PAINT:{PAINTSTRUCT ps;HDC hdc=BeginPaint(h,&ps);BitBlt(hdc,0,0,W,H,memdc,0,0,SRCCOPY);EndPaint(h,&ps);return 0;}
case WM_DESTROY:PostQuitMessage(0);return 0;}return DefWindowProcW(h,m,wp,lp);}
int WINAPI wWinMain(HINSTANCE hi,HINSTANCE,LPWSTR,int){
 WNDCLASSW wc{};wc.lpfnWndProc=WndProc;wc.hInstance=hi;wc.lpszClassName=L"WhiteRoomFPSWindow";wc.hCursor=LoadCursor(nullptr,IDC_CROSS);wc.hbrBackground=(HBRUSH)(COLOR_WINDOW+1);RegisterClassW(&wc);
 RECT r{0,0,W,H};AdjustWindowRect(&r,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,FALSE);
 g_hwnd=CreateWindowW(wc.lpszClassName,L"White Room FPS",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,CW_USEDEFAULT,CW_USEDEFAULT,r.right-r.left,r.bottom-r.top,nullptr,nullptr,hi,nullptr);
 HDC wnd=GetDC(g_hwnd);memdc=CreateCompatibleDC(wnd);bmp=CreateCompatibleBitmap(wnd,W,H);oldbmp=(HBITMAP)SelectObject(memdc,bmp);ReleaseDC(g_hwnd,wnd);
 ShowWindow(g_hwnd,SW_SHOW);resetGame();SetCursorPos(GetSystemMetrics(SM_CXSCREEN)/2,GetSystemMetrics(SM_CYSCREEN)/2);
 LARGE_INTEGER freq,last,now;QueryPerformanceFrequency(&freq);QueryPerformanceCounter(&last);MSG msg{};
 while(true){while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){if(msg.message==WM_QUIT)goto done;TranslateMessage(&msg);DispatchMessageW(&msg);}
 QueryPerformanceCounter(&now);double dt=(double)(now.QuadPart-last.QuadPart)/freq.QuadPart;last=now;dt=std::min(dt,.04);update(dt);render();InvalidateRect(g_hwnd,nullptr,FALSE);UpdateWindow(g_hwnd);Sleep(8);}
 done:SelectObject(memdc,oldbmp);DeleteObject(bmp);DeleteDC(memdc);return 0;
}
