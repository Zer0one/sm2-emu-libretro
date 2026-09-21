#!/usr/bin/env python3
"""VF2 gameplay smoke test using RetroArch's replay-v1 reader.

The replay supplies every digital input, including released buttons, through
RetroArch. It contains no ROM data or save state. This is automated input proof,
not validation of a physical gamepad. See LIBRETRO.md and CI.md for tested frontends.
Replay format reference: official libretro/RetroArch at 05f94af4,
tasks/task_movie.c and input/input_driver.{h,c}.
"""
import argparse
import array
import hashlib
import json
import os
import platform
import time
from pathlib import Path
import struct
import subprocess
import wave

RETRO_DEVICE_JOYPAD = 1
RETRO_DEVICE_MOUSE = 2
RETRO_DEVICE_LIGHTGUN = 4
RETRO_DEVICE_ANALOG = 5


def read_recording(path):
    """Preserve and decode the known padded WAV header in RetroArch 1.22.2.

    record/drivers/record_wav.c writes wav_hdr_t verbatim, with char[5] tags
    and C struct padding. Only accept that exact PCM layout and finalized size.
    Save a standards-compliant copy alongside the untouched original capture.
    """
    try:
        with wave.open(str(path),'rb') as w:
            return w.getparams(), w.readframes(w.getnframes()), 'standard-wav'
    except wave.Error:
        data=path.read_bytes()
        assert data[:8]==b'RIFF\0\0\0\0' and data[12:24]==b'WAVE\0fmt \0\0\0'
        assert data[44:52]==b'data\0\0\0\0'
        assert struct.unpack_from('<I',data,24)[0]==16
        tag,channels,rate,byte_rate,block,bits=struct.unpack_from('<HHIIHH',data,28)
        size=struct.unpack_from('<I',data,52)[0]
        assert tag==1 and channels==2 and rate>0 and bits==16 and block==4
        assert byte_rate==rate*block and size==len(data)-56 and size%block==0
        assert struct.unpack_from('<I',data,8)[0]==len(data)-12
        pcm=data[56:]
        normalized=path.with_name(path.stem+'-normalized.wav')
        with wave.open(str(normalized),'wb') as w:
            w.setparams((channels,bits//8,rate,0,'NONE','not compressed'))
            w.writeframes(pcm)
        with wave.open(str(normalized),'rb') as w:
            return w.getparams(),pcm,'retroarch-1.22-padded-wav'


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ['retroarch','core','system','rom','output']:
        p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--set-name',default='vf2',help='ROM set name used for save and capture files')
    p.add_argument('--driving-inputs',action='store_true',
                   help='Add steering, pedals and four-speed gate input to the replay')
    p.add_argument('--sequential-inputs',action='store_true',
                   help='Add steering, pedals, View 1/2 and sequential shift input to the replay')
    p.add_argument('--analog-joystick-inputs',action='store_true',
                   help='Add left-stick X/Y and Sky Target action buttons to the replay')
    p.add_argument('--desert-inputs',action='store_true',
                   help='Add Desert Tank steering, elevation, accelerator, weapons, Shift and VR inputs')
    p.add_argument('--desert-elevation-control',choices=['relative','absolute'],default='relative')
    p.add_argument('--desert-elevation-speed',choices=[str(value) for value in range(10,201,10)],default='100')
    p.add_argument('--desert-elevation-axis',choices=['normal','inverted'],default='normal')
    p.add_argument('--bel-inputs',action='store_true',
                   help='Add left-stick gun aim plus Shot and Missile to the replay')
    p.add_argument('--gun-inputs',action='store_true',
                   help='Add left-stick gun aim and Shot to the replay')
    p.add_argument('--gun-secondary',choices=['none','reload','missile'],default='none',
                   help='Optional East action for --gun-inputs')
    p.add_argument('--offscreen-reload-shortcut',choices=['enabled','disabled'],default='enabled',
                   help='Core option for explicit RetroPad, Mouse and Lightgun reload inputs')
    p.add_argument('--gamepad-rumble',choices=['enabled','disabled'],default='enabled')
    p.add_argument('--crosshairs',choices=['0','1','2','3'],default='0',
                   help='Crosshair mask: disabled, P1, P2, or both players')
    p.add_argument('--timeout',type=int,default=180)
    p.add_argument('--renderer',choices=['software','vulkan','opengl'],default='software')
    p.add_argument('--opengl-driver',choices=['gl','glcore'],default='glcore',
                   help='RetroArch video driver used with --renderer opengl')
    p.add_argument('--scale',type=int,choices=range(1,5),default=1)
    p.add_argument('--texture-filter',choices=['faithful','2','4','8','16'],default='faithful')
    p.add_argument('--upscale-2d',choices=['faithful','xbr','scalefx'],default='faithful')
    p.add_argument('--av-timing',choices=['native','60hz'],default='native')
    p.add_argument('--timing-overlay',choices=['disabled','auto','11','12','13','14'],default='disabled')
    p.add_argument('--audio-balance',choices=['enabled','disabled'],default='enabled')
    p.add_argument('--music-volume',choices=[str(value) for value in range(0,201,10)],default='100')
    p.add_argument('--aspect-ratio',choices=['auto','4_3','16_9'],default='auto')
    p.add_argument('--initial-nvram-setup',choices=['enabled','disabled'],default='enabled')
    p.add_argument('--nvram-settings',choices=['disabled','enabled'],default='disabled')
    p.add_argument('--vf2-country',choices=['japan','usa','export'],default='japan')
    p.add_argument('--vf2-drink',choices=['ok','ng'],default='ok')
    p.add_argument('--vf2-difficulty',choices=['normal','hard','hardest','easy'],default='normal')
    p.add_argument('--vf2-display-type',choices=['projector','crt'],default='projector')
    p.add_argument('--initial-srm',type=Path,help='Optional frontend save RAM copied in before launch')
    p.add_argument('--initial-state',type=Path,
                   help='Optional Libretro state embedded in the replay and loaded by RetroArch')
    p.add_argument('--replay-reader', choices=['1.21','1.22'], default='1.21',
                   help='1.22 reads 40 header bytes even for a stateless v1 replay')
    p.add_argument('--audio-driver', default='coreaudio' if platform.system()=='Darwin' else 'alsa')
    p.add_argument('--input-driver', default='cocoa' if platform.system()=='Darwin' else 'x')
    p.add_argument('--moltenvk',type=Path,help='Optional macOS MoltenVK library, used only by the test process')
    a=p.parse_args()
    gun_inputs=a.gun_inputs or a.bel_inputs
    input_modes=(a.driving_inputs,a.sequential_inputs,a.analog_joystick_inputs,
                 a.desert_inputs,gun_inputs)
    if sum(input_modes)>1:
        p.error('input replay modes are mutually exclusive')
    out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
    for directory in ['saves','states','screenshots','playlists','config']:
        (out/directory).mkdir()
    if a.initial_srm:
        (out/'saves'/f'{a.set_name}.srm').write_bytes(a.initial_srm.resolve(strict=True).read_bytes())
    initial_state=a.initial_state.resolve(strict=True).read_bytes() if a.initial_state else b''
    movie=bytearray(struct.pack('<6I',0x42535632,1,0,len(initial_state),1,0))
    # RetroArch 1.22.2 bsv_movie_reset_playback reads 40 bytes and only
    # seeks back to byte 24 only when a v1 save state exists. Without one, pad
    # this fixture for that reader while keeping the actual v1 input records.
    if initial_state:
        movie += initial_state
    elif a.replay_reader=='1.22':
        movie += bytes(16)
    for frame in range(2400):
        pressed=set()
        if 800<=frame<810 or 820<=frame<830:pressed.add(2)  # Select / coin
        if 870<=frame<880 or 950<=frame<960 or 1050<=frame<1060:pressed.add(3)
        if frame>1100:
            if a.driving_inputs:
                if frame<1110:pressed.add(1)  # West / Neutral before engaging the gate
            elif a.sequential_inputs:
                if frame%480<120:pressed.add(4)  # D-Pad Up / View 1
                elif frame%480<240:pressed.add(5)  # D-Pad Down / View 2
                if frame%360<12:pressed.add(10)  # L1 / Shift Down
                elif frame%360<24:pressed.add(11)  # R1 / Shift Up
            elif a.analog_joystick_inputs:
                if frame%90<12:pressed.add(0)  # South / Machine Gun
                if frame%360<12:pressed.add(8)  # East / Missile
                if frame%480<12:pressed.add(9)  # North / View Change
            elif a.desert_inputs:
                if frame%90<12:pressed.add(0)  # South / Machine Gun
                if frame%180<12:pressed.add(8)  # East / Cannon
                if frame%480<12:pressed.add(1)  # West / Shift toggle
                if frame%600<12:pressed.add(4)  # D-Pad Up / VR3 (Red)
                elif frame%600<24:pressed.add(5)  # D-Pad Down / VR1 (Blue)
                elif frame%600<36:pressed.add(6)  # D-Pad Left / VR2 (Green)
            elif gun_inputs:
                if frame%90<12:pressed.add(0)  # South / Shot
                if (a.bel_inputs or a.gun_secondary!='none') and frame%360<12:
                    pressed.add(8)  # East / Reload Offscreen or Missile
            else:
                if frame%30<8:pressed.update([0,8,1])  # South, East, West
                if frame%120<60:pressed.add(7)  # Right
        analog_axes={(port,index,axis):0 for port in range(2)
                     for index in range(2) for axis in range(2)}
        analog_buttons={(port,button):0 for port in range(2) for button in range(16)}
        if (a.driving_inputs or a.sequential_inputs) and frame>1100:
            analog_axes[(0,0,0)]=-18000 if (frame//180)%2==0 else 18000
            analog_buttons[(0,13)]=0 if frame%180<20 else 32767  # R2 / Accelerator
            if frame%360<30:analog_buttons[(0,12)]=24576  # L2 / Brake
        if a.analog_joystick_inputs and frame>1100:
            analog_axes[(0,0,0)]=-18000 if (frame//180)%2==0 else 18000
            analog_axes[(0,0,1)]=-14000 if (frame//240)%2==0 else 14000
        if a.desert_inputs and frame>1100:
            analog_axes[(0,0,0)]=-18000 if (frame//180)%2==0 else 18000
            elevation_phase=(frame//180)%3
            elevation_value=(-24000,0,24000)[elevation_phase]
            elevation_stick=0 if (frame//540)%2==0 else 1
            analog_axes[(0,elevation_stick,1)]=elevation_value
            analog_buttons[(0,13)]=0 if frame%180<20 else 32767  # R2 / Accelerator
        if gun_inputs and frame>1100:
            analog_axes[(0,0,0)]=-22000 if (frame//180)%2==0 else 22000
            analog_axes[(0,0,1)]=-18000 if (frame//240)%2==0 else 18000
        if a.driving_inputs and frame>1100:
            quadrant=(frame//240)%4
            analog_axes[(0,1,0)]=(-20000,-20000,20000,20000)[quadrant]
            analog_axes[(0,1,1)]=(-20000,20000,-20000,20000)[quadrant]
        movie+=struct.pack('<BH',0,96)
        for port in range(2):
            for button in range(16):
                movie+=struct.pack('<4BHh',port,RETRO_DEVICE_JOYPAD,0,0,button,
                                   int(port==0 and button in pressed))
            for index in range(2):
                for axis in range(2):
                    movie+=struct.pack('<4BHh',port,RETRO_DEVICE_ANALOG,index,0,axis,
                                       analog_axes[(port,index,axis)])
            for button in range(16):
                movie+=struct.pack('<4BHh',port,RETRO_DEVICE_ANALOG,2,0,button,
                                   analog_buttons[(port,button)])
            for mouse_id in range(4):
                movie+=struct.pack('<4BHh',port,RETRO_DEVICE_MOUSE,0,0,mouse_id,0)
            for lightgun_id in (2,3,6,7,13,14,15,16):
                movie+=struct.pack('<4BHh',port,RETRO_DEVICE_LIGHTGUN,0,0,lightgun_id,0)
        movie+=b'f'
    replay=out/'vf2-inputs.replay';replay.write_bytes(movie)
    hardware=a.renderer!='software'
    cfg={'system_directory':str(a.system.resolve()),'savefile_directory':str(out/'saves'),
         'savestate_directory':str(out/'states'),'screenshot_directory':str(out/'screenshots'),
         'playlist_directory':str(out/'playlists'),'rgui_config_directory':str(out/'config'),
         'video_driver':{'vulkan':'vulkan','opengl':a.opengl_driver,'software':'gl'}[a.renderer],
         'audio_driver':a.audio_driver,'input_driver':a.input_driver,
         'video_fullscreen':'false','video_windowed_fullscreen':'false','video_scale':'2',
         'video_vsync':'false','audio_sync':'true','audio_enable':'true',
         'config_save_on_exit':'false','content_history_enable':'false',
         'sort_savefiles_enable':'false','sort_savefiles_by_content_enable':'false',
         'savestate_auto_save':'false','savestate_auto_load':'false',
         'video_shader_enable':'false','video_threaded':'false','pause_nonactive':'false',
         'record_driver':'wav','video_gpu_screenshot':'true' if hardware else 'false','audio_max_timing_skew':'0.0',
         'auto_overrides_enable':'false','auto_remaps_enable':'false'}
    for key in ['content_history_path','content_favorites_path','content_image_history_path',
                'content_music_history_path','content_video_history_path']:
        cfg[key]=str(out/(key+'.lpl'))
    cfg['video_gpu_record']='true' if hardware else 'false'
    cfg['core_options_path']=str(out/'core-options.cfg')
    (out/'core-options.cfg').write_text(
        f'sm2_renderer = "{a.renderer}"\nsm2_internal_resolution = "{a.scale}"\n'
        f'sm2_texture_filter = "{a.texture_filter}"\nsm2_upscale_2d = "{a.upscale_2d}"\n'
        f'sm2_av_timing = "{a.av_timing}"\n'
        f'sm2_timing_overlay = "{a.timing_overlay}"\n'
        f'sm2_audio_balance = "{a.audio_balance}"\n'
        f'sm2_music_volume = "{a.music_volume}"\n'
        f'sm2_aspect_ratio = "{a.aspect_ratio}"\n'
        f'sm2_gun_input = "{"analog" if gun_inputs else "hybrid"}"\n'
        f'sm2_offscreen_reload_shortcut = "{a.offscreen_reload_shortcut}"\n'
        f'sm2_gamepad_rumble = "{a.gamepad_rumble}"\n'
        f'sm2_crosshairs = "{a.crosshairs}"\n'
        f'sm2_desert_elevation_control = "{a.desert_elevation_control}"\n'
        f'sm2_desert_elevation_speed = "{a.desert_elevation_speed}"\n'
        f'sm2_desert_elevation_axis = "{a.desert_elevation_axis}"\n'
        f'sm2_initial_nvram_setup = "{a.initial_nvram_setup}"\n'
        f'sm2_nvram_settings = "{a.nvram_settings}"\n'
        f'sm2_nvram_vf2_country = "{a.vf2_country}"\n'
        f'sm2_nvram_vf2_drink = "{a.vf2_drink}"\n')
    with (out/'core-options.cfg').open('a') as options:
        options.write(f'sm2_nvram_vf2_difficulty = "{a.vf2_difficulty}"\n')
        options.write(f'sm2_nvram_vf2_display_type = "{a.vf2_display_type}"\n')
    runtime_env=os.environ.copy()
    if a.moltenvk:
        library=a.moltenvk.resolve(strict=True)
        runtime=out/'vulkan-runtime';runtime.mkdir()
        (runtime/'MoltenVK').symlink_to(library)
        runtime_env['DYLD_LIBRARY_PATH']=str(runtime)
        (out/'runtime.json').write_text(json.dumps({'MoltenVK':str(library),'DYLD_LIBRARY_PATH':str(runtime)},indent=2)+'\n')
    config=out/'retroarch.cfg'
    for value in cfg.values():
        if any(c in value for c in ['"','\n','\r']):raise ValueError('Unsupported config path character')
    config.write_text(''.join(f'{k} = "{v}"\n' for k,v in cfg.items()))
    command=[str(a.retroarch.resolve()),'-v','-c',str(config),'-L',str(a.core.resolve()),
             '-P',str(replay),'--max-frames','2300','--max-frames-ss',
             '--max-frames-ss-path',str(out/f'{a.set_name}-gameplay.png'),
             '-r',str(out/f'{a.set_name}.wav'),str(a.rom.resolve())]
    (out/'command.json').write_text(json.dumps(command,indent=2)+'\n')
    started=time.monotonic()
    with (out/'run.log').open('w') as log:
        result=subprocess.run(command,cwd=out,stdout=log,stderr=subprocess.STDOUT,timeout=a.timeout,env=runtime_env)
    elapsed=time.monotonic()-started
    assert result.returncode==0, f'RetroArch exited {result.returncode}; inspect run.log'
    run_log=(out/'run.log').read_text()
    assert '[Replay] Invalid' not in run_log and 'ran out of' not in run_log
    assert 'Failed to initialize audio driver' not in run_log
    if a.timing_overlay!='disabled': assert '[SM2 Timing]' in run_log
    if a.renderer=='vulkan':
        assert '[SM2 GPU] Negotiated Vulkan 1.3:' in run_log
        assert '[SM2 GPU] Ready:' in run_log and 'upstream 2D compute + 3D' in run_log
        assert '[libretro ERROR]' not in run_log
    elif a.renderer=='opengl':
        assert '[SM2 GPU] Ready: OpenGL' in run_log
        assert 'upstream 2D compute + 3D' in run_log
        assert '[libretro ERROR]' not in run_log
    else:
        assert '[SM2 GPU] Ready:' not in run_log
    png=(out/f'{a.set_name}-gameplay.png').read_bytes();assert png.startswith(b'\x89PNG\r\n\x1a\n')
    params,pcm,recording_format=read_recording(out/f'{a.set_name}.wav')
    assert params.nchannels==2 and params.sampwidth==2 and params.framerate>0
    peak=max(map(abs,array.array('h',pcm)))
    assert peak>0 and params.nframes>44100*20
    srm=(out/'saves'/f'{a.set_name}.srm').read_bytes()
    assert len(srm)==64+16384+128 and srm[:8]==b'SM2SRAM\0'
    report={'set_name':a.set_name,'recording_format':recording_format,'renderer':a.renderer,'internal_scale':a.scale,
            'texture_filter':a.texture_filter,'upscale_2d':a.upscale_2d,
            'gamepad_rumble':a.gamepad_rumble,
            'initial_state':bool(initial_state),
            'av_timing':a.av_timing,'timing_overlay':a.timing_overlay,
            'audio_balance':a.audio_balance,'music_volume':a.music_volume,
            'aspect_ratio':a.aspect_ratio,
            'crosshairs':a.crosshairs,'save_ram_size':len(srm),'elapsed_seconds':elapsed,'exit_code':result.returncode,'audio_frames':params.nframes,'audio_rate':params.framerate,
            'audio_peak':peak,'audio_sha256':hashlib.sha256(pcm).hexdigest(),
            'screenshot_sha256':hashlib.sha256(png).hexdigest(),
            'note':'Inspect screenshot for gameplay; audible quality and physical devices require manual validation.'}
    if a.set_name=='vf2':
        country=srm[64+0x3350]
        expected={'japan':0,'usa':1,'export':2}[a.vf2_country]
        drink_ng=bool(srm[64+0x3351]&0x08)
        difficulty=srm[64+0x3342]
        expected_difficulty={'easy':0,'normal':1,'hard':2,'hardest':3}[a.vf2_difficulty]
        display_crt=bool(srm[64+0x3351]&0x04)
        if a.nvram_settings=='enabled':
            assert country==expected
            assert drink_ng=={'ok':False,'ng':True}[a.vf2_drink]
            assert difficulty==expected_difficulty
            assert display_crt==(a.vf2_display_type=='crt')
        report.update({'vf2_country':country,'vf2_drink':'NG' if drink_ng else 'OK',
                       'vf2_difficulty':difficulty,
                       'vf2_display_type':'C.R.T.' if display_crt else 'Projector'})
    (out/'result.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
if __name__=='__main__':main()
