"""build.ini, paths and processes. The kit runs in its Linux container; the Windows programs run under Wine."""
import configparser
import hashlib
import os
import subprocess
import zlib
from pathlib import Path

KIT = Path(__file__).resolve().parents[1]          # buildkit/
REPO = KIT.parent                                  # the repository: sources/, the input
WINE = ["wine"]


class BuildError(Exception):
    pass


def say(*parts):
    print(*parts, flush=True)


class Config:
    """build.ini. A relative path is relative to the repository root."""

    def __init__(self, path):
        self.file = Path(path)
        self.ini = configparser.ConfigParser(interpolation=None, inline_comment_prefixes=None)
        if not self.ini.read(self.file, encoding="utf-8"):
            raise BuildError("no config file %s" % self.file)

    def get(self, section, key, default=None):
        v = self.ini.get(section, key, fallback=default)
        return v.strip() if isinstance(v, str) else v

    def path(self, section, key):
        v = self.get(section, key)
        if not v:
            raise BuildError("build.ini: [%s] %s is not set" % (section, key))
        p = Path(v)
        return p if p.is_absolute() else REPO / p

    def jobs(self, section):
        return max(1, int(self.get(section, "jobs", "2")))

    def out(self, target):
        d = self.path("paths", "out") / target
        d.mkdir(parents=True, exist_ok=True)
        return d


# -- Windows programs: every path handed to one is written as Wine's Z: drive (the Unix root) sees it --
def winpath(p):
    return "Z:" + str(Path(p).resolve()).replace("/", "\\")


def winlist(paths):
    return ";".join(winpath(p) for p in paths)


def exe_env(dll_dirs=(), **vars_):
    """The environment of a Windows program: VARS, plus DLL_DIRS searched for its DLLs (FlexLM)."""
    env = dict(os.environ)
    env.update(vars_)
    if dll_dirs:
        env["WINEPATH"] = winlist(dll_dirs)
    env.setdefault("WINEDEBUG", "-all")
    return env


def run_exe(exe, args, cwd=None, env=None, timeout=1800):
    """Run a Windows program; returns (exit code, output). Paths in ARGS must already be winpath()s."""
    return run(WINE + [str(exe)] + [str(a) for a in args], cwd=cwd, env=env, timeout=timeout)


def run(cmd, cwd=None, env=None, timeout=1800):
    """Run a program; returns (exit code, stdout and stderr as text)."""
    try:
        r = subprocess.run([str(c) for c in cmd], cwd=cwd and str(cwd), env=env, timeout=timeout,
                           stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    except FileNotFoundError:
        raise BuildError("cannot run %s: not found" % cmd[0])
    except subprocess.TimeoutExpired:
        raise BuildError("%s timed out after %d s" % (Path(str(cmd[0])).name, timeout))
    return r.returncode, r.stdout.decode("latin-1")


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def crc32(path):
    return "%08x" % (zlib.crc32(Path(path).read_bytes()) & 0xFFFFFFFF)
