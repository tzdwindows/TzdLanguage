// bench_gmp.cpp - Benchmark GMP vs our template-optimized 2-prime NTT
#include <gmp.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <chrono>
#include <vector>
#include <stdint.h>

static const uint64_t P1=998244353, P2=985661441;
template<uint64_t P> static inline uint64_t mp(uint64_t a,uint64_t b){return (a*b)%P;}
template<uint64_t P> static inline uint64_t ap(uint64_t a,uint64_t b){uint64_t r=a+b;return r>=P?r-P:r;}
template<uint64_t P> static inline uint64_t sp(uint64_t a,uint64_t b){return a>=b?a-b:a+P-b;}
template<uint64_t P> static uint64_t pw(uint64_t b,uint64_t e){uint64_t r=1;b%=P;while(e){if(e&1)r=mp<P>(r,b);b=mp<P>(b,b);e>>=1;}return r;}

struct NR{std::vector<uint64_t> f,i;};
static NR g[2][24]; static bool ok[2][24]={};
template<uint64_t P,uint64_t G>
static void init(int pi,int ln){
    if(ok[pi][ln])return;
    size_t n=(size_t)1<<ln;
    uint64_t w=pw<P>(G,(P-1)/n),wi=pw<P>(w,P-2);
    auto&r=g[pi][ln];r.f.resize(n/2);r.i.resize(n/2);
    uint64_t a=1,ai=1;
    for(size_t j=0;j<n/2;j++){r.f[j]=a;r.i[j]=ai;a=mp<P>(a,w);ai=mp<P>(ai,wi);}
    ok[pi][ln]=true;
}
template<uint64_t P,uint64_t G>
static void xform(uint64_t*a,size_t n,bool inv,int pi){
    if(n<=1)return;
    int ln=0;size_t m=n;while(m>1){m>>=1;ln++;}
    init<P,G>(pi,ln);
    const uint64_t*rt=inv?g[pi][ln].i.data():g[pi][ln].f.data();
    for(size_t i=1,j=0;i<n;++i){size_t b=n>>1;for(;j&b;b>>=1)j^=b;j^=b;if(i<j)std::swap(a[i],a[j]);}
    for(size_t len=2;len<=n;len<<=1){size_t h=len/2,s=n/len;
        for(size_t i=0;i<n;i+=len)for(size_t j=0;j<h;++j){
            uint64_t w=rt[j*s],u=a[i+j],v=mp<P>(a[i+j+h],w);
            a[i+j]=ap<P>(u,v);a[i+j+h]=sp<P>(u,v);}}
    if(inv){uint64_t ni=pw<P>(n,P-2);for(size_t i=0;i<n;i++)a[i]=mp<P>(a[i],ni);}
}
static std::string our_mul(const std::string&a,const std::string&b){
    const uint64_t B=10000;const int D=4;
    auto tl=[&](const std::string&s)->std::vector<uint64_t>{
        std::vector<uint64_t>l;int pd=(D-(int)(s.size()%D))%D;
        std::string p=std::string(pd,'0')+s;
        for(int i=(int)p.size()-D;i>=0;i-=D)l.push_back((uint64_t)std::stoll(p.substr(i,D)));
        return l;};
    auto al=tl(a),bl=tl(b);size_t tot=al.size()+bl.size(),n=1;while(n<tot)n<<=1;
    std::vector<uint64_t> crt(n,0);
    {std::vector<uint64_t> ta(al),tb(bl);ta.resize(n,0);tb.resize(n,0);
     xform<P1,3>(ta.data(),n,false,0);xform<P1,3>(tb.data(),n,false,0);
     for(size_t i=0;i<n;i++)ta[i]=mp<P1>(ta[i],tb[i]);
     xform<P1,3>(ta.data(),n,true,0);
     for(size_t i=0;i<n;i++)crt[i]=ta[i];}
    {std::vector<uint64_t> ta(al),tb(bl);ta.resize(n,0);tb.resize(n,0);
     xform<P2,3>(ta.data(),n,false,1);xform<P2,3>(tb.data(),n,false,1);
     for(size_t i=0;i<n;i++)ta[i]=mp<P2>(ta[i],tb[i]);
     xform<P2,3>(ta.data(),n,true,1);
     uint64_t iv=pw<P2>(P1,P2-2);
     for(size_t i=0;i<n;i++){uint64_t r0=crt[i]%P2,t=sp<P2>(ta[i],r0);
         t=mp<P2>(t,iv);crt[i]=crt[i]+P1*t;}}
    std::vector<uint64_t>res(n+16,0);uint64_t carry=0;
    for(size_t i=0;i<n;i++){uint64_t v=crt[i]+carry;res[i]=v%B;carry=v/B;}
    size_t rl=n;while(carry>0){res[rl]=carry%B;carry/=B;rl++;}
    std::string r;for(int i=(int)rl-1;i>=0;i--){std::string p=std::to_string(res[i]);
        if(i<(int)rl-1)while(p.size()<(size_t)D)p="0"+p;r+=p;}
    size_t st=0;while(st<r.size()-1&&r[st]=='0')st++;return r.substr(st);
}
static std::string gn(int d){return std::string(d,'1');}
int main(){
    printf("=== GMP vs Template NTT (2 primes, compile-time mod) ===\n\n");
    int sz[]={4096,8192,16384,32768,65536,131072,262144};
    for(int si=0;si<7;si++){
        int d=sz[si];std::string a=gn(d),b=gn(d);
        mpz_t ga,gb,gc;mpz_init(ga);mpz_init(gb);mpz_init(gc);
        mpz_set_str(ga,a.c_str(),10);mpz_set_str(gb,b.c_str(),10);mpz_mul(gc,ga,gb);
        int it=(d<=8192)?100:(d<=32768?20:5);
        auto t1=std::chrono::high_resolution_clock::now();
        for(int i=0;i<it;i++)mpz_mul(gc,ga,gb);
        auto t2=std::chrono::high_resolution_clock::now();
        double gms=std::chrono::duration<double,std::milli>(t2-t1).count()/it;
        char*gs=mpz_get_str(nullptr,10,gc);size_t gl=strlen(gs);
        std::string o=our_mul(a,b);
        auto t3=std::chrono::high_resolution_clock::now();
        for(int i=0;i<it;i++)o=our_mul(a,b);
        auto t4=std::chrono::high_resolution_clock::now();
        double oms=std::chrono::duration<double,std::milli>(t4-t3).count()/it;
        bool ok=(o==std::string(gs));
        printf("%7d-digit: GMP=%8.3fms  NTT=%8.3fms  ratio=%5.1fx  %s  (result=%zu)\n",
               d,gms,oms,gms>0?oms/gms:0,ok?"OK":"MISMATCH",gl);
        mpz_clear(ga);mpz_clear(gb);mpz_clear(gc);free(gs);
    }
    printf("\n=== Done ===\n");return 0;
}
