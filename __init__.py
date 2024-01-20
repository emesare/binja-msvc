from pathlib import Path
from platform import system
from shutil import copy2
from hashlib import md5

import binaryninja as bn
import os

PLUGIN_PATH = Path(bn.user_plugin_path()) / "msvc-native"

mgr = bn.RepositoryManager()
for repo in mgr.repositories:
	if any([x.path == "emesare_msvc_native" and x.installed for x in repo.plugins]):
		PLUGIN_PATH = Path(repo.full_path) / "emesare_msvc_native"
		break

LIB_NAME = "libbinja-msvc.so"
if system() == "Windows":
	LIB_NAME = "binja-msvc.dll"
elif system() == "Darwin":
	LIB_NAME = "libbinja-msvc.dylib"

LIB_PATH = Path(bn.user_plugin_path()) / LIB_NAME
LIB_SRC = PLUGIN_PATH / LIB_NAME

def update_binary():
	copy2(LIB_SRC, LIB_PATH)
	bn.log_warn("libbinja-msvc installed, please restart")

if not LIB_PATH.exists(follow_symlinks=True):
	update_binary()
else:
	OLD_CHECKSUM = md5(open(LIB_SRC,"rb").read()).hexdigest()
	NEW_CHECKSUM = md5(open(LIB_PATH,"rb").read()).hexdigest()

	if OLD_CHECKSUM != NEW_CHECKSUM:
		bn.log_info("Old libbinja-msvc found, updating...")
		update_binary()
