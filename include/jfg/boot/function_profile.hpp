#pragma once
#include <array>
#include <cstdint>
#include <unordered_map>
#include <vector>
namespace jfg::boot {
// Diagnostic arithmetic only. Samples belong to one host thread. Wall and
// instruction deltas can include guest work performed while that thread waits.
class FunctionProfile {
public:
  struct Sample {
    std::uint64_t wall=0, cycles=0, wait=0, instructions=0;
    std::uint32_t thread=0;
    bool valid=false;
  };
  struct Row {
    std::uint32_t target=0,parent=0;
    std::uint64_t calls=0, wall=0,self_wall=0,cycles=0,self_cycles=0,
      wait=0,instructions=0,max_wall=0,max_cycles=0;
    bool valid=true;
  };
  struct Frame {
    std::uint32_t target=0,parent=0;
    Sample start;
    std::uint64_t child_wall=0,child_cycles=0;
    bool valid=true;
  };
  static constexpr std::size_t max_depth=512, max_edges=16384;
  bool active() const { return depth_!=0; }
  void reset() {
    depth_=0;overflow_depth_=0;valid_=true;overflow_=false;calls_=0;
    for(auto& row:rows_) { const auto t=row.target,p=row.parent;row=Row{};row.target=t;row.parent=p; }
  }
  void enter(std::uint32_t target,Sample sample) {
    ++calls_;
    if(depth_==max_depth || overflow_depth_) {
      ++overflow_depth_;overflow_=true;valid_=false;return;
    }
    const auto parent=depth_?frames_[depth_-1].target:0U;
    frames_[depth_++]={target,parent,sample,0,0,true};
  }
  bool leave(Sample end) {
    if(overflow_depth_) {--overflow_depth_;return false;}
    if(!depth_) {valid_=false;return false;}
    const auto f=frames_[--depth_];
    const bool valid=f.valid && f.start.valid && end.valid &&
      f.start.thread==end.thread && end.wall>=f.start.wall &&
      end.cycles>=f.start.cycles && end.wait>=f.start.wait &&
      end.instructions>=f.start.instructions;
    const auto wall=valid?end.wall-f.start.wall:0U;
    const auto cycles=valid?end.cycles-f.start.cycles:0U;
    const bool additive=valid && wall>=f.child_wall && cycles>=f.child_cycles;
    valid_=valid_ && additive;
    if(depth_) {
      auto& parent=frames_[depth_-1];parent.child_wall+=wall;
      parent.child_cycles+=cycles;parent.valid=parent.valid && additive;
    }
    const auto key=(std::uint64_t(f.parent)<<32U)|f.target;
    auto it=indices_.find(key);
    if(it==indices_.end()) {
      if(rows_.size()==max_edges) {overflow_=true;valid_=false;return depth_==0;}
      it=indices_.emplace(key,rows_.size()).first;
      rows_.push_back(Row{});rows_.back().target=f.target;rows_.back().parent=f.parent;
    }
    auto& row=rows_[it->second];++row.calls;row.valid=row.valid && additive;
    row.wall+=wall;row.cycles+=cycles;
    if(additive) {row.self_wall+=wall-f.child_wall;row.self_cycles+=cycles-f.child_cycles;}
    if(valid) {row.wait+=end.wait-f.start.wait;row.instructions+=end.instructions-f.start.instructions;}
    if(wall>row.max_wall)row.max_wall=wall;
    if(cycles>row.max_cycles)row.max_cycles=cycles;
    return depth_==0;
  }
  const std::vector<Row>& rows() const {return rows_;}
  bool valid()const{return valid_;}
  bool overflow()const{return overflow_;}
  std::uint64_t calls()const{return calls_;}
private:
  std::array<Frame,max_depth> frames_{};
  std::unordered_map<std::uint64_t,std::size_t> indices_;
  std::vector<Row> rows_;
  std::size_t depth_=0,overflow_depth_=0;
  std::uint64_t calls_=0;
  bool valid_=true,overflow_=false;
};
}
