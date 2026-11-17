#pragma once
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

namespace smile {
struct Point { double x=0,y=0; Point operator+(Point b)const{return {x+b.x,y+b.y};} Point operator-(Point b)const{return {x-b.x,y-b.y};} Point operator*(double s)const{return {x*s,y*s};} };
inline double radial(double d2){return d2>1e-20?d2*std::log(d2):0;}
inline double r2(Point a,Point b){auto d=a-b;return d.x*d.x+d.y*d.y;}
class TPS {
    std::vector<Point> controls,weights;
public:
    TPS(const std::vector<Point>& src,const std::vector<Point>& dst){
        if(src.size()!=dst.size()||src.size()<3)throw std::runtime_error("Invalid guide correspondence.");
        // Merge duplicate current positions only if they have compatible destinations.
        std::vector<Point> targets;
        for(size_t i=0;i<src.size();++i){
            size_t j=0;for(;j<controls.size();++j)if(r2(src[i],controls[j])<1e-14)break;
            if(j<controls.size()){if(r2(dst[i],targets[j])>1e-8)throw std::runtime_error("Conflicting guides at the same position.");}
            else{controls.push_back(src[i]);targets.push_back(dst[i]);}
        }
        const size_t n=controls.size(),m=n+3;
        std::vector<double> a(m*m,0);weights.resize(m);
        for(size_t i=0;i<n;++i){
            weights[i]=targets[i]-controls[i];
            for(size_t j=0;j<n;++j)a[i*m+j]=radial(r2(controls[i],controls[j]));
            a[i*m+i]+=1e-5; // Regularize dense, near-coincident tooth guides.
            a[i*m+n]=a[n*m+i]=1;
            a[i*m+n+1]=a[(n+1)*m+i]=controls[i].x;
            a[i*m+n+2]=a[(n+2)*m+i]=controls[i].y;
        }
        // Gaussian elimination with partial pivoting, two RHS (x and y).
        for(size_t k=0;k<m;++k){
            size_t pivot=k;for(size_t i=k+1;i<m;++i)if(std::abs(a[i*m+k])>std::abs(a[pivot*m+k]))pivot=i;
            if(std::abs(a[pivot*m+k])<1e-15)throw std::runtime_error("Degenerate guides. Spread guide points across the image.");
            if(pivot!=k){for(size_t j=k;j<m;++j)std::swap(a[k*m+j],a[pivot*m+j]);std::swap(weights[k],weights[pivot]);}
            for(size_t i=k+1;i<m;++i){double f=a[i*m+k]/a[k*m+k];if(f==0)continue;
                for(size_t j=k+1;j<m;++j)a[i*m+j]-=f*a[k*m+j];
                weights[i]=weights[i]-weights[k]*f;
            }
        }
        for(size_t ii=m;ii-->0;){for(size_t j=ii+1;j<m;++j)weights[ii]=weights[ii]-weights[j]*a[ii*m+j];weights[ii]=weights[ii]*(1/a[ii*m+ii]);}
    }
    Point map(Point p)const{
        const size_t n=controls.size();Point d=weights[n]+weights[n+1]*p.x+weights[n+2]*p.y;
        for(size_t i=0;i<n;++i)d=d+weights[i]*radial(r2(p,controls[i]));
        return p+d;
    }
};
class Grid {
    int nx,ny;std::vector<Point> points;
public:
    Grid(const TPS& tps,int x,int y):nx(x),ny(y),points(x*y){for(int j=0;j<ny;++j)for(int i=0;i<nx;++i)points[j*nx+i]=tps.map({double(i)/(nx-1),double(j)/(ny-1)});}
    Point map(Point p)const{
        double x=std::clamp(p.x,0.0,1.0)*(nx-1),y=std::clamp(p.y,0.0,1.0)*(ny-1);
        int i=std::min(int(x),nx-2),j=std::min(int(y),ny-2);x-=i;y-=j;
        return (points[j*nx+i]*(1-x)+points[j*nx+i+1]*x)*(1-y)+(points[(j+1)*nx+i]*(1-x)+points[(j+1)*nx+i+1]*x)*y;
    }
};
inline void anchors(std::vector<Point>& a,std::vector<Point>& b,std::vector<Point>& c){
    for(Point p:std::vector<Point>{{0,0},{.5,0},{1,0},{0,.5},{1,.5},{0,1},{.5,1},{1,1}}){a.push_back(p);b.push_back(p);c.push_back(p);}
}
}
