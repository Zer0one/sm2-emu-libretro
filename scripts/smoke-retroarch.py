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


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ['retroarch','core','system','rom','output']:
        p.add_argument('--'+name,type=Path,required=True)
    p.add_argument('--timeout',type=int,default=180)
    p.add_argument('--renderer',choices=['software','vulkan'],default='software')
    p.add_argument('--scale',type=int,choices=range(1,5),default=1)
    p.add_argument('--replay-reader', choices=['1.21','1.22'], default='1.21',
                   help='1.22 reads 40 header bytes even for a stateless v1 replay')
    p.add_argument('--audio-driver', default='coreaudio' if platform.system()=='Darwin' else 'alsa')
    p.add_argument('--input-driver', default='cocoa' if platform.system()=='Darwin' else 'x')
    p.add_argument('--moltenvk',type=Path,help='Optional macOS MoltenVK library, used only by the test process')
    a=p.parse_args()
    out=a.output.resolve();out.mkdir(parents=True,exist_ok=False)
    for directory in ['saves','states','screenshots','playlists','config']:
        (out/directory).mkdir()
    movie=bytearray(struct.pack('<6I',0x42535632,1,0,0,1,0))
    # RetroArch 1.22.2 bsv_movie_reset_playback reads 40 bytes and only
    # seeks back to byte 24 when a v1 save state exists. This core has none.
    # Pad this test fixture for that reader; keep the actual v1 input records.
    if a.replay_reader=='1.22': movie += bytes(16)
    for frame in range(2400):
        pressed=set()
        if 800<=frame<810 or 820<=frame<830:pressed.add(2)  # Select / coin
        if 870<=frame<880 or 950<=frame<960 or 1050<=frame<1060:pressed.add(3)
        if frame>1100:
            if frame%30<8:pressed.update([0,8,1])  # South, East, West
            if frame%120<60:pressed.add(7)  # Right
        movie+=struct.pack('<BH',0,32)
        for port in range(2):
            for button in range(16):
                movie+=struct.pack('<4BHh',port,1,0,0,button,int(port==0 and button in pressed))
        movie+=b'f'
    replay=out/'vf2-inputs.replay';replay.write_bytes(movie)
    cfg={'system_directory':str(a.system.resolve()),'savefile_directory':str(out/'saves'),
         'savestate_directory':str(out/'states'),'screenshot_directory':str(out/'screenshots'),
         'playlist_directory':str(out/'playlists'),'rgui_config_directory':str(out/'config'),
         'video_driver':'vulkan' if a.renderer=='vulkan' else 'gl','audio_driver':a.audio_driver,'input_driver':a.input_driver,
         'video_fullscreen':'false','video_windowed_fullscreen':'false','video_scale':'2',
         'video_vsync':'false','audio_sync':'true','audio_enable':'true',
         'config_save_on_exit':'false','content_history_enable':'false',
         'savestate_auto_save':'false','savestate_auto_load':'false',
         'video_shader_enable':'false','video_threaded':'false','pause_nonactive':'false',
         'record_driver':'wav','video_gpu_screenshot':'true' if a.renderer=='vulkan' else 'false','audio_max_timing_skew':'0.0',
         'auto_overrides_enable':'false','auto_remaps_enable':'false'}
    for key in ['content_history_path','content_favorites_path','content_image_history_path',
                'content_music_history_path','content_video_history_path']:
        cfg[key]=str(out/(key+'.lpl'))
    cfg['video_gpu_record']='true' if a.renderer=='vulkan' else 'false'
    cfg['core_options_path']=str(out/'core-options.cfg')
    (out/'core-options.cfg').write_text(f'sm2_renderer = "{a.renderer}"\nsm2_internal_resolution = "{a.scale}"\n')
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
             '--max-frames-ss-path',str(out/'vf2-gameplay.png'),'-r',str(out/'vf2.wav'),str(a.rom.resolve())]
    (out/'command.json').write_text(json.dumps(command,indent=2)+'\n')
    started=time.monotonic()
    with (out/'run.log').open('w') as log:
        result=subprocess.run(command,cwd=out,stdout=log,stderr=subprocess.STDOUT,timeout=a.timeout,env=runtime_env)
    elapsed=time.monotonic()-started
    assert result.returncode==0, f'RetroArch exited {result.returncode}; inspect run.log'
    run_log=(out/'run.log').read_text()
    assert '[Replay] Invalid' not in run_log and 'ran out of' not in run_log
    assert 'Failed to initialize audio driver' not in run_log
    if a.renderer=='vulkan':
        assert '[SM2 GPU] Negotiated Vulkan 1.3:' in run_log
        assert '[SM2 GPU] Ready:' in run_log and 'upstream 2D compute + 3D' in run_log
        assert '[libretro ERROR]' not in run_log
    else:
        assert '[SM2 GPU] Ready:' not in run_log
    png=(out/'vf2-gameplay.png').read_bytes();assert png.startswith(b'\x89PNG\r\n\x1a\n')
    with wave.open(str(out/'vf2.wav'),'rb') as w:
        params=w.getparams();pcm=w.readframes(w.getnframes())
    assert params.nchannels==2 and params.sampwidth==2 and params.framerate==44100
    peak=max(map(abs,array.array('h',pcm)))
    assert peak>0 and params.nframes>44100*20
    report={'renderer':a.renderer,'internal_scale':a.scale,'elapsed_seconds':elapsed,'exit_code':result.returncode,'audio_frames':params.nframes,'audio_rate':params.framerate,
            'audio_peak':peak,'audio_sha256':hashlib.sha256(pcm).hexdigest(),
            'screenshot_sha256':hashlib.sha256(png).hexdigest(),
            'note':'Inspect screenshot for gameplay; audible quality and physical devices require manual validation.'}
    (out/'result.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
if __name__=='__main__':main()
