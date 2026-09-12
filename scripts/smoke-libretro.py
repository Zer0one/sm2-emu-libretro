#!/usr/bin/env python3
"""Exercise the real Libretro ABI with callbacks and optional external ROMs.

Uses only the standard library. Output directories must be new. For a byte
comparison, --reference points to a sm2-headless output for the same frame count,
ROM selection and fresh NVRAM. No ROMs are distributed with this test.
"""
import argparse
import ctypes as C
import hashlib
import json
from pathlib import Path
import wave

class Game(C.Structure):
    _fields_ = [('path', C.c_char_p), ('data', C.c_void_p), ('size', C.c_size_t), ('meta', C.c_char_p)]
class System(C.Structure):
    _fields_ = [('name', C.c_char_p), ('version', C.c_char_p), ('extensions', C.c_char_p),
                ('fullpath', C.c_bool), ('block_extract', C.c_bool)]
class Geometry(C.Structure):
    _fields_ = [('width', C.c_uint), ('height', C.c_uint), ('max_width', C.c_uint),
                ('max_height', C.c_uint), ('aspect', C.c_float)]
class Timing(C.Structure):
    _fields_ = [('fps', C.c_double), ('rate', C.c_double)]
class AV(C.Structure):
    _fields_ = [('geometry', Geometry), ('timing', Timing)]
class Variable(C.Structure):
    _fields_ = [('key', C.c_char_p), ('value', C.c_char_p)]
class MessageExt(C.Structure):
    _fields_ = [('msg', C.c_char_p), ('duration', C.c_uint), ('priority', C.c_uint),
                ('level', C.c_int), ('target', C.c_int), ('type', C.c_int), ('progress', C.c_int8)]
ENV = C.CFUNCTYPE(C.c_bool, C.c_uint, C.c_void_p)
VIDEO = C.CFUNCTYPE(None, C.c_void_p, C.c_uint, C.c_uint, C.c_size_t)
AUDIO = C.CFUNCTYPE(None, C.c_int16, C.c_int16)
BATCH = C.CFUNCTYPE(C.c_size_t, C.POINTER(C.c_int16), C.c_size_t)
POLL = C.CFUNCTYPE(None)
STATE = C.CFUNCTYPE(C.c_int16, C.c_uint, C.c_uint, C.c_uint, C.c_uint)

def sha(data): return hashlib.sha256(data).hexdigest()
def files(path):
    return {str(p.relative_to(path)): sha(p.read_bytes()) for p in sorted(path.rglob('*')) if p.is_file()}

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--core', type=Path, required=True)
    parser.add_argument('--system', type=Path, required=True)
    parser.add_argument('--rom', type=Path, required=True)
    parser.add_argument('--frames', type=int, default=1800)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--reference', type=Path)
    parser.add_argument('--backpressure', action='store_true')
    parser.add_argument('--exercise', action='store_true')
    parser.add_argument('--switch-rom', type=Path, help='Exercise a second board/rate in the same library instance')
    parser.add_argument('--press', action='store_true', help='Coin then Start then action buttons (not an idle comparison)')
    parser.add_argument('--av-timing', choices=['native','60hz'], default='native')
    parser.add_argument('--timing-overlay', choices=['disabled','enabled'], default='disabled')
    args = parser.parse_args()
    if args.frames <= 0: parser.error('--frames must be positive')
    out = args.output.resolve(); out.mkdir(parents=True, exist_ok=False)
    saves = out/'saves'; saves.mkdir()
    directories = {9: str(args.system.resolve()).encode(), 31: str(saves).encode()}
    option_values = {b'sm2_av_timing': args.av_timing.encode(),
                     b'sm2_timing_overlay': args.timing_overlay.encode()}
    lib = C.CDLL(str(args.core.resolve()))
    for name, arg in [('environment', ENV), ('video_refresh', VIDEO), ('audio_sample', AUDIO),
                      ('audio_sample_batch', BATCH), ('input_poll', POLL), ('input_state', STATE)]:
        getattr(lib, 'retro_set_'+name).argtypes = [arg]
    lib.retro_load_game.argtypes = [C.POINTER(Game)]; lib.retro_load_game.restype = C.c_bool
    lib.retro_get_system_info.argtypes = [C.POINTER(System)]
    lib.retro_get_system_av_info.argtypes = [C.POINTER(AV)]
    lib.retro_serialize_size.restype = C.c_size_t
    lib.retro_serialize.argtypes = [C.c_void_p, C.c_size_t]; lib.retro_serialize.restype = C.c_bool
    lib.retro_unserialize.argtypes = [C.c_void_p, C.c_size_t]; lib.retro_unserialize.restype = C.c_bool
    lib.retro_get_memory_size.argtypes = [C.c_uint]; lib.retro_get_memory_size.restype = C.c_size_t
    lib.retro_get_memory_data.argtypes = [C.c_uint]; lib.retro_get_memory_data.restype = C.c_void_p
    lib.retro_set_controller_port_device.argtypes = [C.c_uint, C.c_uint]
    pixels = b''; pcm = bytearray(); frame = 0; videos = 0; polls = 0; batches = 0
    duplicates = 0; statuses = []
    errors = []; reject_pixel = False; shutdown = False
    @ENV
    def env(cmd, data):
        nonlocal shutdown
        if cmd in directories:
            C.cast(data, C.POINTER(C.c_char_p))[0] = directories[cmd]; return True
        if cmd == 10: return not reject_pixel and C.cast(data, C.POINTER(C.c_int))[0] == 1
        if cmd == 3: C.cast(data, C.POINTER(C.c_bool))[0] = True; return True
        if cmd == 15:
            var=C.cast(data,C.POINTER(Variable)).contents
            var.value=option_values.get(var.key); return var.value is not None
        if cmd == 17: C.cast(data, C.POINTER(C.c_bool))[0] = False; return True
        if cmd == 60:
            statuses.append(C.cast(data,C.POINTER(MessageExt)).contents.msg.decode()); return True
        if cmd == 7: shutdown = True; return True
        if cmd in (11, 18): return True
        return False
    @VIDEO
    def video(data, w, h, pitch):
        nonlocal pixels, videos, duplicates
        videos += 1
        if not data:
            duplicates += 1; return
        if (w,h,pitch) != (496,384,1984):
            errors.append('Invalid geometry/pitch/frame'); return
        pixels = C.string_at(data, pitch*h)
    @BATCH
    def batch(data, count):
        nonlocal batches
        batches += 1
        accepted = count
        if args.backpressure and batches < args.frames and batches % 11 == 1:
            accepted = 0
        elif args.backpressure and batches < args.frames and batches % 11 == 2:
            accepted = count//2
        pcm.extend(C.string_at(data, accepted*4))
        return accepted
    @AUDIO
    def sample(left, right):
        import struct
        pcm.extend(struct.pack('<hh', left, right))
    @POLL
    def poll():
        nonlocal polls
        polls += 1
    @STATE
    def state(port, device, index, button):
        if not args.press or port or device != 1: return 0
        if (800 <= frame < 810 or 820 <= frame < 830) and button == 2: return 1  # Coin
        if 850 <= frame < 860 and button == 3: return 1  # Start
        if frame > 950 and frame % 30 < 8 and button in (0,1,8): return 1
        return 0
    callbacks = [env, video, sample, batch, poll, state]
    for name, callback in zip(['environment','video_refresh','audio_sample','audio_sample_batch','input_poll','input_state'],callbacks):
        getattr(lib, 'retro_set_'+name)(callback)
    lib.retro_init()
    info = System(); lib.retro_get_system_info(C.byref(info))
    assert lib.retro_api_version() == 1 and info.fullpath and info.block_extract
    assert info.extensions == b'zip|7z'
    assert lib.retro_serialize_size() == 0
    assert not lib.retro_serialize(None,0) and not lib.retro_unserialize(None,0)
    assert lib.retro_get_memory_size(0) == 16576 and lib.retro_get_memory_data(0)
    lib.retro_run(); lib.retro_reset()  # no content: safe no-op
    assert not lib.retro_load_game(None)
    game = Game(str(args.rom.resolve()).encode(),None,0,None)
    reject_pixel = True; assert not lib.retro_load_game(C.byref(game)); reject_pixel = False
    old_system = directories.pop(9); assert not lib.retro_load_game(C.byref(game)); directories[9] = old_system
    old_save = directories.pop(31); assert not lib.retro_load_game(C.byref(game)); directories[31] = old_save
    bad = out/'invalid.zip'; bad.write_bytes(b'Not a ROM archive')
    assert not lib.retro_load_game(C.byref(Game(str(bad).encode(),None,0,None)))
    missing = Game(str(out/'missing.zip').encode(),None,0,None)
    assert not lib.retro_load_game(C.byref(missing))
    assert lib.retro_load_game(C.byref(game))
    av = AV(); lib.retro_get_system_av_info(C.byref(av))
    native_fps=25000000/434600
    assert abs(av.timing.fps-(60.0 if args.av_timing=='60hz' else native_fps)) < 1e-9
    assert av.timing.rate in (44100,44642)
    assert abs(av.geometry.aspect-4/3) < 1e-6
    for frame in range(args.frames):
        lib.retro_run()
        assert not shutdown, 'Core requested shutdown'
    assert videos == polls == args.frames and not errors
    if args.av_timing=='60hz':
        expected=args.frames*(1-native_fps/60)
        assert abs(duplicates-expected)<=1,(duplicates,expected)
        assert abs(len(pcm)//4-args.frames*av.timing.rate/60)<av.timing.rate/native_fps+2
    else: assert duplicates==0
    overlays=[entry for entry in statuses if entry]
    if args.timing_overlay=='enabled':
        assert overlays and 'Engine cap:' in overlays[-1] and 'Actual:' in overlays[-1]
    # XRGB8888 little-endian B,G,R,X -> PPM top-to-bottom R,G,B.
    rgb = bytearray(len(pixels)//4*3)
    rgb[0::3] = pixels[2::4]; rgb[1::3] = pixels[1::4]; rgb[2::3] = pixels[0::4]
    ppm = b'P6\n496 384\n255\n'+rgb
    (out/'software_frame.ppm').write_bytes(ppm)
    with wave.open(str(out/'audio.wav'),'wb') as w:
        w.setparams((2,2,int(av.timing.rate),0,'NONE','not compressed')); w.writeframes(pcm)
    lib.retro_unload_game()
    nvram_dirs=list(saves.glob('*/*')); assert len(nvram_dirs)==1
    report = {'frames':videos,'duplicated_frames':duplicates,'fps':av.timing.fps,
              'av_timing':args.av_timing,'timing_overlay':args.timing_overlay,
              'overlay_updates':len(overlays),'audio_rate':av.timing.rate,
              'audio_frames':len(pcm)//4,'video_sha256':sha(ppm),'audio_sha256':sha(pcm),
              'nvram':files(nvram_dirs[0]),'abi_checks':True,'backpressure':args.backpressure}
    if args.reference:
        assert args.av_timing=='native','Headless byte comparison requires native timing'
        reference=args.reference
        with wave.open(str(reference/'audio.wav'),'rb') as w:
            assert (w.getnchannels(),w.getsampwidth(),w.getframerate()) == (2,2,int(av.timing.rate))
            expected=w.readframes(w.getnframes())
        report['comparison']={'video':ppm==(reference/'software_frame.ppm').read_bytes(),
                              'audio':bytes(pcm)==expected,
                              'nvram':files(nvram_dirs[0])==files(reference/'nvram')}
        assert all(report['comparison'].values()), report
    if args.exercise:
        before = videos
        lib.retro_run(); assert videos == before
        assert not lib.retro_load_game(C.byref(missing))
        for cycle in range(3):
            assert lib.retro_load_game(C.byref(game))
            for frame in range(90): lib.retro_run()
            lib.retro_reset()
            for frame in range(90): lib.retro_run()
            assert not shutdown
            lib.retro_unload_game()
        report['load_unload_reset_cycles']=3
        # Exercise sample callback fallback and board-rate changes without dlclose.
        alternate = Game(str(args.switch_rom.resolve()).encode(),None,0,None) if args.switch_rom else game
        for entry in [alternate, game]:
            assert lib.retro_load_game(C.byref(entry))
            current=AV(); lib.retro_get_system_av_info(C.byref(current))
            lib.retro_set_audio_sample_batch(BATCH())
            start_pcm=len(pcm)
            for frame in range(120): lib.retro_run()
            delivered=(len(pcm)-start_pcm)//4
            tolerance=2 if args.av_timing=='native' else current.timing.rate/native_fps+2
            assert abs(delivered-120*current.timing.rate/current.timing.fps) < tolerance
            lib.retro_unload_game()
        lib.retro_set_audio_sample_batch(batch)
        report['sample_callback_and_rate_transitions']=True
    lib.retro_deinit(); lib.retro_init(); lib.retro_deinit()
    (out/'result.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2))
if __name__ == '__main__': main()
