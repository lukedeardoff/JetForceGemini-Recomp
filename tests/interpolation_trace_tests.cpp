#include "jfg/renderer/interpolation_bridge.hpp"
#include <filesystem>
#include <iostream>
#include <set>
#include <sstream>
#include <thread>
#include <vector>
void check(bool value){if(!value)std::abort();}
int main(int argc,char** argv){
 check(argc==2);
#if defined(_MSC_VER)
 check(_putenv_s("JFG_RT64_INTERPOLATION_TRACE",argv[1])==0);
#else
 check(setenv("JFG_RT64_INTERPOLATION_TRACE",argv[1],1)==0);
#endif
 std::vector<std::thread> workers;
 for(unsigned worker=0;worker<4;++worker) workers.emplace_back([worker]{
  for(unsigned i=0;i<1000;++i)
   jfg::trace_interpolation("sample",std::uint64_t(worker)*1000U+i,30,120,i%4,4,
                            float(i%4+1)/4,UINT64_C(0xffffffffffffffff),17);
 });
 for(auto& worker:workers)worker.join();
 jfg::finish_interpolation_trace();
 jfg::trace_interpolation("late",1,30,120,0,4,1,0,0);
 jfg::finish_interpolation_trace();
 std::ifstream input(argv[1]);std::string line;std::set<std::uint64_t> ids;
 bool footer=false;
 while(std::getline(input,line)){
  std::vector<std::string> columns;std::stringstream row(line);std::string column;
  while(std::getline(row,column,'\t'))columns.push_back(column);
  check(columns.size()==10 && !footer);
  if(columns[0]=="trace-end"){
   check(columns[8]=="0"&&columns[9]=="0");footer=true;continue;
  }
  check(columns[0]=="sample");check(ids.insert(std::stoull(columns[2])).second);
  check(columns[3]=="30" && columns[4]=="120" && columns[6]=="4");
  check(std::stoull(columns[8])==UINT64_MAX && columns[9]=="17");
  check(std::stof(columns[7])==float(std::stoul(columns[5])+1U)/4);
 }
 check(footer && ids.size()==4000);
 std::cout<<"4000 concurrent rows, exact fields, shutdown drain and late-write rejection pass\n";
}
