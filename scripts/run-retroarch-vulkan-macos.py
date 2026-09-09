#!/usr/bin/env python3
"""Launch RetroArch with a selected MoltenVK library without changing its app bundle.

Example: --retroarch /path/RetroArch --moltenvk /path/libMoltenVK.dylib -- -L /path/sm2_libretro.dylib /path/game.zip
Everything after -- is passed to RetroArch. The temporary override selects Vulkan
and disables threaded video/config saving for this run. Core options still belong
to RetroArch; select Auto or Vulkan for the SM2 renderer.
"""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--retroarch',type=Path,required=True)
    p.add_argument('--moltenvk',type=Path,required=True)
    p.add_argument('args',nargs=argparse.REMAINDER)
    a=p.parse_args()
    if sys.platform!='darwin':p.error('This launcher is for macOS')
    app=a.retroarch.expanduser().resolve(strict=True)
    library=a.moltenvk.expanduser().resolve(strict=True)
    arguments=a.args[1:] if a.args and a.args[0]=='--' else a.args
    with tempfile.TemporaryDirectory(prefix='sm2-retroarch-vulkan-') as temp:
        runtime=Path(temp)
        (runtime/'MoltenVK').symlink_to(library)
        config=runtime/'vulkan.cfg'
        config.write_text('video_driver = "vulkan"\nvideo_threaded = "false"\nconfig_save_on_exit = "false"\n')
        env=os.environ.copy();env['DYLD_LIBRARY_PATH']=str(runtime)
        return subprocess.call([str(app),*arguments,'--appendconfig',str(config)],env=env)
if __name__=='__main__':raise SystemExit(main())
