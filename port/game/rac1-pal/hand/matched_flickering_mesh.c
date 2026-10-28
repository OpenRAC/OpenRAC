/* SPDX-License-Identifier: GPL-3.0-or-later */
/* func_L08_002F7318 (draw_flickering_mesh_effect), Batalia's animated grid effect. As the port
 * runs it: the decompilation's matched C (src/overlays/l08_batalia/commando_deserter_and_effects.c)
 * with its three variable-length arrays (the grid of (ny+1) x (nx+1) points, the u and v rows)
 * taken from the game stack with alloca instead, which hostgen translates and a variable-length
 * array it does not. The space is the same, kept until the function returns rather than until its
 * block ends, which here is the same point. Otherwise the matched C, line for line; it goes at the
 * end of commando_deserter_and_effects.c. Not a PS2 match; the matched C stays as it matches
 * (hostgen.json "overrides": this C replaces it in the port only). */
void draw_flickering_mesh_effect(M45 *moby) {
    V45 corner[4],step,side_step[2],side[2],point,edge,normal;
    float matrix[16];
    Quad45 quad;
    V45 scratch[5];
    float fraction;
    Grid45 *data;
    if (!moby || !(data=moby->data)) return;
    if (scratch==0 && data==0) data->current=0;
    {
        int row_bytes=(data->nx+1)*sizeof(V45);
        unsigned char *points=(unsigned char*)__builtin_alloca((data->ny+1)*row_bytes);
        float inv_x=1.0f/IntToFloat(data->nx);
        float inv_y=1.0f/IntToFloat(data->ny);
        float *u=(float*)__builtin_alloca((data->nx+1)*sizeof(float));
        float *v=(float*)__builtin_alloca((data->ny+1)*sizeof(float));
        float phase,phase_step,amp,amp_step,amp2,amp2_step;
        float phase0,phase1,phase0_step,phase1_step,time,z0,z1,bias;
        int alpha,frame;
        float row_noise;
        int remaining;
        int i,j;
        float *goal,*noise;
        int ui; float f;
        float pwidth,px,pfraction,psize; short plife; int pr,pg,pb,pa;
        int c0,c1; float fwidth,fsize; int flife; float ffraction,fshape;
        Sound45 *sound; float sx,sy;
        if (!data) qzero(&moby->rotation);
        goal=data->goal; noise=data->noise;
        time=IntToFloat(D_L08_0015F4F8);
        for (remaining=0;remaining<8;goal++,remaining++,noise++) {
            if (*noise<*goal) {
                *noise+=data->noise_range*data->noise_rate;
                if (!(*goal<=*noise)) continue;
            } else {
                *noise-=data->noise_range*data->noise_rate;
                if (!(*noise<=*goal)) continue;
            }
            *goal=random_float_between(1.0f-data->noise_range,1.0f);
        }
        alpha=truncate_float_to_s32(data->alpha_scale*IntToFloat(data->alpha)*IntToFloat(moby->alpha)*0.0078125f)&0xFF;
        if (data->frames) { frame=D_L08_0015F4F8/(data->frame_ticks+1); data->current=data->texture+frame%data->frames; }
        else data->current=data->texture;
        if (data->flags&1) quad.gs[1]=GetEffectTex(data->current);
        else quad.gs[1]=GetEffectTex(data->current+0x28);
        quad.gs[0]=0;
        quad.gs[2]=0xFF9000000260L;
        quad.gs[3]=(long)data->blend0|((long)data->blend1<<2)|((long)data->blend2<<4)|((long)data->blend3<<6)|((long)data->alpha<<32);
        quad.color[3]=quad.color[2]=quad.color[1]=quad.color[0]=(data->alpha<<24)|(data->blue<<16)|(data->green<<8)|data->red;
        qzero(&corner[0]); qzero(&corner[1]); qzero(&corner[2]); qzero(&corner[3]);
        {
        float half_width=data->width*0.5f;
        corner[1][0]=corner[3][0]=half_width;
        corner[0][0]=corner[2][0]=-half_width;
        }
        corner[0][2]=data->z0+0.01f; corner[1][2]=data->z1+0.01f;
        euler_to_matrix(matrix,&moby->rotation);
        transform_vector(&corner[0],&corner[0],matrix);
        transform_vector(&corner[1],&corner[1],matrix);
        transform_vector(&corner[2],&corner[2],matrix);
        transform_vector(&corner[3],&corner[3],matrix);
        FastVecSub(&side_step[0],&corner[2],&corner[0]);
        FastVecScale(&side_step[0],&side_step[0],inv_y);
        FastVecSub(&side_step[1],&corner[3],&corner[1]);
        FastVecScale(&side_step[1],&side_step[1],inv_y);
        phase_step=(data->phase_end-data->phase_start)*inv_y;
        amp_step=(data->amp_end-data->amp_start)*inv_y;
        amp2_step=(data->amp2_end-data->amp2_start)*inv_y;
        phase0_step=(data->phase0_end-data->phase0)*inv_x;
        phase1_step=(data->phase1_end-data->phase1)*inv_y;
        z0=data->height-data->z0; z1=data->height-data->z1;
        FastVecSub(&edge,&corner[1],&corner[0]);
        FastVecCross(&normal,&edge,&side_step[0]);
        vec_scale_to_length(&normal,&normal,1.0f);
        qcopy(&side[0],&corner[0]); qcopy(&side[1],&corner[1]);
        bias=data->noise[0];
        phase=data->phase_start; amp=data->amp_start; amp2=data->amp2_start;
        phase0=data->phase0; phase1=data->phase1;
        {
        float y,x,theta,wave,noise,angle0,angle1,shape,drop,a,b,normal_angle0,normal_angle1;
        float sway0,sway1,lift,weight,swing,turn0,turn1;
        register int j;
        int i;
        for (i=0;i<=data->ny;phase+=phase_step,amp+=amp_step,amp2+=amp2_step,phase0+=phase0_step,phase1+=phase1_step,i++) {
            y=IntToFloat(i)/IntToFloat(data->ny);
            qcopy_nc(&point,&side[0]);
            j=0;
            FastVecSub(&step,&side[1],&side[0]);
            FastVecScale(&step,&step,inv_x);
            row_noise=((float(*)(char*,float))SmoothNoise)((char*)data,y);
            for (;j<=data->nx;j++) {
                x=IntToFloat(j)/IntToFloat(data->nx);
                swing=FastSin(normalize_angle(data->noise_speed*time));
                wave=time*phase+data->noise_amp*swing;
                noise=SmoothNoise(x,(char*)data);
                drop=z0*x+z1*(1.0f-x);
                theta=x*3.1415927f;
                wave+=noise*data->noise_u+row_noise*data->noise_v+bias*data->noise_bias;
                angle0=normalize_angle(wave+phase0+data->wave_u*x+data->wave2_v*y);
                angle1=normalize_angle(wave+phase1+data->wave_v*y+data->wave2_u*x);
                shape=FastSin(theta);
                shape=vec_length(shape);
                drop*=shape;
                drop*=1.0f-y;
                sway0=FastCos(angle0);
                vec_scale_to_length(&scratch[0],&step,amp*sway0*shape*noise);
                sway1=FastSin(angle1);
                vec_scale_to_length(&scratch[1],&side_step[0],amp2*sway1*shape*noise);
                a=FastSin(y*3.1415927f); b=FastSin(theta);
                turn0=normalize_angle(wave+data->wave_u*x);
                normal_angle0=FastSin(turn0);
                turn1=normalize_angle(wave+data->wave2_v*y);
                normal_angle1=FastSin(turn1);
                lift=a*b*(data->normal_u*normal_angle0+data->normal_v*normal_angle1);
                vec_scale_to_length(&scratch[3],&normal,lift);
                vec_scale_to_length(&scratch[4],&side_step[0],drop);
                FastVecAdd(&scratch[2],&scratch[0],&scratch[1]);
                FastVecAdd(&scratch[2],&scratch[2],&scratch[3]);
                FastVecSub(&scratch[2],&scratch[2],&scratch[4]);
                FastVecAdd((V45*)(points+i*row_bytes)+j,&point,&scratch[2]);
                weight=(1.0f-data->alpha_u+data->alpha_u*shape*shape*shape)*(1.0f-data->alpha_v+data->alpha_v*y);
                *(float*)(points+(i*row_bytes+j*sizeof(V45))+12)=1.0f/weight;
                FastVecAdd(&point,&point,&step);
            }
            FastVecAdd(&side[0],&side[0],&side_step[0]);
            FastVecAdd(&side[1],&side[1],&side_step[1]);
        }
        }
        {
            f=split_float_fraction(&fraction,time*data->u_rate+data->u_amp*FastSin(normalize_angle(time*data->u_phase)));
            for (ui=0;ui<=data->nx;ui++) { u[ui]=f; f+=inv_x/data->tex_u_scale; }
            f=split_float_fraction(&fraction,time*data->v_rate+data->v_amp*FastSin(normalize_angle(time*data->v_phase)));
            for (ui=0;ui<=data->ny;ui++) { v[ui]=f; f+=inv_y/data->tex_v_scale; }
        }
        {
        int i;
        for (i=0;i<data->ny;i++) {
            int j;
            for (j=0;j<data->nx;j++) {
                int k=j+1;
                quad.uv[0]=u[j]; quad.uv[1]=v[i];
                quad.uv[2]=u[k]; quad.uv[3]=v[i];
                quad.uv[4]=u[j]; quad.uv[5]=v[i+1];
                quad.uv[6]=u[k]; quad.uv[7]=v[i+1];
                FastVecAdd(&quad.vertex[0],(V45*)(points+i*row_bytes)+j,&moby->position);
                FastVecAdd(&quad.vertex[1],(V45*)(points+i*row_bytes)+(j+1),&moby->position);
                FastVecAdd(&quad.vertex[2],(V45*)(points+(i+1)*row_bytes)+j,&moby->position);
                FastVecAdd(&quad.vertex[3],(V45*)(points+(i+1)*row_bytes)+(j+1),&moby->position);
                quad.color[0]=((alpha/truncate_float_to_s32(*(float*)(points+(i*row_bytes+j*sizeof(V45))+12)))<<24)|(data->blue<<16)|(data->green<<8)|data->red;
                quad.color[1]=((alpha/truncate_float_to_s32(*(float*)(points+(i*row_bytes+k*sizeof(V45))+12)))<<24)|(data->blue<<16)|(data->green<<8)|data->red;
                quad.color[2]=((alpha/truncate_float_to_s32(*(float*)(points+((i+1)*row_bytes+j*sizeof(V45))+12)))<<24)|(data->blue<<16)|(data->green<<8)|data->red;
                quad.color[3]=((alpha/truncate_float_to_s32(*(float*)(points+((i+1)*row_bytes+k*sizeof(V45))+12)))<<24)|(data->blue<<16)|(data->green<<8)|data->red;
                draw_world_quad(&quad,0,0);
            }
        }
        }
        if (data->flags&2) {
            if (fast_dec_timer(&data->particle_timer)) {
                data->particle_timer=scale_ticks(data->particle_ticks);
                for (i=0;i<data->particle_count;i++) {
                    pwidth=data->width*0.5f;
                    px=random_float_between(-1.0f,1.0f)*data->particle_range;
                    pfraction=random_float_between(0.0f,px);
                    vec_scale_to_length(&scratch[2],&side_step[0],-(data->particle_offset+vec_length(FastCos(pfraction*3.1415927f))*data->particle_height));
                    vec_scale_to_length(&scratch[1],&step,pfraction*pwidth);
                    FastVecAdd(&scratch[1],&scratch[1],&moby->position);
                    FastVecAdd(&scratch[1],&scratch[1],&scratch[2]);
                    scratch[0][0]=random_float_between(-data->particle_xy,data->particle_xy)*frame_scale;
                    scratch[0][1]=random_float_between(-data->particle_xy,data->particle_xy)*frame_scale;
                    scratch[0][2]=random_float_between(0.0f,data->particle_z)*frame_scale;
                    psize=random_float_between(data->particle_size0,data->particle_size1);
                    plife=scale_ticks(data->particle_life);
                    pr=data->particle_r; pg=data->particle_g; pb=data->particle_b;
                    pa=truncate_float_to_s32(IntToFloat(data->particle_a)*pfraction);
                    func_L08_0027C218(moby,psize,&scratch[1],&scratch[0],plife,pr,pg,pb,pa);
                }
            }
            if (FastDecTimer(&data->puff_timer)) {
                data->puff_timer=scale_ticks(data->puff_ticks);
                for (i=0;i<data->puff_count;i++) {
                    float one=1.0f;
                    c0=(data->puff_c0[3]<<24)|(data->puff_c0[2]<<16)|(data->puff_c0[1]<<8)|data->puff_c0[0];
                    c1=(data->puff_c1[3]<<24)|(data->puff_c1[2]<<16)|(data->puff_c1[1]<<8)|data->puff_c1[0];
                    fwidth=data->width*0.5f;
                    fsize=random_float_between(data->puff_size0,data->puff_size1);
                    flife=scale_ticks(rand_range(data->puff_life0,data->puff_life1));
                    ffraction=random_float_between(-1.0f,one)*data->puff_range;
                    ffraction=random_float_between(-ffraction,ffraction);
                    fshape=vec_length(FastCos(ffraction*3.1415927f));
                    vec_scale_to_length(&scratch[2],&side_step[0],-(data->puff_offset+fshape*data->puff_height));
                    vec_scale_to_length(&scratch[1],&step,ffraction*fwidth);
                    FastVecAdd(&scratch[1],&scratch[1],&moby->position);
                    FastVecAdd(&scratch[1],&scratch[1],&scratch[2]);
                    scratch[0][0]=random_float_between(-data->puff_xy,data->puff_xy)*frame_scale;
                    scratch[0][1]=random_float_between(-data->puff_xy,data->puff_xy)*frame_scale;
                    scratch[0][2]=random_float_between(0.0f,data->puff_z)*frame_scale;
                    if (scratch==0 && i<0) data->puff_timer=0;
                    FastVecScale(&scratch[0],&scratch[0],one-data->puff_blend+fshape*data->puff_blend);
                    spawn_effect_particle_with_target_scalar(&scratch[1],&scratch[0],c0,c1,fsize,flife);
                }
            }
            if (data->sound_index!=-1) {
                sound=&batalia_lights[data->sound_index];
                if (FastVecDist(&data->sound_position,&sound->position)<data->sound_speed*frame_scale) {
                    sx=(random_float_between(-(int)data->sound_range_x,data->sound_range_x)+data->width)*0.5f;
                    sy=(random_float_between(-(int)data->sound_range_y,data->sound_range_y)+data->height)*0.5f;
                    vec_scale_to_length(&scratch[0],&step,sx);
                    vec_scale_to_length(&scratch[1],&side_step[0],-sy);
                    FastVecAdd(&data->sound_position,&scratch[0],&scratch[1]);
                    FastVecAdd(&data->sound_position,&data->sound_position,&corner[0]);
                    FastVecAdd(&data->sound_position,&data->sound_position,&moby->position);
                } else {
                    FastVecSub(&scratch[0],&data->sound_position,&sound->position);
                    vec_scale_to_length(&scratch[0],&scratch[0],data->sound_speed*frame_scale);
                    FastVecAdd(&sound->position,&sound->position,&scratch[0]);
                    sound->position[3]=IntToFloat(data->sound_value);
                }
            }
            data->flags^=2;
        }
    }
}
