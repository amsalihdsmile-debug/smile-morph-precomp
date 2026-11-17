// Original SmileMorph implementation. Adobe SDK headers are external dependencies.
#define PF_DEEP_COLOR_AWARE 1
#define NOMINMAX
#include "AEConfig.h"
#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_EffectSuites.h"
#include "MorphCore.h"
#include <cstring>
#include <cstdio>
#include <limits>

enum {INPUT, PROGRESS, START, END, GRID, NUM_PARAMS};
constexpr A_long FLAGS=PF_OutFlag_DEEP_COLOR_AWARE|PF_OutFlag_WIDE_TIME_INPUT|PF_OutFlag_NON_PARAM_VARY;
static void diagnostic(const char*){}

template<class S> class Suite {
    SPBasicSuite* basic; const char* name;A_long version;
public:
    const S* p=nullptr;
    Suite(SPBasicSuite* b,const char* n,A_long v):basic(b),name(n),version(v){
        if(basic->AcquireSuite(name,version,reinterpret_cast<const void**>(&p))||!p)throw std::runtime_error("Host path suite unavailable.");
    }
    ~Suite(){if(p)basic->ReleaseSuite(name,version);}
};
struct PathGuard {
    const PF_PathQuerySuite1* q;PF_InData* in;PF_PathID id;PF_PathOutlinePtr path=nullptr;
    PathGuard(const PF_PathQuerySuite1* query,PF_InData* data,PF_PathID pid,A_long time):q(query),in(data),id(pid){
        PF_Err err=q->PF_CheckoutPath(in->effect_ref,id,time,in->time_step,in->time_scale,&path);
        if(err)throw err;
    }
    ~PathGuard(){if(path)q->PF_CheckinPath(in->effect_ref,id,FALSE,path);}
};
static void checked(PF_Err err){if(err)throw err;}
static std::vector<smile::Point> readPath(const PF_PathDataSuite1* d,PF_InData* in,PF_PathOutlinePtr path,int samples){
    std::vector<smile::Point> result;if(!path)throw std::runtime_error("A guide mask is missing at one of the selected times.");
    // Path-suite coordinates follow the render downsample; in->width/height
    // describe the full source. Undo that scale before normalizing.
    const double sx=in->downsample_x.num>0?double(in->downsample_x.den)/in->downsample_x.num:1;
    const double sy=in->downsample_y.num>0?double(in->downsample_y.den)/in->downsample_y.num:1;
    PF_Boolean open=FALSE;checked(d->PF_PathIsOpen(in->effect_ref,path,&open));if(!open)return result;
    A_long segs=0;checked(d->PF_PathNumSegments(in->effect_ref,path,&segs));
    if(segs<1)return result;
    // Segment evaluation by arc length uses AE's own Bezier implementation.
    for(A_long s=0;s<segs;++s){
        PF_PathSegPrepPtr prep=nullptr;
        checked(d->PF_PathPrepareSegLength(in->effect_ref,path,s,64,&prep));
        PF_Err error=PF_Err_NONE;
        try{
            PF_FpLong length=0;checked(d->PF_PathGetSegLength(in->effect_ref,path,s,&prep,&length));
            for(int j=0;j<samples;++j){PF_FpLong x=0,y=0;
                checked(d->PF_PathEvalSegLength(in->effect_ref,path,&prep,s,length*j/samples,&x,&y));
                result.push_back({x*sx/std::max<A_long>(1,in->width-1),y*sy/std::max<A_long>(1,in->height-1)});
            }
        }catch(PF_Err e){error=e;}catch(...){d->PF_PathCleanupSegLength(in->effect_ref,path,s,&prep);throw;}
        PF_Err cleanup=d->PF_PathCleanupSegLength(in->effect_ref,path,s,&prep);checked(error?error:cleanup);
    }
    PF_PathVertex last{};checked(d->PF_PathVertexInfo(in->effect_ref,path,segs,&last));
    result.push_back({last.x*sx/std::max<A_long>(1,in->width-1),last.y*sy/std::max<A_long>(1,in->height-1)});
    return result;
}
struct Pixel {double a=0,r=0,g=0,b=0;Pixel operator+(Pixel q)const{return {a+q.a,r+q.r,g+q.g,b+q.b};}Pixel operator*(double s)const{return {a*s,r*s,g*s,b*s};}};
template<class T> static Pixel at(const PF_EffectWorld& w,int x,int y){
    const T* line=reinterpret_cast<const T*>(reinterpret_cast<const char*>(w.data)+y*w.rowbytes);
    T v=line[x];return {double(v.alpha),double(v.red),double(v.green),double(v.blue)};
}
template<class T> static Pixel sample(const PF_EffectWorld& w,smile::Point p){
    double x=std::clamp(p.x,0.0,1.0)*(w.width-1),y=std::clamp(p.y,0.0,1.0)*(w.height-1);
    int x0=int(x),y0=int(y),x1=std::min(x0+1,int(w.width-1)),y1=std::min(y0+1,int(w.height-1));double fx=x-x0,fy=y-y0;
    return (at<T>(w,x0,y0)*(1-fx)+at<T>(w,x1,y0)*fx)*(1-fy)+(at<T>(w,x0,y1)*(1-fx)+at<T>(w,x1,y1)*fx)*fy;
}
template<class T> static PF_Err pixels(PF_InData* in,const PF_EffectWorld& a,const PF_EffectWorld& b,PF_EffectWorld* out,double t,const smile::Grid* ga,const smile::Grid* gb,int maximum){
    for(int y=0;y<out->height;++y){
        if((y&31)==0){PF_Err e=PF_ABORT(in);if(e)return e;}
        T* row=reinterpret_cast<T*>(reinterpret_cast<char*>(out->data)+y*out->rowbytes);
        for(int x=0;x<out->width;++x){
            smile::Point p{double(x)/std::max<A_long>(1,out->width-1),double(y)/std::max<A_long>(1,out->height-1)};
            Pixel v;
            if(t<=0)v=sample<T>(a,p);else if(t>=1)v=sample<T>(b,p);
            else v=sample<T>(a,ga->map(p))*(1-t)+sample<T>(b,gb->map(p))*t;
            // AE worlds are premultiplied. Interpolate alpha and RGB together.
            auto cv=[maximum](double z){return static_cast<decltype(T{}.alpha)>(std::clamp(std::round(z),0.0,double(maximum)));};
            row[x].alpha=cv(v.a);row[x].red=cv(v.r);row[x].green=cv(v.g);row[x].blue=cv(v.b);
        }
    }return PF_Err_NONE;
}
static PF_Err render(PF_InData* in,PF_OutData* out,PF_ParamDef* params[],PF_LayerDef* output){
    const double start=params[START]->u.fs_d.value,end=params[END]->u.fs_d.value;
    if(end<=start)throw std::runtime_error("End time must be later than Start time (layer seconds).");
    const double t=std::clamp(params[PROGRESS]->u.fs_d.value/100.0,0.0,1.0);
    struct LayerCheckout {
        PF_InData* in;PF_ParamDef value{};bool acquired=false;
        LayerCheckout(PF_InData* data,double seconds):in(data){
            const double ticks=seconds*in->time_scale;
            if(ticks<std::numeric_limits<A_long>::min()||ticks>std::numeric_limits<A_long>::max())
                throw std::runtime_error("Snapshot time is outside the supported range.");
            checked(PF_CHECKOUT_PARAM(in,INPUT,A_long(std::llround(ticks)),in->time_step,in->time_scale,&value));
            acquired=true;
        }
        ~LayerCheckout(){if(acquired)PF_CHECKIN_PARAM(in,&value);}
    } beforeLayer(in,start),afterLayer(in,end);
    const auto& a=beforeLayer.value.u.ld;const auto& b=afterLayer.value.u.ld;
    if(!a.data||!b.data)throw std::runtime_error("A picture endpoint is outside the precomp's available frames.");
    if(a.width!=b.width||a.height!=b.height)throw std::runtime_error("Before and After images must have matching pixel dimensions.");
    if(PF_WORLD_IS_DEEP(&a)!=PF_WORLD_IS_DEEP(&b))throw std::runtime_error("Image channel depths must match.");
    if(t<=0||t>=1){
        return in->utils->copy(in->effect_ref,const_cast<PF_EffectWorld*>(t<=0?&a:&b),output,nullptr,nullptr);
    }
    Suite<PF_PathQuerySuite1> query(in->pica_basicP,kPFPathQuerySuite,kPFPathQuerySuiteVersion1);
    Suite<PF_PathDataSuite1> data(in->pica_basicP,kPFPathDataSuite,kPFPathDataSuiteVersion1);
    A_long n=0;checked(query.p->PF_NumPaths(in->effect_ref,&n));
    std::vector<PF_PathID> groups[3];
    for(A_long i=0;i<n;++i){
        PF_PathID id=0;checked(query.p->PF_PathInfo(in->effect_ref,i,&id));
        A_char name[PF_MAX_PATH_NAME_LEN+1]{};
        checked(data.p->PF_PathGetName(in->effect_ref,id,name));
        if(std::strncmp(name,"SM A ",5)==0)groups[0].push_back(id);
        else if(std::strncmp(name,"SM B ",5)==0)groups[1].push_back(id);
        else if(std::strncmp(name,"SM C ",5)==0)groups[2].push_back(id);
    }
    if(groups[0].empty()||groups[0].size()!=groups[1].size()||groups[0].size()!=groups[2].size())
        throw std::runtime_error("Run the current Setup SmileMorph Precomp helper to create the live A/B/current guide bridge.");
    A_long segmentTotal=0;
    for(auto id:groups[2]){
        PathGuard p(query.p,in,id,in->current_time);A_long count=0;
        if(!p.path)throw std::runtime_error("Missing live guide path.");
        checked(data.p->PF_PathNumSegments(in->effect_ref,p.path,&count));segmentTotal+=count;
    }
    const int openCount=int(groups[2].size());
    if(segmentTotal+openCount>600)throw std::runtime_error("Guide limit: 600 vertices across open masks.");
    const int samples=segmentTotal>0?std::clamp(int((600-openCount)/segmentTotal),1,4):1;
    std::vector<smile::Point> begin,finish,current;
    // Independent expressions evaluate each endpoint at the current host time.
    // This avoids AE expression-path temporal checkout cache aliasing.
    std::vector<smile::Point>* destinations[3]={&begin,&finish,&current};
    for(size_t i=0;i<groups[0].size();++i){
        size_t sizes[3]{};
        for(int role=0;role<3;++role){
            PathGuard p(query.p,in,groups[role][i],in->current_time);
            auto points=readPath(data.p,in,p.path,samples);sizes[role]=points.size();
            destinations[role]->insert(destinations[role]->end(),points.begin(),points.end());
        }
        if(sizes[0]!=sizes[1]||sizes[0]!=sizes[2])throw std::runtime_error("Keep guide vertex counts unchanged.");
    }
    if(begin.empty())throw std::runtime_error("Run Setup SmileMorph Precomp to link the open guide masks.");
    smile::anchors(begin,finish,current);
    smile::TPS ta(current,begin),tb(current,finish);
    int gx=std::clamp(int(params[GRID]->u.fs_d.value),32,256),gy=std::max(8,int(std::round(double(gx)*in->height/std::max<A_long>(1,in->width))));
    smile::Grid ga(ta,gx,gy),gb(tb,gx,gy);
    return PF_WORLD_IS_DEEP(output)?pixels<PF_Pixel16>(in,a,b,output,t,&ga,&gb,PF_MAX_CHAN16):pixels<PF_Pixel8>(in,a,b,output,t,&ga,&gb,255);
}
extern "C" DllExport PF_Err PluginDataEntryFunction2(PF_PluginDataPtr inPtr,PF_PluginDataCB2 inPluginDataCallBackPtr,SPBasicSuite*,const char*,const char*){
    PF_Err result=PF_Err_INVALID_CALLBACK;
    result=PF_REGISTER_EFFECT_EXT2(inPtr,inPluginDataCallBackPtr,"SmileMorph Precomp","SmileMorph Precomp Native","SmileMorph Precomp",AE_RESERVED_INFO,"EffectMain","");return result;
}
extern "C" DllExport PF_Err EffectMain(PF_Cmd cmd,PF_InData* in_data,PF_OutData* out_data,PF_ParamDef* params[],PF_LayerDef* output,void*){
    PF_Err err=PF_Err_NONE;
    if(cmd==PF_Cmd_RENDER)diagnostic("PF_Cmd_RENDER entered");
    try{switch(cmd){
        case PF_Cmd_ABOUT: std::snprintf(out_data->return_msg,sizeof(out_data->return_msg),"SmileMorph Precomp 0.3\rOpen-mask guided morph for two aligned still images. CPU, 8/16 bpc. Experimental.");break;
        case PF_Cmd_GLOBAL_SETUP:
            out_data->my_version=PF_VERSION(0,3,0,PF_Stage_DEVELOP,2);out_data->out_flags=FLAGS;out_data->out_flags2=0;break;
        case PF_Cmd_PARAMS_SETUP:{
            PF_ParamDef def{};PF_ADD_FLOAT_SLIDERX("Transition",0,100,0,100,0,PF_Precision_HUNDREDTHS,0,0,1);
            AEFX_CLR_STRUCT(def);PF_ADD_FLOAT_SLIDERX("Before snapshot (source sec)",-36000,36000,0,10,0,PF_Precision_HUNDREDTHS,0,0,2);
            AEFX_CLR_STRUCT(def);PF_ADD_FLOAT_SLIDERX("After snapshot (source sec)",-36000,36000,0,10,1,PF_Precision_HUNDREDTHS,0,0,3);
            AEFX_CLR_STRUCT(def);PF_ADD_FLOAT_SLIDERX("Warp grid quality",32,256,32,256,128,PF_Precision_INTEGER,0,0,4);
            out_data->num_params=NUM_PARAMS;break;
        }
        case PF_Cmd_RENDER:err=render(in_data,out_data,params,output);break;
        default:break;
    }}catch(PF_Err e){err=e;}catch(const std::bad_alloc&){err=PF_Err_OUT_OF_MEMORY;}
    catch(const std::exception& e){std::snprintf(out_data->return_msg,sizeof(out_data->return_msg),"SmileMorph: %s",e.what());out_data->out_flags|=PF_OutFlag_DISPLAY_ERROR_MESSAGE;err=PF_Err_BAD_CALLBACK_PARAM;}
    catch(...){err=PF_Err_INTERNAL_STRUCT_DAMAGED;}
    return err;
}

