#include "MorphCore.h"
#include <iostream>
int main(){
    std::vector<smile::Point> p{{.2,.3},{.5,.4},{.8,.3}},q=p,c=p;
    q[1]={.6,.5};smile::anchors(p,q,c);
    smile::TPS identity(p,p),move(p,q);double error=0;
    for(size_t i=0;i<p.size();++i){if(smile::r2(identity.map(p[i]),p[i])>1e-14)return 1;error=std::max(error,smile::r2(move.map(p[i]),q[i]));}
    if(error>1e-10)return 2;
    smile::Grid grid(identity,128,72);if(smile::r2(grid.map({.345,.678}),{.345,.678})>1e-14)return 3;
    bool rejected=false;try{auto a=p,b=q;a.push_back(p[0]);b.push_back({.9,.9});smile::TPS bad(a,b);}catch(const std::exception&){rejected=true;}if(!rejected)return 4;
    std::cout<<"PASS identity, guide interpolation, grid interpolation, conflicting guide rejection\n";
}
