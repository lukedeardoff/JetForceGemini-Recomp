#pragma once
#include "jfg/runtime/pc_input.hpp"
#include "jfg/mod/navigation_mod.hpp"

namespace jfg {
// Conservative prefilter: possible contacts still use the guest collision query.
inline bool pc_camera_box_possible(PcOrbitPoint from,PcOrbitPoint to,
                                   PcOrbitPoint low,PcOrbitPoint high) {
  float enter=0.0F,leave=1.0F;
  const std::array<float,3> a{from.x,from.y,from.z},b{to.x,to.y,to.z},
    lo{low.x,low.y,low.z},hi{high.x,high.y,high.z};
  for(unsigned i=0;i<3;++i) {
    if(!std::isfinite(a[i]) || !std::isfinite(b[i]) || !std::isfinite(lo[i]) ||
       !std::isfinite(hi[i]) || lo[i]>hi[i])return true;
    const float d=b[i]-a[i];
    if(std::abs(d)<0.00001F) {if(a[i]<lo[i]-1.0F || a[i]>hi[i]+1.0F)return false;continue;}
    float first=(lo[i]-1.0F-a[i])/d,last=(hi[i]+1.0F-a[i])/d;
    if(first>last)std::swap(first,last);
    enter=std::max(enter,first);leave=std::min(leave,last);
    if(enter>leave)return false;
  }
  return true;
}
inline bool pc_camera_triangle_possible(PcOrbitPoint from,PcOrbitPoint to,
    PcOrbitPoint a,PcOrbitPoint b,PcOrbitPoint c) {
  const auto sub=[](PcOrbitPoint p,PcOrbitPoint q){return PcOrbitPoint{p.x-q.x,p.y-q.y,p.z-q.z};};
  const auto cross=[](PcOrbitPoint p,PcOrbitPoint q){return PcOrbitPoint{p.y*q.z-p.z*q.y,p.z*q.x-p.x*q.z,p.x*q.y-p.y*q.x};};
  const auto dot=[](PcOrbitPoint p,PcOrbitPoint q){return double(p.x)*q.x+double(p.y)*q.y+double(p.z)*q.z;};
  for(const auto p:std::array<PcOrbitPoint,5>{from,to,a,b,c})
    if(!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))return true;
  if(!pc_camera_box_possible(from,to,
      {std::min({a.x,b.x,c.x}),std::min({a.y,b.y,c.y}),std::min({a.z,b.z,c.z})},
      {std::max({a.x,b.x,c.x}),std::max({a.y,b.y,c.y}),std::max({a.z,b.z,c.z})}))return false;
  const auto direction=sub(to,from),edge1=sub(b,a),edge2=sub(c,a),normal=cross(edge1,edge2);
  const double area2=dot(normal,normal);
  if(!std::isfinite(area2) || area2<0.000001)return true;
  const auto p=cross(direction,edge2);const double determinant=dot(edge1,p);
  if(!std::isfinite(determinant))return true;
  if(std::abs(determinant)<0.000001) {
    const double distance=dot(sub(from,a),normal);
    return !std::isfinite(distance) || distance*distance<=area2;
  }
  const auto offset=sub(from,a),q=cross(offset,edge1);
  const double u=dot(offset,p)/determinant,v=dot(direction,q)/determinant,
    t=dot(edge2,q)/determinant;
  if(!std::isfinite(u) || !std::isfinite(v) || !std::isfinite(t))return true;
  // Loose edge/end tolerances admit near contacts for the guest to decide.
  return u>=-0.01 && v>=-0.01 && u+v<=1.01 && t>=-0.05 && t<=1.05;
}
// Finite triangle clearance, including edges and corners. Infinite plane
// height extrapolation is not a camera-volume contact outside the triangle.
inline bool pc_camera_sphere_triangle_possible(PcOrbitPoint point,float radius,
    PcOrbitPoint a,PcOrbitPoint b,PcOrbitPoint c) {
  const auto sub=[](PcOrbitPoint p,PcOrbitPoint q){return PcOrbitPoint{p.x-q.x,p.y-q.y,p.z-q.z};};
  const auto cross=[](PcOrbitPoint p,PcOrbitPoint q){return PcOrbitPoint{p.y*q.z-p.z*q.y,p.z*q.x-p.x*q.z,p.x*q.y-p.y*q.x};};
  const auto dot=[](PcOrbitPoint p,PcOrbitPoint q){return double(p.x)*q.x+double(p.y)*q.y+double(p.z)*q.z;};
  if(!std::isfinite(radius) || radius<0)return true;
  for(const auto p:std::array<PcOrbitPoint,4>{point,a,b,c})
    if(!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z))return true;
  const double r=double(radius)+0.25; // Conservative numerical/edge allowance.
  const float extent=radius+0.25F;
  if(!pc_camera_box_possible(point,point,
      {std::min({a.x,b.x,c.x})-extent,std::min({a.y,b.y,c.y})-extent,std::min({a.z,b.z,c.z})-extent},
      {std::max({a.x,b.x,c.x})+extent,std::max({a.y,b.y,c.y})+extent,std::max({a.z,b.z,c.z})+extent}))return false;
  const auto ab=sub(b,a),ac=sub(c,a),normal=cross(ab,ac);
  const double area=dot(normal,normal),plane=dot(sub(point,a),normal);
  if(!std::isfinite(area) || area<0.000001 || !std::isfinite(plane))return true;
  if(plane*plane>r*r*area)return false;
  const auto projected=PcOrbitPoint{
    float(point.x-normal.x*plane/area),float(point.y-normal.y*plane/area),
    float(point.z-normal.z*plane/area)};
  if(dot(cross(ab,sub(projected,a)),normal)>=-0.000001*area &&
     dot(cross(sub(c,b),sub(projected,b)),normal)>=-0.000001*area &&
     dot(cross(sub(a,c),sub(projected,c)),normal)>=-0.000001*area)return true;
  for(const auto edge:std::array<std::array<PcOrbitPoint,2>,3>{{{a,b},{b,c},{c,a}}}) {
    const auto direction=sub(edge[1],edge[0]),offset=sub(point,edge[0]);
    const double length=dot(direction,direction);
    const double t=length>0?std::clamp(dot(offset,direction)/length,0.0,1.0):0.0;
    const double x=offset.x-t*direction.x,y=offset.y-t*direction.y,z=offset.z-t*direction.z;
    if(x*x+y*y+z*z<=r*r)return true;
  }
  return false;
}
class PcCameraBroadphase {
  mod::Mesh mesh_;
  mod::CollisionInventory models_;
  bool known_=false,complete_=false;
public:
  // Read fresh geometry and registered moving-model bounds every update.
  explicit PcCameraBroadphase(const mod::Memory& ram) {
    try {
      const auto track=ram.u32(0x800A0D60U);
      mesh_=mod::decode_mesh(ram,track);
      models_=mod::collision_models(ram,mod::actors(ram));
      known_=models_.known && !mesh_.triangles.empty();
      complete_=known_;
      // A negative clearance result requires every face, not merely a useful
      // navigation/render subset. Gaps or overlapping batches retain native
      // collision behavior rather than certifying missing geometry as clear.
      const auto blocks=ram.u32(track+4U);
      for(unsigned i=0;i<ram.u16(track+0x1AU);++i) {
        const auto block=blocks+i*0x48U,batches=ram.u32(block+0xCU);
        unsigned covered=0;
        for(unsigned batch=0;batch<ram.u16(block+0x28U);++batch) {
          const auto first=ram.u16(batches+batch*16U+8U),
                     end=ram.u16(batches+batch*16U+24U);
          if(first!=covered || end<first)complete_=false;
          covered=end;
        }
        if(covered!=ram.u16(block+0x26U))complete_=false;
      }
    } catch(const std::runtime_error&) {known_=false;complete_=false;}
  }
  bool volume_possible(PcOrbitPoint point,float radius)const {
    if(!complete_ || !std::isfinite(radius) || radius<0)return true;
    for(const auto& model:models_.models)if(model.enabled &&
      pc_camera_box_possible(point,point,
        {model.lower.x-radius,model.lower.y-radius,model.lower.z-radius},
        {model.upper.x+radius,model.upper.y+radius,model.upper.z+radius}))return true;
    for(const auto& triangle:mesh_.triangles) {
      std::array<PcOrbitPoint,3> points{};
      for(unsigned i=0;i<3;++i) {
        if(triangle.vertices[i]>=mesh_.vertices.size())return true;
        const auto v=mesh_.vertices[triangle.vertices[i]];points[i]={v.x,v.y,v.z};
      }
      if(pc_camera_sphere_triangle_possible(point,radius,points[0],points[1],points[2]))return true;
    }
    return false;
  }
  bool possible(PcOrbitPoint from,PcOrbitPoint to)const {
    if(!known_)return true;
    for(const auto& model:models_.models)if(model.enabled &&
      pc_camera_box_possible(from,to,{model.lower.x,model.lower.y,model.lower.z},
                                    {model.upper.x,model.upper.y,model.upper.z}))return true;
    for(const auto& triangle:mesh_.triangles) {
      std::array<PcOrbitPoint,3> points{};
      for(unsigned i=0;i<3;++i) {
        if(triangle.vertices[i]>=mesh_.vertices.size())return true;
        const auto v=mesh_.vertices[triangle.vertices[i]];points[i]={v.x,v.y,v.z};
      }
      if(pc_camera_triangle_possible(from,to,points[0],points[1],points[2]))return true;
    }
    return false;
  }
};
} // namespace jfg
