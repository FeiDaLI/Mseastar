#include "../include/future/promise.hh"
#include "../include/future/future_state.hh"
#include "iostream"
#include <tuple>
using namespace std;

int main(){
    future_state<int,int,int>fs;
    fs.set(1,2,3);
    auto v = fs.get_value();
    cout<<get<0>(v)<<" "<<get<1>(v)<<" "<<get<2>(v)<<endl;
    return 0;
}