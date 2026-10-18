#pragma once
#include <array>
#include <cmath>

namespace jfg {
// Immutable operands captured at the supported guest's camPushModelMtx return.
// These describe one emitted matrix, never mutable references into guest RAM.
struct RigidPresentationPose {
    std::array<float, 16> world{}, view{}, projection{};
    bool operator==(const RigidPresentationPose& other) const {
        return world==other.world && view==other.view && projection==other.projection;
    }
    bool operator!=(const RigidPresentationPose& other) const { return !(*this==other); }
};
inline bool rigid_pose_matches_matrix(const RigidPresentationPose& pose,
                                     const std::array<float, 16>& combined) {
    for (const auto* m : {&pose.world, &pose.view, &pose.projection, &combined})
        for (float v : *m) if (!std::isfinite(v)) return false;
    // Only the affine model/view and perspective projection used by this
    // guest path qualify. Unknown or degenerate operands keep shape matching.
    for (const auto* m : {&pose.world, &pose.view})
        if ((*m)[3] != 0 || (*m)[7] != 0 || (*m)[11] != 0 || (*m)[15] != 1)
            return false;
    if (std::abs(pose.projection[11]) < 0.000001f || pose.projection[15] != 0)
        return false;
    // mathMtxCatF first builds view * projection, then world * that result.
    // Its single-precision sum is (t1+t2)+(t0+t3). Reassociating these
    // products changes cancellation near the origin enough to reject valid
    // fixed-point endpoints. Volatile intermediates enforce float rounding
    // and prevent contraction, regardless of the host compiler's FMA support.
    const auto concatenate=[](const auto& a,const auto& b) {
        std::array<float,16> out{};
        for(unsigned row=0;row<4;++row) for(unsigned col=0;col<4;++col) {
            const volatile float t0=a[row*4]*b[col];
            const volatile float t1=a[row*4+1]*b[4+col];
            const volatile float t2=a[row*4+2]*b[8+col];
            const volatile float t3=a[row*4+3]*b[12+col];
            const volatile float left=t1+t2, right=t0+t3;
            out[row*4+col]=left+right;
        }
        return out;
    };
    const auto expected=concatenate(pose.world,concatenate(pose.view,pose.projection));
    for (unsigned row=0;row<4;++row) for (unsigned col=0;col<4;++col) {
        const double value=expected[row*4+col];
        if (!std::isfinite(value)) return false;
        // Float guest concatenation followed by signed 16.16 quantization.
        const double endpoint=combined[row*4+col];
        if (std::abs(value-endpoint)>0.0001+std::abs(endpoint)*0.00001) return false;
    }
    return true;
}
inline bool stationary_rigid_camera_motion(const RigidPresentationPose& old,
                                           const RigidPresentationPose& current) {
    // Changing model pose, FOV or projection retains the existing conservative
    // geometry test. This exception only recognizes a stationary owned object.
    if (old.world != current.world || old.projection != current.projection)
        return false;
    std::array<double,3> eyes[2]{}, norms[2]{};
    const std::array<float,16>* views[]{&old.view,&current.view};
    for (unsigned i=0;i<2;++i) {
        const auto& v=*views[i];
        for (float value : v) if (!std::isfinite(value)) return false;
        if (v[3]!=0 || v[7]!=0 || v[11]!=0 || v[15]!=1) return false;
        for (unsigned a=0;a<3;++a) for (unsigned b=0;b<3;++b) {
            double dot=0;
            for (unsigned row=0;row<3;++row) dot+=double(v[row*4+a])*v[row*4+b];
            // Guest lookup-table trigonometry is slightly non-unit (the
            // retained camera has squared axis lengths about 0.99869).
            // Accept only near-unit orthogonal axes and account for their
            // actual lengths when inverting the view translation.
            if (a==b) {
                if (std::abs(dot-1.0)>0.005) return false;
                norms[i][a]=dot;
            } else if (std::abs(dot)>0.001) return false;
        }
        // Invert orthogonal near-unit view axes, including guest trig error.
        for (unsigned row=0;row<3;++row) for (unsigned col=0;col<3;++col)
            eyes[i][row]-=double(v[12+col])*v[row*4+col]/norms[i][col];
    }
    double distance2=0;
    for (unsigned c=0;c<3;++c) {
        const double delta=eyes[1][c]-eyes[0][c]; distance2+=delta*delta;
        double dot=0;
        for (unsigned row=0;row<3;++row) dot+=double(old.view[row*4+c])*current.view[row*4+c];
        if (dot/std::sqrt(norms[0][c]*norms[1][c])<0.94) return false; // About twenty degrees in one source interval.
    }
    // A cut of more than two character heights must not drag nearby scenery.
    return distance2<=128.0*128.0;
}
// Camera generation/mode and emitted matrix bytes are validated by the caller.
// The recorded guest zoom changes only the two positive lens scales (up to
// 1.18% per source frame). Permit a bounded 2% common scale change, while
// retaining the same view-motion checks and all other projection coefficients.
inline bool continuous_owned_camera_motion(const RigidPresentationPose& old,
                                           const RigidPresentationPose& current) {
    for (unsigned i=0;i<16;++i) {
        if (!std::isfinite(old.projection[i]) || !std::isfinite(current.projection[i]))
            return false;
        if (i!=0U && i!=5U && old.projection[i]!=current.projection[i]) return false;
    }
    if (old.projection[0]<=0 || old.projection[5]<=0 ||
        current.projection[0]<=0 || current.projection[5]<=0) return false;
    const double x=double(current.projection[0])/old.projection[0];
    const double y=double(current.projection[5])/old.projection[5];
    if (x<1.0/1.02 || x>1.02 || y<1.0/1.02 || y>1.02 || std::abs(x-y)>0.00001)
        return false;
    auto same_projection=current;
    same_projection.projection=old.projection;
    return stationary_rigid_camera_motion(old,same_projection);
}
} // namespace jfg
