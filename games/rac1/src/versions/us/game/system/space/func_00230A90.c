void update_resident_gameplay_state(void) {
    Vector4_230A90 scratch_vectors[4];
    s32 keyframe_flags;
    s32 ticks_per_bank;
    s32 skip;
    s32 object_index;
    s32 point_index;
    ResidentRenderObject *object;
    ResidentRenderObject *object_cursor;
    ResidentRenderObject *expired_object;
    s32 release_index;
    s32 bank_tick_limit;
    ResidentRenderObject *player;
    s32 path_point_index;
    f32 camera_blend_step;
    CameraKeyframe *keyframe;
    f32 *keyframe_angles;
    u8 *animation_positions;
    s32 current_frame;
    ScriptedPath *path;
    f32 segment_angle;
    f32 maximum_turn;
    f32 turn_scale;
    f32 blend;

    resident_data_0013E130.timer++;
    switch (resident_data_0013E130.state) {
        case 0:
        case 8:
            noop_callback_i();
            update_visible_resident_objects();
            enqueue_voice_request();
            noop_callback_g();
            UpdateParts();
            resident_data_0015F53C -= 0.125f;
            resident_data_0018CC20.frame++;
            resident_data_0018CC20.time++;
            if (resident_data_0015F53C < 0.0f) {
                resident_data_0015F53C = 0.0f;
            }
            ticks_per_bank = resident_data_0015EE80 ? 0x50 : 0x60;
            skip = resident_data_0013E130.skip;
            if ((resident_data_0013CBE4[0] & 0x50) && scale_game_frame_count(30) < resident_data_0018CC20.time &&
                resident_data_0015F53C == 0.0f) {
                if (resident_data_0013E130.state == 0 &&
                    resident_data_0018CC20.time <
                        scale_game_frame_count(
                            ((s32 *)&resident_duration_base)[resident_data_0013E130.content_variant] - 30)) {
                    skip = 1;
                }
                if (resident_data_0013E130.state == 8 &&
                    resident_data_0018CC20.time < resident_data_0018CC20.end - scale_game_frame_count(30)) {
                    skip = 1;
                }
            }
            if (skip) {
                if (resident_data_001517D0.unk5A != 6 && resident_data_001517D0.unk5A != 7) {
                    resident_data_001517D0.unk5A = 5;
                }
                if (resident_data_0013E130.state == 0) {
                    resident_data_0018CC20.time = scale_game_frame_count(
                        ((s32 *)&resident_duration_base)[resident_data_0013E130.content_variant]);
                    resident_data_0018CC20.sequence_frame = resident_data_0018CC20.time / ticks_per_bank;
                    parse_space_scene_chunk(resident_data_0018CC20.sequence_frame);
                    resident_data_0018CC20.frame = resident_data_0018CC20.time % ticks_per_bank;
                    if (resident_data_0013E130.skip) {
                        resident_data_0013E130.skip = 0;
                    } else {
                        FadeToBlack(4);
                    }
                    resident_data_0015F53C = 1.0f;
                } else {
                    resident_data_0018CC20.time = resident_data_0018CC20.end;
                }
            }
            if (resident_data_0018CC20.time >= resident_data_0018CC20.end) {
                if (resident_data_001517D0.unk5A != 6 && resident_data_001517D0.unk5A != 7) {
                    resident_data_001517D0.unk5A = 5;
                }
                select_level_vu_chain();
                resident_data_0018CEB0[0] = 0.63f;
                UpdateViewContext();
                /* Retail releases the attachment once per sequence object, even when that object is null. */
                for (release_index = 0; release_index < resident_data_0018CC20.count; release_index++) {
                    expired_object = resident_data_0018CC20.objects[release_index];
                    if (expired_object != 0) {
                        expired_object->model->count--;
                        expired_object->model->frames[expired_object->model->count] = 0;
                        resident_call_0020D678(expired_object);
                    }
                    if (resident_data_0013E130.attachment != 0) {
                        resident_call_0020D678(resident_data_0013E130.attachment);
                    }
                }
                resident_data_0013E130.player->flags &= ~1;
                for (object_cursor = resident_data_0016001C; object_cursor->state != 0xFF; object_cursor++) {
                    if (!(object_cursor->state & 0x80) &&
                        (object_cursor->class_id == 0x4A || object_cursor->class_id == 0xCB)) {
                        object_cursor->flags &= ~0x80;
                    }
                }
                if (resident_data_0013E130.state == 0) {
                    PauseAllSounds(0);
                    resident_data_001D5F70 = 0xE;
                    resident_data_0015F6FC[0] = 1;
                    return;
                }
                resident_data_0015F6E8 = 0;
                music_Unpause();
                resident_data_0015F6FC[0] = 1;
                resident_data_001414F5[0] = 0;
                noop_callback_j();
                if (resident_data_0013E130.unk2C != 0) {
                    noop_callback_d();
                    return;
                }
                noop_callback_k(&resident_data_0013E130.unk60, &resident_data_0013E130.unk70, 0, 1);
                return;
            }
            bank_tick_limit = resident_data_0015EE80 ? 0x50 : 0x60;
            if (resident_data_0018CC20.frame >= bank_tick_limit) {
                parse_space_scene_chunk(++resident_data_0018CC20.sequence_frame);
            }
            keyframe = &resident_data_0018CC20.frames[resident_data_0018CC20.frame];
            keyframe_angles = keyframe->rotation;
            keyframe_flags = keyframe->position.bytes[12];
            resident_data_0018CEB0[0] = keyframe_angles[3];
            UpdateViewContext();
            qcopy(&resident_data_00187040.position, &keyframe->position);
            transform_vector_by_basis(&resident_data_00187040.position, &resident_data_00187040.position,
                                      resident_data_0013E130.player->transform);
            FastVecAdd(&resident_data_00187040.position, &resident_data_00187040.position,
                       &resident_data_0013E130.player->position);
            segment_angle = FastAddRots(keyframe_angles[2], resident_data_0013E130.player->rotation.components[2]);
            resident_call_00125358(scratch_vectors);
            resident_call_001254A0(scratch_vectors, scratch_vectors, keyframe->rotation[0]);
            resident_call_00125548(scratch_vectors, scratch_vectors, keyframe_angles[1]);
            resident_call_001253F8(scratch_vectors, scratch_vectors, segment_angle);
            resident_data_00187040.forward.components[0] = -scratch_vectors[2].components[0];
            resident_data_00187040.right.components[0] = -scratch_vectors[0].components[0];
            resident_data_00187040.up.components[0] = scratch_vectors[1].components[0];
            resident_data_00187040.forward.components[1] = -scratch_vectors[2].components[1];
            resident_data_00187040.right.components[1] = -scratch_vectors[0].components[1];
            resident_data_00187040.up.components[1] = scratch_vectors[1].components[1];
            resident_data_00187040.forward.components[2] = -scratch_vectors[2].components[2];
            resident_data_00187040.right.components[2] = -scratch_vectors[0].components[2];
            resident_data_00187040.up.components[2] = scratch_vectors[1].components[2];
            if (resident_data_0015EEB4[0] != 0) {
                FastVecCross(&resident_data_00187040.right, &resident_data_00187040.up,
                             &resident_data_00187040.forward);
            }
            for (object_index = 0; object_index < resident_data_0018CC20.count; object_index++) {
                object = resident_data_0018CC20.objects[object_index];
                current_frame = resident_data_0018CC20.frame >> 1;
                object->current_frame = current_frame;
                object->next_frame = current_frame + 1;
                resident_call_0020D6D0(object);
                object->blend = IntToFloat(resident_data_0018CC20.frame & 1) * 0.5f;
                if (keyframe_flags != 0 && (resident_data_0018CC20.frame & 1)) {
                    object->blend = 1.0f;
                }
                animation_positions = object->animation_positions;
                FastVecScale(&scratch_vectors[0], animation_positions + object->current_frame * 16,
                             1.0f - object->blend);
                FastVecScale(&scratch_vectors[1], animation_positions + object->next_frame * 16, object->blend);
                FastVecAdd(&object->position, &scratch_vectors[0], &scratch_vectors[1]);
                transform_vector_by_basis(&object->position, &object->position,
                                          resident_data_0013E130.player->transform);
                FastVecAdd(&object->position, &object->position, &resident_data_0013E130.player->position);
                object->rotation.components[2] = resident_data_0013E130.player->rotation.components[2];
                object->cached_selector = 0xFF;
                resident_call_0020ED48(object);
                if (object->class_id == 0) {
                    if (resident_data_0013E130.state == 0) {
                        if (g_current_level_index_r == LEVEL_ORXON && resident_data_0013D5C8[6] == 0) {
                            object->unk7F = 0;
                        } else if (resident_data_0018CC20.time <= scale_game_frame_count(0x3E)) {
                            object->unk7F = 0x18;
                        } else {
                            object->unk7F = 0;
                        }
                    } else if (scale_game_frame_count(360) < resident_data_0018CC20.time) {
                        if (g_current_level_index_r == LEVEL_ORXON && resident_data_0013D5C8[6] == 0) {
                            object->unk7F = 0;
                        } else {
                            object->unk7F = 0x18;
                        }
                    } else {
                        object->unk7F = 0;
                    }
                } else if (object->class_id == 10) {
                    if (resident_data_0013E130.state == 0) {
                        if (resident_data_0018CC20.time <= scale_game_frame_count(350)) {
                            object->unk7F = 0x18;
                        } else {
                            object->unk7F = 0;
                        }
                    } else if (resident_data_0018CC20.time <= scale_game_frame_count(524)) {
                        object->unk7F = 0;
                    } else {
                        object->unk7F = 0x18;
                    }
                }
                if (object->unk7F != 0) {
                    resident_call_00214550(object);
                }
                if (object->class_id == 10) {
                    noop_callback_f(object);
                }
                if (object->class_id == 0) {
                    resident_call_001E9790(object);
                    if ((g_current_level_index_r == LEVEL_ORXON && resident_data_0013D5C8[6] != 0) ||
                        g_current_level_index_r == LEVEL_GEMLIK) {
                        if (resident_data_0013E130.attachment == 0) {
                            resident_data_0013E130.attachment = resident_call_0020D348(0x509);
                            resident_data_0013E130.attachment->selected_index = 0x40;
                            resident_data_0013E130.attachment->flags |= 0x806;
                            resident_data_0013E130.attachment->lifetime_stamp = object->lifetime_stamp;
                            if (resident_data_0013E130.attachment->model->unk6 != 0) {
                                resident_data_0013E130.attachment->unk73 = 0x18;
                            }
                        }
                        noop_callback_m(object, resident_data_0013E130.attachment);
                    }
                }
                if ((u16)object->class_id - 0x213U < 3) {
                    enqueue_callback_list_4(resident_call_00233AB8, object);
                    resident_call_001F49B0(resident_call_0022F258, object);
                    if (resident_data_0013E130.state == 0) {
                        if (resident_data_0018CC20.time < scale_game_frame_count(360)) {
                            s32 color_intensity =
                                (s32)(FastCos(IntToFloat((resident_data_0018CC20.time & 0x3F) - 32) * 0.09817477f) *
                                      80.0f) +
                                120;
                            if (object->class_id == 0x215) {
                                object->color =
                                    (color_intensity >> 1) | (color_intensity << 8) | (color_intensity << 16);
                            } else {
                                object->color = color_intensity | (color_intensity << 8) | (color_intensity << 16);
                            }
                        } else if (resident_data_0018CC20.time < scale_game_frame_count(0x1F8)) {
                            s32 color_intensity =
                                (s32)(((f32)resident_data_0018CC20.time - multiply_global_scale(360.0f)) *
                                      (resident_data_0015EE60 * 1.25f));
                            if (color_intensity >= 256) {
                                color_intensity = 255;
                            }
                            if (object->class_id == 0x215) {
                                object->color = (color_intensity << 8) | (color_intensity << 16);
                            } else {
                                object->color = color_intensity | (color_intensity << 8) | (color_intensity << 16);
                            }
                            if (scale_game_frame_count(0x1A4) < resident_data_0018CC20.time) {
                                resident_data_0013E130.player->unkBC =
                                    (s32)(((f32)resident_data_0018CC20.time - multiply_global_scale(420.0f)) *
                                          (resident_data_0015EE60 * 1.2f));
                                resident_data_0013E130.player->unkB2 = 0;
                                resident_call_001F49B0(resident_effect_entry + 8, resident_data_0013E130.player);
                            }
                            if (scale_game_frame_count(0x1D0) < resident_data_0018CC20.time) {
                                resident_data_0015F540 += 0.025f;
                                if (resident_data_0015F540 > 1.0f) {
                                    resident_data_0015F540 = 1.0f;
                                }
                            }
                        } else {
                            object->color = 0xA0A0A0;
                            resident_data_0015F540 -= 0.025f;
                            if (resident_data_0015F540 < 0.0f) {
                                resident_data_0015F540 = 0.0f;
                            }
                        }
                    } else if (resident_data_0018CC20.time < scale_game_frame_count(0xF0)) {
                        s32 color_intensity;
                        if (resident_data_0018CC20.time < scale_game_frame_count(0xC8)) {
                            resident_call_002308C8(object, resident_data_0015EE6C * -4.0f);
                            resident_call_002308C8(object, resident_data_0015EE6C * -3.0f);
                        }
                        color_intensity =
                            (s32)(FastCos(IntToFloat((resident_data_0018CC20.time & 0x3F) - 32) * 0.09817477f) *
                                  80.0f) +
                            120;
                        if (object->class_id == 0x215) {
                            object->color = (color_intensity >> 1) | (color_intensity << 8) | (color_intensity << 16);
                        } else {
                            object->color = color_intensity | (color_intensity << 8) | (color_intensity << 16);
                        }
                        object->unkBC = 0x32;
                        object->unkB2 = 10;
                        resident_call_001F49B0(resident_effect_entry + 8, object);
                    } else if (resident_data_0018CC20.time <= scale_game_frame_count(360)) {
                        s32 color_intensity;
                        if (resident_data_0018CC20.time < scale_game_frame_count(300)) {
                            resident_data_0013E130.player->unkBC =
                                (s32)((multiply_global_scale(300.0f) - (f32)resident_data_0018CC20.time) *
                                      (resident_data_0015EE60 * 1.5f));
                            resident_data_0013E130.player->unkB2 = 0;
                            resident_call_001F49B0(resident_effect_entry + 8, resident_data_0013E130.player);
                        }
                        color_intensity = (s32)((multiply_global_scale(360.0f) - (f32)resident_data_0018CC20.time) *
                                                (resident_data_0015EE60 * 1.25f));
                        if (object->class_id == 0x215) {
                            object->color = (color_intensity << 8) | (color_intensity << 16);
                        } else {
                            object->color = color_intensity | (color_intensity << 8) | (color_intensity << 16);
                        }
                    } else {
                        s32 color_intensity =
                            (s32)(FastCos(IntToFloat((resident_data_0018CC20.time & 0x3F) - 32) * 0.09817477f) *
                                  80.0f) +
                            120;
                        if (object->class_id == 0x215) {
                            object->color = (color_intensity >> 1) | (color_intensity << 8) | (color_intensity << 16);
                        } else {
                            object->color = color_intensity | (color_intensity << 8) | (color_intensity << 16);
                        }
                    }
                }
            }
            sound_update();
            UpdateAllPointLights();
            update_resident_packed_direction();
            noop_callback_h();
            break;

        case 3:
            noop_callback_i();
            update_visible_resident_objects();
            enqueue_voice_request();
            noop_callback_g();
            UpdateParts();
            if (resident_data_0013E130.timer == 0) {
                music_Pause(0);
                update_audio_stream_until_idle(0);
                resident_data_001517D0.unk1C = resident_data_0013E130.content_variant + 0x9C4F;
                FadeToBlack(scale_game_frame_count(12));
                while ((s16)resident_data_001517D0.unk5A != 3) {
                    sound_update();
                    sce_gs_sync_v(0);
                }
                resident_call_00216960();
                resident_data_0013E130.player->selected_a = 1;
                resident_data_0013E130.player->selected_b = 2;
                resident_data_0013E130.player->blend = 0.0f;
                resident_call_0020D6D0(resident_data_0013E130.player);
                resident_data_0013E130.player->opacity = 0xFF;
                resident_data_0013E130.player->unk94 = 0;
                if (resident_data_0013E130.path >= 0) {
                    resident_data_0013E130.companion_a = resident_call_0020D348(0);
                    resident_data_0013E130.companion_a->selected_index = 0x1FF;
                    resident_data_0013E130.companion_a->opacity = 0xFF;
                    resident_data_0013E130.companion_a->unk94 = 0;
                    resident_data_0013E130.companion_a->flags |= 6;
                    resident_data_0013E130.companion_a->lifetime_stamp =
                        resident_data_0013F450.reference_object->lifetime_stamp;
                    resident_data_0013E130.companion_b = resident_call_0020D348(10);
                    resident_data_0013E130.companion_b->selected_index = 0x1FF;
                    resident_data_0013E130.companion_b->opacity = 0xFF;
                    resident_data_0013E130.companion_b->unk94 = 0;
                    resident_data_0013E130.companion_b->flags |= 6;
                    resident_data_0013E130.companion_b->lifetime_stamp =
                        resident_data_0013F450.reference_object->lifetime_stamp;
                }
                noop_callback_a(&resident_data_00187040.position, &resident_data_00187040.unk150, 1, 0, 0);
                resident_data_0013E130.blend = 0.0f;
                resident_data_0013E130.interpolation_velocity = 0.0f;
                if (resident_data_0013E130.path >= 0) {
                    path = resident_data_001CC730[resident_data_0013E130.path];
                    maximum_turn = resident_data_0013E130.blend;
                    path->p[0].components[3] = maximum_turn;
                    for (path_point_index = 1; path_point_index < path->point_count - 1; path_point_index++) {
                        segment_angle = fast_arc_tan(
                            path->p[path_point_index].components[0] - path->p[path_point_index - 1].components[0],
                            path->p[path_point_index].components[1] - path->p[path_point_index - 1].components[1]);
                        path->p[path_point_index].components[3] = FastSubRots(
                            fast_arc_tan(
                                path->p[path_point_index + 1].components[0] - path->p[path_point_index].components[0],
                                path->p[path_point_index + 1].components[1] - path->p[path_point_index].components[1]),
                            segment_angle);
                        if (maximum_turn < absolute_float(path->p[path_point_index].components[3])) {
                            maximum_turn = absolute_float(path->p[path_point_index].components[3]);
                        }
                    }
                    segment_angle = fast_arc_tan(
                        path->p[path->point_count - 1].components[0] - path->p[path->point_count - 2].components[0],
                        path->p[path->point_count - 1].components[1] - path->p[path->point_count - 2].components[1]);
                    path->p[path->point_count - 1].components[3] = FastSubRots(
                        fast_arc_tan(path->p[0].components[0] - path->p[path->point_count - 1].components[0],
                                     path->p[0].components[1] - path->p[path->point_count - 1].components[1]),
                        segment_angle);
                    turn_scale = 0.34906584f / maximum_turn;
                    for (point_index = 0; point_index < path->point_count; point_index++) {
                        path->p[point_index].components[3] *= turn_scale;
                    }
                    resident_data_0013E130.path_segment_length = fast_vec_dist(&path->p[0], &path->p[1]);
                    resident_data_0013E130.path_progress = 0.0f;
                    resident_data_0013E130.speed = 0.0f;
                    resident_data_0013E130.trail = 0;
                    resident_data_0013E130.history_count = 0;
                    player = resident_data_0013E130.player;
                    qcopy(&resident_data_0013E130.startPos, &player->position);
                    qcopy(&resident_data_0013E130.pathPos, &path->p[0]);
                    qcopy(&resident_data_0013E130.startRot, &player->rotation);
                    resident_data_0013E130.rotY =
                        fast_arc_tan(resident_call_001F9D48(&path->p[1], &path->p[0], &resident_data_0013E130.startPos),
                                     path->p[0].components[2] - path->p[1].components[2]);
                    resident_data_0013E130.rotZ = fast_arc_tan(path->p[1].components[0] - path->p[0].components[0],
                                                               path->p[1].components[1] - path->p[0].components[1]);
                }
            }
            if (resident_data_0013E130.source_camera_index >= 0 &&
                resident_data_0013E130.destination_camera_index >= 0) {
                camera_blend_step = resident_data_0015EE70 * 0.666f;
                resident_call_00214D88(&resident_data_0013E130.blend, &resident_data_0013E130.interpolation_velocity,
                                       1.0f, camera_blend_step, camera_blend_step, resident_data_0015EE6C * 0.5f);
                lerp_vector(&scratch_vectors[0],
                            &resident_data_00160134[resident_data_0013E130.source_camera_index].position,
                            &resident_data_00160134[resident_data_0013E130.destination_camera_index].position,
                            resident_data_0013E130.blend);
                clear_u64_value(&scratch_vectors[1]);
                scratch_vectors[1].components[1] =
                    FastSubRots(resident_data_00160134[resident_data_0013E130.destination_camera_index].rotY,
                                resident_data_00160134[resident_data_0013E130.source_camera_index].rotY) *
                    resident_data_0013E130.blend;
                scratch_vectors[1].components[1] =
                    FastAddRots(resident_data_00160134[resident_data_0013E130.source_camera_index].rotY,
                                scratch_vectors[1].components[1]);
                scratch_vectors[1].components[2] =
                    FastSubRots(resident_data_00160134[resident_data_0013E130.destination_camera_index].rotZ,
                                resident_data_00160134[resident_data_0013E130.source_camera_index].rotZ) *
                    resident_data_0013E130.blend;
                scratch_vectors[1].components[2] =
                    FastAddRots(resident_data_00160134[resident_data_0013E130.source_camera_index].rotZ,
                                scratch_vectors[1].components[2]);
                noop_callback_b(&scratch_vectors[0]);
                noop_callback_c(&scratch_vectors[1]);
            }
            if (resident_data_0013E130.path >= 0) {
                if (resident_data_0013E130.timer < scale_game_frame_count(150)) {
                    blend = (1.0f - FastCos(IntToFloat(resident_data_0013E130.timer) *
                                            (3.1415927f / IntToFloat(scale_game_frame_count(120))))) *
                            0.5f;
                    if (resident_data_0013E130.timer >= scale_game_frame_count(120)) {
                        blend = 1.0f;
                    }
                    resident_data_0013E130.player->blend = blend;
                    lerp_vector(&resident_data_0013E130.player->position, &resident_data_0013E130.startPos,
                                &resident_data_0013E130.pathPos, blend);
                    resident_data_0013E130.player->rotation.components[0] = 0.0f;
                    resident_data_0013E130.player->rotation.components[1] = FastAddRots(
                        resident_data_0013E130.startRot.components[1],
                        FastSubRots(resident_data_0013E130.rotY, resident_data_0013E130.startRot.components[1]) *
                            blend);
                    resident_data_0013E130.player->rotation.components[2] = FastAddRots(
                        resident_data_0013E130.startRot.components[2],
                        FastSubRots(resident_data_0013E130.rotZ, resident_data_0013E130.startRot.components[2]) *
                            blend);
                    if (resident_data_0015F53C > 0.0f) {
                        resident_data_0015F53C -= 0.125f;
                        if (resident_data_0015F53C < 0.0f) {
                            resident_data_0015F53C = 0.0f;
                        }
                    }
                    if (blend < 1.0f) {
                        resident_call_002308C8(resident_data_0013E130.player, resident_data_0015EE6C * -3.75f);
                    }
                } else {
                    ScriptedPath *active_path;
                    resident_data_0013E130.speed += 0.8f;
                    active_path = resident_data_001CC730[resident_data_0013E130.path];
                    if (resident_data_0013E130.speed > 100.0f) {
                        resident_data_0013E130.speed = 100.0f;
                    }
                    if ((resident_data_0013CBE4[0] & 0x50) && resident_data_0015F53C < 0.0625f) {
                        resident_data_0015F53C = 0.0625f;
                    }
                    if ((f32)(active_path->point_count - 6) < resident_data_0013E130.path_progress &&
                        resident_data_0015F53C < 0.0625f) {
                        resident_data_0015F53C = 0.0625f;
                    }
                    if (resident_data_0013E130.content_variant < 2) {
                        resident_data_0013E130.player->unkBC = 0x32;
                        resident_data_0013E130.player->unkB2 = 10;
                        resident_call_001F49B0(resident_effect_entry + 8, resident_data_0013E130.player);
                    }
                    resident_data_0013E130.path_progress += resident_data_0013E130.speed * resident_data_0015EE6C /
                                                            resident_data_0013E130.path_segment_length;
                    if ((f32)(active_path->point_count - 1) < resident_data_0013E130.path_progress ||
                        resident_data_0015F53C >= 1.0f) {
                        *(s32 *)&resident_data_0015F690 = 1;
                        resident_data_0015F6FC[0] = 1;
                    } else {
                        resident_call_00215CA8(active_path, 1, &resident_data_0013E130.player->position,
                                               &resident_data_0013E130.player->rotation, 0,
                                               resident_data_0013E130.path_progress);
                        resident_data_0013E130.player->rotation.components[0] =
                            resident_data_0013E130.player->rotation.components[3];
                        resident_data_0013E130.trail = (resident_data_0013E130.trail + 1) & 0x1F;
                        if (resident_data_0013E130.history_count < 0x20) {
                            resident_data_0013E130.history_count++;
                        }
                        transform_vector_by_basis(&resident_data_0013E130.trailA[resident_data_0013E130.trail],
                                                  &resident_data_001D9D40[resident_data_0013E130.content_variant].a,
                                                  resident_data_0013E130.player->transform);
                        FastVecAdd(&resident_data_0013E130.trailA[resident_data_0013E130.trail],
                                   &resident_data_0013E130.trailA[resident_data_0013E130.trail],
                                   &resident_data_0013E130.player->position);
                        resident_data_0013E130.trailA[resident_data_0013E130.trail].components[3] = 1.0f;
                        transform_vector_by_basis(&resident_data_0013E130.trailB[resident_data_0013E130.trail],
                                                  &resident_data_001D9D40[resident_data_0013E130.content_variant].b,
                                                  resident_data_0013E130.player->transform);
                        FastVecAdd(&resident_data_0013E130.trailB[resident_data_0013E130.trail],
                                   &resident_data_0013E130.trailB[resident_data_0013E130.trail],
                                   &resident_data_0013E130.player->position);
                        resident_data_0013E130.trailB[resident_data_0013E130.trail].components[3] = 1.0f;
                        resident_call_001F49B0(resident_call_0022F738, resident_data_0013E130.player);
                        if (resident_data_0015F53C > 0.0f) {
                            resident_data_0015F53C += 0.0625f;
                            if (resident_data_0015F53C > 1.0f) {
                                resident_data_0015F53C = 1.0f;
                            }
                        }
                    }
                }
                {
                    ResidentRenderObject *p = resident_data_0013E130.player;
                    ResidentRenderObject *scratch_vectors = resident_data_0013E130.companion_a;
                    qcopy(&scratch_vectors->rotation, &p->rotation);
                    transform_vector_by_basis(&scratch_vectors->position,
                                              &resident_data_001DA010[resident_data_0013E130.content_variant],
                                              p->transform);
                }
                FastVecAdd(&resident_data_0013E130.companion_a->position, &resident_data_0013E130.companion_a->position,
                           &resident_data_0013E130.player->position);
                resident_call_0020ED48(resident_data_0013E130.companion_a);
                {
                    ResidentRenderObject *scratch_vectors = resident_data_0013E130.companion_b;
                    ResidentRenderObject *p = resident_data_0013E130.player;
                    qcopy(&scratch_vectors->rotation, &p->rotation);
                    transform_vector_by_basis(&scratch_vectors->position,
                                              &resident_data_001DA040[resident_data_0013E130.content_variant],
                                              p->transform);
                }
                FastVecAdd(&resident_data_0013E130.companion_b->position, &resident_data_0013E130.companion_b->position,
                           &resident_data_0013E130.player->position);
                resident_call_0020ED48(resident_data_0013E130.companion_b);
            }
            update_camera();
            sound_update();
            UpdateAllPointLights();
            update_resident_packed_direction();
            noop_callback_h();
            break;

        case 4:
            resident_call_0022FDC0();
            break;
        case 1:
        case 2:
        case 5:
        case 6:
        case 7:
            break;
    }
}
