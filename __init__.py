from pathlib import Path
from platform import system
from shutil import copy2
from hashlib import md5
from urllib.request import urlretrieve

import binaryninja as bn
import os
import json

LIB_PATH = Path(bn.user_plugin_path())
PLUGIN_PATH = Path(bn.user_plugin_path()) / "msvc-native"

LIB_NAME = "libbinja-msvc.so"
if system() == "Windows":
	LIB_NAME = "binja-msvc.dll"
elif system() == "Darwin":
	LIB_NAME = "libbinja-msvc.dylib"

DOWNLOAD_URL = "https://github.com/emesare/binja-msvc/releases/latest/download/" + LIB_NAME

# Swap PLUGIN_PATH to use repository path if plugin exists in there.
mgr = bn.RepositoryManager()
for repo in mgr.repositories:
	if any([x.path == "emesare_msvc_native" and x.installed for x in repo.plugins]):
		PLUGIN_PATH = Path(repo.full_path) / "emesare_msvc_native"
		break

def needs_install():
    # If First run, install
    if not (LIB_PATH / LIB_NAME).exists(follow_symlinks=True):
        return True
    # If our plugin updates the manifest version will differ.
    with open(PLUGIN_PATH / "plugin.json") as manifest_file:
        manifest = json.load(manifest_file)
        with open(LIB_PATH / "emesare_msvc_native.json") as installed_manifest_file:
            installed_manifest = json.load(installed_manifest_file)
            if manifest["version"] != installed_manifest["version"]:
                return True
    return False
        

def update_binary():
    urllib.urlretrieve(DOWNLOAD_URL, LIB_INSTALL_PATH / LIB_NAME)
    # Copy over the manifest to compare against in the future.
    copy2(LIB_MANIFEST, LIB_INSTALL_PATH / "emesare_msvc_native.json")
    bn.log_warn("binja-msvc installed, please restart!")

if needs_install():
    bn.log_info("installing binja-msvc...")
    update_binary()
