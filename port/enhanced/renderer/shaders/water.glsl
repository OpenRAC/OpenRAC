// SPDX-License-Identifier: GPL-3.0-or-later
in vec3 v_world;
in vec3 v_water_rest;
in float v_water_swell_weight;
in vec3 v_water_boundary;
uniform samplerBuffer water_bodies;
uniform sampler2D water_coast_map;
uniform sampler2D water_wave_map;
uniform vec4 water_atlas[64];
uniform vec2 water_atlas_extent;
uniform samplerBuffer water_light;
uniform int water_body_count;
uniform int water_surface;
uniform float water_time;
uniform float water_current_scale;
uniform float water_fine_strength;
uniform vec3 water_eye;
uniform int water_debug_view;
uniform float water_debug_height_limit;

vec4 water_cell(samplerBuffer field, int offset, ivec2 extent, ivec2 p) {
    p=clamp(p,ivec2(0),extent-1);
    return texelFetch(field,offset+p.y*extent.x+p.x);
}
vec4 water_cached_light(int offset,ivec2 extent,vec2 p) {
    ivec2 i=ivec2(floor(p));vec2 f=fract(p);
    vec4 a=water_cell(water_light,offset,extent,i),b=water_cell(water_light,offset,extent,i+ivec2(1,0));
    vec4 c=water_cell(water_light,offset,extent,i+ivec2(0,1)),d=water_cell(water_light,offset,extent,i+ivec2(1,1));
    vec4 weights=vec4((1.-f.x)*(1.-f.y),f.x*(1.-f.y),(1.-f.x)*f.y,f.x*f.y)
                 *step(vec4(0),vec4(a.w,b.w,c.w,d.w));
    float sum=dot(weights,vec4(1));
    return sum>.001?(a*weights.x+b*weights.y+c*weights.z+d*weights.w)/sum:vec4(vec3(3.14159265*.65),1);
}
// Smooth simulation slopes, independent of the displacement mesh's triangle edges.
vec4 water_surface_gradients(vec2 cell,vec2 origin,vec2 extent,float dx,out float rotation) {
    vec2 lo=clamp(cell-1.,vec2(.5),extent-.5);
    vec2 hi=clamp(cell+1.,vec2(.5),extent-.5);
    vec2 centre=clamp(cell,vec2(.5),extent-.5);
    vec4 left=textureLod(water_wave_map,(origin+vec2(lo.x,centre.y))/water_atlas_extent,0.);
    vec4 right=textureLod(water_wave_map,(origin+vec2(hi.x,centre.y))/water_atlas_extent,0.);
    vec4 bottom=textureLod(water_wave_map,(origin+vec2(centre.x,lo.y))/water_atlas_extent,0.);
    vec4 top=textureLod(water_wave_map,(origin+vec2(centre.x,hi.y))/water_atlas_extent,0.);
    // Reuse the four filtered taps for physical height and advected surface detail.
    vec2 span=max(hi-lo,vec2(.001))*dx;
    rotation=(right.z-left.z)/span.x-(top.y-bottom.y)/span.y;
    return vec4((right.x-left.x)/span.x,(top.x-bottom.x)/span.y,
                (right.w-left.w)/span.x,(top.w-bottom.w)/span.y);
}
vec2 water_height_gradient(vec2 cell,vec2 origin,vec2 extent,float dx) {
    float rotation;
    return water_surface_gradients(cell,origin,extent,dx,rotation).xy;
}
vec3 water_smooth_normal(vec3 rest_normal,vec2 gradient) {
    // Novalis rest surfaces and displacement are height fields in world XY/Z.
    if(rest_normal.z<0.) rest_normal=-rest_normal;
    vec2 rest_gradient=-rest_normal.xy/max(rest_normal.z,1e-5);
    return normalize(vec3(-(rest_gradient+gradient),1.));
}
// Air/water interface (IOR 1.333), Schlick Fresnel and GGX/Smith specular.
float water_fresnel(float cosine) {
    float f=1.-clamp(cosine,0.,1.);
    return .02037+.97963*f*f*f*f*f;
}
float water_specular(vec3 n,vec3 eye,vec3 light,float roughness) {
    float nv=max(dot(n,eye),0.),nl=max(dot(n,light),0.);
    vec3 sum=eye+light;
    if(nv<=0. || nl<=0. || dot(sum,sum)<1e-8) return 0.;
    vec3 half_direction=normalize(sum);
    float nh=max(dot(n,half_direction),0.);
    float a=roughness*roughness,a2=a*a;
    float denominator=nh*nh*(a2-1.)+1.;
    float distribution=a2/(3.14159265*denominator*denominator);
    // Height-correlated Smith visibility, including 1/(4 NoV NoL).
    float visibility=.5/max(nl*sqrt(nv*nv*(1.-a2)+a2)
                           +nv*sqrt(nl*nl*(1.-a2)+a2),1e-6);
    return distribution*visibility*water_fresnel(dot(eye,half_direction))*nl;
}
vec3 water_transmittance(float depth,float nv) {
    float transmitted_cosine=sqrt(1.-(1.-nv*nv)/(1.333*1.333));
    return exp(-vec3(.85,.24,.12)*max(depth,0.)/transmitted_cosine);
}
vec3 water_direct_specular(vec3 p,vec3 n,vec3 eye,float roughness,float shadow,out vec3 irradiance) {
    vec3 result=vec3(0);
    irradiance=vec3(0);
    for(int light=0;light<2;++light) {
        vec3 direction=light_sets[light*2+1].xyz;
        if(light==0 && probe_sunlight_direction.w!=0.) direction=probe_sunlight_direction.xyz;
        if(dot(direction,direction)<1e-8) continue;
        vec3 radiance=max(light_sets[light*2].rgb,vec3(0));
        if(light==0) radiance*=probe_sunlight_scale*shadow;
        vec3 light_direction=-normalize(direction);
        irradiance+=radiance*max(dot(n,light_direction),0.);
        result+=radiance*water_specular(n,eye,light_direction,roughness);
    }
    for(int i=0;i<8;++i) {
        vec3 delta=probe_point_position[i].xyz-p;
        float radius=probe_point_position[i].w,distance=length(delta);
        if(radius<=0. || distance>=radius || distance<1e-6) continue;
        vec3 radiance=max(probe_point_colour[i].rgb,vec3(0))*(1.-distance/radius);
        irradiance+=radiance*max(dot(n,delta/distance),0.);
        result+=radiance*water_specular(n,eye,delta/distance,roughness);
    }
    return result;
}
// Fine ripples are stylised optical detail independent of simulation resolution.
// Inputs are explicit so the same production helpers can be checked on the GPU.
vec2 water_art_direction(vec2 dpdx,vec2 dpdy,vec2 dtdx,vec2 dtdy,vec2 fallback) {
    vec2 axis=dpdx*dtdy.y-dpdy*dtdx.y;
    if(dot(axis,axis)<1e-14) axis=fallback;
    if(dot(axis,axis)<1e-8) axis=vec2(1,0);
    axis=normalize(axis);
    // Mirrored UVs must not flip the fine pattern at otherwise identical seams.
    if(axis.x<0. || (abs(axis.x)<1e-5 && axis.y<0.)) axis=-axis;
    return axis;
}
float water_ripple_band(vec2 k,vec2 dpdx,vec2 dpdy) {
    float footprint=max(dot(abs(k),abs(dpdx)),dot(abs(k),abs(dpdy)));
    // Fade wavelengths approaching two pixels, avoiding distant sparkling.
    return 1.-smoothstep(1.,3.14159265,footprint);
}
// Analytic slopes of broad, curved wavelets. Low-frequency bending and a
// varying envelope break up the straight repeating lines without texture reads.
vec2 water_stylised_gradient(vec2 p,vec2 axis,float time,vec2 dpdx,vec2 dpdy) {
    vec2 side=vec2(-axis.y,axis.x);
    vec2 bend_k=axis*.61+side*.93;
    float bend_phase=dot(p,bend_k)+time*.19;
    float bend=sin(bend_phase);
    vec2 bend_gradient=bend_k*cos(bend_phase);
    vec2 envelope_k=axis*.83-side*.47;
    float envelope_phase=dot(p,envelope_k)-time*.13;
    float envelope=.7+.3*sin(envelope_phase);
    vec2 envelope_gradient=.3*envelope_k*cos(envelope_phase);
    vec2 k1=(axis*.94+side*.34)*(6.28318531/1.1);
    vec2 k2=(-axis*.35+side*.94)*(6.28318531/.67);
    float phase1=dot(p,k1)+1.6*bend-time*.75;
    float phase2=dot(p,k2)-1.1*bend+time*.53+.8;
    // Filter with a conservative bound on the bent phase derivatives.
    float a1=.014*water_ripple_band(abs(k1)+1.6*abs(bend_k),dpdx,dpdy);
    float a2=.006*water_ripple_band(abs(k2)+1.1*abs(bend_k),dpdx,dpdy);
    float height=a1*sin(phase1)+a2*sin(phase2);
    vec2 slope=a1*(k1+1.6*bend_gradient)*cos(phase1)
              +a2*(k2-1.1*bend_gradient)*cos(phase2);
    return envelope*slope+envelope_gradient*height;
}
vec2 water_fine_gradient(vec2 p,vec2 velocity,vec2 axis,float time,vec2 dpdx,vec2 dpdy,float strength) {
    if(strength<=0.) return vec2(0);
    // Two overlapping flow phases reset distortion invisibly at zero weight.
    // The pattern also moves at zero velocity, including the still pond.
    float phase=fract(time/1.5);
    vec2 q0=p-velocity*(phase*1.5);
    vec2 q1=p-velocity*(fract(phase+.5)*1.5);
    vec2 g0=water_stylised_gradient(q0,axis,time,dpdx,dpdy);
    vec2 g1=water_stylised_gradient(q1,axis,time,dpdx,dpdy);
    return strength*mix(g1,g0,1.-abs(phase*2.-1.));
}
vec3 effect_material(vec3 painted) {
    if (water_surface==0) return painted;
    int chosen=-1; vec2 uv=vec2(0);
    for (int i=0;i<water_body_count;++i) {
        vec4 origin=texelFetch(water_bodies,i*5);
        if (int(origin.w)!=water_surface) continue;
        vec4 u=texelFetch(water_bodies,i*5+1),v=texelFetch(water_bodies,i*5+2);
        float dx=texelFetch(water_bodies,i*5+4).z;
        vec3 delta=v_water_rest-origin.xyz;
        vec2 p=vec2(dot(delta,u.xyz),dot(delta,v.xyz))/dx;
        if (all(greaterThanEqual(p,vec2(0))) && all(lessThan(p,vec2(u.w,v.w)))) {
            chosen=i; uv=p-.5; break;
        }
    }
    if (chosen<0) return painted;
    vec4 origin=texelFetch(water_bodies,chosen*5);
    vec4 u=texelFetch(water_bodies,chosen*5+1),v=texelFetch(water_bodies,chosen*5+2);
    vec4 plane=texelFetch(water_bodies,chosen*5+3);
    vec4 flow=texelFetch(water_bodies,chosen*5+4);
    int offset=int(plane.w); ivec2 size=ivec2(u.w,v.w); float dx=flow.z;
    vec2 map_uv=(water_atlas[chosen].xy+clamp(uv+.5,vec2(.5),vec2(size)-.5))/water_atlas_extent;
    vec4 coast=texture(water_coast_map,map_uv);
    vec2 maintained_current=flow.xy*.8*water_current_scale;
    int sample_body=water_sim_shared_body(chosen,v_water_rest.z);
    vec4 wave=water_sim_sample(water_wave_map,v_water_rest.xy,sample_body,map_uv,maintained_current);
    float h=wave.x;
    if(water_debug_view==1) {
        float value=clamp(h/max(water_debug_height_limit,.001),-1.,1.);
        return vec3(.5)+vec3(value,0,-value)*.5;
    }
    if(water_debug_view==2) return vec3(clamp(.5+wave.yz/5.,vec2(0),vec2(1)),0.);
    if(water_debug_view==3) return vec3(coast.x,exp(-coast.y*.7),0.);
    if(water_debug_view==4) return vec3(wave.w);

    // Keep the actual face for shadow receiver-plane bias, but shade smooth wave slopes.
    vec3 surface_normal=normalize(cross(dFdx(v_world),dFdy(v_world)));
    if(surface_normal.z<0.) surface_normal=-surface_normal;
    vec3 rest_normal=normalize(cross(dFdx(v_water_rest),dFdy(v_water_rest)));
    float rotation;
    vec4 gradients=water_sim_local!=0
        ?water_sim_gradients(water_wave_map,v_water_rest.xy,sample_body,maintained_current,rotation)
        :water_surface_gradients(uv+.5,water_atlas[chosen].xy,vec2(size),dx,rotation);
    // A small optical ripple carried by the simulated flow makes horizontal
    // eddies visible without inventing extra mesh displacement.
    vec2 gradient=gradients.xy+gradients.zw*.045;
    if(water_fine_strength>0.) {
        vec2 dpdx=dFdx(v_water_rest.xy),dpdy=dFdy(v_water_rest.xy);
        vec2 texture_uv=v_stq.xy/v_stq.z;
        vec2 axis=water_art_direction(dpdx,dpdy,dFdx(texture_uv),dFdy(texture_uv),flow.xy);
        float painted_weight=.9+.1*clamp(dot(painted,vec3(.2126,.7152,.0722)),0.,1.);
        // Include the maximum flow-phase stretch in the filter footprint.
        vec2 filter_dx=abs(dpdx)+1.5*abs(dFdx(wave.yz));
        vec2 filter_dy=abs(dpdy)+1.5*abs(dFdy(wave.yz));
        gradient+=water_fine_gradient(v_water_rest.xy,wave.yz,axis,water_time,filter_dx,filter_dy,
                                      water_fine_strength*painted_weight);
    }
    vec3 smooth_normal=water_smooth_normal(rest_normal,gradient);
    // Transform both smooth tangents by the Gerstner Jacobian, including the
    // bank taper. Rest coordinates keep the wave and simulation lookup attached.
    vec2 px=dFdx(v_water_rest.xy),py=dFdy(v_water_rest.xy);
    float determinant=px.x*py.y-px.y*py.x;
    vec2 weight_gradient=vec2(0);
    if(abs(determinant)>1e-10) {
        vec2 dw=vec2(dFdx(v_water_swell_weight),dFdy(v_water_swell_weight));
        weight_gradient=vec2(dw.x*py.y-dw.y*px.y,px.x*dw.y-py.x*dw.x)/determinant;
    }
    vec3 sx,sy;
    vec3 swell=water_swell(v_water_rest.xy,water_time,v_water_swell_weight,weight_gradient,sx,sy);
    vec3 boundary_swell=water_boundary_swell(v_water_boundary,water_time,v_water_swell_weight);
    // Differentiate the continuous boundary wave, including its curved direction
    // and bank taper, rather than using the displaced triangle face normal.
    vec3 bx=dFdx(boundary_swell),by=dFdy(boundary_swell);
    if(abs(determinant)>1e-10) {
        sx+=(bx*py.y-by*px.y)/determinant;
        sy+=(px.x*by-py.x*bx)/determinant;
    }
    swell+=boundary_swell;
    vec2 slope=-smooth_normal.xy/max(smooth_normal.z,1e-5);
    smooth_normal=normalize(cross(vec3(1,0,slope.x)+sx,vec3(0,1,slope.y)+sy));
    vec3 eye=normalize(water_eye-v_world);
    vec3 n=dot(smooth_normal,eye)<0.?-smooth_normal:smooth_normal;
    vec4 cached=water_cached_light(int(flow.w),(size+3)/4+1,uv/4.);
    // Shadow the actual displaced receiver; do not retain the opaque shader's 10% sun floor.
    float visibility=sun_shadow_visibility(v_world,n,surface_normal);
    vec3 direct;
    vec3 specular=water_direct_specular(v_world,n,eye,.22,visibility,direct);
    vec3 illumination=max(cached.rgb/3.14159265+direct,vec3(0));
    float nv=max(dot(n,eye),.001),fresnel=water_fresnel(nv);
    // Refracted ray angle from Snell's law, measured against the surface normal.
    vec3 transmittance=water_transmittance(coast.z+h+swell.z,nv);
    // Existing artwork represents the submerged colour; no opaque scene refraction here.
    float streak=smoothstep(.25,.75,wave.w);
    vec3 bed=probe_linear(painted)*mix(.86,1.14,streak);
    vec3 body_colour=water_art_tint(vec3(.025,.19,.30),probe_linear(painted));
    vec3 transmission=(bed*transmittance+body_colour*(1.-transmittance))*illumination;
    // Low-frequency probe approximation until environment reflection maps are available.
    vec3 reflection=max(cached.rgb/3.14159265,vec3(0));
    vec3 lit=(1.-fresnel)*transmission+fresnel*reflection;
    lit+=specular;
    float shore_phase=coast.y*5.5-water_time*1.4;
    float shore=exp(-coast.y*.7)*coast.x;
    float crest=smoothstep(.006,.025,h)+smoothstep(.83,.97,sin(shore_phase))*shore*.35
                *(1.-smoothstep(.4,1.5,fwidth(shore_phase)));
    float breakup=.5+.5*wave.w;
    vec2 maintained=(water_surface==1?vec2(0):flow.xy*.8)*water_current_scale;
    float activity=smoothstep(.12,.7,abs(rotation))*smoothstep(.05,.25,length(wave.yz-maintained));
    // Thin advected ribbons reveal rotational flow. Only disturbed water gets
    // this foam-like scattering; quiet water retains the original painted art.
    float ribbon_distance=abs(wave.w-.62)/max(length(gradients.zw),.1);
    float edge=max(fwidth(ribbon_distance),.015);
    float ribbon=1.-smoothstep(.025,.025+edge,ribbon_distance);
    float foam=activity*ribbon*.3;
    lit=mix(lit,vec3(.65,.8,.85)*illumination,clamp(crest*breakup*.35+foam,0.,.55));
    return probe_display(max(lit,vec3(0)));
}
